#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#define BUFFER_SIZE 4096
#define STR_SIZE 512

typedef struct params {
	char src[STR_SIZE];
	char dst[STR_SIZE];
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

void *file_thread(void *args) {
	params *param = (params*) args;

	file_copy(param->src, param->dst);

	return NULL;
}

void *dir_thread(void *args) {
	params *param = (params*) args;

	dir_copy(param->src, param->dst);

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

	while ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, statf.st_mode)) == -1) {
	    if (errno == EMFILE) {
	        usleep(1);
	        continue;
	    }
	    perror("Error opening destination file");
	    close(src_fd);
	    return -1;
	}

	while((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {

		bytes_written = write(dst_fd, buffer, bytes_read);

		if (bytes_written != bytes_read) {
			perror("Error writing file");
			close(src_fd);
			close(dst_fd);
			return -1;
		}
	}

	if (bytes_read == -1) {
		perror("Error reading file");
	}

	close(src_fd);
	close(dst_fd);

	return (bytes_read == -1) ? -1 : 0;
}

int dir_copy(const char *src, const char *dst) {
	DIR *directory = opendir(src);
	while (directory == NULL) {
		if (errno == EMFILE) {
			usleep(1);
			continue;
		}
		perror("Error opening directory");
		return -1;
	}

	if (mkdir(dst, S_IRWXU) != 0) {
		perror("Create directory error");
		return -1;
	}

	int src_len = strlen(src);
    int dst_len = strlen(dst);
    char orig_dir[STR_SIZE] = {0};
    char new_dir[STR_SIZE] = {0};

    strncpy(orig_dir, src, src_len);
    strncpy(new_dir, dst, dst_len);

    int orig_end = strlen(orig_dir);
    int new_end = strlen(new_dir);

	int entry_size = sizeof(struct dirent) + pathconf(src, _PC_NAME_MAX) + 1;
	struct dirent *entry = malloc(entry_size);
	if (entry == NULL) {
        perror("Error malloc() buffer");
        closedir(directory);
        return -1;
    }

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

		params param;
		strcpy(param.src, orig_dir);
		strcpy(param.dst, new_dir);

		pthread_t tid;
		int err;

        if (entry->d_type == DT_REG) {
            err = pthread_create(&tid, NULL, file_thread, &param);
        }
		else {
            err = pthread_create(&tid, NULL, dir_thread, &param);
        }

		if (err) {
			perror("pthread_create() failed");
			free(entry);
			closedir(directory);
			return -1;
		}
		pthread_join(tid, NULL);
	}

	free(entry);
	closedir(directory);
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("argc != 3\n");
        return 0;
    }
    dir_copy(argv[1], argv[2]);
	return 0;
}
