# Recruitment Shortlisting System (C backend + Web GUI)

This is your original C project, restructured so the recruitment
logic runs where it always did -- in C -- and the interface is a real
graphical program instead of a console menu.

## How it's split

```
   Browser (GUI)                 shortlist_gui_server (C program)
 ┌───────────────────┐  HTTP    ┌───────────────────────────────┐
 │  web/index.html    │ ───────▶ │ src/main_server.c              │
 │  (HTML/CSS/JS)     │ ◀─────── │ src/httpserver.c  (tiny server)│
 │                     │  JSON    │ src/api.c   (routes requests)  │
 └───────────────────┘          │                                 │
                                 │ src/candidate.c  ┐               │
                                 │ src/filter.c     │  <- your      │
                                 │ src/sort.c       │     original  │
                                 │ src/report.c     │     logic,    │
                                 │ src/fileio.c     ┘     UNCHANGED │
                                 └───────────────────────────────┘
```

**The web page has zero recruitment logic in it.** It only sends
requests like "add this candidate" or "run this filter" to the C
program and displays whatever JSON comes back. Every actual decision
-- who gets shortlisted, how sorting works, what a report contains --
still happens inside `candidate.c`, `filter.c`, `sort.c`, `report.c`,
and `fileio.c`, exactly as you wrote them. **Those five files (and
their headers) are byte-for-byte identical to your original project.**

New files, all just plumbing between HTTP and your C functions:
- `src/httpserver.c` / `include/httpserver.h` -- a small HTTP server (works on Windows via Winsock and on Linux/Mac via BSD sockets)
- `src/api.c` / `include/api.h` -- turns each web request into a call to your existing functions (`addCandidate`, `filterShortlist`, `sortCandidates`, `computeStatistics`, `exportReport`, `loadCandidatesCSV`, `saveCandidatesCSV`, etc.)
- `src/json.c` / `include/json.h` -- tiny helper to read/write JSON, since C has no built-in JSON support
- `src/main_server.c` -- the new entry point: loads `candidates.csv`, starts the server, opens your browser
- `web/index.html` -- the GUI itself (HTML/CSS/JS)

Your original console version is untouched and still buildable --
see "Building" below. `gui.h`/`gui.c` (the ANSI console renderer) and
`main.c` (renamed `main_console.c`) are only used by that build.

## Building

You need `gcc` (MinGW on Windows, same as before) and `make`.

```
make            # builds the GUI version -> shortlist_gui_server(.exe)
make run        # builds (if needed) and runs it
make console    # builds the original console version -> shortlist_console(.exe)
make clean      # removes build output
```

## Running the GUI version

```
./shortlist_gui_server        (or shortlist_gui_server.exe on Windows)
```

This starts a small local web server on `http://localhost:8080` and
opens it in your default browser automatically. A small console
window will stay open in the background while it runs -- that's the C
server itself, printing status messages. All of the actual UI is in
the browser window that opens; you don't type anything into the
console window.

To shut it down cleanly, click **Save & Exit** in the sidebar of the
GUI (not the console window). That saves `candidates.csv` and stops
the server. You can also just close the console window / press
Ctrl+C, but then remember to hit **Save Data** in the Reports tab
first if you've made changes.

## Feature map (same 9 things, now as real UI)

| Original console menu item | Where it is now | What actually runs it |
|---|---|---|
| Add Candidate | "Add Candidate" tab | `addCandidate()` in candidate.c |
| View All Candidates | "All Candidates" tab | reads the in-memory `CandidateDB` |
| Edit Candidate | Click **Edit** on a card | `editCandidate()` in candidate.c |
| Delete Candidate | Click **Delete** on a card | `deleteCandidate()` in candidate.c |
| Search Candidate by Name | Search box (live, case-insensitive substring match) | simple loop in api.c |
| Sort Candidates | Sort dropdown (Name / Age / Experience) | `sortCandidates()` + your original comparators in sort.c |
| Run Shortlisting Filter | "Shortlist Filter" tab | `filterShortlist()` in filter.c |
| Export Report to File | "Reports & Export" tab -> Download Report | `exportReport()` in report.c (writes the exact same `shortlist_report.txt` format) |
| Save & Exit | "Save & Exit" button in the sidebar | `saveCandidatesCSV()` in fileio.c, then the server shuts down |

A couple of things the GUI adds on top, purely as conveniences:
- **CSV import** on the Reports tab (appends rows via `addCandidate()`, reusing `loadCandidatesCSV()` to parse the file)
- **CSV export** and a live **stats dashboard**, both built from data your original functions already produce

## Known behaviour carried over from the original (not bugs)

- Skill matching in the Shortlist Filter is still exact and
  case-sensitive (`"sql"` and `"SQL"` are different), exactly as
  documented in your original README, because `filter.c` is unchanged.
- Add/Edit/Delete only change the in-memory database. Nothing is
  written to `candidates.csv` until you click **Save Data** or
  **Save & Exit** -- same as the original, where nothing was written
  until menu option 9.
- Single-user, single-file storage, no concurrent access handling --
  same as before, since it's still meant to be run by one person
  locally.

## Files

```
include/          headers -- candidate/filter/sort/report/fileio/gui unchanged, json/httpserver/api new
src/               same split -- candidate/filter/sort/report/fileio/gui.c unchanged, json/httpserver/api/main_server new,
                   main_console.c is your original main.c renamed
web/index.html     the GUI (HTML/CSS/JS), talks to the server over fetch()
candidates.csv     your original sample data
Makefile           builds both the GUI server (default) and the original console app (`make console`)
```
