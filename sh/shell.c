#include "ramfs.h"
#include "shell.h"

#ifndef ONLINE_JUDGE
#define print(...) printf("\033[31m");printf(__VA_ARGS__);printf("\033[0m");
#else
#define print(...)
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

char *PATH = NULL;

int sls(const char *pathname) {
    print("ls %s\n", pathname);

    node *dir = FindNode(pathname, false);
    if (dir == NULL) {
        if (CheckErrorType() == 2) {
            printf("ls: cannot access '%s': No such file or directory\n", pathname);
        } else if (CheckErrorType() == 0) {
            printf("ls: cannot access '%s': Not a directory\n", pathname);
        }
        return 1;
    } else if (dir->type == FNODE && pathname[strlen(pathname) - 1] == '/') {
        printf("ls: cannot access '%s': Not a directory\n", pathname);
        return 1;
    }

    if (dir->type == FNODE) {
        printf("%s\n", pathname);
    } else {
        for (int i = 0; i < dir->nrde; ++i) {
            printf("%s ", dir->dirents[i]->name);
        }
        if(dir->nrde != 0)printf("\n");
    }
    return 0;
}

int scat(const char *pathname) {
    print("cat %s\n", pathname);

    node *file = FindNode(pathname, false);
    if (file == NULL) {
        if (CheckErrorType() == 2) {
            printf("cat: %s: No such file or directory\n", pathname);
        } else if (CheckErrorType() == 0) {
            printf("cat: %s: Not a directory\n", pathname);
        }
        return 1;
    } else if (file->type == DNODE) {
        printf("cat: %s: Is a directory\n", pathname);
        return 1;
    } else if (file->type == FNODE && pathname[strlen(pathname) - 1] == '/') {
        printf("cat: %s: Is a directory\n", pathname);
        return 1;
    }

    for (int i = 0; i < file->size; ++i) {
        printf("%c", ((char *) (file->content))[i]);
    }
    if(file->size != 0)printf("\n");
    return 0;
}

int smkdir(const char *pathname) {
    print("mkdir %s\n", pathname);

    if (rmkdir(pathname) == -1) {
        if (CheckErrorType() == 0) {
            printf("mkdir: cannot create directory '%s': Not a directory\n", pathname);
        } else if (CheckErrorType() == 2) {
            printf("mkdir: cannot create directory '%s': No such file or directory\n", pathname);
        } else if (CheckErrorType() == 1) {
            printf("mkdir: cannot create directory '%s': File exists\n", pathname);
        }
        return 1;
    }
    return 0;
}

int stouch(const char *pathname) {
    print("touch %s\n", pathname);

    node *file = FindNode(pathname, false);
    if (file == NULL) {
        int fd = ropen(pathname, O_CREAT);
        if (fd == -1) {
            if (CheckErrorType() == 0) {
                printf("touch: cannot touch '%s': Not a directory\n", pathname);
            } else if (CheckErrorType() == 2) {
                printf("touch: cannot touch '%s': No such file or directory\n", pathname);
            } else if(CheckErrorType() == 3){
                printf("No such file or directory\n");
            }
            return 1;
        } else {
            rclose(fd);
            return 0;
        }
    } else if (file->type == FNODE && pathname[strlen(pathname) - 1] == '/') {
        printf("touch: cannot touch '%s': Not a directory\n", pathname);
        return 1;
    }
    return 0;
}

int secho(const char *content) {
    print("echo %s\n", content);

    size_t lenCon = strlen(content);
    char *input = calloc(lenCon + 10, (lenCon + 10) * sizeof(char));
    char *output = NULL;
    size_t lenOut = 0;
    strcpy(input, content);
    for (int i = 0; i < lenCon + 5;) {
        if (input[i] == '\\') {
            lenOut++;
            output = realloc(output, (lenOut + 1) * sizeof(char));
            output[lenOut - 1] = input[i + 1];
            output[lenOut] = 0;
            i += 2;
        } else if (input[i] == '$' && input[i + 1] == 'P' && input[i + 2] == 'A' && input[i + 3] == 'T' &&
                    input[i + 4] == 'H') {
            lenOut += strlen(PATH);
            output = realloc(output, (lenOut + 1) * sizeof(char));
            strcat(output,PATH);
            i += 5;
        } else {
            lenOut++;
            output = realloc(output, (lenOut + 1) * sizeof(char));
            output[lenOut - 1] = input[i];
            output[lenOut] = 0;
            i++;
        }
    }
    printf("%s\n",output);
    return 0;
}

int swhich(const char *cmd) {
    print("which %s\n", cmd);

    if (PATH == NULL)return 1;

    char *directions[MAX_LEN];
    int count = 0, p = 0, q = 0;
    while (true) {
        while (PATH[p] != ':' && PATH[p] != 0)p++;
        char *temp = calloc(p - q + 1, (p - q + 1) * sizeof(char));
        strncpy(temp, &PATH[q], p - q);
        directions[count++] = temp;
        if (PATH[p] == 0)break;
        p++;
        q = p;
    }

    int found = -1;
    for (int i = 0; i < count; ++i) {
        node *file = FindNodeBelow(FindNode(directions[i], false), cmd);
        if (file != NULL && file->type == FNODE) {
            found = i;
            break;
        }
    }
    if (found != -1) {
        printf("%s/%s\n", directions[found], cmd);
        for (int i = 0; i < count; ++i) free(directions[i]);
        return 0;
    } else {
        for (int i = 0; i < count; ++i) free(directions[i]);
        return 1;
    }
}


void init_shell() {
    node *bash = FindNode("/home/ubuntu/.bashrc", true);

    if (bash == NULL)return;

    char *buf = malloc((bash->size + 1) * sizeof(char));
    ReduceSlashes(bash->content, buf);

    char *temp = calloc(bash->size + 1, (bash->size + 1) * sizeof(char));
    char *p = strstr(buf, "export PATH=");
    while (p != NULL) {
        p += 12;
        char *q = strstr(p, "export PATH=");

        if (q == NULL) {
            q = p;
            while (*q != 0 && *q != '\n')q++;
        } else {
            while (*(q - 1) == '\n')q--;
        }

        if (*p == '$') {
            p += 5;
            strncat(temp, p, q - p);
        } else if (*(q - 5) == '$') {
            q -= 5;
            memmove(&temp[q - p], temp, strlen(temp) + 1);
            memcpy(temp, p, q - p);
        } else {
            memset(temp, 0, (bash->size + 1) * sizeof(char));
            memcpy(temp, p, q - p);
        }

        p = strstr(p, "export PATH=");
    }
    PATH = temp;

    free(buf);
}

void close_shell() {
    free(PATH);
    PATH = NULL;
}