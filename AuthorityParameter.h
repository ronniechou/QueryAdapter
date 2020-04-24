#ifndef AUTHORITYPARAMETER_H
#define AUTHORITYPARAMETER_H

#define CLUSTER                                     // add white list to listen request from other IP (fail over)
#define COVER_IP_LIMIT 20

#include "UFC.h"
#define MSG_CULR_INIT_FAIL                      "FAIL TO INIT CURL"
#define MSG_GET_B2B_UK_FAIL                     "FAIL TO GET B2B UK"
#define REPLY_SUBJECT                           "NEWS.RESPONSE"
#define LISTEN_SUBJECT                          "NEWS.REQUEST.QUERY.WEB"
#define LOGON_TIMEOUT                            15
#define QUERYT_IMEOUT                            5
#define CLIENT_MBUS_KEY                         "ClientMbusKey"
#define QUERY_FUNCTION_NAME                     "QueryFunction"
#define CONFIG_SERVICE                          "SERVICE"
#define CONFIG_FILTER                           "FILTER"
#define CONFIG_URL                              "URL"

#define CLIENT_NID                              "NID"
#define CLIENT_UID                              "UID"
#define CLIENT_KEY                              "KEY"
#define CLIENT_PEERIP                           "PEERIP"
#define CLIENT_PeerIP                           "PeerIP"
#define CLIENT__IP                              "_IP"
#define CLIENT_MARKET                           "MARKET"
#define CLIENT_FUNC                             "FUNC"
#define CLIENT_INDEX                            "INDEX"
#define CLIENT_COUNT                            "COUNT"
#define CLIENT_DATA                             "DATA"
#define CLIENT_DATA_STREAM                      "STREAM"
#define CLIENT_ACTNO                            "actno"
#define CLIENT_REQ_TICK                         "REQ_TICK"
#define CLIENT_STREAM_SIZE                      "STREAMSIZE"

#define MSG_QUERY_FAIL_WRONG_SIZE               "伺服器回傳的資料量超過限制"
#define MSG_QUERY_FAIL_OVERMAXCOUNT             "超過執行次數限制,請稍候再試"

struct FlowControl {
    //UFC::UInt32 iTick;
    unsigned long iTick;
    int iCount;
}; 

extern UFC::AnsiString          g_asQueryFuncTable;
extern UFC::AnsiString          g_asListenSubject;
extern UFC::AnsiString          g_asListenKey;
extern UFC::AnsiString          g_asConfigName;                     // Config file name
extern UFC::AnsiString          g_asLogPath;  
extern UFC::Int32               g_iDEBUG_LEVEL;            
extern BOOL                     g_bRunning;
extern UFC::AnsiString          g_asLocalIP;
extern UFC::AnsiString          g_asB2B_UK;
extern UFC::AnsiString          g_asLogDate;
extern UFC::AnsiString          g_asAppName;
extern BOOL                     g_bFirstRun;
extern BOOL                     g_bUseCURL;
extern UFC::Int32               g_iAllowedPeriod; 
extern UFC::Int32               g_iAllowedCount; 
extern UFC::PHashMap<UFC::AnsiString, UFC::PHashMap<UFC::AnsiString,UFC::AnsiString*>* >   g_QueryFuncMap;
extern UFC::PHashMap<UFC::AnsiString,FlowControl* >   g_FlowControlMap;
//extern UFC::PHashMap<UFC::AnsiString,FlowControl >   g_FlowControlMap_ronnie;

extern UFC::List<UFC::Int32>    g_iListWhiteList;
extern UFC::AnsiString          g_asArrayCoverIPList[COVER_IP_LIMIT][5];
extern UFC::Int32               g_iArrayCoverIPCounts;

#endif /* AUTHORITYPARAMETER_H */

