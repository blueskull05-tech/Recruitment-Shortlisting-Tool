#ifndef GUI_H
#define GUI_H

#include "candidate.h"
#include "filter.h"

/* ---- colour codes (ANSI) ---- */
#define CLR_RESET   0
#define CLR_RED     31
#define CLR_GREEN   32
#define CLR_YELLOW  33
#define CLR_BLUE    34
#define CLR_MAGENTA 35
#define CLR_CYAN    36
#define CLR_WHITE   37

/* Must be called once at program start. On Windows it turns on ANSI
 * escape-code processing for the console (no-op on Linux/macOS). */
void guiInit(void);

void guiClearScreen(void);
void guiGotoXY(int x, int y);
void guiSetColor(int colorCode);
void guiResetColor(void);
void guiDrawBox(int x, int y, int w, int h);
void guiPrintCentered(int y, int width, const char *text);
void guiPause(void);

void guiShowBanner(void);
void guiShowMainMenu(void);
void guiShowMessage(const char *msg, int isError);

/* Renders a table of candidates. If indices is NULL, every candidate
 * in db is printed, in order. Otherwise only db->list[indices[i]] for
 * i in [0, indexCount) is printed -- used for shortlist/reject views. */
void guiPrintCandidateTable(CandidateDB *db, const int *indices, int indexCount,
                             const char *header, int headerColor);

#endif /* GUI_H */
