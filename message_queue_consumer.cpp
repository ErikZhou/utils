#include "message_queue_consumer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>

#include "message_queue_utils.h"

BEGIN_NAMESPACE;

#define SUMMARY_EVERY_US 1000000


CMQConsumer::CMQConsumer():m_amqp_connection(nullptr),m_msocket(nullptr),m_pCaller(nullptr)
{

}

void CMQConsumer::Run() 
{                           
    amqp_connection_state_t conn=(amqp_connection_state_t)m_amqp_connection;
    uint64_t start_time = now_microseconds();
    int received = 0;
    int previous_received = 0;
    uint64_t previous_report_time = start_time;
    uint64_t next_summary_time = start_time + SUMMARY_EVERY_US;

    amqp_frame_t frame;

    uint64_t now;

    for (;;)
    {
        amqp_rpc_reply_t ret;
        amqp_envelope_t envelope;

        now = now_microseconds();
        //Sleep(1000);
        if (now > next_summary_time)
        {

            //printf("%d ms: Received %d - %d since last report (%d Hz)\n",
            //       (int)(now - start_time) / 1000, received, countOverInterval,
            //       (int)intervalRate);

            previous_received = received;
            previous_report_time = now;
            next_summary_time += SUMMARY_EVERY_US;
        }

        amqp_maybe_release_buffers(conn);
        ret = amqp_consume_message(conn, &envelope, NULL, 0);

        if (AMQP_RESPONSE_NORMAL != ret.reply_type)
        {
            if (AMQP_RESPONSE_LIBRARY_EXCEPTION == ret.reply_type &&
                AMQP_STATUS_UNEXPECTED_STATE == ret.library_error) {
                    if (AMQP_STATUS_OK != amqp_simple_wait_frame(conn, &frame)) {
                        return;
                    }

                    if (AMQP_FRAME_METHOD == frame.frame_type) {
                        switch (frame.payload.method.id) {
                        case AMQP_BASIC_ACK_METHOD:
                            /* if we've turned publisher confirms on, and we've published a
                            * message here is a message being confirmed.
                            */
                            break;
                        case AMQP_BASIC_RETURN_METHOD:
                            /* if a published message couldn't be routed and the mandatory
                            * flag was set this is what would be returned. The message then
                            * needs to be read.
                            */
                            {
                                amqp_message_t message;
                                ret = amqp_read_message(conn, frame.channel, &message, 0);
                                if (AMQP_RESPONSE_NORMAL != ret.reply_type) {
                                    return;
                                }

                                amqp_destroy_message(&message);
                            }

                            break;

                        case AMQP_CHANNEL_CLOSE_METHOD:
                            /* a channel.close method happens when a channel exception occurs,
                            * this can happen by publishing to an exchange that doesn't exist
                            * for example.
                            *
                            * In this case you would need to open another channel redeclare
                            * any queues that were declared auto-delete, and restart any
                            * consumers that were attached to the previous channel.
                            */
                            return;

                        case AMQP_CONNECTION_CLOSE_METHOD:
                            /* a connection.close method happens when a connection exception
                            * occurs, this can happen by trying to use a channel that isn't
                            * open for example.
                            *
                            * In this case the whole connection must be restarted.
                            */
                            return;

                        default:
                            fprintf(stderr, "An unexpected method was received %u\n",
                                frame.payload.method.id);
                            return;
                        }
                    }
            }

        }
        else
        {          
            size_t message_size = (envelope.message.body.len + 1)* sizeof(char);
            char* pMess = (char*)malloc(message_size);
            memset(pMess,0,message_size);
            //pMess[x] = envelope.message.body.bytes[x];
            memcpy(pMess, envelope.message.body.bytes, envelope.message.body.len* sizeof(char));
            //  pMess[0]='1';
            printf("received:%d,message:%s\n",received, pMess);
            int iRet = MakeCallback(std::string(pMess));
            if (1 == iRet)
            {
                //_LOG_DEV_WARNING("Exit as got cmd 1");
                printf("Exit as got cmd 1\n");
            }

            free(pMess);
            amqp_destroy_envelope(&envelope);
        }

        received++;
    }
}

int CMQConsumer::Initialize(
    const std::string& sHostname, 
    int nPort,
    const std::string& sUser, 
    const std::string& sPassword,
    const std::string& sQueuename)
{
    int status;
    char const *exchange;
    //char const *bindingkey;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;

    amqp_bytes_t queuename;
    queuename.bytes = (void*)sQueuename.c_str(); 
    queuename.len = strlen(sQueuename.c_str());

    //hostname = "10.9.19.129";//argv[1];
    //port = 5672;//atoi(argv[2]);
    exchange = "amq.direct";   /* argv[3]; */

    const std::string bindingkey = sQueuename + "_ROUTING_KEY";


    conn = amqp_new_connection();


    socket = amqp_tcp_socket_new(conn);
    if (!socket)
    {
        die("creating TCP socket");
        return 1;
    }
    m_msocket = (void*)socket;

    status = amqp_socket_open(socket, sHostname.c_str(), nPort);
    if (status) 
    {
        die("opening TCP socket");
        return 2;
    }

    die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,sUser.c_str(), sPassword.c_str()),
        "Logging in");
    amqp_channel_open(conn, 1);      
    m_amqp_connection = (void*)conn;
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

    //TODO check queue name exist or not? 
    //by qiangqiang.zhou@20181106
    {
        //amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0, 0, 0, 1, amqp_empty_table);
        //amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, queuename, 0, 0, 0, 0, amqp_empty_table);

        //die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
        //queuename = amqp_bytes_malloc_dup(r->queue);
        //if (queuename.bytes == NULL) 
        //{
        //    fprintf(stderr, "Out of memory while copying queue name");
        //    return 1;
        //}
    }

    amqp_queue_bind(conn, 1, queuename, amqp_cstring_bytes(exchange),
        amqp_cstring_bytes(bindingkey.c_str()), amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

    bool no_ack = false;
    amqp_basic_consume(conn, 1, queuename, amqp_empty_bytes, 0, no_ack, 0,
        amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");
    return 0;
}

int CMQConsumer::UnInitialize()
{
    amqp_connection_state_t conn=(amqp_connection_state_t)m_amqp_connection;
    die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
        "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
        "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");
    return 0;
}

END_NAMESPACE;


