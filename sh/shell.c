#include "ramfs.h"
#include "shell.h"
#ifndef ONLINE_JUDGE
  #define print(...) printf("\033[31m");printf(__VA_ARGS__);printf("\033[0m");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

path *head,*tail;

int sls(const char *pathname) {
  print("ls %s\n", pathname);

}

int scat(const char *pathname) {
  print("cat %s\n", pathname);

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
    head = tail = NULL;




}

void close_shell() {

}