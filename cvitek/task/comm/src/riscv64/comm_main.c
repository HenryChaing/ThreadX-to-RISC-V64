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

/* Milk-V Duo */
#include "milkv_duo_io.h"

/* RPMsg includes*/
#include "rpmsg_lite.h"
#include "rpmsg_ns.h"
#include "rpmsg_queue.h"
#include "rpmsg_platform.h"

#define __DEBUG__
#ifdef __DEBUG__
#define debug_printf printf
#else
#define debug_printf(...)
#endif

void tx_mailbox_driver_output_ISR(void);
// DEFINE_CVI_SPINLOCK(mailbox_lock, SPIN_MBOX);
/* mailbox parameters */
volatile struct mailbox_set_register *mbox_reg;
volatile struct mailbox_done_register *mbox_done_reg;
volatile unsigned long *mailbox_context; // mailbox buffer context is 64 Bytess

void main_cvirtos(void)
{
	//arch_usleep(1000 * 1000);
	printf("create cvi task\n");

	/* Start the tasks and timer running. */ // cvitek/driver/common/src/system.c
	request_irq(MBOX_INT_C906_2ND, tx_mailbox_driver_output_ISR, 0, "mailbox", (void *)0);

	tx_mailbox_driver_initialize();

	// tx_send_and_recieve_run_tests();

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
			printf("error: %d at %s\n", __LINE__, __FILE__); printf("error = %d\n",x);\
	}while(0)

#define DEMO_STACK_SIZE 1024
#define DEMO_BYTE_POOL_SIZE configTOTAL_HEAP_SIZE

#define TX_MS_TO_TICKS( xTimeInMs )    ( (unsigned long) ( ( (unsigned long) ( xTimeInMs ) * (unsigned long) TX_TIMER_TICKS_PER_SECOND ) / (unsigned long) 1000U ) )

UCHAR byte_pool_memory[DEMO_BYTE_POOL_SIZE] __attribute__ ( (section( ".heap" )) );

#define USE_MAILBOX_EXAMPLE 1

#if (USE_MAILBOX_EXAMPLE==0)
#define DEMO_BLOCK_POOL_SIZE    100
#define DEMO_QUEUE_SIZE         100

/* Define the ThreadX object control blocks...  */

TX_THREAD               thread_0;
TX_THREAD               thread_1;
TX_THREAD               thread_2;
TX_THREAD               thread_3;
TX_THREAD               thread_4;
TX_THREAD               thread_5;
TX_THREAD               thread_6;
TX_THREAD               thread_7;
TX_QUEUE                queue_0;
TX_SEMAPHORE            semaphore_0;
TX_MUTEX                mutex_0;
TX_EVENT_FLAGS_GROUP    event_flags_0;
TX_BYTE_POOL            byte_pool_0;
TX_BLOCK_POOL           block_pool_0;

/* Define the counters used in the demo application...  */

ULONG                   thread_0_counter;
ULONG                   thread_1_counter;
ULONG                   thread_1_messages_sent;
ULONG                   thread_2_counter;
ULONG                   thread_2_messages_received;
ULONG                   thread_3_counter;
ULONG                   thread_4_counter;
ULONG                   thread_5_counter;
ULONG                   thread_6_counter;
ULONG                   thread_7_counter;

/* Define thread prototypes.  */

void    thread_0_entry(ULONG thread_input);
void    thread_1_entry(ULONG thread_input);
void    thread_2_entry(ULONG thread_input);
void    thread_3_and_4_entry(ULONG thread_input);
void    thread_5_entry(ULONG thread_input);
void    thread_6_and_7_entry(ULONG thread_input);

/* Define what the initial system looks like.  */
void tx_application_define(void *first_unused_memory)
{
	(void)first_unused_memory;
CHAR    *pointer = TX_NULL;

	int ret;

    /* Create a byte memory pool from which to allocate the thread stacks.  */
    ret = tx_byte_pool_create(&byte_pool_0, "byte pool 0", byte_pool_memory, DEMO_BYTE_POOL_SIZE);
	
	IS_TX_ERROR(ret);

    /* Put system definition stuff in here, e.g. thread creates and other assorted
       create information.  */

    /* Allocate the stack for thread 0.  */
    ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

	IS_TX_ERROR(ret);

    /* Create the main thread.  */
    ret = tx_thread_create(&thread_0, "thread 0", thread_0_entry, 0,  
            pointer, DEMO_STACK_SIZE, 
            1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);

	IS_TX_ERROR(ret);

    /* Allocate the stack for thread 1.  */
    ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

	IS_TX_ERROR(ret);
	
    /* Create threads 1 and 2. These threads pass information through a ThreadX 
       message queue.  It is also interesting to note that these threads have a time
       slice.  */
    ret = tx_thread_create(&thread_1, "thread 1", thread_1_entry, 1,  
            pointer, DEMO_STACK_SIZE, 
            16, 16, 4, TX_AUTO_START);

	IS_TX_ERROR(ret);

    /* Allocate the stack for thread 2.  */
    ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

	IS_TX_ERROR(ret);

    ret = tx_thread_create(&thread_2, "thread 2", thread_2_entry, 2,  
            pointer, DEMO_STACK_SIZE, 
            16, 16, 4, TX_AUTO_START);

	IS_TX_ERROR(ret);

    /* Allocate the stack for thread 3.  */
    ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

	IS_TX_ERROR(ret);

    /* Create threads 3 and 4.  These threads compete for a ThreadX counting semaphore.  
       An interesting thing here is that both threads share the same instruction area.  */
    ret = tx_thread_create(&thread_3, "thread 3", thread_3_and_4_entry, 3,  
            pointer, DEMO_STACK_SIZE, 
            8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

	IS_TX_ERROR(ret);

    /* Allocate the stack for thread 4.  */
    ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

	IS_TX_ERROR(ret);

    ret = tx_thread_create(&thread_4, "thread 4", thread_3_and_4_entry, 4,  
            pointer, DEMO_STACK_SIZE, 
            8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

	IS_TX_ERROR(ret);

    /* Allocate the stack for thread 5.  */
    ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

	IS_TX_ERROR(ret);

    /* Create thread 5.  This thread simply pends on an event flag which will be set
       by thread_0.  */
    ret = tx_thread_create(&thread_5, "thread 5", thread_5_entry, 5,  
            pointer, DEMO_STACK_SIZE, 
            4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

	IS_TX_ERROR(ret);

    /* Allocate the stack for thread 6.  */
    ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

	IS_TX_ERROR(ret);

    /* Create threads 6 and 7.  These threads compete for a ThreadX mutex.  */
    ret = tx_thread_create(&thread_6, "thread 6", thread_6_and_7_entry, 6,  
            pointer, DEMO_STACK_SIZE, 
            8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

	IS_TX_ERROR(ret);

    /* Allocate the stack for thread 7.  */
    ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

	IS_TX_ERROR(ret);

    ret = tx_thread_create(&thread_7, "thread 7", thread_6_and_7_entry, 7,  
            pointer, DEMO_STACK_SIZE, 
            8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

	IS_TX_ERROR(ret);

    /* Allocate the message queue.  */
    ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_QUEUE_SIZE*sizeof(ULONG), TX_NO_WAIT);

	IS_TX_ERROR(ret);
	
    /* Create the message queue shared by threads 1 and 2.  */
    ret = tx_queue_create(&queue_0, "queue 0", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE*sizeof(ULONG));

	IS_TX_ERROR(ret);

    /* Create the semaphore used by threads 3 and 4.  */
    ret = tx_semaphore_create(&semaphore_0, "semaphore 0", 1);

	IS_TX_ERROR(ret);

    /* Create the event flags group used by threads 1 and 5.  */
    ret = tx_event_flags_create(&event_flags_0, "event flags 0");

	IS_TX_ERROR(ret);

    /* Create the mutex used by thread 6 and 7 without priority inheritance.  */
    ret = tx_mutex_create(&mutex_0, "mutex 0", TX_NO_INHERIT);

	IS_TX_ERROR(ret);

    /* Allocate the memory for a small block pool.  */
    ret = tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_BLOCK_POOL_SIZE, TX_NO_WAIT);

	IS_TX_ERROR(ret);

    /* Create a block memory pool to allocate a message buffer from.  */
    ret = tx_block_pool_create(&block_pool_0, "block pool 0", sizeof(ULONG), pointer, DEMO_BLOCK_POOL_SIZE);

	IS_TX_ERROR(ret);

    /* Allocate a block and release the block memory.  */
    ret = tx_block_allocate(&block_pool_0, (VOID **) &pointer, TX_NO_WAIT);

	IS_TX_ERROR(ret);

    /* Release the block back to the pool.  */
    ret = tx_block_release(pointer);
	IS_TX_ERROR(ret);
}

/* Define the test threads.  */

void thread_0_entry(ULONG thread_input)
{

	UINT status;

	printf("thread 0 in\n");
    /* This thread simply sits in while-forever-sleep loop.  */
    while(1)
    {

        /* Increment the thread counter.  */
        thread_0_counter++;

        /* Sleep for 10 ticks.  */
        tx_thread_sleep(10);

        /* Set event flag 0 to wakeup thread 5.  */
        status =  tx_event_flags_set(&event_flags_0, 0x1, TX_OR);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

		debug_printf("thread 0 set event flag to 5\n");
    }

	printf("thread 0 out\n");
}

void    thread_1_entry(ULONG thread_input)
{

	UINT    status;

	printf("thread 1 in\n");

    /* This thread simply sends messages to a queue shared by thread 2.  */
    while(1)
    {

        /* Increment the thread counter.  */
        thread_1_counter++;

        /* Send message to queue 0.  */
        status =  tx_queue_send(&queue_0, &thread_1_messages_sent, TX_WAIT_FOREVER);

        /* Check completion status.  */
        if (status != TX_SUCCESS)
            break;

		debug_printf("thread 1 set message %d\n", thread_1_messages_sent);
        /* Increment the message sent.  */
        thread_1_messages_sent++;
    }
	printf("thread 1 out\n");
}

void    thread_2_entry(ULONG thread_input)
{

ULONG   received_message;
UINT    status;

	printf("thread 2 in\n");
    /* This thread retrieves messages placed on the queue by thread 1.  */
    while(1)
    {

        /* Increment the thread counter.  */
        thread_2_counter++;

        /* Retrieve a message from the queue.  */
        status = tx_queue_receive(&queue_0, &received_message, TX_WAIT_FOREVER);

        /* Check completion status and make sure the message is what we 
           expected.  */
        if ((status != TX_SUCCESS) || (received_message != thread_2_messages_received))
            break;
        
		debug_printf("thread 2 recevie message %d\n", thread_2_messages_received);
        /* Otherwise, all is okay.  Increment the received message count.  */
        thread_2_messages_received++;
    }
	printf("thread 2 out\n");
}

void    thread_3_and_4_entry(ULONG thread_input)
{

UINT    status;

	if (thread_input == 3)
		printf("thread 3 in\n");
	else
		printf("thread 4 in\n");
    /* This function is executed from thread 3 and thread 4.  As the loop
       below shows, these function compete for ownership of semaphore_0.  */
    while(1)
    {

        /* Increment the thread counter.  */
        if (thread_input == 3)
            thread_3_counter++;
        else
            thread_4_counter++;

        /* Get the semaphore with suspension.  */
        status =  tx_semaphore_get(&semaphore_0, TX_WAIT_FOREVER);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;
		
		debug_printf("thread %d get semaphore\n", thread_input);

        /* Sleep for 2 ticks to hold the semaphore.  */
        tx_thread_sleep(2);

        /* Release the semaphore.  */
        status =  tx_semaphore_put(&semaphore_0);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;
		
		debug_printf("thread %d put semaphore\n", thread_input);
    }

	if (thread_input == 3)
		printf("thread 3 out\n");
	else
		printf("thread 4 out\n");
}

void    thread_5_entry(ULONG thread_input)
{

UINT    status;
ULONG   actual_flags;

	printf("thread 5 in\n");

    /* This thread simply waits for an event in a forever loop.  */
    while(1)
    {

        /* Increment the thread counter.  */
        thread_5_counter++;

        /* Wait for event flag 0.  */
        status =  tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR, 
                                                &actual_flags, TX_WAIT_FOREVER);

        /* Check status.  */
        if ((status != TX_SUCCESS) || (actual_flags != 0x1))
            break;

		debug_printf("thread 5 get event flag from 0\n");
    }

	printf("thread 5 out\n");
}


void    thread_6_and_7_entry(ULONG thread_input)
{

UINT    status;

	if (thread_input == 6)
		printf("thread 6 in\n");
	else
		printf("thread 7 in\n");

    /* This function is executed from thread 6 and thread 7.  As the loop
       below shows, these function compete for ownership of mutex_0.  */
    while(1)
    {

        /* Increment the thread counter.  */
        if (thread_input == 6)
            thread_6_counter++;
        else
            thread_7_counter++;

        /* Get the mutex with suspension.  */
        status =  tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

		debug_printf("thread %d get mutex once\n", thread_input);

        /* Get the mutex again with suspension.  This shows
           that an owning thread may retrieve the mutex it
           owns multiple times.  */
        
		status =  tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

		debug_printf("thread %d get mutex twice\n", thread_input);

        /* Sleep for 2 ticks to hold the mutex.  */
        tx_thread_sleep(2);

        /* Release the mutex.  */
        status =  tx_mutex_put(&mutex_0);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

		debug_printf("thread %d put mutex once\n", thread_input);

        /* Release the mutex again.  This will actually 
           release ownership since it was obtained twice.  */
        status =  tx_mutex_put(&mutex_0);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

		debug_printf("thread %d put mutex twice\n", thread_input);
    }
	if (thread_input == 6)
		printf("thread 6 out\n");
	else
		printf("thread 7 out\n");
}

#elif (USE_MAILBOX_EXAMPLE==1)

void tx_mailbox_driver_output(ULONG thread_input);
void thread_0_entry(ULONG thread_input);
void thread_1_entry(ULONG thread_input);
void tx_send_and_recieve_run_tests(ULONG thread_input);
// void tc_1_rpmsg_init(ULONG thread_input);
TX_THREAD thread_0;
TX_THREAD thread_1;
TX_THREAD mail_thread;
TX_THREAD rpmsg_thread;
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
			 DEMO_STACK_SIZE , 6, 6, 10,
			 TX_AUTO_START);
	IS_TX_ERROR(ret);

/*
	ret = tx_thread_create(&rpmsg_thread, "rpmsg_thread", tc_1_rpmsg_init, 99, pointer,
			 DEMO_STACK_SIZE, 6, 6, 10,
			 TX_AUTO_START);
	IS_TX_ERROR(ret);
*/

	ret = tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE , TX_NO_WAIT);
	IS_TX_ERROR(ret);

	ret = tx_thread_create(&rpmsg_thread, "rpmsg_thread", tx_send_and_recieve_run_tests, 0, pointer,
			 DEMO_STACK_SIZE , 7, 7, 10,
			 TX_AUTO_START);
	IS_TX_ERROR(ret);

	ret = tx_byte_allocate(&byte_pool_0, (VOID **)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
	IS_TX_ERROR(ret);

	ret = tx_thread_create(&mail_thread, "mail thread", tx_mailbox_driver_output, 0, 
			 pointer, DEMO_STACK_SIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
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


/*
 *
 * mailbox
 *
 */

VOID tx_mailbox_driver_initialize(VOID)
{
    
	/* initialize mailbox global variable */
	unsigned int reg_base = MAILBOX_REG_BASE;
	mbox_reg = (struct mailbox_set_register *)reg_base;
	mbox_done_reg = (struct mailbox_done_register *)(reg_base + 2);
	mailbox_context = (unsigned long *)(MAILBOX_REG_BUFF);
	
	/* Initialize the two counting semaphores used to control
    the simple driver I/O. */

	cvi_spinlock_init();

    /* Setup interrupt vectors for input and output ISRs.
    The initial vector handling should call the ISRs
    defined in this file. */

    /* Configure serial device hardware for RX/TX interrupt
    generation, baud rate, stop bits, etc. */
}

VOID tx_mailbox_driver_output(ULONG thread_input)
{
  
  	/* Remove compiler warning about unused parameter. */
	(void)thread_input;

	cmdqu_t rtos_cmdq;
	cmdqu_t *cmdq;
	cmdqu_t *rtos_cmdqu_t;
	static int stop_ip = 0;
	int flags;
	int valid;
	int send_to_cpu = SEND_TO_CPU1;

	unsigned int reg_base = MAILBOX_REG_BASE;

	/* to compatible code with linux side */
	cmdq = &rtos_cmdq;
	printf("tx_mailbox_driver_output run\n");

	for (;;) {
		  /* Determine if the hardware is ready to transmit a
			character. If not, suspend until the previous output
			completes. */
		tx_queue_receive(&mailbox_queue, &rtos_cmdq, TX_WAIT_FOREVER);

		switch (rtos_cmdq.cmd_id) {
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
			mailbox_send(cmdq);
#if (0)		
			/* used to send command to linux*/
			/* Send the character through the hardware. */
			rtos_cmdqu_t = (cmdqu_t *)mailbox_context;

			debug_printf("RTOS_CMDQU_SEND %d\n", send_to_cpu);
			debug_printf("ip_id=%d cmd_id=%d param_ptr=%x\n",
				     cmdq->ip_id, cmdq->cmd_id,
				     (unsigned int)cmdq->param_ptr);
			debug_printf("mailbox_context = %x\n", mailbox_context);
			debug_printf("linux_cmdqu_t = %x\n", rtos_cmdqu_t);
			debug_printf("cmdq->ip_id = %d\n", cmdq->ip_id);
			debug_printf("cmdq->cmd_id = %d\n", cmdq->cmd_id);
			debug_printf("cmdq->block = %d\n", cmdq->block);
			debug_printf("cmdq->para_ptr = %x\n", cmdq->param_ptr);

			drv_spin_lock_irqsave(&mailbox_lock, flags);
			if (flags == MAILBOX_LOCK_FAILED) {
				printf("[%s][%d] drv_spin_lock_irqsave failed! ip_id = %d , cmd_id = %d\n",
				       cmdq->ip_id, cmdq->cmd_id);
				break;
			}

			for (valid = 0; valid < MAILBOX_MAX_NUM; valid++) {
				if (rtos_cmdqu_t->resv.valid.linux_valid == 0 &&
				    rtos_cmdqu_t->resv.valid.rtos_valid == 0) {
					// mailbox buffer context is 4 bytes write access
					int *ptr = (int *)rtos_cmdqu_t;

					cmdq->resv.valid.rtos_valid = 1;
					*ptr = ((cmdq->ip_id << 0) |
						(cmdq->cmd_id << 8) |
						(cmdq->block << 15) |
						(cmdq->resv.valid.linux_valid
						 << 16) |
						(cmdq->resv.valid.rtos_valid
						 << 24));
					rtos_cmdqu_t->param_ptr =
						cmdq->param_ptr;
					debug_printf(
						"rtos_cmdqu_t->linux_valid = %d\n",
						rtos_cmdqu_t->resv.valid
							.linux_valid);
					debug_printf(
						"rtos_cmdqu_t->rtos_valid = %d\n",
						rtos_cmdqu_t->resv.valid
							.rtos_valid);
					debug_printf(
						"rtos_cmdqu_t->ip_id =%x %d\n",
						&rtos_cmdqu_t->ip_id,
						rtos_cmdqu_t->ip_id);
					debug_printf(
						"rtos_cmdqu_t->cmd_id = %d\n",
						rtos_cmdqu_t->cmd_id);
					debug_printf(
						"rtos_cmdqu_t->block = %d\n",
						rtos_cmdqu_t->block);
					debug_printf(
						"rtos_cmdqu_t->param_ptr addr=%x %x\n",
						&rtos_cmdqu_t->param_ptr,
						rtos_cmdqu_t->param_ptr);
					debug_printf("*ptr = %x\n", *ptr);
					// clear mailbox
					mbox_reg->cpu_mbox_set[send_to_cpu]
						.cpu_mbox_int_clr.mbox_int_clr =
						(1 << valid);
					// trigger mailbox valid to rtos
					mbox_reg->cpu_mbox_en[send_to_cpu]
						.mbox_info |= (1 << valid);
					mbox_reg->mbox_set.mbox_set =
						(1 << valid);
					break;
				}
				rtos_cmdqu_t++;
			}
			drv_spin_unlock_irqrestore(&mailbox_lock, flags);
			if (valid >= MAILBOX_MAX_NUM) {
				printf("No valid mailbox is available\n");
			}
			break;
#endif
		}
	}
}
  
VOID tx_mailbox_driver_output_ISR(VOID)
{
  	printf("tx_mailbox_driver_output_ISR\n");
	unsigned char set_val;
	unsigned char valid_val;
	int i;
	cmdqu_t *cmdq;
	UINT ret;

	set_val = mbox_reg->cpu_mbox_set[RECEIVE_CPU].cpu_mbox_int_int.mbox_int;

	if (set_val) {
		for (i = 0; i < MAILBOX_MAX_NUM; i++) {
			valid_val = set_val & (1 << i);

			if (valid_val) {
				cmdqu_t rtos_cmdq;
				cmdq = (cmdqu_t *)(mailbox_context) + i;

				debug_printf("mailbox_context =%x\n",
					     mailbox_context);
				debug_printf("sizeof mailbox_context =%x\n",
					     sizeof(cmdqu_t));
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
					debug_printf("cmdq=%x\n", cmdq);
					debug_printf("cmdq->ip_id =%d\n",
						     rtos_cmdq.ip_id);
					debug_printf("cmdq->cmd_id =%d\n",
						     rtos_cmdq.cmd_id);
					debug_printf("cmdq->param_ptr =%x\n",
						     rtos_cmdq.param_ptr);
					debug_printf("cmdq->block =%x\n",
						     rtos_cmdq.block);
					debug_printf("cmdq->linux_valid =%d\n",
						     rtos_cmdq.resv.valid
							     .linux_valid);
					debug_printf(
						"cmdq->rtos_valid =%x\n",
						rtos_cmdq.resv.valid.rtos_valid);

					/* Notify thread last character transmit is
					complete. */
					if ((ret = tx_queue_send(&mailbox_queue, &rtos_cmdq, TX_NO_WAIT)) != TX_SUCCESS)
					{
						printf("rtos cmdq send failed: %d\n", ret);
					}

				} else
					printf("rtos cmdq is not valid %d, ip=%d , cmd=%d\n",
					       rtos_cmdq.resv.valid.rtos_valid,
					       rtos_cmdq.ip_id,
					       rtos_cmdq.cmd_id);
			}
		}
	}

	rpmsg_handler();

}

#endif


/*
 *
 * rpmsg init
 *
 */

#define SH_MEM_TOTAL_SIZE (6144)
// char rpmsg_lite_base[SH_MEM_TOTAL_SIZE];

char *rpmsg_lite_base = 0x8fff0000;

VOID tx_rpmsg_driver_initialize (VOID)
{
    struct rpmsg_lite_instance *my_rpmsg;
    struct rpmsg_lite_instance rpmsg_ctxt;
    int32_t result, test_counter = 0;

	my_rpmsg = rpmsg_lite_remote_init(rpmsg_lite_base, 0, RL_NO_FLAGS, &rpmsg_ctxt);

	printf("common_main 885\n");

	/* incomming interrupt changes state to state_created_channel */
	rpmsg_lite_wait_for_link_up(my_rpmsg, RL_BLOCK);
	printf("common_main 895\n");
	result = rpmsg_lite_deinit(my_rpmsg);
	printf("common_main 897\n");
}




/*
 *
 * rpmsg send & recieve
 *
 */

#define TC_TRANSFER_COUNT 10
#define DATA_LEN 45
#define TC_LOCAL_EPT_ADDR (30)
#define TC_REMOTE_EPT_ADDR (40)
#define RPMSG_LITE_NS_ANNOUNCE_STRING "rpmsg-test-channel"

struct rpmsg_lite_endpoint *volatile my_ept = NULL;
struct rpmsg_lite_ept_static_context my_ept_ctxt;

rpmsg_queue_handle my_queue = NULL;
rpmsg_static_queue_ctxt my_queue_ctxt = {0};
uint8_t my_rpmsg_queue_storage[RL_ENV_QUEUE_STATIC_STORAGE_SIZE] = {0};
struct rpmsg_lite_instance *volatile my_rpmsg = NULL;
struct rpmsg_lite_instance rpmsg_ctxt = {0};
rpmsg_ns_handle ns_handle = NULL;
rpmsg_ns_static_context my_ns_ctxt = {0};

static void app_nameservice_isr_cb(uint32_t new_ept, const char *new_ept_name, uint32_t flags, void *user_data)
{
}

// utility: initialize rpmsg and environment
// and wait for default channel
int32_t ts_init_rpmsg(void)
{
    env_init();
    my_rpmsg = rpmsg_lite_remote_init(rpmsg_lite_base, 0, RL_NO_FLAGS, &rpmsg_ctxt);
	
	rpmsg_lite_wait_for_link_up(my_rpmsg, RL_BLOCK);
    
    /* wait for a while to allow the primary side to bind_ns and register the NS callback */
    env_sleep_msec(200);

    ns_handle = rpmsg_ns_bind(my_rpmsg, app_nameservice_isr_cb, ((void *)0), &my_ns_ctxt);
    return 0;
}

// utility: deinitialize rpmsg and environment
int32_t ts_deinit_rpmsg(void)
{
    rpmsg_ns_unbind(my_rpmsg, ns_handle);
    ns_handle = NULL;
    env_memset(&my_ns_ctxt, 0, sizeof(rpmsg_ns_static_context));
    rpmsg_lite_deinit(my_rpmsg);
    my_rpmsg = NULL;
    env_memset(&rpmsg_ctxt, 0, sizeof(struct rpmsg_lite_instance));
    return 0;
}

// utility: create number of epts
int32_t ts_create_epts(rpmsg_queue_handle queues[], uint8_t queues_storage[], rpmsg_static_queue_ctxt queues_ctxt[], struct rpmsg_lite_endpoint *volatile epts[], struct rpmsg_lite_ept_static_context epts_ctxt[], int32_t count, int32_t init_addr)
{

    // invalid params for rpmsg_queue_create

    for (int32_t i = 0; i < count; i++)
    {
        queues[i] = rpmsg_queue_create(my_rpmsg, &queues_storage[i], &queues_ctxt[i]);
        epts[i] = rpmsg_lite_create_ept(my_rpmsg, init_addr + i, rpmsg_queue_rx_cb, queues[i], &epts_ctxt[i]);
    }
    return 0;
}

// utility: destroy number of epts
int32_t ts_destroy_epts(rpmsg_queue_handle queues[], struct rpmsg_lite_endpoint *volatile epts[], int32_t count)
{
    // invalid params for rpmsg_queue_destroy

    for (int32_t i = 0; i < count; i++)
    {
        rpmsg_queue_destroy(my_rpmsg, queues[i]);
        rpmsg_lite_destroy_ept(my_rpmsg, epts[i]);
    }
    return 0;
}

int32_t pattern_cmp(char *buffer, char pattern, int32_t len)
{
    for (int32_t i = 0; i < len; i++)
        if (buffer[i] != pattern)
            return -1;
    return 0;
}

/******************************************************************************
 * Test case 1
 * - verify simple transport between default epts of default channels
 * - verify simple nocopy transport between default epts of default channels
 * - verify simple transport between custom created epts of default channels
 * - verify simple nocopy transport between custom created epts of default channels
 *****************************************************************************/
void tc_1_receive_send(void)
{
    int32_t result = 0;
    char data[DATA_LEN] = {0};
    void *data_addr = NULL;
    uint32_t src;
    uint32_t len;

    for (int32_t i = 0; i < TC_TRANSFER_COUNT; i++)
    {
        result = rpmsg_queue_recv(my_rpmsg, my_queue, &src, data, DATA_LEN, &len, RL_BLOCK);
        result = pattern_cmp(data, i, DATA_LEN);
    }

    for (int32_t i = 0; i < TC_TRANSFER_COUNT; i++)
    {
        result = rpmsg_queue_recv_nocopy(my_rpmsg, my_queue, &src, (char **)&data_addr, &len, RL_BLOCK);
        result = pattern_cmp(data_addr, i, DATA_LEN);
        result = rpmsg_queue_nocopy_free(my_rpmsg, data_addr);
    }

    /* for invalid length remote receive */
    result = rpmsg_lite_send(my_rpmsg, my_ept, TC_REMOTE_EPT_ADDR, data, DATA_LEN, RL_BLOCK);

    for (int32_t i = 0; i < TC_TRANSFER_COUNT; i++)
    {
        env_memset(data, i, DATA_LEN);
        result = rpmsg_lite_send(my_rpmsg, my_ept, TC_REMOTE_EPT_ADDR, data, DATA_LEN, RL_BLOCK);
    }

    for (int32_t i = 0; i < TC_TRANSFER_COUNT; i++)
    {
        env_memset(data, i, DATA_LEN);
        result = rpmsg_lite_send(my_rpmsg, my_ept, TC_REMOTE_EPT_ADDR, data, DATA_LEN, RL_BLOCK);
    }

    // send message to non-existing endpoint address, message will be dropped on the receiver side
    result = rpmsg_lite_send(my_rpmsg, my_ept, TC_REMOTE_EPT_ADDR + 1, data, DATA_LEN, RL_BLOCK);

    // send - invalid rpmsg_lite_dev
    result = rpmsg_lite_send(RL_NULL, my_ept, TC_REMOTE_EPT_ADDR, data, DATA_LEN, RL_BLOCK);

    // invalid params for send
    result = rpmsg_lite_send(my_rpmsg, NULL, TC_REMOTE_EPT_ADDR, data, DATA_LEN, RL_BLOCK);
    result = rpmsg_lite_send(my_rpmsg, my_ept, TC_REMOTE_EPT_ADDR, NULL, DATA_LEN, RL_BLOCK);
    result = rpmsg_lite_send(my_rpmsg, my_ept, TC_REMOTE_EPT_ADDR, data, 0xFFFFFFFF, RL_BLOCK);

    // invalid params for receive
    result = rpmsg_queue_recv(RL_NULL, my_queue, &src, data, DATA_LEN, &len, RL_BLOCK);
    result = rpmsg_queue_recv(my_rpmsg, RL_NULL, &src, data, DATA_LEN, &len, RL_BLOCK);
    result = rpmsg_queue_recv(my_rpmsg, my_queue, &src, RL_NULL, DATA_LEN, &len, RL_BLOCK);
    result = rpmsg_queue_recv(my_rpmsg, my_queue, &src, data, 0, &len, RL_BLOCK);

    // invalid params for receive_nocopy
    result = rpmsg_queue_recv_nocopy(RL_NULL, my_queue, &src, (char **)&data_addr, &len, RL_BLOCK);
    result = rpmsg_queue_recv_nocopy(my_rpmsg, RL_NULL, &src, (char **)&data_addr, &len, RL_BLOCK);
    result = rpmsg_queue_recv_nocopy(my_rpmsg, my_queue, &src, RL_NULL, &len, RL_BLOCK);

    // invalid params for receive_nocopy_free
    result = rpmsg_queue_nocopy_free(RL_NULL, data_addr);
    result = rpmsg_queue_nocopy_free(my_rpmsg, RL_NULL);

    // invalid params for rpmsg_lite_release_rx_buffer - this should not be used in an app and rpmsg_queue_nocopy_free should be used instead
    result = rpmsg_lite_release_rx_buffer(RL_NULL, data_addr);
    result = rpmsg_lite_release_rx_buffer(my_rpmsg, RL_NULL);
}

// VOID tx_send_and_recieve_run_tests(VOID)
void tx_send_and_recieve_run_tests(ULONG thread_input)
{
    /* Remove compiler warning about unused parameter. */
	(void)thread_input;
	
	int32_t result = 0;
    struct rpmsg_lite_endpoint fake_ept = {0};
    struct rpmsg_lite_instance fake_rpmsg = {0};
    uint32_t bufer_size = 1;

	printf("common_main 1078\n");

    // call send before rpmsg_lite is initialized
    result = rpmsg_lite_send(&fake_rpmsg, &fake_ept, TC_REMOTE_EPT_ADDR, (char*)0x12345678, DATA_LEN, RL_BLOCK);
    result = rpmsg_lite_send_nocopy(&fake_rpmsg, &fake_ept, TC_REMOTE_EPT_ADDR, (char*)0x12345678, DATA_LEN);

	printf("common_main 1084\n");

    result = ts_init_rpmsg();
    result = ts_create_epts(&my_queue, &my_rpmsg_queue_storage[0], &my_queue_ctxt, &my_ept, &my_ept_ctxt, 1, TC_LOCAL_EPT_ADDR);

	printf("common_main 1089\n");

    if (!result)
    {
        tc_1_receive_send();
    }
#if (0)
    ts_destroy_epts(&my_queue, &my_ept, 1);

	printf("common_main 1097\n");

    my_queue = NULL;
    env_memset(&my_rpmsg_queue_storage, 0, RL_ENV_QUEUE_STATIC_STORAGE_SIZE);
    env_memset(&my_queue_ctxt, 0, sizeof(rpmsg_static_queue_ctxt));

	printf("common_main 1103\n");

    my_ept = NULL;
    env_memset(&my_ept_ctxt, 0, sizeof(struct rpmsg_lite_ept_static_context));
    result = ts_deinit_rpmsg();
#endif
	printf("common_main 1109\n");
}