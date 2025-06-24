/* Standard includes. */
#include <stdio.h>

/* Kernel includes. */
#include "tx_api.h"
#include "tx_port.h"

/* cvitek includes. */
#include "printf.h"
#include "rtos_cmdqu.h"
#include "cvi_mailbox.h"
#include "intr_conf.h"
#include "top_reg.h"
#include "memmap.h"
#include "comm.h"
#include "cvi_spinlock.h"

/* RPMsg includes*/
#include "rpmsg_lite.h"
#include "rpmsg_ns.h"

/* Milk-V Duo */
#include "milkv_duo_io.h"

#define __DEBUG__
#ifdef __DEBUG__
#define debug_printf printf
#else
#define debug_printf(...)
#endif

void prvQueueISR(void);
/* mailbox parameters */
volatile struct mailbox_set_register *mbox_reg;
volatile struct mailbox_done_register *mbox_done_reg;
volatile unsigned long *mailbox_context; // mailbox buffer context is 64 Bytess

void main_cvirtos(void)
{
	//arch_usleep(1000 * 1000);
	printf("create cvi task\n");

	/* Start the tasks and timer running. */ // cvitek/driver/common/src/system.c
	request_irq(MBOX_INT_C906_2ND, prvQueueISR, 0, "mailbox", (void *)0);

	/* Enter the ThreadX kernel.  */
	tx_kernel_enter();

	/* If all is well, the scheduler will now be running, and the following
    line will never be reached.  If the following line does execute, then
    there was either insufficient FreeRTOS heap memory available for the idle
    and/or timer tasks to be created, or vTaskStartScheduler() was called from
    User mode.  See the memory management section on the FreeRTOS web site for
    more details on the FreeRTOS heap http://www.freertos.org/a00111.html.  The
    mode from which main() is called is set in the C start up code and must be
    a privileged mode (not user mode). */
	printf("cvi task end\n");

	for (;;)
		;
}

#define IS_TX_ERROR(x) \
	do{ \
		if((x) != TX_SUCCESS) \
			printf("error: %d at %s\n", __LINE__, __FILE__); \
	}while(0)

#define DEMO_STACK_SIZE 1024
#define DEMO_BYTE_POOL_SIZE configTOTAL_HEAP_SIZE

#define TX_MS_TO_TICKS( xTimeInMs )    ( (unsigned long) ( ( (unsigned long) ( xTimeInMs ) * (unsigned long) TX_TIMER_TICKS_PER_SECOND ) / (unsigned long) 1000U ) )

UCHAR byte_pool_memory[DEMO_BYTE_POOL_SIZE] __attribute__ ( (section( ".heap" )) );

void prvCmdQuRunTask(ULONG thread_input);
void thread_0_entry(ULONG thread_input);
void thread_1_entry(ULONG thread_input);
void thread_rpmsg_entry(ULONG thread_input);
TX_THREAD thread_0;
TX_THREAD thread_1;
TX_THREAD mail_thread;
TX_THREAD thread_rpmsg;
volatile int thread_0_counter = 10;
volatile int thread_1_counter = 10;
TX_BYTE_POOL byte_pool_0;

TX_QUEUE                mailbox_queue;
#define DEMO_QUEUE_SIZE         30

/* Define what the initial system looks like.  */
void tx_application_define(void *first_unused_memory)
{
	(void)first_unused_memory;

	CHAR *pointer = TX_NULL;
	UINT ret = 0;

	/* Create a byte memory pool from which to allocate the thread stacks.  */
	ret = tx_byte_pool_create(&byte_pool_0, "byte pool 0", byte_pool_memory,DEMO_BYTE_POOL_SIZE);
	IS_TX_ERROR(ret);

	
    /* Allocate the message queue.  */
	ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_QUEUE_SIZE*sizeof(cmdqu_t), TX_NO_WAIT);
	IS_TX_ERROR(ret);

    /* Create the message queue */
	ret = tx_queue_create(&mailbox_queue, "mailbox_queue", sizeof(cmdqu_t), pointer, DEMO_QUEUE_SIZE*sizeof(cmdqu_t));
	IS_TX_ERROR(ret);

	/* Allocate the stack for thread 0.  */
	ret = tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

	IS_TX_ERROR(ret);

	ret = tx_thread_create(&thread_0, "thread 0", thread_0_entry, 0, pointer,
			 DEMO_STACK_SIZE, 6, 6, 10,
			 TX_AUTO_START);
	IS_TX_ERROR(ret);
	
	ret = tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
	IS_TX_ERROR(ret);

	ret = tx_thread_create(&thread_1, "thread 1", thread_1_entry, 99, pointer,
			 DEMO_STACK_SIZE, 6, 6, 10,
			 TX_AUTO_START);
	IS_TX_ERROR(ret);

	ret = tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
	IS_TX_ERROR(ret);

	ret = tx_thread_create(&mail_thread, "mail thread", prvCmdQuRunTask, 0, 
			 pointer, DEMO_STACK_SIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
	IS_TX_ERROR(ret);

	ret = tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE * 10, TX_NO_WAIT);
	IS_TX_ERROR(ret);

	ret = tx_thread_create(&thread_rpmsg, "thread rpmsg", thread_rpmsg_entry, 0, pointer,
			 DEMO_STACK_SIZE * 10, 6, 6, 10,
			 TX_AUTO_START);
	IS_TX_ERROR(ret);
}

void thread_0_entry(ULONG thread_input)
{
	(void)thread_input;

	//UINT status;

	printf("thread 0 in\n");
	double result = 0;
	while (1) {

		
		printf("threadx 0 running: %d\n", thread_0_counter++);

		result = result + thread_0_counter * 1.5;
		printf("float cal: %d\n", (int)result);

		tx_thread_sleep(TX_MS_TO_TICKS(4000));  // 5ms per tick(200Hz)
	}
}

void thread_1_entry(ULONG thread_input)
{
	(void)thread_input;

	printf("thread 1 in\n");
	while (1) {
		printf("threadx 1 running: %d\n", thread_1_counter++);
		tx_thread_sleep(TX_MS_TO_TICKS(8000));  
	}
}

#define LOCAL_EPT_ADDR                (30U)
uint32_t *shared_memory = (uint32_t *)0x8fc00000;
struct rpmsg_lite_instance *gp_rpmsg_dev_inst;
struct rpmsg_lite_endpoint *gp_rpmsg_ept;
struct rpmsg_lite_instance g_rpmsg_ctxt;
struct rpmsg_lite_ept_static_context g_ept_context;
volatile int32_t g_has_received;
static uint32_t g_remote_addr     = 0;

typedef struct the_message
{
    uint32_t DATA;
} THE_MESSAGE, *THE_MESSAGE_PTR;

static THE_MESSAGE volatile g_msg = {0};


/* Internal functions */
static int32_t rpmsg_ept_read_cb(void *payload, uint32_t payload_len, uint32_t src, void *priv)
{
    volatile int32_t *has_received = priv;
    if (payload_len <= sizeof(THE_MESSAGE))
    {
		(void)memcpy((void *)&g_msg, payload, payload_len);
        g_remote_addr = src;
        *has_received = 1;
    }
    return RL_RELEASE;
}

void thread_rpmsg_entry(ULONG thread_input)
{
    (void)thread_input;

    gp_rpmsg_dev_inst = rpmsg_lite_remote_init(shared_memory, 0, RL_NO_FLAGS, &g_rpmsg_ctxt);

    gp_rpmsg_ept = rpmsg_lite_create_ept(gp_rpmsg_dev_inst, LOCAL_EPT_ADDR, rpmsg_ept_read_cb, (void *)&g_has_received,
                                         &g_ept_context);

	gp_rpmsg_dev_inst->link_state = RL_TRUE;
    int result = rpmsg_ns_announce(gp_rpmsg_dev_inst, gp_rpmsg_ept, "rpmsg-sg2002-c906l-channel", (uint32_t)RL_NS_CREATE);
    if (result){
        RL_ASSERT(result == 0);
    }

    for(;;)
    {
        if (1 == g_has_received)
        {
			g_has_received = 0;
            g_msg.DATA++;
			printf("send: %d\n",g_msg.DATA);
            (void)rpmsg_lite_send(gp_rpmsg_dev_inst, gp_rpmsg_ept, g_remote_addr, (char *)&g_msg, sizeof(THE_MESSAGE),
                                  RL_DONT_BLOCK);
        }
    }

    (void)rpmsg_lite_destroy_ept(gp_rpmsg_dev_inst, gp_rpmsg_ept);
    gp_rpmsg_ept = ((void *)0);
    (void)rpmsg_lite_deinit(gp_rpmsg_dev_inst);
    g_msg.DATA = 0U;
}

void prvCmdQuRunTask(ULONG thread_input)
{
	/* Remove compiler warning about unused parameter. */
	(void)thread_input;

	cmdqu_t rtos_cmdq;
	unsigned int reg_base = MAILBOX_REG_BASE;

	/* to compatible code with linux side */
	mbox_reg = (struct mailbox_set_register *)reg_base;
	mbox_done_reg = (struct mailbox_done_register *)(reg_base + 2);
	mailbox_context = (unsigned long *)(MAILBOX_REG_BUFF);

	cvi_spinlock_init();
	printf("prvCmdQuRunTask run\n");

	for (;;) {
		//xQueueReceive(gTaskCtx[0].queHandle, &rtos_cmdq, portMAX_DELAY);
		tx_queue_receive(&mailbox_queue, &rtos_cmdq, TX_WAIT_FOREVER);

		switch (rtos_cmdq.cmd_id) {
		case 0 :
			env_isr(0);
			break;
		case 1 :
			env_isr(1);
			break;
		case CMD_TEST_A:
			//do something
			//send to C906B
			rtos_cmdq.cmd_id = CMD_TEST_A;
			rtos_cmdq.param_ptr = 0x12345678;
			rtos_cmdq.resv.valid.rtos_valid = 1;
			rtos_cmdq.resv.valid.linux_valid = 0;
			printf("recv cmd(%d) from C906B...send [0x%x] to C906B\n",
			       rtos_cmdq.cmd_id, rtos_cmdq.param_ptr);
			goto send_label;
		case CMD_TEST_B:
			//nothing to do
			printf("nothing to do...\n");
			break;
		case CMD_TEST_C:
			rtos_cmdq.cmd_id = CMD_TEST_C;
			rtos_cmdq.param_ptr = 0x55aa;
			rtos_cmdq.resv.valid.rtos_valid = 1;
			rtos_cmdq.resv.valid.linux_valid = 0;
			printf("recv cmd(%d) from C906B...send [0x%x] to C906B\n",
			       rtos_cmdq.cmd_id, rtos_cmdq.param_ptr);
			goto send_label;
		case CMD_DUO_LED:
			rtos_cmdq.cmd_id = CMD_DUO_LED;
			printf("recv cmd(%d) from C906B, param_ptr [0x%x]\n",
			       rtos_cmdq.cmd_id, rtos_cmdq.param_ptr);
			if (rtos_cmdq.param_ptr == DUO_LED_ON) {
				duo_led_control(1);
			} else {
				duo_led_control(0);
			}
			rtos_cmdq.param_ptr = DUO_LED_DONE;
			rtos_cmdq.resv.valid.rtos_valid = 1;
			rtos_cmdq.resv.valid.linux_valid = 0;
			printf("recv cmd(%d) from C906B...send [0x%x] to C906B\n",
			       rtos_cmdq.cmd_id, rtos_cmdq.param_ptr);
			goto send_label;
		default:
		send_label:
			/* used to send command to linux*/
			mailbox_send(1);
			break;
		}
	}
}

void prvQueueISR(void)
{
	unsigned char set_val;
	unsigned char valid_val;
	int i;
	cmdqu_t *cmdq;
	//BaseType_t YieldRequired = pdFALSE;
	UINT ret;

	set_val = mbox_reg->cpu_mbox_set[RECEIVE_CPU].cpu_mbox_int_int.mbox_int;

	if (set_val) {
		for (i = 0; i < MAILBOX_MAX_NUM; i++) {
			valid_val = set_val & (1 << i);

			if (valid_val) {
				cmdqu_t rtos_cmdq;
				cmdq = (cmdqu_t *)(mailbox_context) + i;

				/* mailbox buffer context is send from linux, clear mailbox interrupt */
				mbox_reg->cpu_mbox_set[RECEIVE_CPU]
					.cpu_mbox_int_clr.mbox_int_clr =
					valid_val;
				// need to disable enable bit
				mbox_reg->cpu_mbox_en[RECEIVE_CPU].mbox_info &=
					~valid_val;

				// copy cmdq context (8 bytes) to buffer ASAP
				*((unsigned long *)&rtos_cmdq) =
					*((unsigned long *)cmdq);
				/* need to clear mailbox interrupt before clear mailbox buffer */
				*((unsigned long *)cmdq) = 0;

				/* mailbox buffer context is send from linux*/
				if (rtos_cmdq.resv.valid.linux_valid == 1) {

					if ((ret = tx_queue_send(&mailbox_queue, &rtos_cmdq, TX_NO_WAIT)) != TX_SUCCESS)
					{
						printf("rtos cmdq send failed: %d\n", ret);
					}
					//xQueueSendFromISR(gTaskCtx[0].queHandle,
					// 		  &rtos_cmdq,
					// 		  &YieldRequired);

					//portYIELD_FROM_ISR(YieldRequired);
				} else
					printf("rtos cmdq is not valid %d, ip=%d , cmd=%d\n",
					       rtos_cmdq.resv.valid.rtos_valid,
					       rtos_cmdq.ip_id,
					       rtos_cmdq.cmd_id);
			}
		}
	}
}
