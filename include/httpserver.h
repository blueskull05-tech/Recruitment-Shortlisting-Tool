#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <stddef.h>
#include <stdint.h>

/* Kept deliberately small: one request per connection, blocking,
 * single-threaded. Plenty for a single-user local tool -- the same
 * "single-user, single-file" spirit as the original console app. */

#define HTTP_MAX_PATH 512
#define HTTP_MAX_BODY 262144 /* 256 KB, comfortably more than MAX_CANDIDATES needs */

/* Opaque handle for a connected client socket. Kept as an integer type
 * (rather than pulling OS socket headers into every file that needs
 * to send a response) so api.c / main_server.c never have to care
 * whether they're talking to Winsock or BSD sockets underneath. */
typedef intptr_t ClientSock;

typedef struct {
    char  method[8];
    char  path[HTTP_MAX_PATH];
    char  query[HTTP_MAX_PATH];
    char *body;     /* heap-allocated; always non-NULL, may be "" */
    int   bodyLen;
} HttpRequest;

void sendResponse(ClientSock sock, int statusCode, const char *statusText,
                   const char *contentType, const char *extraHeaders,
                   const char *body, size_t bodyLen);

/* Streams a file from disk as the response body. If asAttachment is
 * true, adds a Content-Disposition header so the browser downloads it
 * (using downloadName) instead of trying to display it. */
void sendFileResponse(ClientSock sock, const char *filepath,
                       const char *contentType, int asAttachment,
                       const char *downloadName);

const char *contentTypeForPath(const char *path);

/* Binds to 127.0.0.1:port and serves forever, calling handler() once
 * per request. Never returns under normal operation. */
void runServer(int port, void (*handler)(HttpRequest *req, ClientSock sock));

#endif /* HTTPSERVER_H */
