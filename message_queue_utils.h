
#ifndef MESSAGE_QUEUE_UTILS_H_
#define MESSAGE_QUEUE_UTILS_H_

#include "app_defs.h"

BEGIN_NAMESPACE;

int die(const char *fmt, ...);
extern int die_on_error(int x, char const *context);
extern int die_on_amqp_error(amqp_rpc_reply_t x, char const *context);

extern void amqp_dump(void const *buffer, size_t len);

extern uint64_t now_microseconds(void);
extern void microsleep(int usec);

END_NAMESPACE;

#endif


