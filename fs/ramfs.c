#include "ramfs.h"
#include <assert.h>
#include <ctype.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>

node *root = NULL;

#define MAX_LEN (4096 + 1)
#define NRFD 4096
FD fdesc[NRFD];

char *strdup(const char *content){
    char *new = malloc(strlen(content) + 1);
    strcpy(new,content);
    return new;
}

void reduce_slashes(const char *input, char *output) {
    int i, j = 0;
    for (i = 0; input[i] != 0; ++i) {
        if (input[i] == '/' && input[i + 1] == '/')
            continue;
        output[j++] = input[i];
    }
    output[j] = 0;
}

node* find_file_all(node* root, const char* name) {
    if (strcmp(root->name, name) == 0) {
        return root;
    }

    if (root->type == DNODE) {
        for (int i = 0; i < root->nrde; ++i) {
            node *found = find_file_all(root->dirents[i], name);
            if (found != NULL) {
                return found;
            }
        }
    }

    return NULL;
}

bool find_file_now(node* now, const char* name) {
    if (now->type == DNODE) {
        for (int i = 0; i < now->nrde; ++i) {
            if(strcmp(now->dirents[i]->name,name) == 0){
                return true;
            }
        }
    }

    return false;
}

node *find(const char *pathname) {
    if (pathname == NULL)return NULL;

    char *pathname_simple = malloc(MAX_LEN * sizeof(char)); //reduce slashes
    reduce_slashes(pathname,pathname_simple);

    if (strcmp("/", pathname_simple) == 0)return root;

    node *now = root;
    const char *p, *q;
    char *directions[MAX_LEN];
    int count = 0;
    p = q = &pathname_simple[1];
    while (true) {
        while (*p != '/' && *p != 0)p++;
        char *temp_dir = malloc((p - q + 1) * sizeof(char));
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

int rmkdir(const char *pathname) { //no error dealing
    if (pathname == NULL) return -1;

    char *pathname_simple = malloc(MAX_LEN * sizeof(char)); //reduce slashes
    reduce_slashes(pathname,pathname_simple);

    if (find(pathname_simple) == root) { //eexist
        free(pathname_simple);
        return -1;
    }

    const char *p, *q;
    char *directions[2048];
    int count = 0;
    p = q = &pathname_simple[1];
    while (true) {
        while (*p != '/' && *p != 0)p++;
        char *temp_dir = malloc((p - q + 1) * sizeof(char));
        strncpy(temp_dir, q, p - q);
        directions[count++] = temp_dir;
        if (*p == 0)break;
        p++;
        q = p;
    }

    for (int i = 0;directions[count - 1][i] != 0; ++i) { //einval
        if(isalnum(directions[count - 1][i]) == 0 && directions[count - 1][i] != '.'){
            free(pathname_simple);
            return -1;
        }
    }

    node *temp = malloc(sizeof(node));
    if (temp == NULL) {
        for (int i = 0; i < count; ++i) free(directions[i]);
        free(pathname_simple);
        return -1;
    }
    temp->type = DNODE;
    temp->dirents = NULL;
    temp->content = NULL;
    temp->nrde = 0;
    temp->name = strdup(directions[count - 1]);
    temp->size = 0;

    if (count == 1) { //below root
        if(find_file_now(root,temp->name)){ //eexist
            for (int i = 0; i < count; ++i) free(directions[i]);
            free(pathname_simple);
            free(temp);
            return -1;
        }

        root->nrde++;
        root->dirents = realloc(root->dirents, root->nrde * sizeof(node));
        if (root->dirents == NULL) {
            for (int i = 0; i < count; ++i) free(directions[i]);
            free(pathname_simple);
            free(temp);
            return -1;
        }
        root->dirents[root->nrde - 1] = temp;

    } else {
        char *up_dir_name = malloc(1 + strlen(pathname_simple));
        strcpy(up_dir_name, pathname_simple);
        up_dir_name[strlen(pathname_simple) - strlen(directions[count - 1]) - 1] = 0;
        node *up_dir = find(up_dir_name);

        if (up_dir == NULL || up_dir->type == FNODE) {//enoent or enotdir
            for (int i = 0; i < count; ++i) free(directions[i]);
            free(up_dir_name);
            free(pathname_simple);
            free(temp);
            return -1;
        }

        if(find_file_now(up_dir,temp->name)){ //eexist
            for (int i = 0; i < count; ++i) free(directions[i]);
            free(pathname_simple);
            free(temp);
            return -1;
        }

        up_dir->nrde++;
        up_dir->dirents = realloc(up_dir->dirents, up_dir->nrde * sizeof(node));

        if (up_dir->dirents == NULL) {
            for (int i = 0; i < count; ++i) free(directions[i]);
            free(pathname_simple);
            free(temp);
            return -1;
        }

        up_dir->dirents[up_dir->nrde - 1] = temp;
    }

    for (int i = 0; i < count; ++i) free(directions[i]);
    free(pathname_simple);
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