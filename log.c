#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>

#define DB_PATH "/home/vincent/my_cd/db.txt"

void log_path(char *path) {
  int fd = open(DB_PATH, O_RDWR);
  assert(fd != -1);

  struct stat st;
  fstat(fd, &st);
  char *region = mmap(
		      NULL, st.st_size + 100,
		      PROT_READ|PROT_WRITE, MAP_SHARED,
		      fd, 0
		      );
  assert(region != (void *)-1);
  char *path_location = strstr(region, path);

  if (NULL == path_location) {
    char new_entry[1024];
    int times_accesed = 1;
    int last_accesed = time(NULL);
    int new_entry_length = sprintf(new_entry, "%d,%d,%s\n", times_accesed, last_accesed, path);

    assert(new_entry_length < 100);
    memcpy(region + st.st_size, new_entry, new_entry_length);
    
    munmap(region, st.st_size);
    ftruncate(fd, st.st_size + new_entry_length);
    close(fd);
    return;
  }
  char *newline = strchr(path_location, '\n')+1;
  char *line_start = memrchr(region, '\n', path_location - region );
  if (NULL == line_start) {
    line_start = region;
  } else {
    line_start++;
  }
	
  int times_accesed = atoi(line_start)+1;
  int last_accesed = time(NULL);
  char new_entry[1024];
  int new_entry_length = sprintf(new_entry, "%d,%d,%s\n", times_accesed, last_accesed, path);
  assert(new_entry_length < 100);

  int size_line_start_to_end = st.st_size - (region - newline);
  int offset = (new_entry_length-(newline-line_start));
  memmove(newline+offset, newline, size_line_start_to_end);
  memcpy(line_start, new_entry, new_entry_length);
				          

  munmap(region, st.st_size);
  ftruncate(fd, st.st_size +offset);
  close(fd);
  return;
}
int main() {
  log_path(getenv("PWD"));
}
