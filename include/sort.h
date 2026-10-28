#ifndef SORT_H
#define SORT_H

#include "candidate.h"

/* A comparator returns <0, 0, >0 the same way strcmp() does. Using
 * function pointers here lets sortCandidates() stay generic -- one
 * sort routine, many possible orderings.
 */
typedef int (*CompareFunc)(const Candidate *a, const Candidate *b);

int compareByName(const Candidate *a, const Candidate *b);
int compareByAgeAsc(const Candidate *a, const Candidate *b);
int compareByExperienceDesc(const Candidate *a, const Candidate *b);

/* In-place insertion sort (stable) driven by the supplied comparator. */
void sortCandidates(CandidateDB *db, CompareFunc cmp);

/* Binary search by name. ASSUMES db is already sorted by name
 * ascending (call sortCandidates(db, compareByName) first).
 * Returns the index of a matching candidate, or -1 if not found.
 */
int binarySearchByName(CandidateDB *db, const char *name);

#endif /* SORT_H */
