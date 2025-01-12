#include "mailbox.h"

DEFINE_CVI_SPINLOCK(mailbox_lock, SPIN_MBOX);

void mailbox_send(cmdqu_t *cmdq){
    
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
    cmdqu_t rtos_cmdq = *cmdq;
    
    int flags;
	int valid;
	int send_to_cpu = SEND_TO_CPU1;

	rtos_cmdq.resv.valid.rtos_valid = 1;
	rtos_cmdq.resv.valid.linux_valid = 0;
    
	/* used to send command to linux*/
	/* Send the character through the hardware. */
	rtos_cmdqu_t = (cmdqu_t *)mailbox_context;

/*
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
*/
	drv_spin_lock_irqsave(&mailbox_lock, flags);
	if (flags == MAILBOX_LOCK_FAILED) {
		printf("[%s][%d] drv_spin_lock_irqsave failed! ip_id = %d , cmd_id = %d\n",
				cmdq->ip_id, cmdq->cmd_id);
		return;
	}

	// trigger mailbox valid to rtos
	mbox_reg->cpu_mbox_en[send_to_cpu]
		.mbox_info |= (1 << 0);
	mbox_reg->mbox_set.mbox_set =
		(1 << 0);
/*
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
*/

	drv_spin_unlock_irqrestore(&mailbox_lock, flags);
	if (valid >= MAILBOX_MAX_NUM) {
		printf("No valid mailbox is available\n");
	}

}