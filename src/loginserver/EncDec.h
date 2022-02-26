#pragma once

#include <iostream>
#include <openssl/aes.h>
#include <openssl/md5.h>

using namespace std;
/*
class Aes
{
public:
    Aes(const string& strKey);
    
    int Encrypt(const char* pInData, uint32_t nInLen, char** ppOutData, uint32_t& nOutLen);
    int Decrypt(const char* pInData, uint32_t nInLen, char** ppOutData, uint32_t& nOutLen);
    void Free(char* pData);
    
private:
    AES_KEY m_cEncKey;
    AES_KEY m_cDecKey;
    
};
*/
class Md5
{
public:
    static void MD5_Calculate (const char* pContent, unsigned int nLen, char* md5);
};