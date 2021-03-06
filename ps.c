#define _GNU_SOURCE

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Global definition of functions
 */
const int
isValidProcessSubdirectory (const struct dirent *dirent);
void
handleProcDirectory (const struct dirent *dirent);
const char*
getStatusFileName (const struct dirent *dirent);
void
handleStatusFile (const char* pid, FILE *file);
void
extractValueFromBuffer (char* destination, const char* buffer);

/*
 * Global constants
 */
static uid_t UID;
static const char* PROC_DIR = "/proc";

/*
 * Little helper that prints an error message to the standard
 * error output stream and exits using an error code (not 0).
 */
void
throw (char* message)
{
  perror (strcat (message, "\n"));
  exit (EXIT_FAILURE);
}

/*
 * Main entry point.
 * Scan /proc directory for processes (= folders) and
 * read pid, comm and rss from status file in that sub-
 * folder. Print the result to standard output stream.

 * @return 0 on success
 */
int
main ()
{
  /* Assign current uid once */
  UID = getuid ();

  static DIR *dir;
  static struct dirent *dirent;

  if (NULL == (dir = opendir (PROC_DIR)))
    {
      throw ("Unable to open directory '/proc'!");
    }

  while (NULL != (dirent = readdir (dir)))
    {
      if (0 != isValidProcessSubdirectory (dirent))
	{
	  handleProcDirectory (dirent);
	}
#ifdef DEBUG
      else
	{
	  printf("Not handling '%s', is not an effective sub-directory!\n",
	      dirent->d_name);
	}
#endif
    }

  if (0 != closedir (dir))
    {
      throw (
	  "Could not close directory '/proc': You might need to do manual cleanup!");
    }

  exit (EXIT_SUCCESS);
}

/*
 * Check if this is effectively a sub-directory containing
 * process information. Matches all directories who's names
 * are of a fully numerical value.
 *
 * @return not 0 if this is a numeric-named directory
 */
const int
isValidProcessSubdirectory (const struct dirent *dirent)
{
#ifdef DEBUG
  printf("Checking if '%s' is an effective sub-directory?\n", dirent->d_name);
#endif

  return (DT_DIR == dirent->d_type && 0 != strtol (dirent->d_name, NULL, 0));
}

/*
 * Search through a specific /proc sub-directory. Trying
 * to detect and print pid, comm and rss.
 */
void
handleProcDirectory (const struct dirent *dirent)
{
#ifdef DEBUG
  printf("Handling status information from %s/%s\n", PROC_DIR,
      dirent->d_name);
#endif

  static FILE *file;
  const char* statusFileName = getStatusFileName (dirent);

  if (0 != access (statusFileName, R_OK))
    {
      /* This is not a breaking error.. One might
       * have accessed an invalid directory */
      return;
    }

  if (NULL == (file = fopen (statusFileName, "r")))
    {
      throw ("Could not open current working file!");
    }

  handleStatusFile (dirent->d_name, file);

  if (0 != fclose (file))
    {
      throw (
	  "Could not close current file: You might need to do manual cleanup!");
    }
}

/*
 * Concatenate a fully qualifying status-file name based
 * on the /proc directory and the current directory name.
 *
 * @return /proc/${dirent->d_name}/status
 */
const char*
getStatusFileName (const struct dirent *dirent)
{
  static char* fileName;
  asprintf (&fileName, "%s/%s/%s", PROC_DIR, dirent->d_name, "status");
  return (fileName);
}

/*
 * Read pid, comm and rss from status file inside
 * /prod/${pid} folder.
 */
void
handleStatusFile (const char* pid, FILE *file)
{
#ifdef DEBUG
  printf("Handling file /proc/%s/status\n", pid);
#endif

  char name[64];
  char uid[64];
  char vmrss[64];

  /* initial value because vmrss may not be present in the current file */
  vmrss[0] = '0';

  static char buffer[64];
  while (NULL != fgets (buffer, 64, file))
    {
      if (0 == strncmp ("Name:", buffer, 5))
	{
	  extractValueFromBuffer (name, buffer);
	}
      else if (0 == strncmp ("Uid:", buffer, 4))
	{
	  extractValueFromBuffer (uid, buffer);
	}
      else if (0 == strncmp ("VmRSS:", buffer, 6))
	{
	  extractValueFromBuffer (vmrss, buffer);
	}
    }

  if ((unsigned int) atoi (uid) == UID)
    {
#ifdef DEBUG
      printf("-----------------------\n");
#endif
      printf ("%5s %-15s %6s\n", pid, name, vmrss);
    }
#ifdef DEBUG
  else
    {
      printf("Process number %d is none of my business!\n", pid);
    }
#endif

#ifdef DEBUG
  printf("Successfully freed allocated memory\n");
#endif
}

/**
 * Extract the value string from a buffer similar to
 * [key]: [value]. It is appended to the destination
 * pointer without any terminating null-byte ('\0').
 */
void
extractValueFromBuffer (char* destination, const char* buffer)
{
#ifdef DEBUG
  printf("Trimming string: '%s', size: %d\n", destination, strlen(temporaryString));
#endif

  destination = strcpy (destination, buffer);

  /* Remove column name including delimiter ':' */
  const int i = strcspn (destination, ":") + 1; /* + 1 to remove ':' */
  memmove (destination, destination + i, strlen (destination) - i);

  /* Look out for actual value, skip tabs and blank spaces */
  const int j = strspn (destination, "\t ");
  memmove (destination, destination + j, strlen (destination) - j);

  /* Trim trailing newline character */
  for (int k = strcspn (destination, "\n"); k < strlen (destination); k++)
    {
      destination[k] = 0;
    }

  /* Remove ' kB' if present */
  static char* kBSubstring;
  if (NULL != (kBSubstring = strstr (destination, "kB")))
    {
      kBSubstring[-1] = 0;
      kBSubstring[0] = 0;
      kBSubstring[1] = 0;
    }

#ifdef DEBUG
  printf("String trimmed to: '%s'\n", destination);
#endif
}
