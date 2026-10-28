#ifndef JSON_H
#define JSON_H

#include <stddef.h>

/* A tiny growable string buffer, used to build JSON responses without
 * worrying about fixed-size buffer overflows (candidate lists can get
 * long). Not a general JSON library -- just enough for this app. */
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} StrBuf;

void sbInit(StrBuf *sb);
void sbFree(StrBuf *sb);
void sbAppend(StrBuf *sb, const char *s);
void sbAppendChar(StrBuf *sb, char c);
void sbAppendEscaped(StrBuf *sb, const char *s); /* wraps in quotes + escapes */
void sbAppendInt(StrBuf *sb, int v);
void sbAppendDouble(StrBuf *sb, double v);

/* Schema-aware helpers for reading the small, fixed-shape JSON bodies
 * this app's own frontend sends (not a general-purpose parser).
 * Return 1 if the key was found, 0 otherwise. */
int jsonGetInt(const char *json, const char *key, int *out);
int jsonGetString(const char *json, const char *key, char *out, size_t outSize);

#endif /* JSON_H */
