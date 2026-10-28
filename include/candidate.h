#ifndef CANDIDATE_H
#define CANDIDATE_H

#define MAX_CANDIDATES 200
#define NAME_LEN       100
#define SKILLS_LEN     300

/* One candidate record.
 * skills is stored as a '|' separated string, e.g. "C|Java|SQL"
 */
typedef struct {
    int  id;
    char name[NAME_LEN];
    int  age;
    int  experience;
    char skills[SKILLS_LEN];
} Candidate;

/* In-memory "database" -- a fixed array + a live count.
 * Kept as a struct (instead of loose globals) so it can be passed
 * around by pointer to every module -- this is the pattern examiners
 * look for when they ask for a "record management system" in C.
 */
typedef struct {
    Candidate list[MAX_CANDIDATES];
    int       count;
    int       nextId;   /* auto-increment id generator */
} CandidateDB;

void initDB(CandidateDB *db);

/* Returns the new candidate's id on success, -1 if the DB is full. */
int addCandidate(CandidateDB *db, const char *name, int age,
                  int experience, const char *skills);

/* Returns 1 if deleted, 0 if no candidate with that id was found. */
int deleteCandidate(CandidateDB *db, int id);

/* Returns 1 if updated, 0 if no candidate with that id was found. */
int editCandidate(CandidateDB *db, int id, const char *name, int age,
                   int experience, const char *skills);

Candidate *findCandidateById(CandidateDB *db, int id);
int        findIndexById(CandidateDB *db, int id);

#endif /* CANDIDATE_H */
