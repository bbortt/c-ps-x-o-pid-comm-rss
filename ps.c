#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

#define DEBUG 1 // 0 for production

/**
 * Global definition of functions
 */
int throwError(char* message);
int isSubdirectory(struct dirent *dirent);
int handleProcDirectory(struct dirent *directory);

/**
 * Main entry point.
 *
 * @return 0 on success
 */
int main() {
	DIR *dir;
	struct dirent *dirent;

	if (NULL == (dir = opendir("/proc"))) {
		return throwError("Unable to open directory '/proc'!");
	}

	while (NULL != (dirent = readdir(dir))) {
		if (1 == isSubdirectory(dirent)) {
			if (0 != handleProcDirectory(dirent)) {
				return throwError(
						"Error while reading proc information from %s!");
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
		return throwError(
				"Could not close directory '/proc': You might need to cleanup manually!");
	}

	return 0;
}

/**
 * Little helper that prints a message to the standard
 * error stream and returns the standard error code (1).
 *
 * @return 1
 */
int throwError(char* message) {
	perror(strcat(message, "\n"));
	return 1;
}

/**
 * Check if this is effectively a sub-directory. Because
 * DT_DIR matches also . and .. which is the current
 * respectively parent directory.
 *
 * @return 1 if this is a sub-directory
 */
int isSubdirectory(struct dirent *dirent) {
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
int handleProcDirectory(struct dirent *directory) {
#if DEBUG
	printf("Handling proc directory '%s'\n", directory->d_name);
#endif

	return 0;
}
