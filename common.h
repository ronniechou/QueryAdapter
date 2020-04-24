#ifndef COMMON_H
#define COMMON_H

#include "UFC.h"
#include "AuthorityParameter.h"

UFC::AnsiString base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& encoded_string);
BOOL IsBigEndian();
void EndianSwap(unsigned int& x);
char* ConvertIntIPtoString(UFC::Int32 intIP);
void PrintWhiteList(void);

#endif /* COMMON_H */

