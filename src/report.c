#include <stdio.h>
#include <string.h>
#include "../include/report.h"

void computeStatistics(CandidateDB *db, IndexList *shortlisted,
                        IndexList *rejected, Statistics *stats)
{
    stats->totalCount       = db->count;
    stats->shortlistedCount = shortlisted ? shortlisted->count : 0;
    stats->rejectedCount    = rejected    ? rejected->count    : 0;

    stats->avgAge        = 0.0;
    stats->avgExperience = 0.0;
    stats->maxExperience = 0;
    stats->minExperience = (db->count > 0) ? db->list[0].experience : 0;
    stats->mostExperiencedName[0] = '\0';

    if (db->count == 0)
        return;

    long ageSum = 0, expSum = 0;

    for (int i = 0; i < db->count; i++)
    {
        Candidate *c = &db->list[i];

        ageSum += c->age;
        expSum += c->experience;

        if (c->experience > stats->maxExperience)
        {
            stats->maxExperience = c->experience;
            strncpy(stats->mostExperiencedName, c->name, NAME_LEN - 1);
            stats->mostExperiencedName[NAME_LEN - 1] = '\0';
        }
        if (c->experience < stats->minExperience)
            stats->minExperience = c->experience;
    }

    stats->avgAge        = (double) ageSum / db->count;
    stats->avgExperience = (double) expSum / db->count;
}

static void writeCandidateRow(FILE *f, Candidate *c)
{
    fprintf(f, "%-15s %-6d %-12d %-35s\n",
            c->name, c->age, c->experience, c->skills);
}

int exportReport(CandidateDB *db, IndexList *shortlisted,
                  IndexList *rejected, Statistics *stats,
                  const char *filename)
{
    FILE *f = fopen(filename, "w");
    if (!f)
        return -1;

    fprintf(f, "========================================================\n");
    fprintf(f, "        RECRUITMENT SHORTLISTING REPORT\n");
    fprintf(f, "========================================================\n\n");

    fprintf(f, "SHORTLISTED CANDIDATES (%d)\n", shortlisted ? shortlisted->count : 0);
    fprintf(f, "%-15s %-6s %-12s %-35s\n", "Name", "Age", "Experience", "Skills");
    if (shortlisted)
        for (int i = 0; i < shortlisted->count; i++)
            writeCandidateRow(f, &db->list[shortlisted->indices[i]]);

    fprintf(f, "\nREJECTED CANDIDATES (%d)\n", rejected ? rejected->count : 0);
    fprintf(f, "%-15s %-6s %-12s %-35s\n", "Name", "Age", "Experience", "Skills");
    if (rejected)
        for (int i = 0; i < rejected->count; i++)
            writeCandidateRow(f, &db->list[rejected->indices[i]]);

    fprintf(f, "\n========================================================\n");
    fprintf(f, "STATISTICS\n");
    fprintf(f, "========================================================\n");
    fprintf(f, "Total candidates      : %d\n", stats->totalCount);
    fprintf(f, "Shortlisted           : %d\n", stats->shortlistedCount);
    fprintf(f, "Rejected              : %d\n", stats->rejectedCount);
    fprintf(f, "Average age           : %.2f\n", stats->avgAge);
    fprintf(f, "Average experience    : %.2f years\n", stats->avgExperience);
    fprintf(f, "Max experience        : %d years", stats->maxExperience);
    if (stats->mostExperiencedName[0])
        fprintf(f, " (%s)", stats->mostExperiencedName);
    fprintf(f, "\n");
    fprintf(f, "Min experience        : %d years\n", stats->minExperience);
    fprintf(f, "========================================================\n");

    fclose(f);
    return 0;
}
