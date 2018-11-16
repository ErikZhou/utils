#ifndef MESSAGE_QUEUE_CONSUMER_H_
#define MESSAGE_QUEUE_CONSUMER_H_

#include <string>
#include <functional>

#include "app_defs.h"

//////////////////////////////////////////////////////////////////////////
BEGIN_NAMESPACE;

//message_queue_consumer
class UAI_EXPORT CMQConsumer
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
    CMQConsumer();

    /////////////////////////////////////////////////////////////////
    ///  \brief destructor
    ///
    ///  \param[in]     None
    ///  \param[out]   None
    ///  \return          None
    ///  \pre \e  
    /////////////////////////////////////////////////////////////////
    virtual ~CMQConsumer(){}
                                            
    void Run();

    int Initialize(
        const std::string& sHostname, 
        int nPort, 
        const std::string& sUser, 
        const std::string& sPassword,
        const std::string& sQueuename);
    int UnInitialize();

    void BindCallback(std::function<int(void*,std::string)> fn) 
    {
        m_pCallback = std::bind(fn, std::placeholders::_1, std::placeholders::_2);
    }

    void SetCaller(void* pCaller)
    {
        m_pCaller = pCaller;
    }

private:
    int MakeCallback(std::string str) 
    {
        int iRet = 0;
        if (m_pCallback)
        {
            iRet = m_pCallback((void*)m_pCaller, str);
        }
        return iRet;
    }

    void*                                   m_amqp_connection;
    void*                                   m_msocket;
    void*                                   m_pCaller;
    std::function<int(void*,std::string)>   m_pCallback;
};

END_NAMESPACE

#endif 
    ///\}
