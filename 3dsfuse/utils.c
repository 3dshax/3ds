#include "utils.h"

void sha256(void* buf, size_t size, uint8_t* result){
	SHA256_CTX ctx;
	SHA256_Init(&ctx);
	SHA256_Update(&ctx, buf, size);
	SHA256_Final(result, &ctx);
}
	
unsigned int getle32(const unsigned char* p)
{
	return (p[0]<<0) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

