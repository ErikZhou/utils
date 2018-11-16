
#include "message_queue_producer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <Windows.h>

#include "message_queue_utils.h"

BEGIN_NAMESPACE;

#define SUMMARY_EVERY_US 1000000

CMQProducer::CMQProducer():m_amqp_connection(nullptr),m_msocket(nullptr)
{

}

void CMQProducer::SendMessageToMQ(const std::string& sData)
{
    const std::string routing_key = m_sQueuename + "_ROUTING_KEY";

    amqp_connection_state_t conn = (amqp_connection_state_t)m_amqp_connection;
    uint64_t start_time = now_microseconds();
    int sent = 0;
    int previous_sent = 0;
    uint64_t previous_report_time = start_time;
    uint64_t next_summary_time = start_time + SUMMARY_EVERY_US;

    amqp_bytes_t message_bytes;

    message_bytes.len = sData.length();
    message_bytes.bytes = (void*)(sData.c_str());

    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("text/plain");
    props.delivery_mode = AMQP_DELIVERY_PERSISTENT; /* persistent delivery mode */

    {
        uint64_t now = now_microseconds();

        die_on_error(amqp_basic_publish(conn, 1, amqp_cstring_bytes("amq.direct"),
            amqp_cstring_bytes(routing_key.c_str()), 0, 0, 
            &props,
            message_bytes),
            "Publishing");
        sent++;
        if (now > next_summary_time) 
        {
            //printf("%d ms: Sent %d - %d since last report (%d Hz)\n",
            //       (int)(now - start_time) / 1000, sent, countOverInterval,
            //       (int)intervalRate);

            //printf("%d ms: Sent %d - %s\n",(int)(now - start_time) / 1000, sent, sData.c_str());

            previous_sent = sent;
            previous_report_time = now;
            next_summary_time += SUMMARY_EVERY_US;
        }
    }

    {
        //uint64_t stop_time = now_microseconds();
        //int total_delta = (int)(stop_time - start_time);
        //printf("Total time, milliseconds: %d\n", total_delta / 1000);
        printf("Data sent:%s\n",sData.c_str());
    }
}

int CMQProducer::Initialize(
    const std::string& sHostname, 
    int nPort, 
    const std::string& sUser, 
    const std::string& sPassword,
    const std::string& sQueuename) 
{
    m_sQueuename =  sQueuename;

    int status;
    int rate_limit;
    int message_count;
    amqp_socket_t *socket = NULL;
    amqp_connection_state_t conn;
    int iRet = 0;

    rate_limit = 1000;//atoi(argv[3]);
    message_count = 0;//atoi(argv[4]);

    conn = amqp_new_connection();
    m_amqp_connection = (void*)conn;

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


    iRet = die_on_amqp_error(amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, sUser.c_str(), sPassword.c_str()), "Logging in");
    if (0 != iRet)
    {
        return iRet;
    }
    amqp_channel_open(conn, 1);
    iRet = die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
    if (0 != iRet)
    {
        return iRet;
    }
    rate_limit = 1000;

    //send_batch("test queue", rate_limit, message_count);

    return 0;
}

int CMQProducer::UnInitialize()
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
