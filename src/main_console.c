#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/candidate.h"
#include "../include/fileio.h"
#include "../include/filter.h"
#include "../include/sort.h"
#include "../include/report.h"
#include "../include/gui.h"

#define DATA_FILE   "candidates.csv"
#define REPORT_FILE "shortlist_report.txt"

/* Reads a line of input safely and strips the trailing newline.
 * Returns 0 on success, -1 if stdin has hit EOF. */
static int readLine(char *buf, int size)
{
    if (fgets(buf, size, stdin) == NULL)
    {
        buf[0] = '\0';
        return -1;
    }
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

/* Reads an integer, re-prompting on invalid input.
 * If stdin hits EOF (e.g. input redirected from a script that ran out),
 * saves nothing further can be typed, so we exit cleanly instead of
 * spinning forever re-printing the prompt. */
static int readInt(const char *prompt)
{
    char line[64];
    int  value;
    while (1)
    {
        printf("%s", prompt);
        fflush(stdout);
        if (readLine(line, sizeof(line)) == -1)
        {
            printf("\nNo more input (EOF) -- exiting.\n");
            exit(0);
        }
        if (sscanf(line, "%d", &value) == 1)
            return value;
        guiShowMessage("Invalid number, please try again.", 1);
    }
}

static void doAddCandidate(CandidateDB *db)
{
    char name[NAME_LEN], skills[SKILLS_LEN];

    guiShowBanner();
    printf("\n-- Add Candidate --\n");

    printf("Name: ");
    readLine(name, sizeof(name));

    int age        = readInt("Age: ");
    int experience = readInt("Experience (years): ");

    printf("Skills (pipe separated, e.g. C|Java|SQL): ");
    readLine(skills, sizeof(skills));

    int id = addCandidate(db, name, age, experience, skills);

    if (id == -1)
        guiShowMessage("Database is full -- could not add candidate.", 1);
    else
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "Candidate added with ID %d.", id);
        guiShowMessage(msg, 0);
    }
    guiPause();
}

static void doViewAll(CandidateDB *db)
{
    guiShowBanner();
    guiPrintCandidateTable(db, NULL, 0, "ALL CANDIDATES", CLR_CYAN);
    guiPause();
}

/* Like readInt, but an empty line means "keep the existing value". */
static int readIntOrKeep(const char *prompt, int oldValue)
{
    char line[64];
    int  value;
    while (1)
    {
        printf("%s", prompt);
        fflush(stdout);
        if (readLine(line, sizeof(line)) == -1)
        {
            printf("\nNo more input (EOF) -- exiting.\n");
            exit(0);
        }
        if (strlen(line) == 0)
            return oldValue;
        if (sscanf(line, "%d", &value) == 1)
            return value;
        guiShowMessage("Invalid number, please try again.", 1);
    }
}

static void doEditCandidate(CandidateDB *db)
{
    guiShowBanner();
    guiPrintCandidateTable(db, NULL, 0, "ALL CANDIDATES", CLR_CYAN);

    int id = readInt("\nEnter ID of candidate to edit: ");
    Candidate *c = findCandidateById(db, id);

    if (!c)
    {
        guiShowMessage("No candidate with that ID.", 1);
        guiPause();
        return;
    }

    char name[NAME_LEN], skills[SKILLS_LEN];

    printf("New Name [%s] (blank = keep): ", c->name);
    readLine(name, sizeof(name));
    if (strlen(name) == 0) strncpy(name, c->name, NAME_LEN - 1);

    char ageBuf[64];
    snprintf(ageBuf, sizeof(ageBuf), "New Age [%d] (blank = keep): ", c->age);
    int age = readIntOrKeep(ageBuf, c->age);

    char expBuf[64];
    snprintf(expBuf, sizeof(expBuf), "New Experience [%d] (blank = keep): ", c->experience);
    int experience = readIntOrKeep(expBuf, c->experience);

    printf("New Skills [%s] (blank = keep): ", c->skills);
    readLine(skills, sizeof(skills));
    if (strlen(skills) == 0) strncpy(skills, c->skills, SKILLS_LEN - 1);

    editCandidate(db, id, name, age, experience, skills);
    guiShowMessage("Candidate updated.", 0);
    guiPause();
}

static void doDeleteCandidate(CandidateDB *db)
{
    guiShowBanner();
    guiPrintCandidateTable(db, NULL, 0, "ALL CANDIDATES", CLR_CYAN);

    int id = readInt("\nEnter ID of candidate to delete: ");

    if (deleteCandidate(db, id))
        guiShowMessage("Candidate deleted.", 0);
    else
        guiShowMessage("No candidate with that ID.", 1);

    guiPause();
}

static void doSearchByName(CandidateDB *db)
{
    guiShowBanner();
    char name[NAME_LEN];
    printf("\nEnter name to search for: ");
    readLine(name, sizeof(name));

    /* Binary search requires sorted-by-name order. We sort a scratch
     * copy so the user's on-screen ordering elsewhere isn't disturbed
     * by a search operation. */
    CandidateDB scratch = *db;
    sortCandidates(&scratch, compareByName);

    int idx = binarySearchByName(&scratch, name);

    if (idx == -1)
    {
        guiShowMessage("No candidate found with that name.", 1);
    }
    else
    {
        int indices[1] = { idx };
        guiPrintCandidateTable(&scratch, indices, 1, "SEARCH RESULT", CLR_GREEN);
    }
    guiPause();
}

static void doSort(CandidateDB *db)
{
    guiShowBanner();
    printf("\nSort by:\n");
    printf("  1. Name (A-Z)\n");
    printf("  2. Age (ascending)\n");
    printf("  3. Experience (descending)\n");
    int choice = readInt("Choice: ");

    switch (choice)
    {
        case 1: sortCandidates(db, compareByName);           break;
        case 2: sortCandidates(db, compareByAgeAsc);          break;
        case 3: sortCandidates(db, compareByExperienceDesc);  break;
        default:
            guiShowMessage("Invalid choice.", 1);
            guiPause();
            return;
    }

    guiShowMessage("Candidates sorted.", 0);
    guiPrintCandidateTable(db, NULL, 0, "ALL CANDIDATES", CLR_CYAN);
    guiPause();
}

static void doFilter(CandidateDB *db)
{
    guiShowBanner();
    printf("\n-- Shortlisting Filter --\n");

    int  minAge        = readInt("Minimum Age: ");
    int  minExperience  = readInt("Minimum Experience: ");
    char requiredSkills[SKILLS_LEN];

    printf("Required Skills (space separated): ");
    readLine(requiredSkills, sizeof(requiredSkills));

    IndexList shortlisted, rejected;
    filterShortlist(db, minAge, minExperience, requiredSkills,
                     &shortlisted, &rejected);

    guiPrintCandidateTable(db, shortlisted.indices, shortlisted.count,
                            "SHORTLISTED CANDIDATES", CLR_GREEN);
    guiPrintCandidateTable(db, rejected.indices, rejected.count,
                            "REJECTED CANDIDATES", CLR_RED);

    printf("\nTotal: %d   Shortlisted: %d   Rejected: %d\n",
           db->count, shortlisted.count, rejected.count);

    guiPause();
}

static void doExportReport(CandidateDB *db)
{
    guiShowBanner();
    printf("\n-- Export Report --\n");

    int  minAge        = readInt("Minimum Age: ");
    int  minExperience  = readInt("Minimum Experience: ");
    char requiredSkills[SKILLS_LEN];

    printf("Required Skills (space separated): ");
    readLine(requiredSkills, sizeof(requiredSkills));

    IndexList shortlisted, rejected;
    filterShortlist(db, minAge, minExperience, requiredSkills,
                     &shortlisted, &rejected);

    Statistics stats;
    computeStatistics(db, &shortlisted, &rejected, &stats);

    if (exportReport(db, &shortlisted, &rejected, &stats, REPORT_FILE) == 0)
    {
        char msg[128];
        snprintf(msg, sizeof(msg), "Report written to %s", REPORT_FILE);
        guiShowMessage(msg, 0);
    }
    else
    {
        guiShowMessage("Could not write report file.", 1);
    }
    guiPause();
}

int main(void)
{
    guiInit();

    CandidateDB db;
    if (loadCandidatesCSV(&db, DATA_FILE) == -1)
    {
        /* No file yet -- start with an empty, but valid, database. */
        initDB(&db);
    }

    int running = 1;
    while (running)
    {
        guiShowMainMenu();
        int choice = readInt("");

        switch (choice)
        {
            case 1: doAddCandidate(&db);    break;
            case 2: doViewAll(&db);         break;
            case 3: doEditCandidate(&db);   break;
            case 4: doDeleteCandidate(&db); break;
            case 5: doSearchByName(&db);    break;
            case 6: doSort(&db);            break;
            case 7: doFilter(&db);          break;
            case 8: doExportReport(&db);    break;
            case 9:
                saveCandidatesCSV(&db, DATA_FILE);
                guiShowMessage("Data saved. Goodbye!", 0);
                running = 0;
                break;
            default:
                guiShowMessage("Invalid choice, please pick 1-9.", 1);
                guiPause();
        }
    }

    return 0;
}
