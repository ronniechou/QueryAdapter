
#include <sys/types.h>
#include <unistd.h>
#include "CheckSystemListener.h"
#include "AuthorityParameter.h"
#include "common.h"
//----------------------------------------------------------------------------------------------------------------------
CheckSystemListener::CheckSystemListener(UFC::AnsiString AppName, UFC::AnsiString MBusIP, UFC::Int32 MBusPort)
:FAppName(AppName)
,FMBusIP(MBusIP)
,FIsConnectMBus(FALSE)
,FMBusPort(MBusPort)
{
    FMessageObj = new MessageObject(FAppName, "1.0",  "SystemListener", FMBusPort);
    FMessageObj->SetHost(FMBusIP);
    FMessageObj->SetMonitorListener( this );
}
//----------------------------------------------------------------------------------------------------------------------
CheckSystemListener::~CheckSystemListener()
{
    UFC::BufferedLog::Printf(" [%s][%s]  StopService...", __FILE__,__FUNCTION__);
    StopService();
    UFC::BufferedLog::Printf(" [%s][%s]  StopService...done", __FILE__,__FUNCTION__);
    // if an instance found it is not the first instance and want to terminate itself,
    // do not delete FMessageObj, otherwise the instance will halt there
//    if ( FMessageObj )
//    {
//        delete FMessageObj; FMessageObj = NULL;
//    }
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::StartService()
{
    UFC::BufferedLog::Printf(" [%s][%s] %s  %s:%d", __FILE__,__FUNCTION__,FAppName.c_str(), FMBusIP.c_str(), FMBusPort);
    FIsConnectMBus = FALSE;
    FMessageObj->Start(); ///< Start Migo Message pump.
    FMessageObj->WaitForConnected();
    UFC::SleepMS(500);
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::StopService()
{
    UFC::BufferedLog::Printf(" [%s][%s] %s  %s:%d", __FILE__,__FUNCTION__,FAppName.c_str(), FMBusIP.c_str(), FMBusPort);
    FMessageObj->Terminate();
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::AddListener( const UFC::AnsiString& Subject, const UFC::AnsiString& Key, MessageListener *Listener)
{
    FMessageObj->AddListener( Subject, Key, Listener );
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::AddMonitoringProcess(const UFC::AnsiString& Host,  const UFC::AnsiString& AppName )
{
    FMessageObj->AddMonitoringProcess( Host, AppName );
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::StartCommandListener()
{
    AddListener( RQCOMMAND_Subject, "all", this );
}
//----------------------------------------------------------------------------------------------------------------------
BOOL CheckSystemListener::Send( const UFC::AnsiString& Subject, const UFC::AnsiString& Key, MTree &Data, BOOL KeepUnsent )
{
    return FMessageObj->Send(Subject, Key, Data, KeepUnsent) ;
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::GetRequestCommandName(REQUESTCOMMAND command, UFC::AnsiString & Name)
{
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::OnProcessConnected( BOOL IsTheFirstOne )
{
    if( IsTheFirstOne == FALSE )
    {
        UFC::BufferedLog::Printf( " [%s][%s] Process %s already exists!", __FILE__,__FUNCTION__,FAppName.c_str()  );
        g_bRunning = FALSE;
        UFC::SleepMS( 2000 );
        exit( EXIT_SUCCESS );
    }
} 
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::OnProcessStartup( const UFC::AnsiString& Host, const UFC::AnsiString& AppName )  
{
    UFC::BufferedLog::Printf(" [%s][%s] Host(%s) AppName(%s)", __FILE__,__FUNCTION__, Host.c_str(), AppName.c_str());

#ifdef CLUSTER      
    ///> check g_asArrayCoverIPList
    UFC::Int32 iIP = 0,iIndex = -1;
    
    for(int i=0;i<g_iArrayCoverIPCounts;i++)
    {
        if( g_asArrayCoverIPList[i][1] == Host && g_asArrayCoverIPList[i][4] == AppName )
        {           
            iIP = atoi(g_asArrayCoverIPList[i][2]);           
            g_asArrayCoverIPList[i][3] = "1";
         
            ///> modify whitelist
            iIndex = g_iListWhiteList.IndexOf(iIP);
            if(iIndex >0 )
            { 
                g_iListWhiteList.Delete(iIndex);
                UFC::BufferedLog::Printf(" [%s][%s] delete IP=%d (%s) from white list.", __FILE__,__FUNCTION__,iIP,ConvertIntIPtoString(iIP) );
                PrintWhiteList();
            }            
            break;
        }
    }
#endif  
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::OnProcessStopped( const UFC::AnsiString& Host, const UFC::AnsiString& AppName )  
{
    UFC::BufferedLog::Printf(" [%s][%s]  Host(%s) AppName(%s)", __FILE__,__FUNCTION__, Host.c_str(), AppName.c_str());  

#ifdef CLUSTER    
    UFC::Int32 iIP = 0;
    for(int i=0;i<g_iArrayCoverIPCounts;i++)
    {
        if( g_asArrayCoverIPList[i][1] == Host && g_asArrayCoverIPList[i][4] == AppName )
        {
            iIP = atoi(g_asArrayCoverIPList[i][2]);
            g_asArrayCoverIPList[i][3] = "0";
            
            ///> modify whitelist
            if( g_iListWhiteList.IndexOf(iIP) < 0 )
            {
                g_iListWhiteList.Add(iIP);
                UFC::BufferedLog::Printf(" [%s][%s] Add IP=%d (%s) into white list.", __FILE__,__FUNCTION__,iIP,ConvertIntIPtoString(iIP) );
                PrintWhiteList();
                
                ///send failover ok to mbus
                MTree mtData;    
                mtData.append("CMD",ADMIN_APP_ONLINE);  
                mtData.append("APP",FAppName);                
                mtData.append("HOST",Host);
                FMessageObj->Send("MBUS", "all", mtData, FALSE );
                
                //FMessageObj->Send(MBUS_REPLY_SUBJECT,"LOGON_CLIENT_example",mtData,FALSE);
                UFC::BufferedLog::Printf(" [%s][%s] CMD=%d,APP=%s,HOST=%s", __FILE__,__FUNCTION__,ADMIN_APP_ONLINE,FAppName.c_str(),Host.c_str() );
            }
            break;
        }
    }
#endif     
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::OnProcessList( UFC::PStringList& Processs ) 
{
    UFC::BufferedLog::Printf(" [%s][%s]  Processs(%d)", __FILE__,__FUNCTION__, Processs.ItemCount() );
    //for (int i=0 ; i < Processs.ItemCount() ; i++)    
    //    UFC::BufferedLog::Printf(" [%s][%s] Process: [%d][%s]", __FILE__,__FUNCTION__, i, Processs[i].c_str());
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::OnConnected(  )
{
    FIsConnectMBus = TRUE;
    UFC::BufferedLog::Printf( " [%s][%s] %s is connected to MBus %s:%d", __FILE__,__FUNCTION__, FAppName.c_str(), FMBusIP.c_str(), FMBusPort);
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::OnDisconnected( )
{
    FIsConnectMBus = FALSE;
    UFC::BufferedLog::Printf( " [%s][%s] %s is disconnected from MBus %s:%d", __FILE__,__func__,FAppName.c_str(), FMBusIP.c_str(), FMBusPort);
}
//----------------------------------------------------------------------------------------------------------------------
void CheckSystemListener::OnMigoMessage( const UFC::AnsiString& Subject, const UFC::AnsiString& Key,  MTree* pTree )
{
}
//----------------------------------------------------------------------------------------------------------------------
BOOL CheckSystemListener::IsConnected()
{
    return FIsConnectMBus;
}



