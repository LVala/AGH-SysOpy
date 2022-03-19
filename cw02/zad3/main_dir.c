#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>

struct FilesCount {
    int file;
    int dir;
    int char_dev;
    int block_dev;
    int fifo;
    int slink;
    int sock;
};

static int get_filetype(unsigned char d_type, char *buffer, struct FilesCount *files_count) {
    switch(d_type) {
        case DT_REG:
            strcpy(buffer, "file");
            ++(files_count->file);
            return 0;
        case DT_DIR:
            strcpy(buffer, "dir");
            ++(files_count->dir);
            return 0;
        case DT_CHR:
            strcpy(buffer, "char dev");
            ++(files_count->char_dev);
            return 0;
        case DT_BLK:
            strcpy(buffer, "block dev");
            ++(files_count->block_dev);
            return 0;
        case DT_FIFO:
            strcpy(buffer, "fifo");
            ++(files_count->fifo);
            return 0;
        case DT_LNK:
            strcpy(buffer, "slink");
            ++(files_count->slink);
            return 0;
        case DT_SOCK:
            strcpy(buffer, "sock");
            ++(files_count->sock);
            return 0;
        default:
            return 1;
    }
}

void traverse_dir(char *dir_path, struct FilesCount *files_count) {
    char abs_dirpath[256];
    if (!realpath(dir_path, abs_dirpath)) {
        fprintf(stderr, "Error while obtaining absolute directory path: %s\n", strerror(errno));
        exit(1);
    }

    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Directory could not be opened: %s\n", strerror(errno));
        return;
    }

    char file_type[128];
    char file_name[512];
    struct stat stat_obj;
    struct dirent *dir_obj;
    while ((dir_obj = readdir(dir))) {
        if (!strcmp(dir_obj->d_name, ".") || !strcmp(dir_obj->d_name, ".."))
            continue;

        if (abs_dirpath[0] == '/' && abs_dirpath[1] == '\0') {
            snprintf(file_name, sizeof(file_name), "%s%s", abs_dirpath, dir_obj->d_name);
        }
        else {
            snprintf(file_name, sizeof(file_name), "%s/%s", abs_dirpath, dir_obj->d_name);
        }

        if (stat(file_name, &stat_obj) == -1) {
            fprintf(stderr, "Additional file information could not be obtained: %s\n", strerror(errno));
            return;
        }

        get_filetype(dir_obj->d_type, file_type, files_count);

        printf("%s    ", file_name);
        printf("%ld    ", stat_obj.st_nlink);
        printf("%s    ", file_type);
        printf("%ld    ", stat_obj.st_size);

        struct tm *dt;
        dt = gmtime(&stat_obj.st_atime);
        printf("%d-%d-%d %d:%d:%d    ", dt->tm_mday, dt->tm_mon, dt->tm_year + 1900, dt->tm_hour, dt->tm_min, dt->tm_sec);
        dt = gmtime(&stat_obj.st_ctime);
        printf("%d-%d-%d %d:%d:%d    ", dt->tm_mday, dt->tm_mon, dt->tm_year + 1900, dt->tm_hour, dt->tm_min, dt->tm_sec);

        printf("\n");

        if (dir_obj->d_type == DT_DIR)
            traverse_dir(file_name, files_count);
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Invalid number of arguments, expected 1\n");
        exit(1);
    }
    struct FilesCount files_count = {.file = 0, };  // other members are automatically initialized to 0 

    printf("FILEPATH    NLINKS    FILETYPE    SIZE    ACCTIME    MODTIME\n");
    traverse_dir(argv[1], &files_count);
    
    printf("\n");
    printf("REGULAR FIELS: %u\n", files_count.file);
    printf("DIRECTORIES: %u\n", files_count.dir);
    printf("CHAR DEVICES: %u\n", files_count.char_dev);
    printf("BLOCK DEVICES: %u\n", files_count.block_dev);
    printf("FIFO: %u\n", files_count.fifo);
    printf("SYMLINKS: %u\n", files_count.slink);
    printf("SOCKETS: %u\n", files_count.sock);

    return 0;
}