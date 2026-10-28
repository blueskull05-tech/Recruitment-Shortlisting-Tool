/* A deliberately small HTTP/1.1 server: no threading, no keep-alive,
 * one request handled per connection, then the connection is closed.
 * That's all a single local user's browser tab needs.
 *
 * Windows (Winsock) and POSIX (BSD sockets) are both supported via the
 * ifdefs below -- the same pattern already used in gui.c for the
 * ANSI-colour console code.
 */
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define CLOSESOCK closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define CLOSESOCK close
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR   (-1)
    typedef int SOCKET;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/httpserver.h"

static void socketsInit(void)
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

const char *contentTypeForPath(const char *path)
{
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".css")  == 0) return "text/css";
    if (strcmp(dot, ".js")   == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".csv")  == 0) return "text/csv";
    if (strcmp(dot, ".txt")  == 0) return "text/plain";
    if (strcmp(dot, ".ico")  == 0) return "image/x-icon";
    return "application/octet-stream";
}

void sendResponse(ClientSock sock, int statusCode, const char *statusText,
                   const char *contentType, const char *extraHeaders,
                   const char *body, size_t bodyLen)
{
    char header[1024];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "%s"
        "\r\n",
        statusCode, statusText, contentType, bodyLen,
        extraHeaders ? extraHeaders : "");

#ifdef _WIN32
    send((SOCKET) sock, header, hlen, 0);
    if (bodyLen > 0) send((SOCKET) sock, body, (int) bodyLen, 0);
#else
    send((int) sock, header, hlen, 0);
    if (bodyLen > 0) send((int) sock, body, bodyLen, 0);
#endif
}

void sendFileResponse(ClientSock sock, const char *filepath,
                       const char *contentType, int asAttachment,
                       const char *downloadName)
{
    FILE *f = fopen(filepath, "rb");
    if (!f)
    {
        const char *msg = "Not found";
        sendResponse(sock, 404, "Not Found", "text/plain", NULL, msg, strlen(msg));
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc((size_t) size);
    if (fread(buf, 1, (size_t) size, f) != (size_t) size)
    {
        /* still send whatever we got -- short reads are extremely
         * unlikely for local files but we don't want to crash */
    }
    fclose(f);

    char extra[300] = "";
    if (asAttachment && downloadName)
        snprintf(extra, sizeof(extra),
                 "Content-Disposition: attachment; filename=\"%s\"\r\n",
                 downloadName);

    sendResponse(sock, 200, "OK",
                 contentType ? contentType : contentTypeForPath(filepath),
                 extra, buf, (size_t) size);
    free(buf);
}

void runServer(int port, void (*handler)(HttpRequest *req, ClientSock sock))
{
    socketsInit();

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char *) &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* localhost only */
    addr.sin_port        = htons((unsigned short) port);

    if (bind(listenSock, (struct sockaddr *) &addr, sizeof(addr)) != 0)
    {
        printf("Could not bind to port %d -- is it already in use?\n", port);
        return;
    }
    listen(listenSock, 8);
    printf("Listening on http://localhost:%d\n", port);

    for (;;)
    {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        SOCKET clientSock = accept(listenSock, (struct sockaddr *) &clientAddr, &clientLen);
        if (clientSock == INVALID_SOCKET) continue;

        char *raw = malloc(HTTP_MAX_BODY);
        int total = 0, bodyStart = -1, contentLength = 0;

        while (total < HTTP_MAX_BODY - 1)
        {
#ifdef _WIN32
            int n = recv((SOCKET) clientSock, raw + total, HTTP_MAX_BODY - 1 - total, 0);
#else
            int n = recv((int) clientSock, raw + total, HTTP_MAX_BODY - 1 - total, 0);
#endif
            if (n <= 0) break;
            total += n;
            raw[total] = '\0';

            char *sep = strstr(raw, "\r\n\r\n");
            if (sep)
            {
                bodyStart = (int) (sep - raw) + 4;
                char *cl = strstr(raw, "Content-Length:");
                if (!cl) cl = strstr(raw, "content-length:");
                if (cl) contentLength = atoi(cl + 15);
                if (total - bodyStart >= contentLength) break;
            }
        }

        if (bodyStart >= 0)
        {
            HttpRequest req;
            memset(&req, 0, sizeof(req));

            sscanf(raw, "%7s %511s", req.method, req.path);
            char *q = strchr(req.path, '?');
            if (q)
            {
                strncpy(req.query, q + 1, sizeof(req.query) - 1);
                *q = '\0';
            }

            req.bodyLen = total - bodyStart;
            if (req.bodyLen < 0) req.bodyLen = 0;
            req.body = malloc((size_t) req.bodyLen + 1);
            memcpy(req.body, raw + bodyStart, (size_t) req.bodyLen);
            req.body[req.bodyLen] = '\0';

            handler(&req, (ClientSock) clientSock);
            free(req.body);
        }

        free(raw);
        CLOSESOCK((SOCKET) clientSock);
    }
}
