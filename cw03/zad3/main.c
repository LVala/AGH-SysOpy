#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

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

        // TODO weryfikacja czy string jest w pliku 
        printf("NAME: %s/%s, PID: %d\n", rel_path, dir_obj->d_name, (int)pid);

        if (dir_obj->d_type == DT_DIR)
            if (fork() == 0) {
                if (path[0] == '/' && path[1] == '\0') {
                    snprintf(file_name, sizeof(file_name), "%s%s", path, dir_obj->d_name);
                }
                else {
                    snprintf(file_name, sizeof(file_name), "%s/%s", path, dir_obj->d_name);
                }
                snprintf(new_rel_path, sizeof(new_rel_path), "%s/%s", rel_path, dir_obj->d_name);
                traverse_dir(file_name, to_find, new_rel_path, depth-1);
                exit(0);
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