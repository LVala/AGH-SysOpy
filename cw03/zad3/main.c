#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <magic.h>

void traverse_dir(char *path, char *to_find, char *rel_path, int depth) {
    if (depth == 0)
        return;
    
    pid_t pid = getpid();

    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Directory could not be opened: %s\n", strerror(errno));
        return;
    }

    char file_name[512];
    char new_rel_path[512];
    struct dirent *dir_obj;

    while ((dir_obj = readdir(dir))) {
        if (!strcmp(dir_obj->d_name, ".") || !strcmp(dir_obj->d_name, ".."))
            continue;

        if (path[0] == '/' && path[1] == '\0') {
            snprintf(file_name, sizeof(file_name), "%s%s", path, dir_obj->d_name);
        }
        else {
            snprintf(file_name, sizeof(file_name), "%s/%s", path, dir_obj->d_name);
        }

        if (dir_obj->d_type == DT_REG) {
            // check if file is a text file using libmagic
            magic_t cookie = magic_open(MAGIC_MIME);
            if (cookie == NULL) {
                fprintf(stderr, "Error opening libmagic library\n");
                exit(1);
            }
            if (magic_load(cookie, NULL) != 0) {
                printf("cannot load magic database - %s\n", magic_error(cookie));
                magic_close(cookie);
                exit(1);
            }
            const char *magic_full = magic_file(cookie, file_name);
            
            if (strstr(magic_full, "text") != NULL) {
                // check if file contains to_find string
                FILE *file;
                char * buffer = NULL;
                size_t len = 0;

                file = fopen(file_name, "r");
                if (file == 0) {
                    fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
                    exit(1);
                }
                while (getline(&buffer, &len, file) != EOF) {
                    if (strstr(buffer, to_find) != NULL) {
                        printf("NAME: %s/%s, PID: %d\n", rel_path, dir_obj->d_name, (int)pid);
                        break;
                    }
                }
                fclose(file);
                free(buffer);
            }
            magic_close(cookie);
        }
        else if (dir_obj->d_type == DT_DIR) {
            if (fork() == 0) {
                snprintf(new_rel_path, sizeof(new_rel_path), "%s/%s", rel_path, dir_obj->d_name);
                traverse_dir(file_name, to_find, new_rel_path, depth-1);
                exit(0);
            }
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Error: invalid number of arguments, expected 2\n");
        exit(1);
    }

    int depth = strtol(argv[3], NULL, 10);
    if (depth == 0) {
        fprintf(stderr, "Error: second argument must be a positive integer\n");
        exit(1);
    }

    char abs_path[100];
    if (!realpath(argv[1], abs_path)) {
        fprintf(stderr, "Error while obtaining absolute directory path: %s\n", strerror(errno));
        exit(1);
    }

    char *i, *rel_path;
    for (i=abs_path; *i; ++i)
        if (*i == '/')
            rel_path = i+1;

    traverse_dir(argv[1], argv[2], rel_path, depth);

    return 0;
}