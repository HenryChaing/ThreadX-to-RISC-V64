#include "mailbox.h"

DEFINE_CVI_SPINLOCK(mailbox_lock, SPIN_MBOX);

void mailbox_send(int cmd_id){

    /* mailbox parameters */
	volatile struct mailbox_set_register *mbox_reg;
	volatile struct mailbox_done_register *mbox_done_reg;
	volatile unsigned long *mailbox_context; // mailbox buffer context is 64 Bytess

	/* initialize mailbox global variable */
	unsigned int reg_base = MAILBOX_REG_BASE;
	mbox_reg = (struct mailbox_set_register *)reg_base;
	mbox_done_reg = (struct mailbox_done_register *)(reg_base + 2);
	mailbox_context = (unsigned long *)(MAILBOX_REG_BUFF);
	
	cmdqu_t *rtos_cmdqu_t;
    cmdqu_t rtos_cmdq;
    
    int flags;
	int valid;
	int send_to_cpu = SEND_TO_CPU1;

	rtos_cmdq.resv.valid.rtos_valid = 1;
	rtos_cmdq.resv.valid.linux_valid = 0;
	rtos_cmdq.block = 0;
	rtos_cmdq.ip_id = 0;
	rtos_cmdq.cmd_id = cmd_id;
	rtos_cmdq.param_ptr = 0xf;
    
	/* used to send command to linux*/
	/* Send the character through the hardware. */
	rtos_cmdqu_t = (cmdqu_t *)mailbox_context;

	drv_spin_lock_irqsave(&mailbox_lock, flags);
	if (flags == MAILBOX_LOCK_FAILED) {
		printf("[%s][%d] drv_spin_lock_irqsave failed! ip_id = %d , cmd_id = %d\n",
				rtos_cmdq.ip_id, rtos_cmdq.cmd_id);
		return;
	}

	// trigger mailbox valid to rtos
	for (valid = 0; valid < MAILBOX_MAX_NUM; valid++) {
		if (rtos_cmdqu_t->resv.valid.linux_valid == 0 &&
			rtos_cmdqu_t->resv.valid.rtos_valid == 0) {

				// mailbox buffer context is 4 bytes write access
				int *ptr = (int *)rtos_cmdqu_t;

				rtos_cmdq.resv.valid.rtos_valid = 1;
				*ptr = ((rtos_cmdq.ip_id << 0) |
					(rtos_cmdq.cmd_id << 8) |
					(rtos_cmdq.block << 15) |
					(rtos_cmdq.resv.valid.linux_valid
						<< 16) |
					(rtos_cmdq.resv.valid.rtos_valid
						<< 24));
				rtos_cmdqu_t->param_ptr =
					rtos_cmdq.param_ptr;



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

}