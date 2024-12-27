#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#define BUFFER_SIZE 4096

int file_copy(const char *src, const char *dst);
int dir_copy(const char *src, const char *dst);

typedef struct params {
	char src[PATH_MAX];
	char dst[PATH_MAX];
} params;

void add_to_path(char *path, char *filename) {
	int path_len = strlen(path);
	int file_len = strlen(filename);
	path[path_len] = '/';
	path[path_len + 1] = '\0';
	strncat(path, filename, file_len);
}

void update_path(char* orig_path, int op_length, char* new_path, int np_length) {
	orig_path[op_length] = '\0';
	new_path[np_length] = '\0';
}

int is_subdirectory(const char *parent, const char *child) {
	char parent_path[PATH_MAX];
	char child_path[PATH_MAX];

	if (realpath(parent, parent_path) == NULL) {
		perror("Error resolving path");
		return 1;
	}

	if (realpath(child, child_path) == NULL) {
		perror("Error resolving path");
		return 1;
	}

	int parent_len = strlen(parent_path);
	int child_len = strlen(child_path);

	if (child_len <= parent_len) {
		return 0;
	}

	return strncmp(parent_path, child_path, parent_len) == 0 && child_path[parent_len] == '/';
}

void *file_thread(void *args) {
	params *param = (params*) args;

	file_copy(param->src, param->dst);

	free(args);

	return NULL;
}

void *dir_thread(void *args) {
	params *param = (params*) args;

	dir_copy(param->src, param->dst);

	free(args);

	return NULL;
}

int file_copy(const char *src, const char *dst) {
	int src_fd, dst_fd;
	int bytes_read, bytes_written;
	char buffer[BUFFER_SIZE] = {0};

	while ((src_fd = open(src, O_RDONLY)) == -1) {
		if (errno == EMFILE) {
			usleep(1);
			continue;
		}
		perror("Error opening source file");
		return -1;
	}

	struct stat statf;
	if (fstat(src_fd, &statf) == -1) {
		perror("Error: fstat");
		close(src_fd);
		return -1;
	}

	while ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1) {
		if (errno == EMFILE) {
			usleep(1);
			continue;
		}
		perror("Error opening destination file");
		close(src_fd);
		return -1;
	}

	while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
		int total_written = 0;
		while (total_written < bytes_read) {

			bytes_written = write(dst_fd, buffer + total_written, bytes_read - total_written);

			if (bytes_written == -1) {
				perror("Error writing file");
				close(src_fd);
				close(dst_fd);
				return -1;
			}
			total_written += bytes_written;
		}
	}

	if (bytes_read == -1) {
		perror("Error reading file");
	}

	close(src_fd);
	close(dst_fd);

	chmod(dst, statf.st_mode);

	return (bytes_read == -1) ? -1 : 0;
}

int dir_copy(const char *src, const char *dst) {
	DIR *directory;
	while ((directory = opendir(src)) == NULL) {
		if (errno == EMFILE) {
			usleep(1);
			continue;
		}
		perror("Error opening directory");
		return -1;
	}

	struct stat statbuf;

	if (stat(src, &statbuf) != 0) {
		perror("stat error");
		closedir(directory);
		return -1;
	}

	if (mkdir(dst, 0777) != 0) {
		perror("Create directory error");
		closedir(directory);
		return -1;
	}

	int src_len = strlen(src);
	int dst_len = strlen(dst);
	char orig_dir[PATH_MAX] = {0};
	char new_dir[PATH_MAX] = {0};

	strncpy(orig_dir, src, src_len);
	strncpy(new_dir, dst, dst_len);

	if (is_subdirectory(orig_dir, new_dir)) {
		closedir(directory);
		rmdir(dst);
		printf("Error: can't copy directory into itself\n");
		return -1;
	}

	int orig_end = strlen(orig_dir);
	int new_end = strlen(new_dir);

	int entry_size = sizeof(struct dirent) + pathconf(src, _PC_NAME_MAX) + 1;
	struct dirent *entry = malloc(entry_size);
	if (entry == NULL) {
		perror("Error malloc() buffer");
		closedir(directory);
		rmdir(dst);
		return -1;
	}
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	while (1) {
		struct dirent *result;
		if (readdir_r(directory, entry, &result) != 0) {
			perror("Error reading directory");
			free(entry);
			closedir(directory);
			return -1;
		}

		if (result == NULL)
			break;

		struct stat statf;
		char full_path[PATH_MAX];
		snprintf(full_path, sizeof(full_path), "%s/%s", src, entry->d_name);

		if (lstat(full_path, &statf) != 0) {
			perror("lstat error");
			continue;
		}

		if (S_ISLNK(statf.st_mode)) {
			printf("Ignoring symbolic link: %s\n", full_path);
			continue;
		}

		if (entry->d_type != DT_REG && entry->d_type != DT_DIR)
			continue;

		char *slash_ptr = strrchr(entry->d_name, '/');   //Указатель на '/' или NULL
		int len = slash_ptr == NULL ? strlen(entry->d_name) : strlen(slash_ptr) - 1;  //Длина названия без '/'
		if (entry->d_name[len-1] == '~' || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		update_path(orig_dir, orig_end, new_dir, new_end);

		add_to_path(orig_dir, entry->d_name);
		add_to_path(new_dir, entry->d_name);

		params *param = malloc(sizeof(params));
		if (param == NULL) {
			perror("Error malloc params");
			free(entry);
			closedir(directory);
			pthread_attr_destroy(&attr);
			return -1;
		}
		strcpy(param->src, orig_dir);
		strcpy(param->dst, new_dir);

		pthread_t tid;
		int err;

		if (entry->d_type == DT_REG) {
			err = pthread_create(&tid, &attr, file_thread, param);
		}
		else {
			err = pthread_create(&tid, &attr, dir_thread, param);
		}

		if (err) {
			perror("pthread_create() failed");
			free(entry);
			free(param);
			closedir(directory);
			pthread_attr_destroy(&attr);
			return -1;
		}
	}

	free(entry);
	closedir(directory);
	pthread_attr_destroy(&attr);
	chmod(dst, statbuf.st_mode);
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("argc != 3\n");
		return 0;
	}

	dir_copy(argv[1], argv[2]);
	pthread_exit(NULL);
}
