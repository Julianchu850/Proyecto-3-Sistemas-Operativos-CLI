#ifndef GSEA_UTIL_H
#define GSEA_UTIL_H

#include <unistd.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CHUNK_SZ 65536

void die(const char* msg);
void warnx(const char* msg);

int     xopen_rd(const char* path);
int     xopen_wr_trunc(const char* path, mode_t mode);
ssize_t xread(int fd, void* buf, size_t n);
ssize_t xwrite(int fd, const void* buf, size_t n);
int     xclose(int fd);

#endif
