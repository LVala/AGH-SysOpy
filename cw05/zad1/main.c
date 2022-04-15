#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_SIZE 10  // limit number of piped commands

struct Component {
    char *name;
    char ***cmds;
    size_t length;
};

struct Executable {
    char **cmds;
    size_t length;
};

void count_lines(FILE *fp, int count[2]) {
    int c, i, was_n;
    count[0] = count[1] = i = was_n = 0;
    while ((c = getc(fp)) != EOF) {
        if (c == '\n') {
            if (was_n) {
                i = 1;
            }
            else if (!was_n)
                ++count[i];
            was_n = 1;
        }
        else {
            was_n = 0;
        }
    }
    if (c == EOF && !was_n) ++count[1];
}

int count_words(char *str) {
    int counted = 0;
    int inword = 0;

    do {
        if (isspace(*str) || *str == '\0') {
            if (inword) {
                inword = 0;
                ++counted;
            }
        }
        else {
            inword = 1;
        }
    } while(*str++);

    return counted;
}

void parse_file(char *filepath, struct Component **comps, struct Executable **to_exec, int count[2]) {
    // does not work, when command line argument includes whitespace, i.e. grep "sysopy fajne"
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        fprintf(stderr, "File could be opened: %s\n", strerror(errno));
        exit(1);
    }

    count_lines(fp, count);
    fseek(fp, 0, SEEK_SET);

    *comps = calloc(count[0], sizeof(struct Component));

    char *line = NULL;
    size_t len = 0;
    int i = 0;
    while (getline(&line, &len, fp) != -1) {
        if (line[0] == '\n') break;

        char *saveptr = NULL;
        char *token = strtok_r(line, "=", &saveptr);

        (*comps)[i].name = calloc(strlen(token)+1, sizeof(char));
        (*comps)[i].cmds = calloc(MAX_SIZE, sizeof(char**));
        (*comps)[i].length = 0;
        strcpy((*comps)[i].name, token);
        if (isspace((*comps)[i].name[strlen((*comps)[i].name)-1]))
            (*comps)[i].name[strlen((*comps)[i].name)-1] = '\0';

        while((token = strtok_r(NULL, "|", &saveptr)) && (*comps)[i].length < MAX_SIZE) {
            char *temp = calloc(strlen(token)+1, sizeof(char));
            strcpy(temp, token);
            int words = count_words(temp);
            (*comps)[i].cmds[(*comps)[i].length] = calloc(words+1, sizeof(char*));
            (*comps)[i].cmds[(*comps)[i].length][words] = NULL;

            char *saveptr2 = NULL;
            char *subtoken = strtok_r(temp, " ", &saveptr2);
            int j = 0;
            while (subtoken) {
                int word_len = strlen(subtoken);
                (*comps)[i].cmds[(*comps)[i].length][j] = calloc(word_len+1, sizeof(char));
                strcpy((*comps)[i].cmds[(*comps)[i].length][j], subtoken);

                if ((*comps)[i].cmds[(*comps)[i].length][j][word_len-1] == '\n')
                    (*comps)[i].cmds[(*comps)[i].length][j][word_len-1] = '\0';

                subtoken = strtok_r(NULL, " ", &saveptr2);
                ++j;
            }
            free(temp);
            ++(*comps)[i].length;
        }
        ++i;
    }

    *to_exec = calloc(count[1], sizeof(struct Component));
    i = 0;
    while (getline(&line, &len, fp) != -1) {
        (*to_exec)[i].cmds = calloc(MAX_SIZE, sizeof(char*));
        (*to_exec)[i].length = 0;
        char *saveptr = NULL;
        char *token = strtok_r(line, " | ", &saveptr);
        while (token && (*to_exec)[i].length < MAX_SIZE) {
            int word_len = strlen(token);
            (*to_exec)[i].cmds[(*to_exec)[i].length] = calloc(strlen(token)+1, sizeof(char));
            strcpy((*to_exec)[i].cmds[(*to_exec)[i].length], token);

            if ((*to_exec)[i].cmds[(*to_exec)[i].length][word_len-1] == '\n')
                    (*to_exec)[i].cmds[(*to_exec)[i].length][word_len-1] = '\0';

            token = strtok_r(NULL, " | ", &saveptr);
            ++(*to_exec)[i].length;
        }
        ++i;
    }
    free(line);
    fclose(fp);
}

void free_all(struct Component *comps, struct Executable *to_exec, int count[2]) {
    for (int i=0; i<count[0]; ++i) {
        for (int j=0; j < comps[i].length; ++j) {
            for (int k=0; comps[i].cmds[j][k]; ++k) {
                free(comps[i].cmds[j][k]);
            }
            free(comps[i].cmds[j]);
        }
        free(comps[i].cmds);
        free(comps[i].name);
    }
    free(comps);

    for (int i=0; i < count[1]; ++i) {
        for (int j=0; j< to_exec[i].length; ++j) {
            free( to_exec[i].cmds[j]);
        }
        free(to_exec[i].cmds);
    }
    free(to_exec);
}

void exec_commands(struct Component *comps, struct Executable to_exec, int comps_len) {
    int fd[2][2];
    pipe(fd[0]);
    pipe(fd[1]);
    int mode = 0;
    int last_pid = 0;
    for (int i=0; i<to_exec.length; ++i) {
        int k = 0;
        while (k < comps_len && strcmp(to_exec.cmds[i], comps[k].name))
            ++k;

        if (k == comps_len) {
            fprintf(stderr, "Error: invalid command in file input\n");
            exit(1);
        }

        for (int j=0; j<comps[k].length; ++j) {
            pid_t pid = fork();
            if (pid == -1) {
                fprintf(stderr, "Failed to fork: %s\n", strerror(errno));
                exit(1);
            }
            else if (pid > 0) {
                if (i == to_exec.length-1 && j == comps[k].length-1)
                    last_pid = pid;
                mode = (mode+1)%2;
            }
            else if (pid == 0) {
                if (j > 0 || i > 0)
                    dup2(fd[mode][0], STDIN_FILENO);
                if (i < to_exec.length-1 || j < comps[k].length-1) 
                    dup2(fd[(mode+1)%2][1], STDOUT_FILENO);
                close(fd[mode][1]);
                close(fd[mode][0]);
                close(fd[(mode+1)%2][0]);
                close(fd[(mode+1)%2][1]);
                execvp(comps[k].cmds[j][0], comps[k].cmds[j]);
                exit(0);
            }
        }
    }
    
    close(fd[0][0]);
    close(fd[0][1]);
    close(fd[1][0]);
    close(fd[1][1]);
    
    waitpid(last_pid, NULL, 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: invalid number of arguments, expected 1\n");
        exit(1);
    }

    struct Component *comps = NULL;
    struct Executable *to_exec = NULL;
    int count[2];

    parse_file(argv[1], &comps, &to_exec, count);

    for (int i=0; i<count[0]; ++i) {
        exec_commands(comps, to_exec[i], count[0]);
    }

    free_all(comps, to_exec, count);
    return 0;
}