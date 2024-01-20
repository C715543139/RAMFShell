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

    node *dir = find(pathname, false);
    if (dir == NULL) {
        if (check_status() == 2) {
            printf("ls: cannot access '%s': No such file or directory\n", pathname);
        } else if (check_status() == 0) {
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
        printf("\n");
    }
    return 0;
}

int scat(const char *pathname) {
    print("cat %s\n", pathname);

    node *file = find(pathname, false);
    if (file == NULL) {
        if (check_status() == 2) {
            printf("ls: cannot access '%s': No such file or directory\n", pathname);
        } else if (check_status() == 0) {
            printf("ls: cannot access '%s': Not a directory\n", pathname);
        }
        return 1;
    } else if (file->type == DNODE) {
        printf("cat: %s: Is a directory\n", pathname);
        return 1;
    } else if (file->type == FNODE && pathname[strlen(pathname) - 1] == '/') {
        printf("cat: %s: Is a directory\n", pathname);
    }

    for (int i = 0; i < file->size; ++i) {
        printf("%c", ((char *) (file->content))[i]);
    }
    printf("\n");
    return 0;
}

int smkdir(const char *pathname) {
    print("mkdir %s\n", pathname);

    if (rmkdir(pathname) == -1) {
        if (check_status() == 0) {
            printf("mkdir: cannot create directory '%s': Not a directory\n", pathname);
        } else if (check_status() == 2) {
            printf("mkdir: cannot create directory '%s': No such file or directory\n", pathname);
        } else if (check_status() == 1) {
            printf("mkdir: cannot create directory '%s': File exists\n", pathname);
        }
        return 1;
    }
    return 0;
}

int stouch(const char *pathname) {
    print("touch %s\n", pathname);

    node *file = find(pathname, false);
    if (file == NULL) {
        int fd = ropen(pathname, O_CREAT);
        if (fd == -1) {
            if (check_status() == 0) {
                printf("touch: cannot touch '%s': Not a directory\n", pathname);
            } else if (check_status() == 2) {
                printf("touch: cannot touch '%s': No such file or directory\n", pathname);
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

    size_t len = strlen(content);
    for (int i = 0; i < len - 1; ++i) {
        if (content[i] == '\\' && content[i + 1] == '$') {
            printf("$");
            i++;
        } else if (content[i] == '\\' && content[i + 1] == '\\') {
            printf("\\");
            i++;
        } else if (content[i] == '\\' && content[i + 1] != '\\') {
            printf("%c", content[i + 1]);
            i++;
        } else if (i < len - 4 &&
                   (content[i] == '$' && content[i + 1] == 'P' && content[i + 2] == 'A' && content[i + 3] == 'T' &&
                    content[i + 4] == 'H')) {
            printf("%s", PATH);
            i += 5;
        } else printf("%c", content[i]);
    }
    if (content[len - 2] != '\\' && content[len - 5] != '$')printf("%c", content[len - 1]);
    printf("\n");
    return 0;
}

int swhich(const char *cmd) {
    print("which %s\n", cmd);

    if (PATH == NULL)return 1;

    char *directions[65540];
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
        if (find_file_below(find(directions[i], false), cmd) != NULL) {
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
    //read .bashrc
    node *bash = find("/home/ubuntu/.bashrc", true);

    if (bash == NULL)return;

    char *buf = malloc((bash->size + 1) * sizeof(char));
    reduce_slashes(bash->content, buf);

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