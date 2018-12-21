#define _GNU_SOURCE
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define DEBUG 1 // 0 for production

/**
 * Global definition of functions
 */
void throw(char* message);
const int isSubdirectory(const struct dirent *dirent);
const int handleProcDirectory(const struct dirent *dirent);
const char* getStatusFileName(const struct dirent *dirent);

char* PROC_DIR = "/proc";

/**
 * Main entry point.
 *
 * @return 0 on success
 */
int main() {
	static DIR *dir;
	static struct dirent *dirent;

	if (NULL == (dir = opendir(PROC_DIR))) {
		throw("Unable to open directory '/proc'!");
	}

	while (NULL != (dirent = readdir(dir))) {
		if (0 != isSubdirectory(dirent)) {
			if (0 != handleProcDirectory(dirent)) {
				throw("Error while reading proc information from %s!");
			}
		}
#if DEBUG
		else {
			printf("Not handling '%s', is not an effective sub-directory!\n",
					dirent->d_name);
		}
#endif
	}

	if (0 != closedir(dir)) {
		throw(
				"Could not close directory '/proc': You might need to do manual cleanup!");
	}

	exit(EXIT_SUCCESS);
}

/**
 * Little helper that prints a message to the standard
 * error output and exits using an error code (not 0).
 */
void throw(char* message) {
	perror(strcat(message, "\n"));
	exit(EXIT_FAILURE);
}

/**
 * Check if this is effectively a sub-directory. Because
 * DT_DIR matches also . and .. which is the current
 * respectively parent directory.
 *
 * @return not 0 if this is a sub-directory
 */
const int isSubdirectory(const struct dirent *dirent) {
#if DEBUG
	printf("Checking if '%s' is an effective sub-directory?\n", dirent->d_name);
#endif

	return DT_DIR == dirent->d_type && 0 != strcmp(".", dirent->d_name)
			&& 0 != strcmp("..", dirent->d_name);
}

/**
 * Search through a specific /proc sub-directory. Trying
 * to detect and print pid, comm and rss.
 *
 * @return 0 upon successful read
 */
const int handleProcDirectory(const struct dirent *dirent) {
#if DEBUG
	printf("Handling status information from %s/%s\n", PROC_DIR,
			dirent->d_name);
#endif

	static FILE *file;
	const char* statusFileName = getStatusFileName(dirent);

	if (access(statusFileName, R_OK) != 0) {
		// This is not a breaking error..
		// One might have accessed an invalid directory
		return 0;
	}

#if DEBUG
	printf("Working on file '%s'\n", statusFileName);
#endif

	if (NULL == (file = fopen(statusFileName, "r"))) {
		throw("Could not open current working file!");
	}

	if (0 != fclose(file)) {
		throw(
				"Could not close current file: You might need to do manual cleanup!");
	}

	return 0;
}

/**
 * Concatenate a fully qualifying status-file name based
 * on the /proc directory and the current directory name.
 *
 * @return /proc/${dirent->d_name}/status
 */
const char* getStatusFileName(const struct dirent *dirent) {
	static char* fileName;
	asprintf(&fileName, "%s/%s/%s", PROC_DIR, dirent->d_name, "status");
	return fileName;
}
