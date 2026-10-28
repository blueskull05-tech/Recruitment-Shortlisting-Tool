/* Entry point for the GUI version. Instead of running a console menu
 * loop (see main_console.c for that, still available via `make console`),
 * this starts a small local web server and opens the GUI in the
 * user's browser. All the recruitment-shortlisting logic itself is
 * unchanged -- it still lives in candidate.c / filter.c / sort.c /
 * report.c / fileio.c, called from api.c.
 */
#include <stdio.h>
#include <stdlib.h>

#include "../include/httpserver.h"
#include "../include/api.h"

#define DATA_FILE "candidates.csv"
#define PORT      8080

static void openBrowser(const char *url)
{
    char cmd[300];
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", url);
#elif __APPLE__
    snprintf(cmd, sizeof(cmd), "open \"%s\"", url);
#else
    snprintf(cmd, sizeof(cmd), "xdg-open \"%s\" > /dev/null 2>&1", url);
#endif
    system(cmd);
}

int main(void)
{
    apiInit(DATA_FILE);

    printf("========================================\n");
    printf(" RVCE Recruitment Shortlisting Desk\n");
    printf("========================================\n");
    printf("The recruitment logic is running in this C program.\n");
    printf("Opening the GUI in your browser at http://localhost:%d ...\n", PORT);
    printf("(If it doesn't open automatically, visit that address yourself.)\n");
    printf("Leave this window open while you use the app.\n");
    printf("Use \"Save & Exit\" in the GUI to close everything down cleanly.\n\n");

    openBrowser("http://localhost:8080");

    runServer(PORT, handleRequest);
    return 0;
}
