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
  if(dir == NULL){
      if(find_flags == EEXIST){
          printf("ls: cannot access '%s': No such file or directory\\n",pathname);
      } else if(find_flags == ENOTDIR){
          printf("ls: cannot access '%s': Not a directory\\n",pathname);
      }
      return 1;
  }

    for (int i = 0; i < dir->nrde; ++i) {
        printf("%s ",dir->dirents[i]->name);
    }
    printf("\n");
    return 0;
}

int scat(const char *pathname) {
  print("cat %s\n", pathname);

    node *file = find(pathname);
    if(file == NULL){
        if(find_flags == EEXIST){
            printf("ls: cannot access '%s': No such file or directory\\n",pathname);
        } else if(find_flags == ENOTDIR){
            printf("ls: cannot access '%s': Not a directory\\n",pathname);
        }
        return 1;
    } else if(file->type == DNODE){
        printf("cat: %s: Is a directory\\n",pathname);
        return 1;
    }

    for (int i = 0; i < file->size; ++i) {
        printf("%c", ((char *)(file->content))[i]);
    }
    printf("\n");
    return 0;
}

int smkdir(const char *pathname) {
  print("mkdir %s\n", pathname);

}

int stouch(const char *pathname) {
  print("touch %s\n", pathname);

}

int secho(const char *content) {
  print("echo %s\n", content);

}

int swhich(const char *cmd) {
  print("which %s\n", cmd);

}

void init_shell() {
    //read .bashrc
    node *bash = find("/home/ubuntu/.bashrc");

    if(bash == NULL)return;

    char *buf = malloc((bash->size + 1) * sizeof(char));
    memcpy(buf,bash->content,bash->size);

    char *temp = malloc((bash->size + 1) * sizeof(char));
    for (int i = 0,j = 0; i < bash->size;) {
        temp[j] = buf[i];
        if(strcmp("export PATH=",temp) == 0){
            bool include$ = false;
            for (int k = i + 1;k < bash->size && buf[k] != '\n'; ++k) {
                if(buf[k] == '$'){
                    include$ = true;
                    break;
                }
            }

            if(include$){
                if(buf[i + 1] == '$'){ //end
                    j = 0;
                    i += 5;
                    while (buf[i] != '\n' && i < bash->size){
                        temp[j] = buf[i];
                        i++;
                        j++;
                    }
                    PATH = realloc(PATH,PATH_LEN + j);
                    memcpy(&PATH[PATH_LEN],temp,j);
                    PATH_LEN += j;
                } else{ //head
                    j = 0;
                    i++;
                    while (buf[i] != '$' && i < bash->size){
                        temp[j] = buf[i];
                        i++;
                        j++;
                    }
                    while (buf[i] != '\n' && i < bash->size)i++;
                    PATH = realloc(PATH,PATH_LEN + j);
                    memmove(&PATH[j],PATH,PATH_LEN);
                    memcpy(PATH,temp,j);
                    PATH_LEN += j;
                }

                j = 0;
                if(buf[i] == '\n')i++;
                memset(temp,0,bash->size + 1);
                continue;
            } else{ //set
                j = 0;
                i++;
                while (buf[i] != '\n' && i < bash->size){
                    temp[j] = buf[i];
                    i++;
                    j++;
                }
                if(PATH != NULL)free(PATH);
                PATH = malloc((j + 1) * sizeof(char));
                memcpy(PATH,temp,j);
                PATH_LEN = j;

                j = 0;
                if(buf[i] == '\n')i++;
                memset(temp,0,bash->size + 1);
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