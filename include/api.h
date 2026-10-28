#ifndef API_H
#define API_H

#include "httpserver.h"

/* Loads candidates.csv (or starts empty if it doesn't exist yet) into
 * the in-memory CandidateDB that every endpoint below operates on. */
void apiInit(const char *dataFile);

/* Routes one HTTP request to the right piece of the original C logic
 * (candidate.c / filter.c / sort.c / report.c / fileio.c) and writes
 * a response. This is the ONLY place that knows about HTTP -- all the
 * actual recruitment-shortlisting logic still lives in those original
 * files, completely unmodified. */
void handleRequest(HttpRequest *req, ClientSock sock);

#endif /* API_H */
