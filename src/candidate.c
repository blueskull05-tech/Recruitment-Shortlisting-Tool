#include <stdio.h>
#include <string.h>
#include "../include/candidate.h"

void initDB(CandidateDB *db)
{
    db->count  = 0;
    db->nextId = 1;
}

int addCandidate(CandidateDB *db, const char *name, int age,
                  int experience, const char *skills)
{
    if (db->count >= MAX_CANDIDATES)
        return -1;

    Candidate *c = &db->list[db->count];

    c->id         = db->nextId;
    c->age        = age;
    c->experience = experience;

    strncpy(c->name, name, NAME_LEN - 1);
    c->name[NAME_LEN - 1] = '\0';

    strncpy(c->skills, skills, SKILLS_LEN - 1);
    c->skills[SKILLS_LEN - 1] = '\0';

    db->count++;
    return db->nextId++;
}

int findIndexById(CandidateDB *db, int id)
{
    for (int i = 0; i < db->count; i++)
        if (db->list[i].id == id)
            return i;
    return -1;
}

Candidate *findCandidateById(CandidateDB *db, int id)
{
    int idx = findIndexById(db, id);
    return (idx == -1) ? NULL : &db->list[idx];
}

int deleteCandidate(CandidateDB *db, int id)
{
    int idx = findIndexById(db, id);
    if (idx == -1)
        return 0;

    /* shift everything after idx one place to the left */
    for (int i = idx; i < db->count - 1; i++)
        db->list[i] = db->list[i + 1];

    db->count--;
    return 1;
}

int editCandidate(CandidateDB *db, int id, const char *name, int age,
                   int experience, const char *skills)
{
    Candidate *c = findCandidateById(db, id);
    if (!c)
        return 0;

    c->age        = age;
    c->experience = experience;

    strncpy(c->name, name, NAME_LEN - 1);
    c->name[NAME_LEN - 1] = '\0';

    strncpy(c->skills, skills, SKILLS_LEN - 1);
    c->skills[SKILLS_LEN - 1] = '\0';

    return 1;
}
