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
void
scanProcDirectory ();
const int
isValidProcessSubdirectory (const struct dirent *dirent);
const int
handleProcDirectory (const struct dirent *dirent);
const char*
getStatusFileName (const struct dirent *dirent);
const int
handleStatusFile (const char* pid, FILE *file);
char*
trimStatusLine (char* line);

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
 *
 * @return 0 on success
 */
int
main ()
{
  /* Assign current uid once */
  UID = getuid ();

  scanProcDirectory ();

  exit (EXIT_SUCCESS);
}

/*
 * Scan /proc directory for processes (= folders) and
 * read pid, comm and rss from status file in that sub-
 * folder. Prints the result to standard output stream.
 */
void
scanProcDirectory ()
{
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
	  if (0 != handleProcDirectory (dirent))
	    {
	      throw ("Error while working on /proc directory!");
	    }
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
 *
 * @return 0 upon successful status print
 */
const int
handleProcDirectory (const struct dirent *dirent)
{
#ifdef DEBUG
  printf("Handling status information from %s/%s\n", PROC_DIR,
      dirent->d_name);
#endif

  static int returnCode = EXIT_SUCCESS;

  static FILE *file;
  const char* statusFileName = getStatusFileName (dirent);

  if (access (statusFileName, R_OK) != 0)
    {
      /* This is not a breaking error.. One might
       * have accessed an invalid directory */
      return (0);
    }

  if (NULL == (file = fopen (statusFileName, "r")))
    {
      throw ("Could not open current working file!");
    }

  if (0 != handleStatusFile (dirent->d_name, file))
    {
      perror ("Error while reading status file from /proc/${pid}!\n");
      returnCode = EXIT_FAILURE;
    }

  if (0 != fclose (file))
    {
      throw (
	  "Could not close current file: You might need to do manual cleanup!");
    }

  return (returnCode);
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
 *
 * @return 0 upon successful status print
 */
const int
handleStatusFile (const char* pid, FILE *file)
{
#ifdef DEBUG
  printf("Handling file /proc/%s/status\n", pid);
#endif

  char* name;
  if (NULL == (name = malloc (64)))
    {
      throw ("Could not allocate memory: You might need to cleanup manually!");
    }

  char* uid;
  if (NULL == (uid = malloc (64)))
    {
      throw ("Could not allocate memory: You might need to cleanup manually!");
    }

  char* vmrss;
  if (NULL == (vmrss = malloc (64)))
    {
      throw ("Could not allocate memory: You might need to cleanup manually!");
    }

  /* initial value because vmrss might not be present
   * in the current file */
  vmrss[0] = '0';

  static char buffer[64];
  while (NULL != fgets (buffer, 64, file))
    {
      if (NULL != strstr (buffer, "Name:"))
	{
	  name = strcpy (name, buffer);
	  name = trimStatusLine (name);
	}
      else if (NULL != strstr (buffer, "Uid:"))
	{
	  uid = strcpy (uid, buffer);
	  uid = trimStatusLine (uid);
	}
      else if (NULL != strstr (buffer, "VmRSS:"))
	{
	  vmrss = strcpy (vmrss, buffer);
	  vmrss = trimStatusLine (vmrss);
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

  free (vmrss);
  free (uid);
  free (name);

#ifdef DEBUG
  printf("Successfully freed allocated memory\n");
#endif

  return (0);
}

/**
 * Trims a line in a status file of form [name]: [value]
 * to a string containing just the value.
 *
 * @return the value srting (by pointer)
 */
char*
trimStatusLine (char* line)
{
#ifdef DEBUG
  printf("Trimming line: '%s', size: %d\n", line, strlen(line));
#endif

  /* Remove column name including delimiter ':' */
  const int i = strcspn (line, ":") + 1; /* + 1 to remove ':' */
  memmove (line, line + i, strlen (line) - i);

  /* Look out for actual value, skip tabs and blank spaces */
  const int j = strspn (line, "\t ");
  memmove (line, line + j, strlen (line) - j);

  /* Trim trailing newline */
  for (int k = strcspn (line, "\n"); k < strlen (line); k++)
    {
      line[k] = 0;
    }

  /* Remove 'kB' if present */
  static char* kBSubstring;
  if (NULL != (kBSubstring = strstr (line, "kB")))
    {
      kBSubstring[0] = 0;
      kBSubstring[1] = 0;
    }

#ifdef DEBUG
  printf("Line trimmed to: '%s'\n", line);
#endif

  return (line);
}
