#include "rtos_cmdqu.h"
#include "cvi_spinlock.h"
#include "cvi_mailbox.h"

#define __DEBUG__
#ifdef __DEBUG__
#define debug_printf printf
#else
#define debug_printf(...)
#endif

#define MAILBOX_REG_BASE                        0x01900000
#define MAILBOX_REG_BUFF                        (MAILBOX_REG_BASE + 0x0400)
#define SPINLOCK_REG_BASE                       (MAILBOX_REG_BASE + 0x00c0)

void mailbox_send(cmdqu_t *cmdq);