/*
* cryptopp_rsa_test.cc
*/
#include <cryptopp/randpool.h> 
#include <cryptopp/rsa.h> 
#include <cryptopp/hex.h>
#include <cryptopp/files.h> 
#include <iostream> 

using namespace std; 
using namespace CryptoPP; 

//#pragma comment(lib, "cryptlib.lib") 
//char priKey[] = "30820276020100300D06092A864886F70D0101010500048202603082025C02010002818100E2497AA07A7D9AD090BE33D485B3643F6B495D1E2A1B4275F9BAF65263A8C392A86EC62ADE81801AE1C36D600F83A480907575DC426C17E466789481267A880BF29747FD440E02A25D069D5B263038A0311394E6A5947360DF9DE40A65E08B388601CA45FFF7676346B590E38FD95C781A4EECCB6288324F3A0DD75A15050DC70201110281800D4F9DCD3461BDD00883A8B225FB7E5E155EAB1FE45BF4D9C36559C89C7356CC6442C05CDFE9800194CF42AB4C34EB8F178E7058402479E04243540798DA08009794BDA3746D7727F225E783783E6DCF6E9288249857C0057A2BBBBC90F11AF77C4B9DBA99D508F3344EE0D9701F9E474B6CEC0CD1B1C8B66D1D9CD9F9F0AE11024100F9F0EF0B3DC838F62DDF9F23FF169A2D0DAE7011479B33E0E21E82CBCD43C83FD514FEA5E64187AD4BFA09DA6444824243F6957A0B386A9289C65D91AF01B4F7024100E7C5C1174B00E0051AA29E7C2AF453ACCBAA1A6740257F22E097E9B8F899F8896FE7523BE290478F817E9898B97B5779D41CAA776A81739F71500F4ECD07C9B10241008452606F5CF187916394816D68EDD926E9201D365315EE49E11F362FA8E7A63FE94759A31F8C1AA70A1AF62853335404F6CDD6AA05F0B0E42AD26DC598E2C937024100CC8150057E6A2F138FF8E6315313D15C3B3BBCF1B11206C46BD155C153D3269762BD0C52F515E4C9EABAFF1D585DC5A7BB285A2D21BD842345DD3AAEF124FD41024100C009216716A87B7B85CBBA57971AFF1A0207F702350E6FFD88F1121324510E08380533CA5D2EB1A8CB5F02384A8DD29295A914C29F6291AC852BF5E63809A25E" ;
//char pubKey[] = "30819D300D06092A864886F70D010101050003818B0030818702818100E2497AA07A7D9AD090BE33D485B3643F6B495D1E2A1B4275F9BAF65263A8C392A86EC62ADE81801AE1C36D600F83A480907575DC426C17E466789481267A880BF29747FD440E02A25D069D5B263038A0311394E6A5947360DF9DE40A65E08B388601CA45FFF7676346B590E38FD95C781A4EECCB6288324F3A0DD75A15050DC7020111" ;
//char seed[] = "PeterWSH2010" ;
extern char priKey[] ;
extern char pubKey[] ;
extern char seed[] ;

//------------------------ 
// 函数声明
//------------------------ 
void GenerateRSAKey(unsigned int keyLength, const char *privFilename, const char *pubFilename, const char *seed); 
string RSAEncryptString(const char *pubFilename, const char *seed, const char *message); 
string RSADecryptString(const char *privFilename, const char *ciphertext); 
RandomPool & GlobalRNG(); 

//------------------------ 
// 主程序
//------------------------ 
/***
int main() 
{ 
    //char priKey[128] = {0}; 
    //char pubKey[128] = {0}; 
    //char seed[1024] = {0}; 

    // 生成 RSA 密钥对
    //strcpy(seed, "PeterWSH2010"); 

    //RSA 加解密
    char message[1024] = {0}; 
    strcpy(message, "just a test adkjfalfdjlksa;fdksa;fdsa; testttttt !"); 
    cout<<"Origin Text:\t"<<message<<endl<<endl; 
    string encryptedText = RSAEncryptString(pubKey, seed, message); // RSA 加密
    cout<<"Encrypted Text:\t"<<encryptedText<<endl<<endl; 
    string decryptedText = RSADecryptString(priKey, encryptedText.c_str()); // RSA  解密
    cout<<"Decrypted Text:\t"<<decryptedText<<endl<<endl; 
    
    return 0;
} 
***/

//------------------------ 
//生成 RSA 密钥对
//------------------------ 
/***
void GenerateRSAKey(unsigned int keyLength, const char *privFilename, const char *pubFilename, const char *seed) 
{ 
       RandomPool randPool; 
       randPool.Put((byte *)seed, strlen(seed)); 

       RSAES_OAEP_SHA_Decryptor priv(randPool, keyLength); 
       HexEncoder privFile(new FileSink(privFilename)); 
       priv.DEREncode(privFile); 
       privFile.MessageEnd(); 

       RSAES_OAEP_SHA_Encryptor pub(priv); 
       HexEncoder pubFile(new FileSink(pubFilename)); 
       pub.DEREncode(pubFile); 
       pubFile.MessageEnd(); 
} 
***/
//------------------------ 
// RSA 加密
//------------------------ 
string RSAEncryptString(const char *pubString, const char *seed, const char *message)
{
       StringSource pubSrc(pubString, true, new HexDecoder);
       RSAES_OAEP_SHA_Encryptor pub(pubSrc);

       RandomPool randPool;
       randPool.Put((byte *)seed, strlen(seed));

       string result;
       StringSource(message, true, new PK_EncryptorFilter(randPool, pub, new HexEncoder(new StringSink(result))));
       return result;
}

//------------------------ 
// RSA  解密
//------------------------ 
string RSADecryptString(const char *privString, const char *ciphertext)
{
       StringSource privSrc(privString, true, new HexDecoder);
       RSAES_OAEP_SHA_Decryptor priv(privSrc);

       string result;
       StringSource(ciphertext, true, new HexDecoder(new PK_DecryptorFilter(GlobalRNG(), priv, new StringSink(result))));
       return result;

}

//------------------------ 
// 定义全局的随机数池
//------------------------ 
RandomPool & GlobalRNG()
{
       static RandomPool randomPool;
       return randomPool;
}

