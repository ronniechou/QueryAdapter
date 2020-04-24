#include "common.h"

static const string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; 

UFC::AnsiString base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) 
{
    //std::string ret;
    
    UFC::AnsiString ret;
    
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
 
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3){
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
 
    if (i)
    {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '='; 
    } 
    return ret;
}
//------------------------------------------------------------------------------
static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}
//------------------------------------------------------------------------------
std::string base64_decode(std::string const& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j <4; j++)
            char_array_4[j] = 0;

        for (j = 0; j <4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) 
            ret += char_array_3[j];
    }
    return ret;
}
//------------------------------------------------------------------------------
BOOL IsBigEndian()
{
    unsigned int i = 0x0001;
    char *c = (char*)&i;    
    if (*c)    
        return false;
    else
        return true;
}
//------------------------------------------------------------------------------
void EndianSwap(unsigned int& x)
{
x = (x>>24) |
    ((x<<8) & 0x00FF0000) |
    ((x>>8) & 0x0000FF00) |
    (x<<24);
}
//------------------------------------------------------------------------------
char* ConvertIntIPtoString(UFC::Int32 intIP)
{
    UFC::AnsiString asIP;
    char* ptr;
    struct sockaddr_in SocketAddress;
    
    SocketAddress.sin_addr.s_addr = intIP;
    //memcpy(  &SocketAddress.sin_addr ,   &intIP,   sizeof(struct sockaddr) );
    ptr = inet_ntoa( SocketAddress.sin_addr );      
    return ptr;    
}
//------------------------------------------------------------------------------
void PrintWhiteList(void)
{
    UFC::AnsiString asString = "white list=";
    asString.Printf("counts=%d,",g_iListWhiteList.ItemCount());
    for(int i=0;i<g_iListWhiteList.ItemCount();i++)
    {
        UFC::Int32 iIP = g_iListWhiteList[i];
        asString.AppendPrintf("ip=%d(%s),",iIP,ConvertIntIPtoString(iIP) );
    }
    UFC::BufferedLog::Printf(" [%s][%s] %s", __FILE__,__func__,asString.c_str() ); 
}
//------------------------------------------------------------------------------



