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
#define STR_SIZE 256

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

int file_copy(const char *src, const char *dst) {
	int src_fd, dst_fd;
	int bytes_read, bytes_written;
	char buffer[BUFFER_SIZE] = {0};

	if ((src_fd = open(src, O_RDONLY)) == -1) {
		perror("Error opening file");
		return -1;
	}

	struct stat statf;
	if (fstat(src_fd, &statf) == -1) {
		perror("Error: fstat");
		close(src_fd);
		return -1;
	}

	if ((dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, statf.st_mode)) == -1) {
		perror("Error creating file");
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
	if (directory == NULL) {
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

	struct dirent *file;

    while ((file = readdir(directory)) != NULL) {
		if (file->d_type != DT_REG && file->d_type != DT_DIR) {
            continue;
        }
        char *slash_ptr = strrchr(file->d_name, '/');   //Указатель на '/' или NULL
        int len = slash_ptr == NULL ? strlen(file->d_name) : strlen(slash_ptr) - 1;  //Длина названия без '/'
        if (file->d_name[len-1] == '~' || strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) {
            continue;
		}
        update_path(orig_dir, orig_end, new_dir, new_end);
        add_to_path(orig_dir, file->d_name);

        add_to_path(new_dir, file->d_name);

        if (file->d_type == DT_REG) {
            file_copy(orig_dir, new_dir);
        }
        else {
            dir_copy(orig_dir, new_dir);
        }
	}

	closedir(directory);
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("argc != 3\n");
        return 0;
    }
    dir_copy(argv[1], argv[2]);
}
