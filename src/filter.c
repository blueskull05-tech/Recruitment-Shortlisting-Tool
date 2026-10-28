#include <string.h>
#include "../include/filter.h"

int hasAllSkills(const char *candidateSkills, const char *requiredSkills)
{
    char reqCopy[SKILLS_LEN];
    strncpy(reqCopy, requiredSkills, SKILLS_LEN - 1);
    reqCopy[SKILLS_LEN - 1] = '\0';

    char *required = strtok(reqCopy, " ");

    while (required != NULL)
    {
        int found = 0;

        char candCopy[SKILLS_LEN];
        strncpy(candCopy, candidateSkills, SKILLS_LEN - 1);
        candCopy[SKILLS_LEN - 1] = '\0';

        char *skill = strtok(candCopy, "|");

        while (skill != NULL)
        {
            if (strcmp(skill, required) == 0)
            {
                found = 1;
                break;
            }
            skill = strtok(NULL, "|");
        }

        if (!found)
            return 0;

        required = strtok(NULL, " ");
    }

    return 1;
}

void filterShortlist(CandidateDB *db, int minAge, int minExperience,
                      const char *requiredSkills,
                      IndexList *shortlisted, IndexList *rejected)
{
    if (shortlisted) shortlisted->count = 0;
    if (rejected)     rejected->count    = 0;

    for (int i = 0; i < db->count; i++)
    {
        int eligible = db->list[i].age >= minAge &&
                       db->list[i].experience >= minExperience &&
                       hasAllSkills(db->list[i].skills, requiredSkills);

        if (eligible)
        {
            if (shortlisted)
                shortlisted->indices[shortlisted->count++] = i;
        }
        else
        {
            if (rejected)
                rejected->indices[rejected->count++] = i;
        }
    }
}
