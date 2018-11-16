#ifndef MESSAGE_QUEUE_PRODUCER_H_
#define MESSAGE_QUEUE_PRODUCER_H_

#include <string>

#include "app_defs.h"

//////////////////////////////////////////////////////////////////////////
BEGIN_NAMESPACE;

//message_queue producer
class EXPORT CMQProducer
{
public:

    /////////////////////////////////////////////////////////////////
    ///  \brief Constructor
    ///
    ///  \param[in]     None
    ///  \param[out]   None
    ///  \return          None
    ///  \pre \e  
    /////////////////////////////////////////////////////////////////
    CMQProducer();

    /////////////////////////////////////////////////////////////////
    ///  \brief destructor
    ///
    ///  \param[in]     None
    ///  \param[out]   None
    ///  \return          None
    ///  \pre \e  
    /////////////////////////////////////////////////////////////////
    virtual ~CMQProducer(){}
                                            
    int Initialize(
        const std::string& sHostname, 
        int nPort, 
        const std::string& sUser, 
        const std::string& sPassword,
        const std::string& sQueuename);
    int UnInitialize();

    void SendMessageToMQ(const std::string& sData);

private:
    void*           m_amqp_connection;
    void*           m_msocket;
    std::string     m_sQueuename;
  
};

END_NAMESPACE

#endif 
