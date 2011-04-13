#ifndef __HELPER_H__
#define __HELPER_H__

void md5_print(u8 *buf);
void md5_buf(u8 *buf, u8 *out, size_t len);
void xor(u8 *in, u32 len, u8 *out, u8 *key, u32 keylen);
void hexdump(void *ptr, int buflen);

#endif
