#include "ramfs.h"
#include <assert.h>
#include <ctype.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>

node *root = NULL;

#define NRFD 4096
FD fdesc[NRFD];

node *find(const char *pathname) {
    if (pathname == NULL)return NULL;

    if (strcmp("/", pathname) == 0)return root;

    node *now = root;
    const char *p, *q;
    char *directions[2048];
    int count = 0;
    p = q = &pathname[1];
    while (true) {
        while (*p != '/' && *p != 0)p++;
        char *temp_dir = malloc(p - q + 1);
        strncpy(temp_dir, q, p - q);
        directions[count++] = temp_dir;
        if (*p == 0)break;
        p++;
        q = p;
    }

    for (int i = 0; i < count; ++i) {
        if (now->type == FNODE || now->nrde == 0) {
            for (int j = 0; j < count; ++j) free(directions[j]);
            return NULL;
        }

        bool found = false;
        for (int j = 0; j < now->nrde; ++j) {
            if (strcmp(directions[i], now->dirents[j]->name) == 0) {
                now = now->dirents[j];
                found = true;
                break;
            }
        }
        if (found == false) {
            for (int j = 0; j < count; ++j) free(directions[j]);
            return NULL;
        }
    }

    for (int i = 0; i < count; ++i) free(directions[i]);
    return now;
}

int ropen(const char *pathname, int flags) {

}

int rclose(int fd) {

}

ssize_t rwrite(int fd, const void *buf, size_t count) {

}

ssize_t rread(int fd, void *buf, size_t count) {

}

off_t rseek(int fd, off_t offset, int whence) {

}

int rmkdir(const char *pathname) {
    if (pathname == NULL || strcmp("/", pathname) == 0) return -1;

    const char *p, *q;
    char *directions[2048];
    int count = 0;
    p = q = &pathname[1];
    while (true) {
        while (*p != '/' && *p != 0)p++;
        char *temp_dir = malloc(p - q + 1);
        strncpy(temp_dir, q, p - q);
        directions[count++] = temp_dir;
        if (*p == 0)break;
        p++;
        q = p;
    }

    node *temp = malloc(sizeof(node));
    if (temp == NULL) {
        for (int i = 0; i < count; ++i) free(directions[i]);
        return -1;
    }
    temp->type = DNODE;
    temp->dirents = NULL;
    temp->content = NULL;
    temp->nrde = 0;
    temp->name = directions[count - 1];
    temp->size = 0;

    if (count == 1) { //root
        root->nrde++;
        root->dirents = realloc(root->dirents, root->nrde * sizeof(node));
        if (root->dirents == NULL) {
            for (int i = 0; i < count; ++i) free(directions[i]);
            return -1;
        }
        root->dirents[root->nrde - 1] = temp;

    } else {
        char *up_dir_name = malloc(1 + strlen(pathname));
        strcpy(up_dir_name, pathname);
        up_dir_name[strlen(pathname) - strlen(directions[count - 1]) - 1] = 0;
        node *up_dir = find(up_dir_name);

        if (up_dir->type == FNODE) {
            for (int i = 0; i < count; ++i) free(directions[i]);
            free(up_dir_name);
            return -1;
        }
        up_dir->nrde++;
        up_dir->dirents = realloc(up_dir->dirents, up_dir->nrde * sizeof(node));
        if (up_dir->dirents == NULL) {
            for (int i = 0; i < count; ++i) free(directions[i]);
            return -1;
        }
        up_dir->dirents[up_dir->nrde - 1] = temp;
    }
    return 0;
}

int rrmdir(const char *pathname) {

}

int runlink(const char *pathname) {

}

void init_ramfs() {
    node *temp = malloc(sizeof(node));
    if (temp == NULL) return;

    temp->type = DNODE;
    temp->dirents = NULL;
    temp->content = NULL;
    temp->nrde = 0;
    temp->name = malloc(2 * sizeof(char));
    strcpy(temp->name, "/");
    temp->size = 0;

    root = temp;
}

void close_ramfs() {
    root = NULL;
}