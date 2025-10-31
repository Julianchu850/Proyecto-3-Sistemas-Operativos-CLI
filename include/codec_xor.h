#ifndef GSEA_CODEC_XOR_H
#define GSEA_CODEC_XOR_H

#include <stddef.h>

int xor_core_fd(int in_fd, int out_fd, const unsigned char* key, size_t klen);

#endif
