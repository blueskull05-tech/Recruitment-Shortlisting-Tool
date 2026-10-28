#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/json.h"

void sbInit(StrBuf *sb)
{
    sb->cap  = 256;
    sb->len  = 0;
    sb->data = malloc(sb->cap);
    sb->data[0] = '\0';
}

void sbFree(StrBuf *sb)
{
    free(sb->data);
    sb->data = NULL;
    sb->len = sb->cap = 0;
}

static void sbEnsure(StrBuf *sb, size_t extra)
{
    if (sb->len + extra + 1 > sb->cap)
    {
        while (sb->len + extra + 1 > sb->cap)
            sb->cap *= 2;
        sb->data = realloc(sb->data, sb->cap);
    }
}

void sbAppendChar(StrBuf *sb, char c)
{
    sbEnsure(sb, 1);
    sb->data[sb->len++] = c;
    sb->data[sb->len]   = '\0';
}

void sbAppend(StrBuf *sb, const char *s)
{
    size_t n = strlen(s);
    sbEnsure(sb, n);
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

void sbAppendEscaped(StrBuf *sb, const char *s)
{
    sbAppendChar(sb, '"');
    for (const char *p = s; *p; p++)
    {
        if (*p == '"' || *p == '\\')
        {
            sbAppendChar(sb, '\\');
            sbAppendChar(sb, *p);
        }
        else if (*p == '\n')
        {
            sbAppend(sb, "\\n");
        }
        else if ((unsigned char) *p < 0x20)
        {
            /* skip other control chars */
        }
        else
        {
            sbAppendChar(sb, *p);
        }
    }
    sbAppendChar(sb, '"');
}

void sbAppendInt(StrBuf *sb, int v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", v);
    sbAppend(sb, buf);
}

void sbAppendDouble(StrBuf *sb, double v)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f", v);
    sbAppend(sb, buf);
}

/* Finds "key": in json and returns a pointer just past the colon
 * (whitespace skipped), or NULL if not found. */
static const char *findValue(const char *json, const char *key)
{
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char *p = strstr(json, pattern);
    if (!p) return NULL;
    p += strlen(pattern);

    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != ':') return NULL;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

int jsonGetInt(const char *json, const char *key, int *out)
{
    const char *p = findValue(json, key);
    if (!p) return 0;
    char *end;
    long v = strtol(p, &end, 10);
    if (end == p) return 0;
    *out = (int) v;
    return 1;
}

int jsonGetString(const char *json, const char *key, char *out, size_t outSize)
{
    const char *p = findValue(json, key);
    if (!p || *p != '"')
    {
        out[0] = '\0';
        return 0;
    }
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < outSize)
    {
        if (*p == '\\' && *(p + 1))
        {
            p++;
            out[i++] = (*p == 'n') ? '\n' : *p;
            p++;
        }
        else
        {
            out[i++] = *p++;
        }
    }
    out[i] = '\0';
    return 1;
}
