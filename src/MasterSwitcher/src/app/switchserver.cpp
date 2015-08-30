#include "switchserver.h"

#include "log.h"

SwitchServer::SwitchServer(Gateway *gateway, LocalPC *local, RemotePC *remote)
    : IGateway()
    , IRemotePC()
    , mLocal(local), mRemote(remote), mGateway(gateway)
{
}

SwitchServer::~SwitchServer()
{

}

int SwitchServer::start()
{
    mGateway->setStateChange(this);
    mRemote->setHandler(this);

    mLocal->start();
    mGateway->start();
    mRemote->start();

    while(true)
    {
        Sleep(100);
    }

    return 0;
}

void SwitchServer::gatewayStateChanged()
{
    if(GATEWAY_OFFLINE==mGateway->getState())
    {
        mLocal->makeSlave();
    }
}

void SwitchServer::canBeMaster()
{
    mGateway->checkOnline();
    if(GATEWAY_ONLINE==mGateway->getState())
    {
        APP_LOG("[SERVER]remote is slave \n");
        mLocal->makeMaster();
    }
}

void SwitchServer::switchMaster()
{
    printGatewayState();
    LocalState state = adjudge(mGateway, mRemote);
    APP_LOG("[SERVER]make local %s\n", LOCAL_MASTER==state?"MASTER":"SLAVE");
    printf("[SERVER]make local %s\n", LOCAL_MASTER==state?"MASTER":"SLAVE");
    switch (state) {
    case LOCAL_MASTER:
        mLocal->makeMaster();
        break;
    default:
        mLocal->makeSlave();
        break;
    }
}

void SwitchServer::printGatewayState()
{    
    APP_LOG("[GATEWAY]%s\n", GATEWAY_ONLINE==mGateway->getState()?"online":"offline");
    printf("[GATEWAY]%s\n", GATEWAY_ONLINE==mGateway->getState()?"online":"offline");
}

LocalState SwitchServer::adjudge(Gateway *gateway, RemotePC *remote)
{
    switch (gateway->getState()) {
    case GATEWAY_ONLINE:
    {
        if(!remote->isConnected())
        {
            if(!remote->execPing())
            {
                return LOCAL_MASTER;
            }
        }
        else
        {

        }
    }
        break;
    default:
        break;
    }
    return LOCAL_SLAVE;
}

