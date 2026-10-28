#include <string.h>
#include "../include/sort.h"

int compareByName(const Candidate *a, const Candidate *b)
{
    return strcmp(a->name, b->name);
}

int compareByAgeAsc(const Candidate *a, const Candidate *b)
{
    return a->age - b->age;
}

int compareByExperienceDesc(const Candidate *a, const Candidate *b)
{
    return b->experience - a->experience;
}

void sortCandidates(CandidateDB *db, CompareFunc cmp)
{
    /* Insertion sort: simple, stable, and easy to reason about/explain
     * in a viva -- good fit for an academic exam answer. */
    for (int i = 1; i < db->count; i++)
    {
        Candidate key = db->list[i];
        int j = i - 1;

        while (j >= 0 && cmp(&db->list[j], &key) > 0)
        {
            db->list[j + 1] = db->list[j];
            j--;
        }
        db->list[j + 1] = key;
    }
}

int binarySearchByName(CandidateDB *db, const char *name)
{
    int lo = 0, hi = db->count - 1;

    while (lo <= hi)
    {
        int mid = lo + (hi - lo) / 2;
        int cmp = strcmp(db->list[mid].name, name);

        if (cmp == 0)
            return mid;
        else if (cmp < 0)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return -1;
}
