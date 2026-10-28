#ifndef FILTER_H
#define FILTER_H

#include "candidate.h"

/* Holds the indices (into a CandidateDB.list[]) that matched/didn't
 * match a filter, so we never need to copy whole Candidate structs.
 */
typedef struct {
    int indices[MAX_CANDIDATES];
    int count;
} IndexList;

/* Returns 1 if candidateSkills contains every space-separated skill
 * listed in requiredSkills, 0 otherwise. Skills in candidateSkills are
 * '|' separated, skills in requiredSkills are ' ' separated.
 */
int hasAllSkills(const char *candidateSkills, const char *requiredSkills);

/* Splits db into two IndexLists based on minAge / minExperience /
 * requiredSkills. Either output pointer may be NULL if not needed.
 */
void filterShortlist(CandidateDB *db, int minAge, int minExperience,
                      const char *requiredSkills,
                      IndexList *shortlisted, IndexList *rejected);

#endif /* FILTER_H */
