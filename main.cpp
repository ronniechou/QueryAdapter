#include <signal.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include "CheckSystemListener.h"
#include "MBusAuthorityListener.h"
#include "NameValueMessage.h"
#include "AuthorityParameter.h"
#include "common.h"
#include "iniFile.h"
#include "tinyxml2.h"
#include <curl/curl.h>


#include <ifaddrs.h>
//#include <netinet/in.h> 
//#include <arpa/inet.h>

using namespace std;

//-----------------------------------------------------------------------------------
// global Objects
//-----------------------------------------------------------------------------------
CheckSystemListener*        FAPPListener            = NULL;         // MBus event listener
MBusAuthorityListener*      FMBusAuthorityThread    = NULL;         // MBus Authority event listener thread
UFC::AnsiString             g_asAppName             = "QueryAdapter";   // Application name
UFC::AnsiString             FLogPrefixName          = g_asAppName;

UFC::AnsiString             FMBusServerIP           = "127.0.0.1";
UFC::Int32                  FMBusServerPort         = 12345 ;        //< default : 12345
UFC::AnsiString             g_asListenSubject       = LISTEN_SUBJECT;       // MBus Subject
UFC::AnsiString             g_asListenKey           = "all";                // MBus Key
UFC::Int32                  g_iDEBUG_LEVEL          = 0;            
BOOL                        g_bRunning              = FALSE;
UFC::AnsiString             g_asLocalIP             = "127.0.0.1";

UFC::AnsiString             g_asB2B_URL             = "http://10.214.19.1/FutureGLWeb/login/atmOnlineLogin.asp?UserID=b2ygw&userpsw=123456";
UFC::AnsiString             g_asB2B_UK              = "";
UFC::AnsiString             g_asLogDate             = "";
BOOL                        g_bFirstRun             = FALSE;
BOOL                        g_bUseCURL              = TRUE;

UFC::Int32                  g_iAllowedPeriod        = 300; 
UFC::Int32                  g_iAllowedCount         = 2; 
UFC::AnsiString             g_asQueryFuncTable          = "/oms/Speedy/cfg/QueryFuncTable.ini";
UFC::PHashMap<UFC::AnsiString, UFC::PHashMap<UFC::AnsiString,UFC::AnsiString*>* >   g_QueryFuncMap;
UFC::PHashMap<UFC::AnsiString,FlowControl* >   g_FlowControlMap;
//UFC::PHashMap<UFC::AnsiString,FlowControl >   g_FlowControlMap_ronnie;

///> default failover info
UFC::Int32                  g_iMBusServerIP             = 0;
UFC::List<UFC::Int32>       g_iListWhiteList;
UFC::List<UFC::Int32>       g_iListLocalIPList;
UFC::AnsiString             g_asArrayCoverIPList[COVER_IP_LIMIT][5];    //IP,Host,int IP,Active(0/1)
UFC::Int32                  g_iArrayCoverIPCounts       = 0;
UFC::AnsiString             g_asConfigName              = "QueryAdapter.ini";     // Config file name
UFC::AnsiString             g_asConfigPath              = ""; 
UFC::AnsiString             g_asLogPath                 = "";                   // Log path

//-----------------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------------
void AtStart( void );
void AtSignal( int signum );
void AtExit( void );
void StopObjects( void );
void PrintStartUp( void );
void ParseConfig( void );
void Initialize( void );
void CreateLogObject(void);
void AddListenerFromConfig();
//------------------------------------------------------------------------------
///> signal handler
void AtSignal( int signum )
{
    UFC::BufferedLog::Printf(" [%s][%s] Got signal=%d", __FILE__,__FUNCTION__,signum);    
    
    if(signum == 2) //crtl+c
        exit(0);
    
    //if( FAPPListener->IsConnected() == FALSE)
    //    exit(0);
    g_bRunning = FALSE;
}
//------------------------------------------------------------------------------
void AtExit( void )
{
    g_bRunning = FALSE;
    UFC::BufferedLog::Printf(" [%s][%s]", __FILE__,__FUNCTION__);
    UFC::BufferedLog::FlushToFile(); 
    //StopObjects(); 
}
//------------------------------------------------------------------------------
void StopObjects( void )
{        
        UFC::BufferedLog::Printf(" [%s][%s] STOP FAPPListener...", __FILE__,__FUNCTION__);
        if ( FAPPListener )
        {
            delete FAPPListener; 
            FAPPListener = NULL;
        }
        UFC::BufferedLog::Printf(" [%s][%s] STOP FAPPListener...done", __FILE__,__FUNCTION__);
        
        UFC::BufferedLog::Printf(" [%s][%s] STOP FMBusAuthorityThread...", __FILE__,__FUNCTION__);
        if ( FMBusAuthorityThread )
        {
            delete FMBusAuthorityThread; 
            FMBusAuthorityThread = NULL;
        }    
        UFC::BufferedLog::Printf(" [%s][%s] STOP FMBusAuthorityThread...done", __FILE__,__FUNCTION__);
}
//------------------------------------------------------------------------------
size_t CURLCallbackFun(void * ptr, size_t size, size_t nmemb, void *data)
{
    UFC::AnsiString* pMsg = (UFC::AnsiString*)data;
    size_t totalSize = size*nmemb;
    pMsg->AppendPrintf("%s",(char*)ptr);
    return totalSize; 
}
//------------------------------------------------------------------------------
BOOL CURLFunc(UFC::AnsiString asURL,UFC::AnsiString *pasErrMsg)
{
    CURL *curl = NULL;
    CURLcode res;
    //UFC::AnsiString asQueryURL;    
    //asQueryURL.Printf("%s?UserID=%s&userpsw=%s",g_asB2B_URL.c_str(),g_asB2B_ID.c_str(),g_asB2B_PW.c_str() );
    
    try
    {
        if(!curl )
            curl_easy_cleanup(curl);
        curl = curl_easy_init();
        if(!curl)
        {
            pasErrMsg->Printf("%s",MSG_CULR_INIT_FAIL); 
            return FALSE;
        }
        curl_easy_setopt(curl,CURLOPT_URL,asURL.c_str() );    
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);      ///> trust server ca
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLCallbackFun); ///> setup callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)pasErrMsg);   ///> setup input parameter for callback
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, LOGON_TIMEOUT);           ///> set timeout        

        res = curl_easy_perform(curl);    
        if (res != CURLE_OK)
        {
            pasErrMsg->Printf("%s",curl_easy_strerror(res));
            return FALSE;
        }
        curl_easy_cleanup(curl);
    }
    catch( UFC::Exception &e )
    {
        pasErrMsg->Printf("UFC exception occurred:%s",e.what());        
        return FALSE;
    }
    catch(...)
    {
        pasErrMsg->Printf("Unknown exception occurred");        
        return FALSE;
    }    
    return TRUE;
}
//------------------------------------------------------------------------------
BOOL GetB2BUK(UFC::AnsiString *pasErrMsg)
{
    //UFC::BufferedLog::Printf( " [%s][%s] URL=%s ",  __FILE__,__FUNCTION__,g_asB2B_URL.c_str() );   
    if (! CURLFunc(g_asB2B_URL,pasErrMsg) )
        return FALSE;
    
    // parser xml
    try
    {
        tinyxml2::XMLDocument doc;
        int error = doc.Parse(pasErrMsg->c_str() );   
        if(error == 0)
        { 
            tinyxml2::XMLHandle docHandle( &doc );
            tinyxml2::XMLElement* subElement = docHandle.FirstChildElement( "root" ).FirstChildElement( "result" ).ToElement();

            if(subElement)
            {     
                if( strcmp(subElement->GetText(),"00") == 0 )
                {
                    subElement = docHandle.FirstChildElement( "root" ).FirstChildElement( "UK" ).ToElement();  
                    if(subElement)
                    {                
                        pasErrMsg->Printf("%s",subElement->GetText());
                        return TRUE;
                    }else
                        return FALSE;
                }else{
                    subElement = docHandle.FirstChildElement( "root" ).FirstChildElement( "message" ).ToElement();
                    if(subElement)
                        UFC::BufferedLog::Printf( " [%s][%s] Fail to get UK,msg=%s ",  __FILE__,__FUNCTION__,subElement->GetText() );   
                    return FALSE;
                }
            }else{
                return FALSE;
            }
        }else
            UFC::BufferedLog::Printf( " [%s][%s]doc.Parse fail , msg=%s ",  __FILE__,__FUNCTION__,doc.GetErrorStr1());   
        return FALSE;
    }
    catch( UFC::Exception &e )
    {
        pasErrMsg->Printf("UFC exception occurred:%s",e.what());        
        return FALSE;
    }
    catch(...)
    {
        pasErrMsg->Printf("Unknown exception occurred");        
        return FALSE;
    }    
}
//------------------------------------------------------------------------------
void ParseArg(int argc, char** argv)
{
    g_asConfigPath.Printf("%s/../cfg/",UFC::GetCurrentDir().c_str());
    g_asConfigName.Printf("%s%s",g_asConfigPath.c_str() ,"QueryAdapter.ini");
    
    for(int i=1;i<argc;i++)
    {        
        UFC::AnsiString asArgv = argv[i];
        
        if(strcmp(argv[i],"-F") == 0)        
            g_bFirstRun = TRUE;
        else if( asArgv.AnsiPos("-host=") >= 0 )
        {
            UFC::AnsiString asHost = asArgv.SubString( strlen("-host=") , -1 );
            asHost.TrimRight();
            if(asHost.Length() == 0)
                asHost = "127.0.0.1";
            
            FLogPrefixName.Printf("%s%s",g_asAppName.c_str(),asHost.c_str() );
            g_asConfigName.Printf("%s%s.ini",g_asConfigPath.c_str(),FLogPrefixName.c_str() );
        }
    }
}
//------------------------------------------------------------------------------
void GetLogDate()
{
    UFC::GetYYYYMMDD(g_asLogDate,FALSE);
}
//------------------------------------------------------------------------------
void GetLocalIP()
{    
    struct sockaddr_in SocketAddress;
    UFC::Int32  intLocalIPAddress = UFC::PSocket::GetLocalIPAddress();
    //UFC::BufferedLog::Printf(" [%s][%s] intLocalIPAddress=%d,%08X", __FILE__,__func__,intLocalIPAddress,intLocalIPAddress );
    
    ///> If NOT BigEndian => reverse order of byte
    if( !IsBigEndian() )
        EndianSwap( (unsigned int&) intLocalIPAddress);  
    memcpy(  &SocketAddress.sin_addr ,   &intLocalIPAddress,   sizeof(struct sockaddr) );    
    g_asLocalIP = inet_ntoa( SocketAddress.sin_addr );
    //UFC::BufferedLog::Printf(" [%s][%s] g_asLocalIP=%s", __FILE__,__func__,g_asLocalIP.c_str() );
}
//------------------------------------------------------------------------------
///> Load config for  FAppName/FMBusServerIP/FMBusServerPort
void ParseConfig()
{
     if( !UFC::FileExists(g_asConfigName) )           
     {
         UFC::BufferedLog::Printf( " [%s][%s] %s not exist",  __FILE__,__FUNCTION__,g_asConfigName.c_str() );  
        return;  
     }
    
    UFC::UiniFile Config(g_asConfigName);    
    UFC::AnsiString Value;    
    if( Config.GetValue( "Setting", "AppName",Value ) == TRUE )
        g_asAppName = Value;
    if( Config.GetValue( "Setting", "MBusServerIP",Value ) == TRUE )    
        FMBusServerIP = Value;
    if( Config.GetValue( "Setting", "MBusServerPort",Value ) == TRUE )    
        FMBusServerPort = Value.ToInt();    
    if( Config.GetValue( "Setting", "B2B_URL",Value ) == TRUE )    
        g_asB2B_URL = Value;
    if( Config.GetValue( "Setting", "QueryFuncTable",Value ) == TRUE )    
        g_asQueryFuncTable = Value;
    if( Config.GetValue( "Setting", "AllowedPeriod",Value ) == TRUE ) 
        g_iAllowedPeriod = Value.ToInt();
    if( Config.GetValue( "Setting", "AllowedCount",Value ) == TRUE ) 
        g_iAllowedCount = Value.ToInt();
    
}
//------------------------------------------------------------------------------
///> Create Log Object
void CreateLogObject()
{    
    try
    {
        g_asLogPath.Printf("%s/../log/",UFC::GetCurrentDir().c_str());
        if( UFC::FileExists(g_asLogPath) == false)           
            mkdir(g_asLogPath,S_IRWXU);       
        UFC::AnsiString TradeDate;
        UFC::GetTradeYYYYMMDD( TradeDate );
        UFC::BufferedLog::SetLogObject( new UFC::BufferedLog( g_asLogPath + FLogPrefixName + "." + TradeDate + ".log" ,10240,true,true) );   
        UFC::BufferedLog::SetDebugMode( g_iDEBUG_LEVEL );
        UFC::BufferedLog::SetPrintToStdout( TRUE );      
        }
    catch( UFC::Exception &e )
    {
        UFC::BufferedLog::Printf(" [%s][%s] UFC exception occurred <Reason:%s>\n", __FILE__,__func__, e.what() );
    }
    catch(...)
    {
        UFC::BufferedLog::Printf(" [%s][%s] Unknown exception occurred\n", __FILE__,__func__ );
    }
}
//------------------------------------------------------------------------------
void GetB2BUK_UntilSuccess(UFC::AnsiString *pasMsg)
{
    while(TRUE)
    {
        *pasMsg = "";
        if ( GetB2BUK(pasMsg) == TRUE )
            break;
        else
            UFC::BufferedLog::Printf( " [%s][%s] %s,msg=%s ",  __FILE__,__FUNCTION__,MSG_GET_B2B_UK_FAIL,pasMsg->c_str() ); 
        UFC::SleepMS(30*1000);
    }
}
//------------------------------------------------------------------------------
void ParseQueryFuncTable()
{    
    if( !UFC::FileExists(g_asQueryFuncTable) )
    {
        UFC::BufferedLog::Printf( " [%s][%s] %s not exist",  __FILE__,__FUNCTION__,g_asQueryFuncTable.c_str() );  
        return;
    }
    UFC::AnsiString asValue,asQID;
    UFC::UiniFile Config(g_asQueryFuncTable);    
    
    if( !Config.SectionExists( QUERY_FUNCTION_NAME ) )
    {
        UFC::BufferedLog::Printf( " [%s][%s] section:%s not exist in %s",  __FILE__,__FUNCTION__,QUERY_FUNCTION_NAME,g_asQueryFuncTable.c_str() );  
        return;
    }
    UFC::Section*  HostSection;
    HostSection= Config.GetSection(QUERY_FUNCTION_NAME);
    for(int i=0;i<HostSection->ItemCount();i++)
    {
        HostSection->GetNameValue(i,asQID,asValue);

        UFC::NameValueMessage NV_ConfigLine(",","=");
        UFC::AnsiString asService,asFilter,asURL,asDesc;
        NV_ConfigLine.FromString( asValue );
        NV_ConfigLine.Get(CONFIG_SERVICE, asService);
        NV_ConfigLine.Get(CONFIG_FILTER, asFilter);
        NV_ConfigLine.Get(CONFIG_URL, asURL); 

        UFC::PHashMap<UFC::AnsiString,UFC::AnsiString*> SubMap;
        SubMap.Add(CONFIG_SERVICE, new UFC::AnsiString(asService.c_str()) );
        SubMap.Add(CONFIG_FILTER, new UFC::AnsiString(asFilter.c_str()) );
        SubMap.Add(CONFIG_URL, new UFC::AnsiString(asURL.c_str()) );

        if( !g_QueryFuncMap.IsExists(asQID) )            
            g_QueryFuncMap.Add(asQID, new UFC::PHashMap<UFC::AnsiString,UFC::AnsiString*>(SubMap)  );
    }
    
    if(g_iDEBUG_LEVEL ==1)
    {
        UFC::BufferedLog::Printf( " [%s][%s] g_QueryFuncMap.ItemCount()=%d",  __FILE__,__FUNCTION__,g_QueryFuncMap.ItemCount() );  
        for(int i=0;i<g_QueryFuncMap.ItemCount();i++)
        {
            UFC::AnsiString astmp;
            UFC::PHashMap<UFC::AnsiString,UFC::AnsiString*>* Sub;
            g_QueryFuncMap.GetItem(i,astmp,Sub);
            UFC::BufferedLog::Printf( " [%s][%s] ID=%s",  __FILE__,__FUNCTION__, astmp.c_str());   

            UFC::BufferedLog::Printf( " [%s][%s] \tSub.ItemCount()=%d",  __FILE__,__FUNCTION__,Sub->ItemCount() ); 
            for(int j=0;j<Sub->ItemCount();j++)
            {
                UFC::AnsiString key,*value;
                Sub->GetItem(j,key,value);
                UFC::BufferedLog::Printf( " [%s][%s] \tkey=%s,value=%s",  __FILE__,__FUNCTION__,key.c_str(),value->c_str() ); 
            }
        } 
    }  
}
//------------------------------------------------------------------------------
///> set signal handlers
void AtStart( void )
{
    atexit( AtExit );
    signal( SIGINT,  AtSignal );
    signal( SIGQUIT, AtSignal );
    signal( SIGTERM, AtSignal );
    signal( SIGHUP,  AtSignal );
    signal( SIGKILL, AtSignal );
    signal( SIGQUIT, AtSignal );
    signal( SIGABRT, AtSignal );
    signal( SIGFPE, AtSignal );
    signal( SIGILL, AtSignal );    
//    signal( SIGSEGV, AtSignal );  // capture core dump event
}
//------------------------------------------------------------------------------
///> Print startup screen.
void PrintStartUp( void )
{
    UFC::AnsiString asFlag;
    #ifdef CLUSTER  
        asFlag.AppendPrintf("%s, ","CLUSTER");
    #endif 
    
    // Print startup screen
    UFC::BufferedLog::Printf( " ____________________________________________" );
    UFC::BufferedLog::Printf( " " );
    UFC::BufferedLog::Printf( "    Yuanta %s", g_asAppName.c_str());
    UFC::BufferedLog::Printf( "    Startup on %s at %s ", UFC::Hostname, UFC::GetDateString().c_str() );
    UFC::BufferedLog::Printf( " ");
    UFC::BufferedLog::Printf( "    Ver : 1.0.0 Build Date:%s ",  __DATE__ );
    UFC::BufferedLog::Printf( "    [%d bits version]          ",  sizeof(void*)*8 );
    UFC::BufferedLog::Printf( "    FirstRun                : %d         ", g_bFirstRun  );    
    UFC::BufferedLog::Printf( "    cfg                     : %s         ",  g_asConfigName.c_str() );
    UFC::BufferedLog::Printf( "    LogDate                 : %s         ",  g_asLogDate.c_str() );
    UFC::BufferedLog::Printf( "    MBusServer              : %s:%d         ",FMBusServerIP.c_str(),FMBusServerPort   );
    UFC::BufferedLog::Printf( "    B2B Login URL           : %s         ",g_asB2B_URL.c_str()   );    
    UFC::BufferedLog::Printf( "    B2B UK                  : %s         ",g_asB2B_UK.c_str()   );
    UFC::BufferedLog::Printf( "    QueryFuncTable          : %s         ",g_asQueryFuncTable.c_str()   );
    UFC::BufferedLog::Printf( "    QueryFunc.ItemCount     : %d         ",  g_QueryFuncMap.ItemCount() ); 
    UFC::BufferedLog::Printf( "    AllowedPeriod           : %d         ",g_iAllowedPeriod   );
    UFC::BufferedLog::Printf( "    AllowedCount            : %d         ",g_iAllowedCount   );
    UFC::BufferedLog::Printf( "    flag                    : %s         ",asFlag.c_str()   );  
    UFC::BufferedLog::Printf( " ");
    UFC::BufferedLog::Printf( " ____________________________________________" );
}
//------------------------------------------------------------------------------
void Initialize( void )
{  
    if (  FAPPListener == NULL)
    {
        FAPPListener = new CheckSystemListener( g_asAppName, FMBusServerIP, FMBusServerPort );
        UFC::BufferedLog::DebugPrintf( UFC::dlInformation, " [%s] Register MBus <appname:%s>", __func__ , g_asAppName.c_str() );
        FAPPListener->StartService();
    }
    if ( FMBusAuthorityThread == NULL)        
        FMBusAuthorityThread = new MBusAuthorityListener(FAPPListener);
    
    ///> get localhost ip
    struct ifaddrs * ifAddrStruct=NULL, *ipa = NULL;
    void * tmpAddrPtr=NULL;
    getifaddrs(&ifAddrStruct);  
    ipa =  ifAddrStruct;
    
    while (ipa!=NULL) 
    {
        if (ipa->ifa_addr->sa_family==AF_INET) 
        {
            tmpAddrPtr=&((struct sockaddr_in *)ipa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);                
            g_iListLocalIPList.Add( inet_addr(addressBuffer) );
        }
        ipa=ipa->ifa_next;
    } 
    freeifaddrs(ifAddrStruct);
    
    /*  
    //會memory leak
    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;
    getifaddrs(&ifAddrStruct);    
    while (ifAddrStruct!=NULL) 
    {
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET) 
        {
            tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);                
            g_iListLocalIPList.Add( inet_addr(addressBuffer) );
        }
        ifAddrStruct=ifAddrStruct->ifa_next;
    } 
    */
    ///> If target MBusServerIP is not LocalIP, create another MBus connection for LocalIP to monitor in SpeedyCenter           
    if(FMBusServerIP != "127.0.0.1" && g_iListLocalIPList.IndexOf( inet_addr( FMBusServerIP.c_str() ) ) < 0 )
    {  
        UFC::BufferedLog::Printf(" [%s][%s] connect local MBus  127.0.0.1", __FILE__,__func__ );
        CheckSystemListener *FCenterListener = new CheckSystemListener( g_asAppName, "127.0.0.1", 12345 );
        FCenterListener->StartService();
    }
    
#ifdef CLUSTER 
    ///> Add FMBusServerIP to WhiteList  
    if(FMBusServerIP == "127.0.0.1")
    { 
        for(int i=0;i<g_iListLocalIPList.ItemCount();i++)
            g_iListWhiteList.Add( g_iListLocalIPList[i] );
    }else{
        g_iMBusServerIP = inet_addr(FMBusServerIP.c_str());
        g_iListWhiteList.Add(g_iMBusServerIP);
    } 
    PrintWhiteList(); 
#endif 
}
//------------------------------------------------------------------------------
void AddListenerFromConfig()
{
    if(FAPPListener == NULL || FMBusAuthorityThread == NULL)
        return; 
    BOOL bHaveAddListener = FALSE;    
    fstream fin;    
    try
    {
        fin.open(g_asConfigName.c_str(),ios::in);    
        if(!fin)        
            throw bHaveAddListener;
            
        char caTempLine[128];
        UFC::AnsiString asLine;    

        while( fin.getline(caTempLine,sizeof(caTempLine)) )
        {
            if(strlen(caTempLine) == 0)
                continue;
            UFC::AnsiString asSubject,asKey,asMonitoringProcess,asCoverIP,asHost,asCoverAppName;      
            asLine = caTempLine;
            if(asLine.FirstChar() == '#' || asLine.Length() == 0)
                continue;
            UFC::NameValueMessage NV_ConfigLine( "|" );
            NV_ConfigLine.FromString( asLine );    
            NV_ConfigLine.Get("MonitoringProcess", asMonitoringProcess);
            NV_ConfigLine.Get("Subject", asSubject);
            NV_ConfigLine.Get("Key", asKey); 
            
#ifdef CLUSTER        
            ///> add CoverIP to ArrayCoverIP
            if( NV_ConfigLine.IsExists("CoverIP") && NV_ConfigLine.IsExists("Host") )
            {
                if(g_iArrayCoverIPCounts < COVER_IP_LIMIT)
                { 
                    NV_ConfigLine.Get("CoverIP", asCoverIP);
                    NV_ConfigLine.Get("Host", asHost);
                    if( NV_ConfigLine.IsExists("CoverAppName") )
                        NV_ConfigLine.Get("CoverAppName", asCoverAppName); 
                    else
                        asCoverAppName = g_asAppName;
                    
                    UFC::Int32 iIP = inet_addr(asCoverIP.c_str());
                    g_asArrayCoverIPList[g_iArrayCoverIPCounts][0] = asCoverIP;
                    g_asArrayCoverIPList[g_iArrayCoverIPCounts][1] = asHost;                 
                    g_asArrayCoverIPList[g_iArrayCoverIPCounts][2].Printf("%d",iIP ); 
                    g_asArrayCoverIPList[g_iArrayCoverIPCounts][3] = "0"; 
                    g_asArrayCoverIPList[g_iArrayCoverIPCounts][4] = asCoverAppName;

                    ///> add monitor process
                    FAPPListener->AddMonitoringProcess( asHost, asCoverAppName); 
                    UFC::BufferedLog::Printf(" [%s][%s] Add MonitoringProcess Host=%s,AppName=%s", __FILE__,__FUNCTION__,asHost.c_str() , asCoverAppName.c_str() );

                    ///> add CoverIP to Whitelist
                    g_iListWhiteList.Add( iIP );
                    UFC::BufferedLog::Printf(" [%s][%s] Add IP=%d,%s into whitelist", __FILE__,__func__,iIP,ConvertIntIPtoString(iIP) );                 
                    g_iArrayCoverIPCounts++;

                }else{
                    UFC::BufferedLog::Printf(" [%s][%s] counts of coverIPList over limit", __FILE__,__FUNCTION__);
                }            
            }
#endif             

            ///> 設定從MBUS監控的Hostname 
            if(asMonitoringProcess.c_str() != NULL)
            {
                if(NV_ConfigLine.IsExists("MonitoringHost"))
                    NV_ConfigLine.Get("MonitoringHost", asHost);
                else
                    asHost = UFC::Hostname;
                
                FAPPListener->AddMonitoringProcess( asHost, asMonitoringProcess);  
                UFC::BufferedLog::Printf(" [%s][%s] Add MonitoringProcess Host=%s,AppName=%s", __FILE__,__FUNCTION__,asHost.c_str() , asMonitoringProcess.c_str());       
            }
            ///> 設定從MBUS接收的Subject及Key，
            if(asSubject.c_str() != NULL && asKey.c_str() != NULL)
            {       
                bHaveAddListener = TRUE;
                FAPPListener->AddListener(asSubject, asKey, FMBusAuthorityThread);
                UFC::BufferedLog::Printf(" [%s][%s] Add subject(%s) & key(%s) ", __FILE__,__FUNCTION__,asSubject.c_str() , asKey.c_str());
                g_asListenSubject = asSubject;
                g_asListenKey = asKey;
            }        
        }    
        fin.close();
        PrintWhiteList();
        if( bHaveAddListener == FALSE )
            throw bHaveAddListener;
    }
    catch(BOOL err)
    {
        UFC::BufferedLog::Printf(" [%s][%s] Use default Subject/Key into Listener.(%s/%s)", __FILE__,__FUNCTION__,g_asListenSubject.c_str(), g_asListenKey.c_str());
        FAPPListener->AddListener(g_asListenSubject, g_asListenKey, FMBusAuthorityThread);
        return; 
    }
}
//------------------------------------------------------------------------------
int main(int argc, char** argv)
{    
    ParseArg(argc,argv);
    GetLogDate();
    GetLocalIP();    
    ParseConfig();      ///> get AppName/MBusServerIP/Port from config.
    
    CreateLogObject();
    GetB2BUK_UntilSuccess(&g_asB2B_UK);
    ParseQueryFuncTable();  /// get IDvsURL from QueryFuncTable.ini
    AtStart();          ///> set signal handlers.     
    PrintStartUp();    
    Initialize();       ///> new listeners.
   
    try
    {        
        g_bRunning = TRUE; 
        UFC::BufferedLog::Printf(" [%s][%s] Start MBus Authority listener", __FILE__,__func__ );
        
        FMBusAuthorityThread->Run(); 
        AddListenerFromConfig();  
        //FAPPListener->StartService(); 
        
        while ( g_bRunning )
        {  
            UFC::SleepMS( 5*1000 );
        }
        UFC::BufferedLog::Printf(" [%s][%s] Daemon job interrupted!", __FILE__, __func__ );
    }
    catch( UFC::Exception &e )
    {
        UFC::BufferedLog::Printf(" *ERR* [%s] UFC exception occurred <Reason:%s>\n", __func__, e.what() );
    }
    catch(...)
    {
        UFC::BufferedLog::Printf(" *ERR* [%s] Unknown exception occurred\n", __func__ );
    }
    return 0;
}




