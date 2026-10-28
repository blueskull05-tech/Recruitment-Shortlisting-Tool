#ifndef FILEIO_H
#define FILEIO_H

#include "candidate.h"

/* Loads candidates from a CSV file with header row:
 *   id,name,age,experience,skills
 * Existing DB contents are cleared first.
 * Returns number of records loaded, or -1 if the file could not be opened.
 */
int loadCandidatesCSV(CandidateDB *db, const char *filename);

/* Writes the whole DB back out to CSV (with header row).
 * Returns 0 on success, -1 on failure.
 */
int saveCandidatesCSV(CandidateDB *db, const char *filename);

#endif /* FILEIO_H */
