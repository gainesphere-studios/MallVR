/* MallVR Quest: Winsock names on BSD sockets (vpix_net.cpp builds unchanged) */
#pragma once
#include "windows.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
typedef int SOCKET; typedef unsigned long u_long; typedef unsigned short u_short;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
typedef struct { WORD wVersion; } WSADATA;
static inline int WSAStartup(WORD, WSADATA *) { return 0; }
static inline int WSACleanup(void) { return 0; }
static inline int WSAGetLastError(void) { return errno; }
static inline int closesocket(SOCKET s) { return close(s); }
static inline int ioctlsocket(SOCKET s, long cmd, u_long *v) { int i = (int)*v; return ioctl(s, cmd, &i); }
/* Winsock takes SO_RCVTIMEO / SO_SNDTIMEO as a DWORD of milliseconds, POSIX as a timeval */
static inline int vpix_setsockopt(SOCKET s, int lvl, int opt, const void *v, socklen_t n) {
    if (lvl == SOL_SOCKET && (opt == SO_RCVTIMEO || opt == SO_SNDTIMEO) && n == sizeof(DWORD)) {
        DWORD ms = *(const DWORD *)v; struct timeval tv; tv.tv_sec = ms / 1000; tv.tv_usec = (ms % 1000) * 1000;
        return setsockopt(s, lvl, opt, &tv, sizeof(tv));
    }
    return setsockopt(s, lvl, opt, v, n);
}
#define setsockopt vpix_setsockopt
/* Winsock recvfrom takes int * for the address length */
static inline ssize_t recvfrom(SOCKET s, char *b, int l, int f, struct sockaddr *a, int *al) { socklen_t n = al ? (socklen_t)*al : 0; ssize_t r = recvfrom(s, (void *)b, (size_t)l, f, a, al ? &n : NULL); if (al) *al = (int)n; return r; }
