#ifndef REPORT_H
#define REPORT_H

#include "candidate.h"
#include "filter.h"

typedef struct {
    int    totalCount;
    int    shortlistedCount;
    int    rejectedCount;
    double avgAge;
    double avgExperience;
    int    maxExperience;
    int    minExperience;
    char   mostExperiencedName[NAME_LEN];
} Statistics;

void computeStatistics(CandidateDB *db, IndexList *shortlisted,
                        IndexList *rejected, Statistics *stats);

/* Writes a formatted text report (shortlisted, rejected, statistics)
 * to filename. Returns 0 on success, -1 on failure.
 */
int exportReport(CandidateDB *db, IndexList *shortlisted,
                  IndexList *rejected, Statistics *stats,
                  const char *filename);

#endif /* REPORT_H */
