#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <errno.h>
#include <time.h>
#include <string.h>

struct FilesCount {
    int file;
    int dir;
    int char_dev;
    int block_dev;
    int fifo;
    int slink;
    int sock;
};

struct FilesCount files_count = {.dir = -1, };  // dir to -1 (to not count input directory), rest to 0

static int get_filetype(mode_t st_mode, char *buffer) {
    switch(st_mode & S_IFMT) {
        case S_IFREG:
            strcpy(buffer, "file");
            ++(files_count.file);
            return 0;
        case S_IFDIR:
            strcpy(buffer, "dir");
            ++(files_count.dir);
            return 0;
        case S_IFCHR:
            strcpy(buffer, "char dev");
            ++(files_count.char_dev);
            return 0;
        case S_IFBLK:
            strcpy(buffer, "block dev");
            ++(files_count.block_dev);
            return 0;
        case S_IFIFO:
            strcpy(buffer, "fifo");
            ++(files_count.fifo);
            return 0;
        case S_IFLNK:
            strcpy(buffer, "slink");
            ++(files_count.slink);
            return 0;
        case S_IFSOCK:
            strcpy(buffer, "sock");
            ++(files_count.sock);
            return 0;
        default:
            return 1;
    }
}

int print_file_info(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    char file_type[128];
    get_filetype(sb->st_mode, file_type);

    printf("%s    ", fpath);
    printf("%ld    ", sb->st_nlink);
    printf("%s    ", file_type);
    printf("%ld    ", sb->st_size);
    
    struct tm *dt;
    dt = gmtime(&(sb->st_atime));
    printf("%d-%d-%d %d:%d:%d    ", dt->tm_mday, dt->tm_mon, dt->tm_year + 1900, dt->tm_hour, dt->tm_min, dt->tm_sec);
    dt = gmtime(&(sb->st_ctime));
    printf("%d-%d-%d %d:%d:%d    ", dt->tm_mday, dt->tm_mon, dt->tm_year + 1900, dt->tm_hour, dt->tm_min, dt->tm_sec);

    printf("\n");
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: Invalid number of arguments, expected 1\n");
        exit(1);
    }

    char abs_dirpath[256];
    if (!realpath(argv[1], abs_dirpath)) {
        fprintf(stderr, "Error while obtaining absolute directory path: %s\n", strerror(errno));
        exit(1);
    }
    printf("ASDASD: %s\n", abs_dirpath);

    printf("FILEPATH    NLINKS    FILETYPE    SIZE    ACCTIME    MODTIME\n");
    if (nftw(abs_dirpath, &print_file_info, 100, FTW_PHYS) == -1) {
        fprintf(stderr, "Error while traversing directory tree: %s\n", strerror(errno));
        exit(1);
    }

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