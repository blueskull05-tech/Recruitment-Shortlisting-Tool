/* This file is the ONLY bridge between the web GUI and your original
 * recruitment-shortlisting logic. Every endpoint below just calls
 * straight into candidate.c / filter.c / sort.c / report.c / fileio.c
 * -- none of that logic is reimplemented here or in JavaScript.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/candidate.h"
#include "../include/filter.h"
#include "../include/sort.h"
#include "../include/report.h"
#include "../include/fileio.h"
#include "../include/json.h"
#include "../include/api.h"

#define DATA_FILE_DEFAULT "candidates.csv"

static CandidateDB db;
static char        dataFilePath[256] = DATA_FILE_DEFAULT;

void apiInit(const char *dataFile)
{
    strncpy(dataFilePath, dataFile, sizeof(dataFilePath) - 1);

    if (loadCandidatesCSV(&db, dataFilePath) == -1)
        initDB(&db); /* no file yet -- start empty, same as original main() */
}

/* ---------------------------------------------------------------- */
/* small local helpers (HTTP/JSON glue only -- no business logic)   */
/* ---------------------------------------------------------------- */

static int containsCaseInsensitive(const char *haystack, const char *needle)
{
    if (!needle[0]) return 1;
    size_t hn = strlen(haystack), nn = strlen(needle);
    for (size_t i = 0; i + nn <= hn; i++)
    {
        size_t j = 0;
        while (j < nn && tolower((unsigned char) haystack[i + j]) ==
                          tolower((unsigned char) needle[j]))
            j++;
        if (j == nn) return 1;
    }
    return 0;
}

/* Parses "a=1&b=hello%20there" style query strings. */
static int queryGet(const char *query, const char *key, char *out, size_t outSize)
{
    out[0] = '\0';
    size_t keyLen = strlen(key);
    const char *p = query;

    while (p && *p)
    {
        const char *eq  = strchr(p, '=');
        if (!eq) break;
        const char *amp = strchr(p, '&');
        size_t klen = (size_t) (eq - p);
        const char *valStart = eq + 1;
        size_t vlen = amp ? (size_t) (amp - valStart) : strlen(valStart);

        if (klen == keyLen && strncmp(p, key, klen) == 0)
        {
            size_t oi = 0;
            for (size_t i = 0; i < vlen && oi + 1 < outSize; i++)
            {
                char ch = valStart[i];
                if (ch == '+') out[oi++] = ' ';
                else if (ch == '%' && i + 2 < vlen)
                {
                    char hex[3] = { valStart[i + 1], valStart[i + 2], 0 };
                    out[oi++] = (char) strtol(hex, NULL, 16);
                    i += 2;
                }
                else out[oi++] = ch;
            }
            out[oi] = '\0';
            return 1;
        }
        if (!amp) break;
        p = amp + 1;
    }
    return 0;
}

static void appendCandidateJSON(StrBuf *sb, Candidate *c)
{
    sbAppendChar(sb, '{');
    sbAppend(sb, "\"id\":");         sbAppendInt(sb, c->id);
    sbAppend(sb, ",\"name\":");      sbAppendEscaped(sb, c->name);
    sbAppend(sb, ",\"age\":");       sbAppendInt(sb, c->age);
    sbAppend(sb, ",\"experience\":");sbAppendInt(sb, c->experience);
    sbAppend(sb, ",\"skills\":");    sbAppendEscaped(sb, c->skills); /* '|' separated, same as CSV */
    sbAppendChar(sb, '}');
}

static void appendStatsJSON(StrBuf *sb, Statistics *s)
{
    sbAppendChar(sb, '{');
    sbAppend(sb, "\"totalCount\":");        sbAppendInt(sb, s->totalCount);
    sbAppend(sb, ",\"shortlistedCount\":");  sbAppendInt(sb, s->shortlistedCount);
    sbAppend(sb, ",\"rejectedCount\":");     sbAppendInt(sb, s->rejectedCount);
    sbAppend(sb, ",\"avgAge\":");            sbAppendDouble(sb, s->avgAge);
    sbAppend(sb, ",\"avgExperience\":");     sbAppendDouble(sb, s->avgExperience);
    sbAppend(sb, ",\"maxExperience\":");     sbAppendInt(sb, s->maxExperience);
    sbAppend(sb, ",\"minExperience\":");     sbAppendInt(sb, s->minExperience);
    sbAppend(sb, ",\"mostExperiencedName\":");sbAppendEscaped(sb, s->mostExperiencedName);
    sbAppendChar(sb, '}');
}

static void sendJSON(ClientSock sock, int status, const char *statusText, StrBuf *sb)
{
    sendResponse(sock, status, statusText, "application/json", NULL, sb->data, sb->len);
    sbFree(sb);
}

static void sendJSONError(ClientSock sock, int status, const char *statusText, const char *message)
{
    StrBuf sb; sbInit(&sb);
    sbAppend(&sb, "{\"error\":"); sbAppendEscaped(&sb, message); sbAppendChar(&sb, '}');
    sendJSON(sock, status, statusText, &sb);
}

/* ---------------------------------------------------------------- */
/* endpoint handlers                                                 */
/* ---------------------------------------------------------------- */

static void handleListCandidates(HttpRequest *req, ClientSock sock)
{
    char search[NAME_LEN] = "", sortKey[32] = "";
    queryGet(req->query, "search", search, sizeof(search));
    queryGet(req->query, "sort", sortKey, sizeof(sortKey));

    /* Sorting mutates db in place, exactly like the original console
     * app's "Sort Candidates" menu option -- the new order sticks
     * around for subsequent views until sorted again. */
    if      (strcmp(sortKey, "name") == 0)       sortCandidates(&db, compareByName);
    else if (strcmp(sortKey, "age") == 0)        sortCandidates(&db, compareByAgeAsc);
    else if (strcmp(sortKey, "experience") == 0) sortCandidates(&db, compareByExperienceDesc);

    StrBuf sb; sbInit(&sb);
    sbAppendChar(&sb, '[');
    int first = 1;
    for (int i = 0; i < db.count; i++)
    {
        Candidate *c = &db.list[i];
        if (search[0] && !containsCaseInsensitive(c->name, search)) continue;
        if (!first) sbAppendChar(&sb, ',');
        appendCandidateJSON(&sb, c);
        first = 0;
    }
    sbAppendChar(&sb, ']');
    sendJSON(sock, 200, "OK", &sb);
}

static void handleAddCandidate(HttpRequest *req, ClientSock sock)
{
    char name[NAME_LEN] = "", skills[SKILLS_LEN] = "";
    int age = 0, experience = 0;

    jsonGetString(req->body, "name", name, sizeof(name));
    jsonGetInt(req->body, "age", &age);
    jsonGetInt(req->body, "experience", &experience);
    jsonGetString(req->body, "skills", skills, sizeof(skills));

    if (name[0] == '\0')
    {
        sendJSONError(sock, 400, "Bad Request", "Name is required.");
        return;
    }

    int id = addCandidate(&db, name, age, experience, skills);
    if (id == -1)
    {
        sendJSONError(sock, 400, "Bad Request", "Database is full -- could not add candidate.");
        return;
    }

    Candidate *c = findCandidateById(&db, id);
    StrBuf sb; sbInit(&sb);
    appendCandidateJSON(&sb, c);
    sendJSON(sock, 201, "Created", &sb);
}

static void handleEditCandidate(HttpRequest *req, ClientSock sock, int id)
{
    Candidate *existing = findCandidateById(&db, id);
    if (!existing)
    {
        sendJSONError(sock, 404, "Not Found", "No candidate with that ID.");
        return;
    }

    char name[NAME_LEN], skills[SKILLS_LEN];
    strncpy(name, existing->name, NAME_LEN - 1);   name[NAME_LEN - 1] = '\0';
    strncpy(skills, existing->skills, SKILLS_LEN - 1); skills[SKILLS_LEN - 1] = '\0';
    int age = existing->age, experience = existing->experience;

    jsonGetString(req->body, "name", name, sizeof(name));
    jsonGetInt(req->body, "age", &age);
    jsonGetInt(req->body, "experience", &experience);
    jsonGetString(req->body, "skills", skills, sizeof(skills));

    editCandidate(&db, id, name, age, experience, skills);

    Candidate *updated = findCandidateById(&db, id);
    StrBuf sb; sbInit(&sb);
    appendCandidateJSON(&sb, updated);
    sendJSON(sock, 200, "OK", &sb);
}

static void handleDeleteCandidate(ClientSock sock, int id)
{
    if (deleteCandidate(&db, id))
    {
        StrBuf sb; sbInit(&sb);
        sbAppend(&sb, "{\"deleted\":true}");
        sendJSON(sock, 200, "OK", &sb);
    }
    else
    {
        sendJSONError(sock, 404, "Not Found", "No candidate with that ID.");
    }
}

/* Shared by /api/filter and /api/report -- both read the same three
 * criteria fields, exactly like doFilter()/doExportReport() in the
 * original console main.c. */
static void readFilterCriteria(const char *body, int *minAge, int *minExp, char *skills, size_t skillsSize)
{
    *minAge = 0; *minExp = 0; skills[0] = '\0';
    jsonGetInt(body, "minAge", minAge);
    jsonGetInt(body, "minExperience", minExp);
    jsonGetString(body, "skills", skills, skillsSize);
}

static void handleFilter(HttpRequest *req, ClientSock sock)
{
    int minAge, minExp;
    char skills[SKILLS_LEN];
    readFilterCriteria(req->body, &minAge, &minExp, skills, sizeof(skills));

    IndexList shortlisted, rejected;
    filterShortlist(&db, minAge, minExp, skills, &shortlisted, &rejected);

    Statistics stats;
    computeStatistics(&db, &shortlisted, &rejected, &stats);

    StrBuf sb; sbInit(&sb);
    sbAppend(&sb, "{\"shortlisted\":[");
    for (int i = 0; i < shortlisted.count; i++)
    {
        if (i) sbAppendChar(&sb, ',');
        appendCandidateJSON(&sb, &db.list[shortlisted.indices[i]]);
    }
    sbAppend(&sb, "],\"rejected\":[");
    for (int i = 0; i < rejected.count; i++)
    {
        if (i) sbAppendChar(&sb, ',');
        appendCandidateJSON(&sb, &db.list[rejected.indices[i]]);
    }
    sbAppend(&sb, "],\"stats\":");
    appendStatsJSON(&sb, &stats);
    sbAppendChar(&sb, '}');

    sendJSON(sock, 200, "OK", &sb);
}

static void handleReport(HttpRequest *req, ClientSock sock)
{
    int minAge, minExp;
    char skills[SKILLS_LEN];
    readFilterCriteria(req->body, &minAge, &minExp, skills, sizeof(skills));

    IndexList shortlisted, rejected;
    filterShortlist(&db, minAge, minExp, skills, &shortlisted, &rejected);

    Statistics stats;
    computeStatistics(&db, &shortlisted, &rejected, &stats);

    const char *reportFile = "shortlist_report.txt";
    if (exportReport(&db, &shortlisted, &rejected, &stats, reportFile) != 0)
    {
        sendJSONError(sock, 500, "Internal Server Error", "Could not write report file.");
        return;
    }
    sendFileResponse(sock, reportFile, "text/plain", 1, "shortlist_report.txt");
}

static void handleExportCSV(ClientSock sock)
{
    const char *tmpFile = "candidates_export.csv";
    if (saveCandidatesCSV(&db, tmpFile) != 0)
    {
        sendJSONError(sock, 500, "Internal Server Error", "Could not write CSV file.");
        return;
    }
    sendFileResponse(sock, tmpFile, "text/csv", 1, "candidates.csv");
}

static void handleImportCSV(HttpRequest *req, ClientSock sock)
{
    const char *tmpFile = "import_tmp.csv";
    FILE *f = fopen(tmpFile, "w");
    if (!f)
    {
        sendJSONError(sock, 500, "Internal Server Error", "Could not stage import file.");
        return;
    }
    fwrite(req->body, 1, (size_t) req->bodyLen, f);
    fclose(f);

    CandidateDB scratch;
    int n = loadCandidatesCSV(&scratch, tmpFile);
    remove(tmpFile);

    if (n < 0)
    {
        sendJSONError(sock, 400, "Bad Request", "Could not read that CSV file.");
        return;
    }

    int imported = 0;
    for (int i = 0; i < scratch.count; i++)
    {
        Candidate *c = &scratch.list[i];
        if (addCandidate(&db, c->name, c->age, c->experience, c->skills) != -1)
            imported++;
    }

    StrBuf sb; sbInit(&sb);
    sbAppend(&sb, "{\"imported\":"); sbAppendInt(&sb, imported); sbAppendChar(&sb, '}');
    sendJSON(sock, 200, "OK", &sb);
}

static void handleStats(ClientSock sock)
{
    Statistics stats;
    computeStatistics(&db, NULL, NULL, &stats);
    StrBuf sb; sbInit(&sb);
    appendStatsJSON(&sb, &stats);
    sendJSON(sock, 200, "OK", &sb);
}

static void handleSave(ClientSock sock)
{
    int ok = (saveCandidatesCSV(&db, dataFilePath) == 0);
    StrBuf sb; sbInit(&sb);
    sbAppend(&sb, "{\"saved\":"); sbAppend(&sb, ok ? "true" : "false"); sbAppendChar(&sb, '}');
    sendJSON(sock, ok ? 200 : 500, ok ? "OK" : "Internal Server Error", &sb);
}

static void handleExit(ClientSock sock)
{
    saveCandidatesCSV(&db, dataFilePath);
    StrBuf sb; sbInit(&sb);
    sbAppend(&sb, "{\"saved\":true,\"exiting\":true}");
    sendJSON(sock, 200, "OK", &sb);
    exit(0); /* response above has already been sent over the socket */
}

/* ---------------------------------------------------------------- */
/* router                                                            */
/* ---------------------------------------------------------------- */

void handleRequest(HttpRequest *req, ClientSock sock)
{
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/") == 0)
    {
        sendFileResponse(sock, "web/index.html", "text/html", 0, NULL);
        return;
    }
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/candidates") == 0)
    {
        handleListCandidates(req, sock);
        return;
    }
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/candidates") == 0)
    {
        handleAddCandidate(req, sock);
        return;
    }
    if (strncmp(req->path, "/api/candidates/", 16) == 0)
    {
        int id = atoi(req->path + 16);
        if (strcmp(req->method, "PUT") == 0)    { handleEditCandidate(req, sock, id); return; }
        if (strcmp(req->method, "DELETE") == 0) { handleDeleteCandidate(sock, id);    return; }
    }
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/filter") == 0)
    {
        handleFilter(req, sock);
        return;
    }
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/report") == 0)
    {
        handleReport(req, sock);
        return;
    }
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/export/csv") == 0)
    {
        handleExportCSV(sock);
        return;
    }
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/import/csv") == 0)
    {
        handleImportCSV(req, sock);
        return;
    }
    if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/api/stats") == 0)
    {
        handleStats(sock);
        return;
    }
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/save") == 0)
    {
        handleSave(sock);
        return;
    }
    if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/api/exit") == 0)
    {
        handleExit(sock);
        return;
    }

    const char *msg = "Not found";
    sendResponse(sock, 404, "Not Found", "text/plain", NULL, msg, strlen(msg));
}
