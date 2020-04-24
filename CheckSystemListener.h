/* 
 * File:   CheckSystemListener.h
 * Author: pmf
 *
 * Created on Feb 24, 2017
 */

#ifndef _CHECKSYSTEMLISTENER_H
#define	_CHECKSYSTEMLISTENER_H

#include "UFC.h"
#include "Sigo.h"

//------------------------------------------------------------------------------
#define RQCOMMAND_Subject     "RQCMD" //  Key is AppName
#define RQCOMMAND_ALLAPP_KEY  "ALLAPP" // means for all app

//------------------------------------------------------------------------------
enum _REQUESTCOMMAND
{
    //request
    RQCMD_NONE                  = 0,
    RQCMD_Recover               = 1,
    RQCMD_Resend                = 2,
    RQCMD_ShowPosition          = 3,
    //RQCMD_RecoverFromDate       = 4,
    RQCMD_GetPosition           = 11,
    RQCMD_SubscribeRealtimePosition   = 12,
    //response
    RPCMD_Recover               = 51,
    RPCMD_Resend                = 52,
    RPCMD_ShowPosition          = 53,
    //RPCMD_RecoverFromDate       = 54,
    RPCMD_GetPosition           = 61,
    RPCMD_SubscribeRealtimePosition   = 62
};
typedef _REQUESTCOMMAND REQUESTCOMMAND;
//------------------------------------------------------------------------------
class CheckSystemListener : public MonitorListener, public MessageListener
{
private:
    UFC::AnsiString    FAppName;
    MessageObject     *FMessageObj;
    BOOL               FIsConnectMBus;

private:
    UFC::AnsiString    FMBusIP ;
    UFC::Int32         FMBusPort;

protected:/// implement interface MonitorListener
    virtual void OnProcessStartup( const UFC::AnsiString& Host, const UFC::AnsiString& AppName ) ;
    virtual void OnProcessStopped( const UFC::AnsiString& Host, const UFC::AnsiString& AppName ) ;
    virtual void OnProcessConnected( BOOL IsTheFirstOne );
    virtual void OnProcessList( UFC::PStringList& Processs ) ;
    virtual void OnConnected( void );
    virtual void OnDisconnected( void );

private:///implement interface MessageListener
      virtual void OnMigoMessage( const UFC::AnsiString& Subject, const UFC::AnsiString& Key, MTree* Data );

public:
    static void GetRequestCommandName(REQUESTCOMMAND command, UFC::AnsiString & Name);

public:
    void StartService();
    void StopService();
    void StartCommandListener();
    void AddListener( const UFC::AnsiString& Subject, const UFC::AnsiString& Key, MessageListener *Listener);
    void AddMonitoringProcess(const UFC::AnsiString& Host,  const UFC::AnsiString& AppName );   
    BOOL Send( const UFC::AnsiString& Subject, const UFC::AnsiString& Key, MTree &Data, BOOL KeepUnsent = TRUE );
    BOOL IsConnected();

public:
    CheckSystemListener(UFC::AnsiString AppName, UFC::AnsiString MBusIP, UFC::Int32 MBusPort);
//    CheckSystemListener(const CheckSystemListener& orig);
    virtual ~CheckSystemListener();
};

#endif	/* _CHECKSYSTEMLISTENER_H */

