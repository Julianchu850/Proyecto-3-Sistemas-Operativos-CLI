#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void die(const char* msg) { perror(msg); exit(1); }
void warnx(const char* msg) { fprintf(stderr, "%s\n", msg); }

int xopen_rd(const char* path) {
    int fd = open(path, O_RDONLY);
    return fd;
}
int xopen_wr_trunc(const char* path, mode_t mode) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
    return fd;
}
ssize_t xread(int fd, void* buf, size_t n) {
    for (;;) {
        ssize_t r = read(fd, buf, n);
        if (r < 0 && errno == EINTR) continue;
        return r;
    }
}
ssize_t xwrite(int fd, const void* buf, size_t n) {
    const char* p = (const char*)buf;
    size_t left = n;
    while (left > 0) {
        ssize_t w = write(fd, p, left);
        if (w < 0 && errno == EINTR) continue;
        if (w < 0) return w;
        left -= (size_t)w;
        p    += w;
    }
    return (ssize_t)n;
}
int xclose(int fd) {
    for (;;) {
        int r = close(fd);
        if (r < 0 && errno == EINTR) continue;
        return r;
    }
}
