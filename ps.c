#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

#define DEBUG 1 // 0 for production

/**
 * Global definition of functions
 */
void throw(char* message);
const int isSubdirectory(const struct dirent *dirent);
const int handleProcDirectory(const struct dirent *directory);
const char* getStatusFileName(const struct dirent *directory);

char* PROC_DIR = "/proc";

/**
 * Main entry point.
 *
 * @return 0 on success
 */
int main() {
	DIR *dir;
	struct dirent *dirent;

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
				"Could not close directory '/proc': You might need to cleanup manually!");
	}

	exit(EXIT_SUCCESS);
}

/**
 * Little helper that prints a message to the standard
 * error output and returns the standard error code (1).
 * Works basically like a commonly known exception.
 *
 * @return 1
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
const int handleProcDirectory(const struct dirent *directory) {
#if DEBUG
	printf("Handling status information from %s/%s\n", PROC_DIR,
			directory->d_name);
#endif

	const char* statusFileName = getStatusFileName(directory);

	printf("%s\n", statusFileName);

	return 0;
}

const char* getStatusFileName(const struct dirent *directory) {
	char* fileName;
	asprintf(&fileName, "%s/%s/%s", PROC_DIR, directory->d_name, "status");
	return fileName;
}
