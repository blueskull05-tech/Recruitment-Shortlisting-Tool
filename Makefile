CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude

# Logic files, unchanged from the original console project.
COMMON_SRC = src/candidate.c src/fileio.c src/filter.c src/sort.c src/report.c

# GUI build: a small HTTP server (httpserver.c) + a thin API layer
# (api.c) that calls straight into COMMON_SRC. web/index.html is the
# actual GUI, served to the browser.
SERVER_SRC = src/main_server.c src/httpserver.c src/api.c src/json.c $(COMMON_SRC)
TARGET_SERVER = shortlist_gui_server

# Original console build, kept available in case you still need it.
CONSOLE_SRC = src/main_console.c src/gui.c $(COMMON_SRC)
TARGET_CONSOLE = shortlist_console

ifeq ($(OS),Windows_NT)
    TARGET_SERVER  := shortlist_gui_server.exe
    TARGET_CONSOLE := shortlist_console.exe
    LDLIBS := -lws2_32
else
    LDLIBS :=
endif

all: $(TARGET_SERVER)

$(TARGET_SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(SERVER_SRC) $(LDLIBS)

console: $(TARGET_CONSOLE)

$(TARGET_CONSOLE): $(CONSOLE_SRC)
	$(CC) $(CFLAGS) -o $(TARGET_CONSOLE) $(CONSOLE_SRC)

run: $(TARGET_SERVER)
	./$(TARGET_SERVER)

clean:
	rm -f $(TARGET_SERVER) $(TARGET_CONSOLE) shortlist_report.txt candidates_export.csv import_tmp.csv

.PHONY: all run clean console
