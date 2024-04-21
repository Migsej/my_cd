#include <stddef.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>

#define DB_PATH "/home/vincent/my_cd/db.txt"

struct entry {
  int score;
  char *path;
	size_t path_len;
};

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

int match(int n, char **words, char *line) {
	assert(line != NULL);
	char *pointer = line;
	for (int i = 0; i < n; i++) {
		pointer = strstr(pointer, words[i]);
		if (NULL == pointer) return 0;
	}
	return 1;
}

struct entry get_entry(char *line) {
	int times_accesed = atoi(line);
	char *time_start = strchr(line, ',')+1;
	int last_accesed = atoi(time_start);
	int difference = time(NULL) - last_accesed;
	struct entry result;
	if (difference < 3600) { // within hour
		result.score =  times_accesed * 4; 
	} else if (difference < 86400) { // within day
		result.score =  times_accesed * 2; 
	} else if (difference < 604800) { // within week
		result.score =  times_accesed / 2; 
	} else {
		result.score =  times_accesed / 4; 
	}
	char *path_start = strchr(time_start+1, ',')+1;
	int length = strlen(path_start);
	result.path = strndup(path_start, length);
	result.path_len = length;
	return result;
}

// db format
// timesaccesed>,<lastaccesed>,<path>
char *get_best_match(int n, char **words) {
  FILE *file = fopen(DB_PATH, "r");
  assert(file != NULL);
  char *line = NULL;
  size_t len = 0;
  int read;
  struct entry best = {0};
  while ((read = getline(&line, &len, file)) != -1) {
    if (match(n, words, line)) {
			struct entry entry = get_entry(line);
      if (entry.score < best.score) {
				free(entry.path);
				continue;
			}
      if (best.path != NULL) free(best.path);
			memcpy(&best, &entry,  sizeof(best));
    }
  }
  fclose(file);
  free(line);
  return best.path;
}

void exec_fzf(char *data, size_t len) {
	FILE *program = popen("fzf", "w");
	fwrite(data, sizeof(*data), len, program);
	fclose(program);

}

int get_all_matches(int n, char **words) {
  FILE *file = fopen(DB_PATH, "r");
  assert(file != NULL);
  char *line = NULL;
  size_t len = 0;
  int read;
  struct entry matches[1024];
	int num_matches = 0;
  while ((read = getline(&line, &len, file)) != -1) {
		if (match(n, words, line)) {
			assert(n < 1024);
			matches[num_matches++] = get_entry(line);
		}
	}
	size_t all_paths_len = 0;
	for (int i = 0; i < num_matches; i++) {
		all_paths_len += matches[i].path_len;
	}
	char all_paths[all_paths_len+1+num_matches];
	all_paths[all_paths_len] = 0;
	char *end = all_paths;
	for (int i = 0; i < num_matches; i++) {
		memcpy(end, matches[i].path, matches[i].path_len);
		end += matches[i].path_len;
		*(end-1) = '\n';
		free(matches[i].path);
	}
	exec_fzf(all_paths, all_paths_len);
  fclose(file);
	free(line);
	return 1;
}

int main(int argc, char *argv[]) {
  if (argc == 2) {
    DIR* dir = opendir(argv[1]);
    if (dir) {
      closedir(dir);
      puts(argv[1]);
      return 0;
    } else if (0 == strcmp("-", argv[1])) {
      puts("-");
      return 0;
    } 
  } else if (argc == 1) {
    char *HOME = getenv("HOME");
    puts(HOME);
    return 0;
  }
	if (0 == strcmp("-f", argv[1])) {
		return get_all_matches(argc-2,argv+2);
	}
  char *result_path = get_best_match(argc-1,argv+1);
  assert(result_path != NULL);
  printf("%s\n", result_path);
	*strchr(result_path, '\n') = 0;
	log_path(result_path);
  free(result_path);
  return 0;
}
