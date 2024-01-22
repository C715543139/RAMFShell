#include "ramfs.h"
#include <assert.h>
#include <ctype.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>

#define NRFD 8192
FD fdesc[NRFD];

node *root = NULL;

enum {
    ENOTDIR, EEXIST, ENOENT, SPECIAL //0,1,2,3
} g_error;

int CheckErrorType() {
    return g_error;
}

char *strdup(const char *content) {
    char *new = malloc(strlen(content) + 1);
    strcpy(new, content);
    return new;
}

bool CheckLegal(const char *pathname) {
    for (int i = 0; pathname[i] != 0; ++i) {
        if (isalnum(pathname[i]) == 0 && pathname[i] != '.' && pathname[i] != '/') {
            return false;
        }
    }
    return true;
}

void SplitPathname(char **directions, const char *pathnameSimplified, int *count, bool *isSlashEnd) {
    *count = 0;
    *isSlashEnd = false;
    char *p, *q;

    p = strchr(pathnameSimplified, '/');
    while (p != NULL) {
        q = strchr(++p, '/');

        char *temp;
        if (q == NULL) { // end is '\0', so use strdup
            temp = strdup(p);
        } else if (*q == '/' && *(q + 1) == 0) {
            *isSlashEnd = true;
            *q = 0;
            temp = strdup(p);
            q = NULL;
        } else { // end is not '/0', so calculate
            temp = calloc(q - p + 1, (q - p + 1) * sizeof(char));
            strncpy(temp, p, q - p);
        }

        directions[(*count)++] = temp;
        p = q;
    }
}

char *ReduceSlashes(const char *input) {
    char *output = calloc((strlen(input) + 10), (strlen(input) + 10) * sizeof(char));
    int i, j = 0;
    for (i = 0; input[i] != 0; ++i) {
        if (input[i] == '/' && input[i + 1] == '/') {
            continue;
        }
        output[j++] = input[i];
    }
    return output;
}

node *FindNodeBelow(node *dir, const char *name) {
    if (dir != NULL && dir->type == DNODE) {
        for (int i = 0; i < dir->nrde; ++i) {
            if (strcmp(dir->dirents[i]->name, name) == 0) {
                return dir->dirents[i];
            }
        }
    }
    return NULL;
}

node *FindNode(const char *pathname, bool simplified) {
    if (pathname == NULL) {
        return NULL;
    }

    char *pathnameSimplified;
    if (simplified) {
        pathnameSimplified = malloc(MAX_LEN * sizeof(char));
        strcpy(pathnameSimplified, pathname);
    } else {
        pathnameSimplified = ReduceSlashes(pathname);
    }

    if (strcmp("/", pathnameSimplified) == 0) {
        free(pathnameSimplified);
        return root;
    }

    node *now = root;
    char *directions[MAX_LEN];
    int count = 0;
    bool isSlashEnd = false;
    SplitPathname(directions, pathnameSimplified, &count, &isSlashEnd);

    for (int i = 0; i < count; ++i) {
        if (now == NULL || (now->type == DNODE && now->nrde == 0)) {
            now = NULL;
            g_error = ENOENT;
            break;
        } else if (now->type == FNODE) {
            now = NULL;
            g_error = ENOTDIR;
            break;
        }
        now = FindNodeBelow(now, directions[i]);
    }

    for (int i = 0; i < count; ++i) free(directions[i]);
    free(pathnameSimplified);
    return now;
}

int ropen(const char *pathname, int flags) {
    if (pathname == NULL) {
        return -1;
    }

    bool append = flags & O_APPEND,
         create = flags & O_CREAT,
         truncate = flags & O_TRUNC;
    int rw = 0; // rdonly 0,wronly 1,rdwr 2
    if ((flags & O_RDONLY) == O_RDONLY) rw = 0;
    if ((flags & O_WRONLY) == O_WRONLY) rw = 1;
    if ((flags & O_RDWR) == O_RDWR && rw == 0) rw = 2;

    node *file = FindNode(pathname, false);
    if (file == NULL) {
        if (create == false) {
            return -1;
        } else { // create
            if (CheckLegal(pathname) == false) { // illegal
                g_error = ENOENT;
                return -1;
            }

            char *pathnameSimplified = ReduceSlashes(pathname);
            char *directions[MAX_LEN];
            int count = 0;
            bool isSlashEnd = false;
            SplitPathname(directions, pathnameSimplified, &count, &isSlashEnd);

            if (isSlashEnd || strlen(directions[count - 1]) > 32) { // basename or not file
                for (int i = 0; i < count; ++i) free(directions[i]);
                free(pathnameSimplified);
                g_error = SPECIAL;
                return -1;
            }

            node *temp = malloc(sizeof(node));
            *temp = (node) {
                    .type = FNODE,
                    .nrde = 0,
                    .size = 0,
                    .upper = NULL,
                    .dirents = NULL,
                    .content = NULL,
                    .name = strdup(directions[count - 1]),
            };

            if (count == 1) { // below root
                root->dirents = realloc(root->dirents, ++(root->nrde) * sizeof(node));
                root->dirents[root->nrde - 1] = temp;
                temp->upper = root;
            } else {
                char *upperName = strdup(pathnameSimplified);
                upperName[strlen(pathnameSimplified) - strlen(directions[count - 1]) - 1] = 0;

                node *upperNode = FindNode(upperName, true);

                if (upperNode == NULL || upperNode->type == FNODE) {//enoent or enotdir
                    g_error = upperNode == NULL ? ENOENT : ENOTDIR;
                    for (int i = 0; i < count; ++i) free(directions[i]);
                    free(upperName);
                    free(pathnameSimplified);
                    free(temp->name);
                    free(temp);
                    return -1;
                }

                upperNode->dirents = realloc(upperNode->dirents, ++(upperNode->nrde) * sizeof(node));
                upperNode->dirents[upperNode->nrde - 1] = temp;
                temp->upper = upperNode;
                free(upperName);
            }

            file = temp;
            for (int i = 0; i < count; ++i) free(directions[i]);
            free(pathnameSimplified);
        }
    }

    if (file->type == DNODE && rw != 0) { // directory without read only is illegal
        return -1;
    } else if (file->type == FNODE && pathname[strlen(pathname) - 1] == '/') {
        g_error = ENOENT;
        return -1;
    }

    int fdescAvailable;
    for (fdescAvailable = 0; fdescAvailable < NRFD; ++fdescAvailable) {
        if (fdesc[fdescAvailable].used == false) {
            break;
        }
    }

    if (append) { // offset
        fdesc[fdescAvailable].offset = file->size;
    } else {
        fdesc[fdescAvailable].offset = 0;
    }

    if (truncate && rw != 0) {
        if (file->content != NULL) {
            free(file->content);
            file->content = NULL;
            file->size = 0;
        }
    }

    fdesc[fdescAvailable].used = true;
    fdesc[fdescAvailable].flags = flags;
    fdesc[fdescAvailable].f = file;
    return fdescAvailable;
}

int rclose(int fd) {
    if (fd < 0 || fdesc[fd].used == false) {
        return -1;
    }

    fdesc[fd] = (FD) {
            .offset = 0,
            .used = false,
            .f = NULL,
            .flags = 0
    };
    return 0;
}

ssize_t rwrite(int fd, const void *buf, size_t count) {
    if (fd < 0 || fdesc[fd].used == false || fdesc[fd].f->type == DNODE || buf == NULL) { // ebadf or eisdir
        return -1;
    }

    int rw = 0; // rdonly 0,wronly 1,rdwr 2
    if ((fdesc[fd].flags & O_RDONLY) == O_RDONLY) rw = 0;
    if ((fdesc[fd].flags & O_WRONLY) == O_WRONLY) rw = 1;
    if ((fdesc[fd].flags & O_RDWR) == O_RDWR && rw == 0) rw = 2;

    if (rw == 0) { // ebadf
        return -1;
    }

    if (count + fdesc[fd].offset > fdesc[fd].f->size) {
        fdesc[fd].f->content = realloc(fdesc[fd].f->content, count + fdesc[fd].offset);
        if (fdesc[fd].offset > fdesc[fd].f->size) { // fill with '\0'
            memset((char *) fdesc[fd].f->content + fdesc[fd].f->size, 0, count + fdesc[fd].offset - fdesc[fd].f->size);
        }
        fdesc[fd].f->size = count + fdesc[fd].offset;
    }

    memcpy((char *) fdesc[fd].f->content + fdesc[fd].offset, buf, count);
    fdesc[fd].offset += count;
    return count;
}

ssize_t rread(int fd, void *buf, size_t count) {
    if (fd < 0 || fdesc[fd].used == false || fdesc[fd].f->type == DNODE || buf == NULL) { // ebadf or eisdir
        return -1;
    }

    int rw = 0; // rdonly 0,wronly 1,rdwr 2
    if ((fdesc[fd].flags & O_RDONLY) == O_RDONLY) rw = 0;
    if ((fdesc[fd].flags & O_WRONLY) == O_WRONLY) rw = 1;
    if ((fdesc[fd].flags & O_RDWR) == O_RDWR && rw == 0) rw = 2;

    if (rw == 1) { // ebadf
        return -1;
    }

    if (fdesc[fd].offset >= fdesc[fd].f->size || count == 0) {
        return 0;
    } else if (count + fdesc[fd].offset > fdesc[fd].f->size) {
        count = fdesc[fd].f->size - fdesc[fd].offset;
    }

    memcpy(buf, (char *) fdesc[fd].f->content + fdesc[fd].offset, count);
    fdesc[fd].offset += count;
    return count;
}

off_t rseek(int fd, off_t offset, int whence) {
    if (fd < 0 || fd >= NRFD) {
        return -1;
    }

    switch (whence) {
        case SEEK_SET:
            if (offset < 0) {
                return -1;
            } else {
                fdesc[fd].offset = offset;
                return offset;
            }
        case SEEK_CUR:
            if (fdesc[fd].offset + offset < 0) {
                return -1;
            } else {
                fdesc[fd].offset += offset;
                return fdesc[fd].offset;
            }
        case SEEK_END:
            if (fdesc[fd].f->size + offset < 0) {
                return -1;
            } else {
                fdesc[fd].offset = fdesc[fd].f->size + offset;
                return fdesc[fd].offset;
            }
        default:
            return 0;
    }
}

int rmkdir(const char *pathname) {
    if (pathname == NULL) {
        return -1;
    }

    if (CheckLegal(pathname) == false) {
        g_error = ENOENT;
        return -1;
    }

    char *pathnameSimplified = ReduceSlashes(pathname);
    if (FindNode(pathnameSimplified, true) != NULL) { // eexist
        free(pathnameSimplified);
        g_error = EEXIST;
        return -1;
    }

    char *directions[MAX_LEN];
    int count = 0;
    bool isSlashEnd = false;
    SplitPathname(directions, pathnameSimplified, &count, &isSlashEnd);

    if (strlen(directions[count - 1]) > 32) { // basename
        for (int i = 0; i < count; ++i) free(directions[i]);
        free(pathnameSimplified);
        g_error = ENOENT;
        return -1;
    }

    node *temp = malloc(sizeof(node));
    *temp = (node) {
            .type = DNODE,
            .nrde = 0,
            .size = 0,
            .upper = NULL,
            .dirents = NULL,
            .content = NULL,
            .name = strdup(directions[count - 1]),
    };

    if (count == 1) { // below root
        root->dirents = realloc(root->dirents, ++(root->nrde) * sizeof(node));
        root->dirents[root->nrde - 1] = temp;
        temp->upper = root;
    } else {
        char *upperName = strdup(pathnameSimplified);
        upperName[strlen(pathnameSimplified) - strlen(directions[count - 1]) - 1] = 0;

        node *upperNode = FindNode(upperName, true);
        if (upperNode == NULL || upperNode->type == FNODE) { // enoent or enotdir
            g_error = upperNode == NULL ? ENOENT : ENOTDIR;
            for (int i = 0; i < count; ++i) free(directions[i]);
            free(upperName);
            free(pathnameSimplified);
            free(temp->name);
            free(temp);
            return -1;
        }

        upperNode->dirents = realloc(upperNode->dirents, ++(upperNode->nrde) * sizeof(node));
        upperNode->dirents[upperNode->nrde - 1] = temp;
        temp->upper = upperNode;
        free(upperName);
    }

    for (int i = 0; i < count; ++i) free(directions[i]);
    free(pathnameSimplified);
    return 0;
}

int rrmdir(const char *pathname) {
    if (pathname == NULL) {
        return -1;
    }

    node *temp = FindNode(pathname, false);
    if (temp == NULL || temp->type == FNODE || temp->nrde != 0 || temp == root) {
        return -1;
    }

    node *upper = temp->upper;
    int location = 0;
    for (int i = 0; i < upper->nrde; ++i) {
        if (strcmp(upper->dirents[i]->name, temp->name) == 0) {
            free(temp->name);
            free(upper->dirents[i]);
            location = i;
            break;
        }
    }

    for (int i = location; i < upper->nrde - 1; ++i) { // move
        upper->dirents[i] = upper->dirents[i + 1];
    }
    upper->nrde--;
    return 0;
}

int runlink(const char *pathname) {
    if (pathname == NULL) {
        return -1;
    }

    node *temp = FindNode(pathname, false);
    if (temp == NULL || temp->type == DNODE) { // enoent or eisdir
        return -1;
    }

    node *upper = temp->upper;
    int location = 0;
    for (int i = 0; i < upper->nrde; ++i) {
        if (strcmp(upper->dirents[i]->name, temp->name) == 0) {
            free(temp->name);
            free(temp->content == NULL ? NULL : temp->content);
            free(upper->dirents[i]);
            location = i;
            break;
        }
    }

    for (int i = location; i < upper->nrde - 1; ++i) { // move
        upper->dirents[i] = upper->dirents[i + 1];
    }
    upper->nrde--;
    return 0;
}

void init_ramfs() {
    node *temp = malloc(sizeof(node));
    *temp = (node) {
            .type = DNODE,
            .nrde = 0,
            .size = 0,
            .upper = NULL,
            .dirents = NULL,
            .content = NULL,
            .name = strdup("/"),
    };
    root = temp;

    for (int i = 0; i < NRFD; ++i) {
        fdesc[i] = (FD) {
                .offset = 0,
                .f = NULL,
                .flags = 0,
                .used = false
        };
    }
}

void FreeNode(node *node) {
    if (node == NULL) {
        return;
    }

    if (node->type == FNODE) {
        free(node->content == NULL ? NULL : node->content);
    } else {
        for (int i = 0; i < node->nrde; ++i) FreeNode(node->dirents[i]);
    }

    free(node->name);
    free(node);
}

void close_ramfs() {
    if (root == NULL) {
        return;
    }

    FreeNode(root);
    root = NULL;
}