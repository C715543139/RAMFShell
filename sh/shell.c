#include "ramfs.h"
#include "shell.h"

#ifndef ONLINE_JUDGE
#define print(...) printf("\033[31m");printf(__VA_ARGS__);printf("\033[0m");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

char *PATH = NULL;
long PATH_LEN = 0;

int sls(const char *pathname) {
    print("ls %s\n", pathname);

    node *dir = find(pathname);
    if (dir == NULL) {
        if (check_status() == 1) {
            printf("ls: cannot access '%s': No such file or directory\\n", pathname);
        } else if (check_status() == 0) {
            printf("ls: cannot access '%s': Not a directory\\n", pathname);
        }
        return 1;

    }

    for (int i = 0; i < dir->nrde; ++i) {
        printf("%s ", dir->dirents[i]->name);
    }
    printf("\n");
    return 0;
}

int scat(const char *pathname) {
    print("cat %s\n", pathname);

    node *file = find(pathname);
    if (file == NULL) {
        if (check_status() == 2) {
            printf("ls: cannot access '%s': No such file or directory\\n", pathname);
        } else if (check_status() == 0) {
            printf("ls: cannot access '%s': Not a directory\\n", pathname);
        }
        return 1;
    } else if (file->type == DNODE) {
        printf("cat: %s: Is a directory\\n", pathname);
        return 1;
    }

    for (int i = 0; i < file->size; ++i) {
        printf("%c", ((char *) (file->content))[i]);
    }
    printf("\n");
    return 0;
}

int smkdir(const char *pathname) {
    print("mkdir %s\n", pathname);

    node *dir = find(pathname);
    if (dir != NULL) {
        printf("mkdir: cannot create directory '%s': File exists\\n", pathname);
        return 1;
    } else {
        if (rmkdir(pathname) == -1) {
            if (check_status() == 0) {
                printf("mkdir: cannot create directory '%s': Not a directory\\n", pathname);
            } else if (check_status() == 2) {
                printf("mkdir: cannot create directory '%s': No such file or directory\\n", pathname);
            }
            return 1;
        }
    }
    return 0;
}

int stouch(const char *pathname) {
    print("touch %s\n", pathname);

    node *file = find(pathname);
    if (file == NULL) {
        int fd = ropen(pathname, O_CREAT | O_RDONLY);
        if (fd == -1) {
            if (check_status() == 0) {
                printf("touch: cannot touch '%s': Not a directory\\n", pathname);
            } else if (check_status() == 2) {
                printf("touch: cannot touch '%s': No such file or directory\\n", pathname);
            }
            return 1;
        } else {
            rclose(fd);
            return 0;
        }
    }
    return 0;
}

int secho(const char *content) {
    print("echo %s\n", content);

    size_t len = strlen(content), real_len = 0;
    char *real_content = malloc((len + 1) * sizeof(char));
    for (int i = 0; i < len - 1; ++i) {
        if ((content[i] == '\\' && content[i + 1] == '\\') || (content[i] == '\\' && content[i + 1] == '$'))i++;
        real_content[real_len++] = content[i];
    }

    size_t fin_len = 0;
    char *fin_content = malloc((real_len + 1) * sizeof(char));
    for (int i = 0; i < real_len - 1; ++i) {
        if (real_content[i] == '\\' && real_content[i + 1] == '\\') {
            i++;
            fin_content[fin_len++] = real_content[i];
        } else if (real_content[i] == '\\' && real_content[i + 1] == '$') {
            i++;
            fin_content[fin_len++] = 0; //$
        }

    }

    for (int i = 0; i < fin_len - 4; ++i) {
        if (fin_content[i] == '$' && fin_content[i + 1] == 'P' && fin_content[i + 2] == 'A' &&
            fin_content[i + 3] == 'T' && fin_content[i + 4] == 'H') {
            fin_content[i] = 1; //$PATH
        }
    }

    for (int i = 0; i < fin_len; ++i) {
        if (fin_content[i] == 0) {
            printf("$");
        } else if (fin_content[i] == 1) {
            printf("%s", PATH);
            i += 5;
        } else {
            printf("%c", fin_content[i]);
        }
    }
    return 0;
}

int swhich(const char *cmd) {
    print("which %s\n", cmd);

    char *directions[4097];
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
        if(find_file_below(find(directions[i]),cmd)){
            found = i;
            break;
        }
    }
    if(found != -1){
        printf("%s",directions[found]);
        for (int i = 0; i < count; ++i) free(directions[i]);
        return 0;
    } else{
        for (int i = 0; i < count; ++i) free(directions[i]);
        return 1;
    }
}


void init_shell() {
    //read .bashrc
    node *bash = find("/home/ubuntu/.bashrc");

    if (bash == NULL)return;

    char *buf = malloc((bash->size + 1) * sizeof(char));
    memcpy(buf, bash->content, bash->size);

    char *temp = malloc((bash->size + 1) * sizeof(char));
    for (int i = 0, j = 0; i < bash->size;) {
        temp[j] = buf[i];
        if (strcmp("export PATH=", temp) == 0) {
            bool include$ = false;
            for (int k = i + 1; k < bash->size && buf[k] != '\n'; ++k) {
                if (buf[k] == '$') {
                    include$ = true;
                    break;
                }
            }

            if (include$) {
                if (buf[i + 1] == '$') { //end
                    j = 0;
                    i += 5;
                    while (buf[i] != '\n' && i < bash->size) {
                        temp[j] = buf[i];
                        i++;
                        j++;
                    }
                    PATH = realloc(PATH, PATH_LEN + j);
                    memcpy(&PATH[PATH_LEN], temp, j);
                    PATH_LEN += j;
                } else { //head
                    j = 0;
                    i++;
                    while (buf[i] != '$' && i < bash->size) {
                        temp[j] = buf[i];
                        i++;
                        j++;
                    }
                    while (buf[i] != '\n' && i < bash->size)i++;
                    PATH = realloc(PATH, PATH_LEN + j);
                    memmove(&PATH[j], PATH, PATH_LEN);
                    memcpy(PATH, temp, j);
                    PATH_LEN += j;
                }

                j = 0;
                if (buf[i] == '\n')i++;
                memset(temp, 0, bash->size + 1);
                continue;
            } else { //set
                j = 0;
                i++;
                while (buf[i] != '\n' && i < bash->size) {
                    temp[j] = buf[i];
                    i++;
                    j++;
                }
                if (PATH != NULL)free(PATH);
                PATH = malloc((j + 1) * sizeof(char));
                memcpy(PATH, temp, j);
                PATH_LEN = j;

                j = 0;
                if (buf[i] == '\n')i++;
                memset(temp, 0, bash->size + 1);
                continue;
            }
        }

        i++;
        j++;
    }

    free(buf);
    free(temp);
}

void close_shell() {
    free(PATH);
    PATH = NULL;
}