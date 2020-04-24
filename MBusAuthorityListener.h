/* 
 * File:   CheckSystemListener.cpp
 * Created on Jun 08, 2018
 */

#ifndef MBUSAUTHORITYLISTENER_H_
#define MBUSAUTHORITYLISTENER_H_

#include "MSubscriber.h"
#include "List.h"
#include "CheckSystemListener.h"
#include "NameValueMessage.h"
#include <curl/curl.h>

//------------------------------------------------------------------------------
typedef UFC::PtrQueue<MTree>  TMemoryQueue;

class MBusAuthorityListener : public UFC::PThread, public MessageListener ,public UFC::SocketClientListener
{
protected:
    CheckSystemListener*    FAPPListener;
    UFC::AnsiString         FThreadName;
    TMemoryQueue            FAuthorityQueue;
    
    UFC::AnsiString         FClientMbusKey;    
    UFC::AnsiString         FMarket;
    UFC::AnsiString         FFunc;    
    UFC::AnsiString         FData;    
    UFC::AnsiString         FCount;
    UFC::AnsiString         FURL;  
    UFC::AnsiString         FAccount;
    //UFC::AnsiString         FMsg;
    //unsigned int            FReqTick;
    UFC::AnsiString         FKey;
    int                     FIndex;
    int                     FUID;
    unsigned long           FReqTick;
    unsigned int            FStreamSize;
    CURL                    *Fcurl;
    
    UFC::PEvent             FLogonEvent; 
    UFC::PClientSocket      *FSocketObject;
    UFC::MemoryStream       FResponseData;    

protected: /// implement MessageListener interface
    virtual void OnMigoMessage( const UFC::AnsiString& Subject, const UFC::AnsiString& Key, MTree* Data );

protected: /// Implement interface PThread
    void Execute( void );    

protected:  ///Implement interface SocketClientListener
    void OnConnect( UFC::PClientSocket *Socket);
    void OnDisconnect( UFC::PClientSocket *Socket);
    BOOL OnDataArrived( UFC::PClientSocket *Socket);
    void OnIdle( UFC::PClientSocket *Socket);    

public:
    void Run( void );
    BOOL ProcessQueryRequest(UFC::AnsiString *pasErrMsg ); 
    void InQueue(MTree* mtData);
    void InitParameter(); 
    void ProcessQueryResponse(UFC::UInt8 *rcvb); 
    BOOL CURLFunc(UFC::AnsiString asURL,UFC::AnsiString *pasErrMsg);    
    void ReturnResponse(BOOL bAccept,UFC::AnsiString Msg,MTree* pTree);
    void ReturnResponse(BOOL bAccept,UFC::AnsiString Msg,MTree* pTree,UFC::AnsiString asMBusKey);
    
    //BOOL ProcFlowControl(MTree* mtData ,UFC::UInt32 iNowTick,UFC::AnsiString *pasErrMsg);
    BOOL ProcFlowControl(MTree* mtData ,UFC::AnsiString *pasErrMsg);
    BOOL ProcFlowControl(UFC::AnsiString asAccount,UFC::AnsiString QID,UFC::UInt32 iNowTick,UFC::AnsiString *pasErrMsg);     
    void MakeXMLErrMsg(UFC::AnsiString asErrMsg ,UFC::AnsiString *XMLMsg);
    
public:    
    MBusAuthorityListener(CheckSystemListener* pCheckSystemListener);
    virtual ~MBusAuthorityListener();
};

#endif /* MBUSAUTHORITYLISTENER_H_ */
