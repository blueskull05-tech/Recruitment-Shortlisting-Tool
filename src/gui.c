#include <stdio.h>
#include <string.h>
#include "../include/gui.h"

#ifdef _WIN32
#include <windows.h>
#endif

void guiInit(void)
{
#ifdef _WIN32
    /* Windows 10+ consoles support ANSI escape codes, but the mode has
     * to be switched on explicitly the first time. On Linux/macOS
     * terminals this whole block is compiled out and nothing happens. */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

void guiClearScreen(void)
{
    printf("\033[2J\033[H");
}

void guiGotoXY(int x, int y)
{
    printf("\033[%d;%dH", y, x);
}

void guiSetColor(int colorCode)
{
    printf("\033[%dm", colorCode);
}

void guiResetColor(void)
{
    printf("\033[0m");
}

void guiDrawBox(int x, int y, int w, int h)
{
    guiGotoXY(x, y);
    printf("+");
    for (int i = 0; i < w - 2; i++) printf("-");
    printf("+");

    for (int row = 1; row < h - 1; row++)
    {
        guiGotoXY(x, y + row);
        printf("|");
        guiGotoXY(x + w - 1, y + row);
        printf("|");
    }

    guiGotoXY(x, y + h - 1);
    printf("+");
    for (int i = 0; i < w - 2; i++) printf("-");
    printf("+");
}

void guiPrintCentered(int y, int width, const char *text)
{
    int len = (int) strlen(text);
    int pad = (width - len) / 2;
    if (pad < 0) pad = 0;

    guiGotoXY(1, y);
    for (int i = 0; i < pad; i++) printf(" ");
    printf("%s", text);
}

void guiPause(void)
{
    printf("\nPress ENTER to continue...");
    fflush(stdout);
    /* consume characters up to and including the next newline */
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
        ;
}

void guiShowBanner(void)
{
    guiClearScreen();
    guiSetColor(CLR_CYAN);
    printf("========================================================\n");
    printf("            RVCE RECRUITMENT SHORTLISTING SYSTEM\n");
    printf("========================================================\n");
    guiResetColor();
}

void guiShowMainMenu(void)
{
    guiShowBanner();
    guiSetColor(CLR_YELLOW);
    printf("\n  1. Add Candidate\n");
    printf("  2. View All Candidates\n");
    printf("  3. Edit Candidate\n");
    printf("  4. Delete Candidate\n");
    printf("  5. Search Candidate by Name\n");
    printf("  6. Sort Candidates\n");
    printf("  7. Run Shortlisting Filter\n");
    printf("  8. Export Report to File\n");
    printf("  9. Save & Exit\n");
    guiResetColor();
    printf("\n--------------------------------------------------------\n");
    printf("Enter your choice (1-9): ");
}

void guiShowMessage(const char *msg, int isError)
{
    guiSetColor(isError ? CLR_RED : CLR_GREEN);
    printf("\n%s\n", msg);
    guiResetColor();
}

void guiPrintCandidateTable(CandidateDB *db, const int *indices, int indexCount,
                             const char *header, int headerColor)
{
    guiSetColor(headerColor);
    printf("\n%s\n", header);
    printf("--------------------------------------------------------------------\n");
    guiResetColor();

    printf("%-4s %-15s %-6s %-12s %-35s\n",
           "ID", "Name", "Age", "Experience", "Skills");

    int n = indices ? indexCount : db->count;

    if (n == 0)
    {
        printf("  (none)\n");
        return;
    }

    for (int i = 0; i < n; i++)
    {
        Candidate *c = indices ? &db->list[indices[i]] : &db->list[i];
        printf("%-4d %-15s %-6d %-12d %-35s\n",
               c->id, c->name, c->age, c->experience, c->skills);
    }
}
