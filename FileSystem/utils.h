#include "header.h"

BYTE* decToHexaLE(unsigned int num, int n);
BYTE* createBlankOffets(int n);
BYTE hexToByte(const std::string& hexString);
string byteToHex(unsigned char byte);
int reverseByte(BYTE* byte, unsigned int count);
LPCWSTR stringToLPCWSTR(string orig);