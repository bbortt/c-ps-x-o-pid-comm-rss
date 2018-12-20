#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>

#define DEBUG 1 // 0 for production

/**
 * Global definition of functions
 */
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

#if DEBUG
	// do some debugging
#endif

	if (NULL == (dir = opendir("/proc"))) {
		perror("Unable to open directory '/proc'!\n");
		return 1;
	}

	while (NULL != (dirent = readdir(dir))) {
		if (0 == isSubdirectory(dirent)) {
			if (0 != handleProcDirectory(dirent)) {
				perror("Error while reading proc information from %s!\n");
				return 1;
			}
		}
	}

	closedir(dir);

	return 0;
}

/**
 * Check if this is effectively a sub-directory. Because
 * DT_DIR matches also . and .. which is the current
 * respectively parent directory.
 *
 * @return 0 if this is a sub-directory
 */
int isSubdirectory(struct dirent *dirent) {
	return DT_DIR == dirent->d_type && "." != dirent->d_name
			&& ".." != dirent->d_name;
}

/**
 * Search through a specific /proc sub-directory. Trying
 * to detect and print pid, comm and rss.
 *
 * @return 0 upon successful read
 */
int handleProcDirectory(struct dirent *directory) {
	printf("directory %s\n", directory->d_name);

	return 0;
}
