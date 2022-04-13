#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_SIZE 10

struct Component {
    char *name;
    char ***cmds;
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

void parse_file(char *filepath, struct Component *comps, char **to_exec) {
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        fprintf(stderr, "File could be opened: %s\n", strerror(errno));
        exit(1);
    }

    int count[2];
    count_lines(fp, count);
    fseek(fp, 0, SEEK_SET);

    comps = calloc(count[0], sizeof(struct Component));
    to_exec = calloc(count[1], sizeof(char*));

    char *line = NULL;
    size_t len = 0;
    int i = 0;
    while (getline(&line, &len, fp) != -1) {
        if (line[0] == '\n') break;

        char *saveptr = NULL;
        char *token = strtok_r(line, "=", &saveptr);

        comps[i].name = calloc(strlen(token)+1, sizeof(char));
        comps[i].cmds = calloc(MAX_SIZE, sizeof(char**));
        comps[i].length = 0;
        strcpy(comps[i].name, token);

        while((token = strtok_r(NULL, "|", &saveptr)) && comps[i].length < MAX_SIZE) {
            char *temp = calloc(strlen(token)+1, sizeof(char));
            strcpy(temp, token);
            int words = count_words(temp);
            comps[i].cmds[comps[i].length] = calloc(words+1, sizeof(char*));
            comps[i].cmds[comps[i].length][words] = NULL;

            char *saveptr2 = NULL;
            char *subtoken = strtok_r(temp, " ", &saveptr2);
            int j = 0;
            while (subtoken) {
                int word_len = strlen(subtoken);
                comps[i].cmds[comps[i].length][j] = calloc(word_len+1, sizeof(char));
                strcpy(comps[i].cmds[comps[i].length][j], subtoken);

                if (comps[i].cmds[comps[i].length][j][word_len-1] == '\n')
                    comps[i].cmds[comps[i].length][j][word_len-1] = '\0';

                printf("%s\n", comps[i].cmds[comps[i].length][j]);
                subtoken = strtok_r(NULL, " ", &saveptr2);
                ++j;
            }
            free(temp);
            ++comps[i].length;
        }
        ++i;
    }
    free(line);
    fclose(fp);
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Error: invalid number of arguments, expected 1\n");
        exit(1);
    }

    struct Component *comps = NULL;
    char **to_exec = NULL;
    parse_file(argv[1], comps, to_exec);

    // TODO freeing memory
    return 0;
}