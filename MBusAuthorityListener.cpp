/* 
 * File:   MBusAuthorityListener.cpp
 * Author: Yuanta
 *
 * Created on Feb 24, 2017
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "AuthorityParameter.h"
#include "MBusAuthorityListener.h"
#include "ecgw.h"
#include "common.h"
#include <iostream>
#define MEMORY_STREAM_SIZE 1024000
using namespace std;

//------------------------------------------------------------------------------
void MBusAuthorityListener::OnConnect( UFC::PClientSocket *Socket)
{    
    UFC::BufferedLog::Printf( " [%s][%s] connect to %s:%d succeed.", __FILE__,__func__,FSocketObject->GetPeerIPAddress().c_str(),FSocketObject->GetPort());     
    return;
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::OnDisconnect( UFC::PClientSocket *Socket)
{
    UFC::BufferedLog::Printf( " [%s][%s] OnDisconnect!", __FILE__,__func__);
    return;
}
//------------------------------------------------------------------------------
BOOL MBusAuthorityListener::OnDataArrived( UFC::PClientSocket *Socket)
{
    //UFC::BufferedLog::Printf( " [%s][%s] OnDataArrived!", __FILE__,__func__);
    UFC::UInt8      rcvb[4096];
    int iRecvSize   =0;
    
    ///> receive data from EC server
    memset(rcvb,0,sizeof(rcvb));
    iRecvSize = Socket->RecvBuffer( rcvb,sizeof(rcvb) );    
    UFC::BufferedLog::Printf( " [%s][%s] receive buffer=%s", __FILE__,__func__,rcvb);
    
    ///> check result
    ProcessQueryResponse(rcvb);  
    
    ///> triggle event to release next process for ProcessQueryRequest()   
    FLogonEvent.SetEvent();  
    return true;
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::OnIdle( UFC::PClientSocket *Socket)
{
    UFC::BufferedLog::Printf( " [%s][%s] OnIdle!", __FILE__,__func__);
    return;
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::OnMigoMessage( const UFC::AnsiString& Subject, const UFC::AnsiString& Key, MTree* pTree )
{   
    //unsigned long iREG_Tick = UFC::GetTickCountMS();
    unsigned long iREG_Tick = UFC::GetTickCountUS();    
    //UFC::BufferedLog::Printf( " [%s][%s] iREG_Tick=%ll",  __FILE__,__FUNCTION__,iTick);
    UFC::BufferedLog::Printf(" ");    
    
    if (Subject != g_asListenSubject)
    {
        UFC::BufferedLog::Printf( " [%s][%s] Subject %s not Match!",  __FILE__,__FUNCTION__,Subject.c_str() );
        return;
    }
    pTree->append(CLIENT_REQ_TICK,iREG_Tick);

    ///> show input MTree
    UFC::AnsiString asFUNC,asDATA,asMARKET,asPeerIP,asKEY,as_IP,asPEERIP,asFunc,asNID;
    int iUID = 0;   
    int i_IP = 0;
    pTree->get(CLIENT_UID, iUID);     
    pTree->get(CLIENT_MARKET, asMARKET);          //SERVICENAME  (QUERY.WEB)
    pTree->get(CLIENT_DATA, asDATA);              //參數
    pTree->get(CLIENT_PEERIP, asPEERIP);
    pTree->get(CLIENT_PEERIP, asPeerIP);
    pTree->get(CLIENT_KEY, asKEY);
    pTree->get(CLIENT__IP, i_IP);
    pTree->get(CLIENT_FUNC, asFunc);
    as_IP = ConvertIntIPtoString(i_IP); 
    UFC::BufferedLog::Printf( " [%s][%s][NID=%lu-%d] %s=%d,%s=%s,%s=%s,%s=%s,%s=%s,%s=%s,%s=%s,QItemCount=%d",  __FILE__,__FUNCTION__,iREG_Tick,iUID, CLIENT_UID,iUID,CLIENT_MARKET, asMARKET.c_str(), 
             CLIENT_DATA, asDATA.c_str(),CLIENT_PEERIP, asPEERIP.c_str(),CLIENT_PeerIP, asPeerIP.c_str() ,CLIENT_KEY, asKEY.c_str(),CLIENT__IP, as_IP.c_str(),FAuthorityQueue.ItemCount() );
    /*
    for(int i=0;i<pTree->getNodeCount();i++)
    {
        MNode *pNode = pTree->get(i);         
        
        if(pNode->getName() == "_IP")
        {
            int *pIP = (int*)pNode->getData();            
            asTemp.AppendPrintf("%s=%s,",pNode->getName().c_str(), ConvertIntIPtoString(*pIP) );             
        }
        else
            asTemp.AppendPrintf("%s=%s,",pNode->getName().c_str(),pNode->getData()); 
        
        //UFC::BufferedLog::Printf( " [%s][%s]  key=%s,value=%s", __FILE__,__func__,pNode->getName().c_str(),(char*)pNode->getData());
    }
    UFC::BufferedLog::Printf( " [%s][%s][%s=%u] %s",  __FILE__,__FUNCTION__,CLIENT_REQ_TICK,iREG_Tick,asTemp.c_str() );
    */    
    
#ifdef CLUSTER  
        UFC::Int32         iRemoteIP;
        pTree->get( "_IP", iRemoteIP );
        if( g_iListWhiteList.IndexOf(iRemoteIP) < 0 )
        {
            UFC::BufferedLog::Printf(" [%s][%s] _IP=%d(%s) not in white list.", __FILE__,__func__, iRemoteIP,ConvertIntIPtoString(iRemoteIP) ); 
            return;
        }
#endif
    
    //---- flow control
    /*
    UFC::AnsiString asErrMsg = "";    
    //if ( !ProcFlowControl(pTree,iREG_Tick,&asErrMsg) )    
    if ( !ProcFlowControl(pTree,&asErrMsg) )
    {   
        ReturnResponse(FALSE,asErrMsg,pTree,Key);
        return;
    }
    */
    //---- in Queue  
    pTree->append(CLIENT_MBUS_KEY,Key);
    FAuthorityQueue.Inqueue( new MTree(*pTree) );
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::InitParameter()
{    
    FClientMbusKey ="";
    FUID = 0;
    FMarket = "";
    FFunc = "";    
    FData = "";
    FIndex = 0;
    FCount ="";
    FURL ="";
    FAccount = "";
    FKey = "";
    FReqTick = 0;
    FResponseData.Seek(0,0);
    FStreamSize = 0;
    curl_easy_reset( Fcurl );
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::InQueue(MTree* mtData)
{
    if( !mtData->Exists(CLIENT_MBUS_KEY) )
        mtData->append(CLIENT_MBUS_KEY,FThreadName.c_str() ); 
    FAuthorityQueue.Inqueue( new MTree(*mtData) );
}
//------------------------------------------------------------------------------
MBusAuthorityListener::MBusAuthorityListener(CheckSystemListener* pCheckSystemListener)
: PThread( NULL )
, FThreadName( "FMBusAuthorityThread" )
, FAuthorityQueue()
, FAPPListener(pCheckSystemListener)
{   
    FResponseData.SetSize(MEMORY_STREAM_SIZE);
    
    if(g_bUseCURL)
    {
        FSocketObject = NULL;
        curl_global_init( CURL_GLOBAL_DEFAULT );
        Fcurl = curl_easy_init();
    }
    else
    {
        //FSocketObject = new UFC::PClientSocket(g_asCAServerIP,g_iCAServerPort);
    
        ///setup Listener for PClientSocket    
        //FSocketObject->SetListener(this);
        //FSocketObject->Start();
    }
}
//------------------------------------------------------------------------------
MBusAuthorityListener::~MBusAuthorityListener()
{
    Terminate();    
    UFC::SleepMS(1000); // to avoid core dump.
    if(FSocketObject != NULL)
    {
        UFC::BufferedLog::Printf(" [%s][%s]  delete FSocketObject...", __FILE__,__FUNCTION__);
        delete FSocketObject;
        FSocketObject = NULL;
        UFC::BufferedLog::Printf(" [%s][%s]  delete FSocketObject...done", __FILE__,__FUNCTION__);
    }
    if(Fcurl )
    {          
        UFC::BufferedLog::Printf(" [%s][%s]  delete CURL...", __FILE__,__FUNCTION__);
        curl_easy_cleanup(Fcurl);
        curl_global_cleanup();
        UFC::BufferedLog::Printf(" [%s][%s]  delete CURL...done", __FILE__,__FUNCTION__);
    }
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::MakeXMLErrMsg(UFC::AnsiString asErrMsg ,UFC::AnsiString *XMLMsg)
{
    XMLMsg->Printf("<?xml version='1.0' encoding='utf-8'?><root><ERROR>Y</ERROR><RtnStatus>-1</RtnStatus><RtnMsg>%s</RtnMsg></root>",asErrMsg.c_str() );   
}
//------------------------------------------------------------------------------
//BOOL MBusAuthorityListener::ProcFlowControl( MTree* GetMTree ,UFC::UInt32 iREG_Tick,UFC::AnsiString *pasErrMsg )
BOOL MBusAuthorityListener::ProcFlowControl( MTree* GetMTree ,UFC::AnsiString *pasErrMsg )
{       
    UFC::AnsiString asParameter,asFunc;
    unsigned long iREG_Tick;
    GetMTree->get(CLIENT_REQ_TICK, iREG_Tick );
    //UFC::BufferedLog::Printf(" [%s][%s]  iREG_Tick=%lu, iTick=%lu", __FILE__,__FUNCTION__,iREG_Tick,iTick);
    
    ///> get data from MTree
    if( !GetMTree->get(CLIENT_FUNC, asFunc ) )
    {         
        MakeXMLErrMsg("MTree遺失 Func ID 資料",pasErrMsg);
        FResponseData.Write(pasErrMsg->c_str(),pasErrMsg->Length());          
        return FALSE;    
    }   
    if( !GetMTree->get(CLIENT_DATA, asParameter) )
    { 
        MakeXMLErrMsg("MTree遺失 DATA 資料",pasErrMsg);
        FResponseData.Write(pasErrMsg->c_str(),pasErrMsg->Length());    
        return FALSE;    
    }
    
    ///> get actno 資訊 from parameter
    UFC::NameValueMessage NV_ConfigLine("&","=");
    UFC::AnsiString asActno="";
    NV_ConfigLine.FromString( asParameter );

    if( !NV_ConfigLine.IsExists(CLIENT_ACTNO) )            
    { 
        MakeXMLErrMsg("URL 參數無帳號資訊",pasErrMsg);
        FResponseData.Write(pasErrMsg->c_str(),pasErrMsg->Length());    
        return FALSE;                
    }
    NV_ConfigLine.Get(CLIENT_ACTNO, asActno);
    return ProcFlowControl( asActno, asFunc ,iREG_Tick,pasErrMsg);
}
//------------------------------------------------------------------------------
BOOL MBusAuthorityListener::ProcFlowControl( UFC::AnsiString asAccount,UFC::AnsiString QID,UFC::UInt32 iREG_Tick,UFC::AnsiString *pasErrMsg )
{
    BOOL bResult = FALSE;
    UFC::AnsiString asKey;
    unsigned int iDiff = 0;
    int iCount =0;
    //UFC::UInt32 iREG_Tick = UFC::GetTickCountMS();
    asKey.Printf("%s-%s",asAccount.c_str(),QID.c_str() ); 
    
    //if( g_FlowControlMap_ronnie.IsExists(asKey) )
    if( g_FlowControlMap.IsExists(asKey) )
    {
        FlowControl *pflowControl;
        g_FlowControlMap.GetObjectByKey(asKey,pflowControl);
        
        //FlowControl flowControl,*pflowControl;;
        //g_FlowControlMap_ronnie.GetObjectByKey(asKey,flowControl);
        //pflowControl = & flowControl;
        
        iDiff = iREG_Tick - pflowControl->iTick;
        
        if( iDiff > g_iAllowedPeriod *1000*1000)
        {
            iCount = 1;
            pflowControl->iCount = iCount;
            pflowControl->iTick = iREG_Tick;
            bResult = TRUE;
        }else{
            iCount = pflowControl->iCount +1;
            pflowControl->iCount = iCount;
            if( iCount <= g_iAllowedCount )
                bResult = TRUE;
            else
                bResult = FALSE;
        } 
    }else{
        iCount = 1;
        FlowControl flowControl;
        flowControl.iCount = iCount;
        flowControl.iTick = iREG_Tick;
        g_FlowControlMap.Add(asKey, new FlowControl(flowControl) );   
        //g_FlowControlMap_ronnie.Add(asKey, flowControl ); 
        bResult = TRUE;
    }    
    if(bResult)
        //UFC::BufferedLog::Printf(" [%s][%s][%s=%u] Accept,key=%s,count=%d,diff ms=%d", __FILE__,__FUNCTION__,CLIENT_REQ_TICK,iREG_Tick,asKey.c_str(),iCount,iDiff/1000);
        UFC::BufferedLog::Printf(" [%s][%s][NID=%lu-%d] Accept,key=%s,count=%d,diff ms=%d", __FILE__,__FUNCTION__,FReqTick,FUID,asKey.c_str(),iCount,iDiff/1000);    
    else
    {            
        MakeXMLErrMsg(MSG_QUERY_FAIL_OVERMAXCOUNT,pasErrMsg);
        FResponseData.Write(pasErrMsg->c_str(),pasErrMsg->Length()); 
        //UFC::BufferedLog::Printf(" [%s][%s][%s=%u] Reject,key=%s,count=%d,diff ms=%d", __FILE__,__FUNCTION__,CLIENT_REQ_TICK,iREG_Tick,asKey.c_str(),iCount,iDiff/1000);
        UFC::BufferedLog::Printf(" [%s][%s][NID=%lu-%d] Reject,key=%s,count=%d,diff ms=%d", __FILE__,__FUNCTION__,FReqTick,FUID,asKey.c_str(),iCount,iDiff/1000);
    }
    return bResult;
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::Run( void )
{
    Start();
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::Execute( void )
{
    //UFC::BufferedLog::Printf( " [%s][%s] ",  __FILE__,__FUNCTION__);    
    while ( ! IsTerminated()  )
    { 
        MTree* GetMTree = FAuthorityQueue.Dequeue( 1 ); 
        if(GetMTree == NULL)
            continue;
        
        InitParameter();
        BOOL bResult = TRUE;
        UFC::AnsiString asErrMsg=""; 
            
        if( !GetMTree->get(CLIENT_FUNC, FFunc ) )
        {
            UFC::BufferedLog::Printf( " [%s][%s] loss info for Query ID ",  __FILE__,__FUNCTION__);
            //continue;
        }            
        //GetMTree->get(CLIENT_FUNC, FFunc);
        GetMTree->get(CLIENT_MBUS_KEY, FClientMbusKey);
        GetMTree->get(CLIENT_UID, FUID);                //NID of News (Auto)
        GetMTree->get(CLIENT_MARKET, FMarket);          //SERVICENAME  (QUERY.WEB)
        GetMTree->get(CLIENT_DATA, FData);              //參數
        GetMTree->get(CLIENT_INDEX, FIndex);            //0/序號
        GetMTree->get(CLIENT_COUNT, FCount);            //0
        GetMTree->get(CLIENT_REQ_TICK,FReqTick); 
        GetMTree->get(CLIENT_KEY,FKey); 

        if ( !ProcFlowControl(GetMTree,&asErrMsg) )        
        {   
            ReturnResponse(FALSE,asErrMsg,GetMTree);  
            delete GetMTree;
            GetMTree = NULL;   
            continue;
        }

        if(g_iDEBUG_LEVEL ==1)
        {
            UFC::BufferedLog::Printf( " [%s][%s] FFunc=%s", __FILE__,__func__,FFunc.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FClientMbusKey=%s", __FILE__,__func__,FClientMbusKey.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FUID=%d", __FILE__,__func__,FUID );
            UFC::BufferedLog::Printf( " [%s][%s] FMarket=%s", __FILE__,__func__,FMarket.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FData=%s", __FILE__,__func__,FData.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FIndex=%d", __FILE__,__func__,FIndex );
            UFC::BufferedLog::Printf( " [%s][%s] FCount=%s", __FILE__,__func__,FCount.c_str() );
            UFC::BufferedLog::Printf( " [%s][%s] FReqTick=%lu", __FILE__,__func__,FReqTick );
        } 
        ///> send request to Server
        bResult = ProcessQueryRequest(&asErrMsg);
        //bResult = ProcessQueryRequest(&FMsg);

        ///> return response
        ReturnResponse(bResult,asErrMsg,GetMTree); 
        //ReturnResponse(bResult,FMsg,GetMTree); 

        delete GetMTree;
        GetMTree = NULL;   
    }
}
//------------------------------------------------------------------------------
BOOL MBusAuthorityListener::ProcessQueryRequest(UFC::AnsiString *pasErrMsg )
{    
    UFC::AnsiString *pasBuffer,asURL;
    
    if( !g_QueryFuncMap.IsExists(FFunc) )
    {        
        pasErrMsg->Printf("query ID不存在或 Server 不支援");
        MakeXMLErrMsg(pasErrMsg->c_str(),pasErrMsg);
        FResponseData.Write(pasErrMsg->c_str(),pasErrMsg->Length()); 
        return FALSE;
    }
    ///> get URL of query ID
    UFC::PHashMap<UFC::AnsiString,UFC::AnsiString*>* SubMap;    
    g_QueryFuncMap.GetObjectByKey(FFunc, SubMap );    
    
    if( !SubMap->IsExists(CONFIG_URL) )
    {        
        pasErrMsg->Printf("query ID:%s 無URL資訊",FFunc.c_str());
        MakeXMLErrMsg(pasErrMsg->c_str(),pasErrMsg);
        FResponseData.Write(pasErrMsg->c_str(),pasErrMsg->Length()); 
        return FALSE;
    }

    SubMap->GetObjectByKey(CONFIG_URL,pasBuffer);

    FURL = pasBuffer->c_str();    
    FURL.TrimLeft();
    FURL.TrimRight();
    FData.TrimLeft();
    FData.TrimRight();
    
    asURL.Printf("%s?UK=%s&From=YTAPI&%s",FURL.c_str(),g_asB2B_UK.c_str(),FData.c_str());
    UFC::BufferedLog::Printf( " [%s][%s][NID=%lu-%d] QueryID=%s,URL=%s", __FILE__,__func__,FReqTick,FUID,FFunc.c_str(),asURL.c_str()  );
    BOOL bResult = CURLFunc(asURL,pasErrMsg);
    
    //if (__builtin_expect(bResult, 1))
    if( bResult )   
    {
        FResponseData.Write( "\0",  1); 
        
        //---- remove B2B UK info
        UFC::MemoryStream tempMemory( MEMORY_STREAM_SIZE, 0 );
        tempMemory.Write( FResponseData.GetBuffer(),FResponseData.GetPosition() );
        char* ptr = (char*)tempMemory.GetBuffer();
        char* ptr1 = strstr( (char*)tempMemory.GetBuffer(),"<UK>");
        char* ptr2 = strstr( (char*)tempMemory.GetBuffer(),"</UK>");
        if( ptr2 > ptr1 )
        {
            ptr2 +=5;            
            FResponseData.Seek(0,0);     
            FResponseData.Write(ptr,(ptr1-ptr));
            FResponseData.Write(ptr2,strlen(ptr2)); 
        }        
        //---- 確認資料大小
        if( FResponseData.GetPosition() >  pasErrMsg->MAX_STR_BUFFER )
        {
            //UFC::BufferedLog::Printf( " [%s][%s][%s=%lu] %s,max=%d,ResponseData.len=%d", __FILE__,__func__,CLIENT_REQ_TICK,FReqTick,MSG_QUERY_FAIL_WRONG_SIZE,pasErrMsg->MAX_STR_BUFFER,FResponseData.GetPosition()  );                 
            UFC::BufferedLog::Printf( " [%s][%s][NID=%lu-%d] %s,max=%d,ResponseData.len=%d", __FILE__,__func__,FReqTick,FUID,MSG_QUERY_FAIL_WRONG_SIZE,pasErrMsg->MAX_STR_BUFFER,FResponseData.GetPosition()  );            
            MakeXMLErrMsg(MSG_QUERY_FAIL_WRONG_SIZE,pasErrMsg);
        }
        else
            pasErrMsg->Copy( (char*)FResponseData.GetBuffer(),  FResponseData.GetPosition()); 
        
        /*
        int index1 = pasErrMsg->AnsiPos("<UK>");
        int index2 = pasErrMsg->AnsiPos("</UK>");
        if( index1 >=0 && index2 >=0 && index2 > index1 )
        {
            int iMsgLen = pasErrMsg->Length();
            int iTagLen = 5;    //</UK>
            UFC::AnsiString asMsg1,asMsg2;
            asMsg1 = pasErrMsg->SubString(0,index1);
            asMsg2 = pasErrMsg->SubString(index2 + iTagLen,iMsgLen-index2-iTagLen);
            pasErrMsg->Printf("%s%s",asMsg1.c_str(),asMsg2.c_str());
        }
        */ 
        UFC::BufferedLog::Printf( " [%s][%s][NID=%lu-%d] CURL Query,Result=%d", __FILE__,__func__,FReqTick,FUID,bResult );        
    }
    else
    {
        UFC::BufferedLog::Printf( " [%s][%s][NID=%lu-%d] bResult=%d,Msg=%s", __FILE__,__func__,FReqTick,FUID,bResult,pasErrMsg->c_str()  );        
        MakeXMLErrMsg(pasErrMsg->c_str(),pasErrMsg);
        FResponseData.Seek(0,0);
        FResponseData.Write(pasErrMsg->c_str(),pasErrMsg->Length()); 
    }    
    return bResult;
}
//------------------------------------------------------------------------------
size_t CURLCallback(void * ptr, size_t size, size_t nmemb, void *data)
{
    /*
    UFC::AnsiString* pMsg = (UFC::AnsiString*)data;
    size_t totalSize = size*nmemb;
    pMsg->AppendPrintf("%s",(char*)ptr);
    return totalSize; 
    */
    
    //UFC::BufferedLog::Printf( " [%s][%s] size=%d,nmemb=%d,ptr=%s", __FILE__,__func__,size,nmemb,(char*)ptr);
    size_t totalSize = size*nmemb;
    UFC::MemoryStream* pMsg  = ( UFC::MemoryStream*) data;

    if ( totalSize <= 0 ) 
        return 0; 
    
    int iWriteSize = pMsg->Write( ptr,  totalSize); 
    return  iWriteSize;
    //return   pMsg->Write( ptr,  totalSize);
}
//------------------------------------------------------------------------------
BOOL MBusAuthorityListener::CURLFunc(UFC::AnsiString asURL,UFC::AnsiString *pasErrMsg)
{
    //CURL *curl = NULL;
    CURLcode res;
    
    try
    {
        if(!Fcurl)
        {            
            pasErrMsg->Printf("%s",MSG_CULR_INIT_FAIL); 
            return FALSE;
        }
        //---- ronnie debug
        //asURL = "http://10.214.19.67/FutureGLWeb/fbwost.aspx?UK=CEtcM3_wE2EY&From=YTAPI&company=F021000&actno=9850410&currency=ALL&offdt1=20190326&offdt2=20190411";
        //asURL = "http://10.214.19.67/FutureGLWeb/fbwost.aspx?UK=CEtcM3_wE2EY&From=YTAPI&company=F021000&actno=9850410&currency=ALL&offdt1=20181001&offdt2=20190411";
       
        curl_easy_setopt(Fcurl,CURLOPT_URL,asURL.c_str() );    
        curl_easy_setopt(Fcurl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(Fcurl, CURLOPT_SSL_VERIFYPEER, false);      ///> trust server ca
        curl_easy_setopt(Fcurl, CURLOPT_WRITEFUNCTION, CURLCallback); ///> setup callback
        //curl_easy_setopt(Fcurl, CURLOPT_WRITEDATA, (void *)pasErrMsg);   ///> setup input parameter for callback
        curl_easy_setopt(Fcurl, CURLOPT_WRITEDATA, (void *)&FResponseData);   ///> setup input parameter for callback
        curl_easy_setopt(Fcurl, CURLOPT_TIMEOUT, QUERYT_IMEOUT);           ///> set timeout     
        curl_easy_setopt(Fcurl, CURLOPT_POST, 1);            ///> use POST
        
        res = curl_easy_perform(Fcurl);    
        
        if (res != CURLE_OK)
        {
            pasErrMsg->Printf("%s",curl_easy_strerror(res));
            return FALSE;
        }            
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
    //---------------------------------------------------------------------------------------------------
    /*
    try
    {
        if(curl )
        {
            UFC::BufferedLog::Printf( " [%s][%s]!!!!!!!!!!!", __FILE__,__func__);    
            curl_easy_cleanup(curl);
        }
        curl = curl_easy_init();
        
        if(!curl)
        {
            pasErrMsg->Printf("%s",MSG_CULR_INIT_FAIL); 
            return FALSE;
        } 
        
        //---- ronnie debug
        //asURL = "http://10.214.19.67/FutureGLWeb/fbwost.aspx?UK=CEtcM3_wE2EY&From=YTAPI&company=F021000&actno=9850410&currency=ALL&offdt1=20190326&offdt2=20190411";
        //asURL = "http://10.214.19.67/FutureGLWeb/fbwost.aspx?UK=CEtcM3_wE2EY&From=YTAPI&company=F021000&actno=9850410&currency=ALL&offdt1=20181001&offdt2=20190411";
       
        curl_easy_setopt(curl,CURLOPT_URL,asURL.c_str() );    
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);      ///> trust server ca
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CURLCallback); ///> setup callback
        //curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)pasErrMsg);   ///> setup input parameter for callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&FResponseData);   ///> setup input parameter for callback
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, QUERYT_IMEOUT);           ///> set timeout     
        curl_easy_setopt(curl, CURLOPT_POST, 1);            ///> use POST
        
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
    */
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::ProcessQueryResponse(UFC::UInt8 *rcvb)
{
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::ReturnResponse(BOOL Accept,UFC::AnsiString asMsg,MTree *pTree)
{  
    ReturnResponse(Accept, asMsg,pTree,FClientMbusKey); 
}
//------------------------------------------------------------------------------
void MBusAuthorityListener::ReturnResponse(BOOL Accept,UFC::AnsiString asMsg,MTree *pTree,UFC::AnsiString asMBusKey)
{
    /*
    MTree mtData;
    UFC::AnsiString asTemp;    
    if( pTree->get(CLIENT_FUNC,asTemp)  )
        mtData.append(CLIENT_FUNC,asTemp);
    if( pTree->get(CLIENT_UID,asTemp)  )
        mtData.append(CLIENT_UID,asTemp);
    if( pTree->get(CLIENT_MARKET,asTemp)  )
        mtData.append(CLIENT_MARKET,asTemp);
    if( pTree->get(CLIENT_INDEX,asTemp)  )
        mtData.append(CLIENT_INDEX,asTemp);
    */
    //mtData.append(CLIENT_FUNC,asTemp.c_str());
    
    /*
    for(int i=0;i< pTree->getNodeCount();i++)
    {
        MNode *pNode = pTree->get(i);             
        //mtData.append(pNode->getName().c_str(), (char*)pNode->getData());  
        UFC::BufferedLog::Printf( " [%s][%s]  key=%s,value=%s", __FILE__,__func__,pNode->getName().c_str(),(char*)pNode->getData());
    }   
    */
    
    MTree mtData; 
    mtData.append(CLIENT_UID,FUID);
    mtData.append(CLIENT_INDEX,FIndex);  
    if( !FMarket.IsEmpty() )
        mtData.append(CLIENT_MARKET,FMarket);
    if( !FFunc.IsEmpty() )
        mtData.append(CLIENT_FUNC,FFunc); 
    if( !FKey.IsEmpty() )
        mtData.append(CLIENT_KEY,FKey); 
    if(Accept)
        FCount = "0";
    else
        FCount = "-1";
    if( asMsg.Length() == 0 )
        asMsg = " ";  
   
    mtData.append(CLIENT_COUNT,FCount);
    mtData.append(CLIENT_DATA,asMsg);  

    if( FResponseData.GetPosition() == 0 )
        FResponseData.Write( " ",  1);
    FResponseData.Write( "\0",  1);    
    FStreamSize = FResponseData.GetPosition();    
    mtData.append(CLIENT_DATA_STREAM,(const unsigned char*)FResponseData.GetBuffer(),FStreamSize,true);
    
    if(FReqTick == 0)
        pTree->get(CLIENT_REQ_TICK,FReqTick);
    
    ///> show input MTree 
    FAPPListener->Send(REPLY_SUBJECT,asMBusKey,mtData);    
    UFC::BufferedLog::Printf( " [%s][%s][NID=%lu-%d] subject=%s,key=%s,%s=%d,%s=%s,%s=%s,%s=%d,%s=%s",__FILE__,__FUNCTION__,FReqTick,FUID,REPLY_SUBJECT,asMBusKey.c_str(),CLIENT_UID,FUID
    ,CLIENT_MARKET,FMarket.c_str(),CLIENT_COUNT,FCount.c_str() ,CLIENT_STREAM_SIZE,FStreamSize,CLIENT_DATA_STREAM,FResponseData.GetBuffer());
}
//------------------------------------------------------------------------------
