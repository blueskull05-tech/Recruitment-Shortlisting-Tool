#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/fileio.h"

/* Counts how many ',' separated fields are on a line -- lets us accept
 * both the old 4-column format (name,age,experience,skills) and the
 * new 5-column format (id,name,age,experience,skills).
 */
static int countFields(const char *line)
{
    int fields = 1;
    for (const char *p = line; *p; p++)
        if (*p == ',')
            fields++;
    return fields;
}

int loadCandidatesCSV(CandidateDB *db, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
        return -1;

    initDB(db);

    char line[600];

    /* skip header row */
    if (fgets(line, sizeof(line), file) == NULL)
    {
        fclose(file);
        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL &&
           db->count < MAX_CANDIDATES)
    {
        line[strcspn(line, "\r\n")] = '\0';

        if (strlen(line) == 0)
            continue;

        int fiveColumns = (countFields(line) >= 5);

        char lineCopy[600];
        strncpy(lineCopy, line, sizeof(lineCopy) - 1);
        lineCopy[sizeof(lineCopy) - 1] = '\0';

        char *tok;
        int  fileId = -1;
        char name[NAME_LEN]     = "";
        int  age = 0, experience = 0;
        char skills[SKILLS_LEN] = "";

        tok = strtok(lineCopy, ",");

        if (fiveColumns)
        {
            if (tok) { fileId = atoi(tok); tok = strtok(NULL, ","); }
        }

        if (tok) { strncpy(name, tok, NAME_LEN - 1); tok = strtok(NULL, ","); }
        if (tok) { age = atoi(tok);                  tok = strtok(NULL, ","); }
        if (tok) { experience = atoi(tok);            tok = strtok(NULL, ","); }
        if (tok) { strncpy(skills, tok, SKILLS_LEN - 1); }

        int newId = addCandidate(db, name, age, experience, skills);

        /* Preserve the id that was stored on disk (if any) so that
         * re-saving the file doesn't silently renumber candidates. */
        if (fiveColumns && fileId > 0 && newId != -1)
        {
            db->list[db->count - 1].id = fileId;
            if (fileId >= db->nextId)
                db->nextId = fileId + 1;
        }
    }

    fclose(file);
    return db->count;
}

int saveCandidatesCSV(CandidateDB *db, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
        return -1;

    fprintf(file, "id,name,age,experience,skills\n");

    for (int i = 0; i < db->count; i++)
    {
        Candidate *c = &db->list[i];
        fprintf(file, "%d,%s,%d,%d,%s\n",
                c->id, c->name, c->age, c->experience, c->skills);
    }

    fclose(file);
    return 0;
}
