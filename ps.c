#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

#define DEBUG 1 // 0 for production

int
main ()
{
  DIR *dir;
  struct dirent *de;

  #if DEBUG
  // do some debugging
  #endif
  
  dir = opendir ("/proc");
  while ( NULL != (de = readdir (dir)))
    printf ("%s\n", de -> d_name );
  closedir (dir);
}
