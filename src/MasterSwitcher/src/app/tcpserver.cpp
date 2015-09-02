#include "tcpserver.h"

#if QT
#include "log.h"
#define DEBUG_OUTPUT APP_LOG
#else
#define DEBUG_OUTPUT printf
#endif


THREAD_API tcpserver_receive_thread(void *param)
{
    TcpServer *server = (TcpServer *)param;
    while(true)
    {
        Sleep(10);
        THREAD_WAITEXIT();

        if(server->isExiting())
        {
            break;
        }

        bool isvalid = server->checkStarted();
        if(!isvalid)
        {
            continue;
        }

        server->waitForNewConnection();
        Sleep(10);

    }//true

    return NULL;
}

THREAD_API tcpserver_guard_thread(void *param)
{
    TcpServer *server = (TcpServer *)param;
    while(true)
    {
        Sleep(10);
        THREAD_WAITEXIT();

        server->deleteCloseClient();
        Sleep(500);

    }//true

    return NULL;
}

TcpServer::TcpServer()
    : BaseObject()
    , ITcpClient()
{
    memset(&mTcp, 0, sizeof(tcp_t));
    sprintf(mTcp.hostname, "0.0.0.0");
    mTcp.fd = -1;
    mTcp.local_if = 1;

    mStarted = false;
    mExiting = false;

    int ret;
    THREAD_CREATE(&receive_thread, tcpserver_receive_thread, this, ret);
    if(ret)
    {
        THREAD_RUN(receive_thread, false);
    }
    THREAD_CREATE(&guard_thread, tcpserver_guard_thread, this, ret);
    if(ret)
    {
        THREAD_RUN(guard_thread, false);
    }
}

TcpServer::~TcpServer()
{
    THREAD_CLOSE(guard_thread);
    THREAD_CLOSE(receive_thread);
}

void TcpServer::setPort(int port)
{
    enter();
    mPort = port;
    leave();
}

void TcpServer::setHandler(ITcpServer *handler)
{
    enter();
    mHandler = handler;
    leave();
}

int TcpServer::start()
{
    int ret = 0;

    enter();
    mTcp.port = mPort;
    tcp_connect(&mTcp);
    ret = tcp_isvalid(&mTcp);
    DEBUG_OUTPUT("listen %d %s \n",  mTcp.port, ret?"success":"fail");
    if(ret)
    {
        mStarted = true;
    }
    leave();

    return ret;
}

void TcpServer::close()
{
    enter();
    mExiting = true;
    leave();

    Sleep(200);
}

bool TcpServer::checkStarted()
{
    bool result = false;

    enter();
    if(mStarted && tcp_isvalid(&mTcp))
    {
        result = true;
    }
    leave();

    return result;
}

void TcpServer::waitForNewConnection()
{
    bool found = false;
    char *ip = NULL;
    USHORT port = 0;
    SOCKET_HANDLE in_fd;
    enter();

    fd_set fds;
    FD_ZERO(&fds);

    {
        FD_SET(mTcp.fd, &fds);

        //设置等待时间100ms
        struct timeval tv;
        tv.tv_sec =0;
        tv.tv_usec = 100000;

        int ret = select(mTcp.fd+1, &fds, NULL, NULL, &tv);
        if(ret > 0)
        {
            bool received = FD_ISSET(mTcp.fd, &fds);

            if(received)
            {

                struct sockaddr_in in;
                memset(&in, 0, sizeof(struct sockaddr_in));
                socklen_t in_len = sizeof(struct sockaddr_in);
                in_fd=accept(mTcp.fd, (struct sockaddr *)(&in), &in_len);
                if(0<(int)(in_fd))
                {
                    ip = inet_ntoa(in.sin_addr);
                    port = ntohs(in.sin_port);

                    found = true;
                }
            }//receive
        }//ret
    }
    leave();
    if(found)
    {
        DEBUG_OUTPUT("connect from %s:%u ...\n", ip, port);
        TcpClient *client = new TcpClient();
        client->setIp(ip);
        client->setPort(port);
        client->setFd(in_fd);
        client->setEnableReconnect(false);

        client->setHandler(this);
        mClients.push_back(client);

//        ITcpClient *handler = NULL;

//        addNewConnection(&handler);
//        if(NULL!=handler)
//        {
//            DEBUG_OUTPUT("[TcpServer]handler set\n");
//            client->setHandler(handler);
//        }
        client->start(false);
    }

}

bool TcpServer::isExiting()
{
    bool result = false;
    enter();
    result = mExiting;
    leave();
    return result;
}

void TcpServer::deleteCloseClient()
{
    enter();
    vector<TcpClient *>::iterator i = mClients.begin();
    vector<TcpClient *>::iterator t;
    while(i!=mClients.end())
    {
        break;
        if(mExiting)
        {
            break;
        }
        TcpClient *client = *i;
        bool found = false;
        if(client->isExiting())
        {
            t = i;
            found = true;
        }
        i++;
        if(found)
        {
            mClients.erase(t);
        }
    }
    DEBUG_OUTPUT("[TcpServer]client count:%d \n", mClients.size());

    leave();
}

void TcpServer::addNewConnection(void *tcp)
{
    DEBUG_OUTPUT("[TcpServer]add client 1 \n");
    if(NULL!=mHandler)
    {
        mHandler->addNewClient(tcp);
    }
    leave();
    return;
}

void TcpServer::tcpClientReceiveData(void *tcp, char *buffer, int size)
{

}

void TcpServer::tcpClientConnected(void *tcp)
{
}

void TcpServer::tcpClientDisconnected(void *tcp)
{
    enter();
    TcpClient *client = (TcpClient *)tcp;
    client->close();
    delete client;
    leave();

}

void TcpServer::tcpClientError(void *tcp)
{

}

