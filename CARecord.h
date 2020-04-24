#ifndef CARECORD_H
#define CARECORD_H

#include <iostream>
#include <fstream>

#include "AuthorityParameter.h"
#include "MSubscriber.h"
#include "MBusAuthorityListener.h"
//#include <map>

typedef UFC::PtrQueue<MTree>  TMemoryQueue;

class CARecord : public UFC::PThread
{
protected: 
    //TMemoryQueue                                      FAuthorityQueue;    
    BOOL                                                FIsStop;
    MBusAuthorityListener*                              FCAHandel;
    //UFC::List<UFC::AnsiString>                          FCAOkList;
    UFC::List<UFC::AnsiString>                          FRecordOkList;    
protected: /// Implement interface PThread
    void Execute( void );  
public:    
    CARecord();
    CARecord(MBusAuthorityListener* pCAHandel);
    virtual ~CARecord();    
public:
    void Run( void ); 
    void StopService();
    BOOL IsStop();        
    void ProcessCAData(UFC::AnsiString asCALogFile,int *len);
    BOOL RecordCAData(UFC::PStringList StrList,UFC::AnsiString *pasErrMsg,UFC::AnsiString *pasPostData);
    BOOL RecordCAData(UFC::AnsiString pasPostData,UFC::AnsiString *pasErrMsg);
    void CheckRecordResult();
    
    void ProcessCAData(UFC::AnsiString asCALogFile);    
    void ProcessCAData(fstream *File);
};

#endif /* CARECORD_H */

