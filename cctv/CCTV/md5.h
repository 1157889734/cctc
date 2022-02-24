#ifndef _MD5_H_
#define _MD5_H_
/* POINTER defines a generic pointer type */  
typedef unsigned char * POINTER;  

/* UINT2 defines a two byte word */  
//typedef unsigned short int UINT2;  

/* UINT4 defines a four byte word */  
typedef unsigned long int UINT4;  


/* MD5 context. */  
typedef struct {  
    UINT4 state[4];                                   /* state (ABCD) */  
    UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */  
    unsigned char buffer[64];                         /* input buffer */  
}__attribute__((packed)) MD5_CTX;  
#ifdef __cplusplus
extern "C"{
#endif 

void MD5_Init (MD5_CTX *context);  
void MD5_Update (MD5_CTX *context, unsigned char *input, unsigned int inputLen);  
void MD5_UpdaterString(MD5_CTX *context,const char *string);
int  MD5_FileUpdateFile (MD5_CTX *context,char *filename);
void MD5_Final (unsigned char digest[16], MD5_CTX *context);  
void MD5_String (char *string,unsigned char digest[16]);  
int  MD5_File (char *filename,unsigned char digest[16]);  

/*example
    unsigned char digest[16];  //存放结果
    unsigned char digest2[16];  //存放结果
    int i =0;
    //第一种用法:

    MD5_CTX md5c;
    memset(digest,0,16);
    memset(digest2,0,16);

    MD5Init(&md5c); //初始化
    MD5UpdaterString(&md5c,"123456");
    //MD5FileUpdateFile(&md5c,"你要测试的文件路径");
    MD5Final(digest2,&md5c);
    for(i =0;i<16;i++)
    {
        printf("%02x ",digest2[i]);
    }
    printf("\n");

    //第二种用法:
    MDString("123456",digest); //直接输入字符串并得出结果
   for(i =0;i<16;i++)
   {
       printf("%02x ",digest[i]);
   }
    //第三种用法:
    MD5File("你要测试的文件路径",digest); //直接输入文件路径并得出结果
*/
#ifdef __cplusplus
}
#endif 
	
		
#endif


