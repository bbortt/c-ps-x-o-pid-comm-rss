#define _GNU_SOURCE

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define DEBUG 1 // 0 = production, 1 = debug

/**
 * Global definition of functions
 */
void scanProcDirectory();
const int isSubdirectory(const struct dirent *dirent);
const int handleProcDirectory(const struct dirent *dirent);
const char* getStatusFileName(const struct dirent *dirent);
const int handleStatusFile(const char* pid, FILE *file);
char* trimStatusLine(char* line);

/**
 * Global constants
 */
static uid_t UID;
static const char* PROC_DIR = "/proc";

/**
 * Little helper that prints an error message to the standard
 * error output stream and exits using an error code (not 0).
 */
void throw(char* message) {
	perror(strcat(message, "\n"));
	exit(EXIT_FAILURE);
}

/**
 * Main entry point.
 *
 * @return 0 on success
 */
int main() {
	// Assign current uid once
	UID = getuid();

	scanProcDirectory();

	exit(EXIT_SUCCESS);
}

/**
 * Scan /proc directory for processes (= folders) and
 * read pid, comm and rss from status file in that sub-
 * folder. Prints the result to standard output stream.
 */
void scanProcDirectory() {
	static DIR *dir;
	static struct dirent *dirent;

	if (NULL == (dir = opendir(PROC_DIR))) {
		throw("Unable to open directory '/proc'!");
	}

	while (NULL != (dirent = readdir(dir))) {
		if (0 != isSubdirectory(dirent)) {
			if (0 != handleProcDirectory(dirent)) {
				throw("Error while working on /proc directory!");
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
 * @return 0 upon successful status print
 */
const int handleProcDirectory(const struct dirent *dirent) {
#if DEBUG
	printf("Handling status information from %s/%s\n", PROC_DIR,
			dirent->d_name);
#endif

	static int returnCode = EXIT_SUCCESS;

	static FILE *file;
	const char* statusFileName = getStatusFileName(dirent);

	if (access(statusFileName, R_OK) != 0) {
		// This is not a breaking error..
		// One might have accessed an invalid directory
		return 0;
	}

	if (NULL == (file = fopen(statusFileName, "r"))) {
		throw("Could not open current working file!");
	}

	if (0 != handleStatusFile(dirent->d_name, file)) {
		perror("Error while reading status file from /proc/${pid}!\n");
		returnCode = EXIT_FAILURE;
	}

	if (0 != fclose(file)) {
		throw(
				"Could not close current file: You might need to do manual cleanup!");
	}

	return returnCode;
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

/**
 * Read pid, comm and rss from status file inside
 * /prod/${pid} folder.
 *
 * @return 0 upon successful status print
 */
const int handleStatusFile(const char* pid, FILE *file) {
#if DEBUG
	printf("Handling file /proc/%s/status\n", pid);
#endif

	char* name = malloc(128);
	char* uid = malloc(128);
	char* vmrss = malloc(128);

	static char buffer[128];
	while (NULL != fgets(buffer, 128, file)) {
		if (NULL != strstr(buffer, "Name")) {
			strcat(name, buffer);
			name = trimStatusLine(name);
		} else if (NULL != strstr(buffer, "Uid")) {
			strcat(uid, buffer);
			uid = trimStatusLine(uid);
		} else if (NULL != strstr(buffer, "VmRSS")) {
			strcat(vmrss, buffer);
			vmrss = trimStatusLine(vmrss);
		}
	}

	if ((unsigned int) atoi(uid) == UID) {
		printf("%s\t%s\t%s\n", pid, name, vmrss);
	}
#if DEBUG
	else {
		printf("Process number %d is none of my business!\n", pid);
	}
#endif

	free(name);
	free(uid);
	free(vmrss);

#if DEBUG
	printf("Successfully freed allocated memory\n");
#endif

	return 0;
}

char* trimStatusLine(char* line) {
#if DEBUG
	printf("Trimming line: '%s'", line);
#endif

	int i = strcspn(line, ":") + 1; // + 1 to remove ':'
	memmove(line, line + i, strlen(line));

	int j = strspn(line, "\t ");
	memmove(line, line + j, strlen(line));

	int k = strcspn(line, "\n\t ");
	for (; k < strlen(line); k++) {
		line[k] = 0;
	}

#if DEBUG
	printf("Line trimmed to: '%s'\n", line);
#endif

	return line;
}
