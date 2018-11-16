
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <amqp.h>
#include <amqp_framing.h>
#include <stdint.h>
#include <Windows.h>

#include "message_queue_utils.h"

BEGIN_NAMESPACE;

int die(const char *fmt, ...) 
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    return (1);
}

int die_on_error(int x, char const *context)
{
    if (x < 0) 
    {
        fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x));
        return (1);
    }

    return 0;
}

int die_on_amqp_error(amqp_rpc_reply_t x, char const *context) 
{
    switch (x.reply_type)
    {
    case AMQP_RESPONSE_NORMAL:
        return 0;

    case AMQP_RESPONSE_NONE:
        fprintf(stderr, "%s: missing RPC reply type!\n", context);
        break;

    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
        fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x.library_error));
        break;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
        switch (x.reply.id) {
        case AMQP_CONNECTION_CLOSE_METHOD: {
            amqp_connection_close_t *m =
                (amqp_connection_close_t *)x.reply.decoded;
            fprintf(stderr, "%s: server connection error %uh, message: %.*s\n",
                context, m->reply_code, (int)m->reply_text.len,
                (char *)m->reply_text.bytes);
            break;
                                           }
        case AMQP_CHANNEL_CLOSE_METHOD: {
            amqp_channel_close_t *m = (amqp_channel_close_t *)x.reply.decoded;
            fprintf(stderr, "%s: server channel error %uh, message: %.*s\n",
                context, m->reply_code, (int)m->reply_text.len,
                (char *)m->reply_text.bytes);
            break;
                                        }
        default:
            fprintf(stderr, "%s: unknown server error, method id 0x%08X\n",
                context, x.reply.id);
            break;
        }
        break;
    }

    return 1;
}

static void dump_row(long count, int numinrow, int *chs) {
    int i;

    printf("%08lX:", count - numinrow);

    if (numinrow > 0) {
        for (i = 0; i < numinrow; i++) {
            if (i == 8) {
                printf(" :");
            }
            printf(" %02X", chs[i]);
        }
        for (i = numinrow; i < 16; i++) {
            if (i == 8) {
                printf(" :");
            }
            printf("   ");
        }
        printf("  ");
        for (i = 0; i < numinrow; i++) {
            if (isprint(chs[i])) {
                printf("%c", chs[i]);
            } else {
                printf(".");
            }
        }
    }
    printf("\n");
}

static int rows_eq(int *a, int *b) {
    int i;

    for (i = 0; i < 16; i++)
        if (a[i] != b[i]) {
            return 0;
        }

        return 1;
}

uint64_t now_microseconds(void) 
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return (((uint64_t)ft.dwHighDateTime << 32) | (uint64_t)ft.dwLowDateTime) /
        10;
}

void microsleep(int usec) { Sleep(usec / 1000); }


void amqp_dump(void const *buffer, size_t len) {
    unsigned char *buf = (unsigned char *)buffer;
    long count = 0;
    int numinrow = 0;
    int chs[16];
    int oldchs[16] = {0};
    int showed_dots = 0;
    size_t i;

    for (i = 0; i < len; i++) {
        int ch = buf[i];

        if (numinrow == 16) {
            int j;

            if (rows_eq(oldchs, chs)) {
                if (!showed_dots) {
                    showed_dots = 1;
                    printf(
                        "          .. .. .. .. .. .. .. .. : .. .. .. .. .. .. .. ..\n");
                }
            } else {
                showed_dots = 0;
                dump_row(count, numinrow, chs);
            }

            for (j = 0; j < 16; j++) {
                oldchs[j] = chs[j];
            }

            numinrow = 0;
        }

        count++;
        chs[numinrow++] = ch;
    }

    dump_row(count, numinrow, chs);

    if (numinrow != 0) {
        printf("%08lX:\n", count);
    }
}

END_NAMESPACE;
