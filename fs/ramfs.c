#include "ramfs.h"
#include <assert.h>
#include <ctype.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>
#include "limits.h"

node *root = NULL;

#define MAX_LEN 65536
#define NRFD 4096
FD fdesc[NRFD];

enum {
    ENOTDIR, EEXIST, ENOENT //0,1,2
} status;

int check_status() {
    return status;
}

char *strdup(const char *content) {
    char *new = malloc(strlen(content) + 1);
    strcpy(new, content);
    return new;
}

void split_path(char **directions,const char *pathname_simple,int *count,bool *is_slash_end){
    *count = 0;
    *is_slash_end = false;
    char *p, *q;

    p = strchr(pathname_simple,'/');
    while (p != NULL){
        p++;
        q = strchr(p,'/');

        char *temp;
        if(q == NULL){
            temp = strdup(p);
        } else if(*q == '/' && *(q + 1) == 0){
            *q = 0;
            temp = strdup(p);
            *is_slash_end = true;
            q = NULL;
        } else {
            temp = calloc(q - p + 1,(q - p + 1) * sizeof(char));
            strncpy(temp,p,q - p);
        }
        directions[*count] = temp;
        *count += 1;

        p = q;
    }
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

node *find_file_below(node *now, const char *name) {
    if (now != NULL && now->type == DNODE) {
        for (int i = 0; i < now->nrde; ++i) {
            if (strcmp(now->dirents[i]->name, name) == 0) {
                return now->dirents[i];
            }
        }
    }

    return NULL;
}

node *find(const char *pathname, bool simple) {
    if (pathname == NULL)return NULL;

    char *pathname_simple = malloc(MAX_LEN * sizeof(char)); //reduce slashes
    if (simple) {
        strcpy(pathname_simple, pathname);
    } else {
        reduce_slashes(pathname, pathname_simple);
    }

    if (strcmp("/", pathname_simple) == 0) {
        free(pathname_simple);
        return root;
    }

    node *now = root;
    char *directions[MAX_LEN];
    int count = 0;
    bool is_slash_end = false;
    split_path(directions,pathname_simple,&count,&is_slash_end);

    for (int i = 0; i < count; ++i) {
        if (now == NULL || now->type == FNODE) {
            now = NULL;
            status = ENOENT;
            break;
        } else if (now->nrde == 0) {
            now = NULL;
            status = ENOTDIR;
            break;
        }

        now = find_file_below(now,directions[i]);
    }

    for (int i = 0; i < count; ++i) free(directions[i]);
    free(pathname_simple);
    return now;
}

int ropen(const char *pathname, int flags) {
    if (pathname == NULL) return -1;

    bool append = false, create = false, trunc = false;
    int rw = 0; //rdonly 0,wronly 1,rdwr 2
    if (flags & O_APPEND) append = true;
    if (flags & O_CREAT) create = true;
    if (flags & O_TRUNC) trunc = true;
    if ((flags & O_RDONLY) == O_RDONLY) rw = 0;
    if ((flags & O_WRONLY) == O_WRONLY) rw = 1;
    if ((flags & O_RDWR) == O_RDWR && rw == 0) rw = 2;

    node *file = find(pathname, false);

    if (file == NULL) {
        if (create == true) { //create
            char *pathname_simple = malloc((strlen(pathname) + 1) * sizeof(char)); //reduce slashes
            reduce_slashes(pathname, pathname_simple);

            for (int i = 0;pathname_simple[i] != 0; ++i) { //einval
                if (isalnum(pathname_simple[i]) == 0 && pathname_simple[i] != '.' && pathname_simple[i] != '/') {
                    free(pathname_simple);
                    status = ENOENT;
                    return -1;
                }
            }

            char *directions[MAX_LEN];
            int count = 0;
            bool is_slash_end = false;
            split_path(directions,pathname_simple,&count,&is_slash_end);

            if (is_slash_end || strlen(directions[count - 1]) > 32) { //basename
                for (int i = 0; i < count; ++i) free(directions[i]);
                free(pathname_simple);
                status = ENOENT;
                return -1;
            }

            node *temp = malloc(sizeof(node));
            temp->type = FNODE;
            temp->dirents = NULL;
            temp->content = NULL;
            temp->nrde = 0;
            temp->name = strdup(directions[count - 1]);
            temp->size = 0;

            if (count == 1) { //below root
                root->nrde++;
                root->dirents = realloc(root->dirents, root->nrde * sizeof(node));
                root->dirents[root->nrde - 1] = temp;
                temp->upper = root;
            } else {
                char *up_dir_name = malloc(1 + strlen(pathname_simple));
                strcpy(up_dir_name, pathname_simple);
                up_dir_name[strlen(pathname_simple) - strlen(directions[count - 1]) - 1] = 0;
                node *up_dir = find(up_dir_name, true);

                if (up_dir == NULL || up_dir->type == FNODE) {//enoent or enotdir
                    if (up_dir != NULL && up_dir->type == FNODE) {
                        status = ENOTDIR;
                    }
                    for (int i = 0; i < count; ++i) free(directions[i]);
                    free(up_dir_name);
                    free(pathname_simple);
                    free(temp->name);
                    free(temp);
                    return -1;
                }

                up_dir->nrde++;
                up_dir->dirents = realloc(up_dir->dirents, up_dir->nrde * sizeof(node));
                up_dir->dirents[up_dir->nrde - 1] = temp;
                temp->upper = up_dir;

                free(up_dir_name);
            }

            for (int i = 0; i < count; ++i) free(directions[i]);
            free(pathname_simple);

            file = temp;
        } else {
            return -1;
        }
    }

    if (file->type == DNODE && rw != 0) {
        return -1;
    } else if(file->type == FNODE && pathname[strlen(pathname) - 1] == '/'){
        return -1;
    }

    int fdesc_available;
    for (fdesc_available = 0; fdesc_available < NRFD; ++fdesc_available) {
        if (fdesc[fdesc_available].used == false)break;
    }

    if (append) { //offset
        fdesc[fdesc_available].offset = file->size;
    } else {
        fdesc[fdesc_available].offset = 0;
    }

    if (trunc && rw != 0) {
        if (file->content != NULL) {
            free(file->content);
            file->content = NULL;
            file->size = 0;
        }
    }

    fdesc[fdesc_available].used = true;
    fdesc[fdesc_available].flags = flags;
    fdesc[fdesc_available].f = file;

    return fdesc_available;
}

int rclose(int fd) {
    if (fd < 0 || fdesc[fd].used == false)return -1;

    fdesc[fd].offset = 0;
    fdesc[fd].used = false;
    fdesc[fd].f = NULL;
    fdesc[fd].flags = 0;

    return 0;
}

ssize_t rwrite(int fd, const void *buf, size_t count) {
    if (fd < 0 || fdesc[fd].used == false || fdesc[fd].f->type == DNODE) { //ebadf or eisdir
        return -1;
    }

    int rw = 0; //rdonly 0,wronly 1,rdwr 2
    if ((fdesc[fd].flags & O_RDONLY) == O_RDONLY) rw = 0;
    if ((fdesc[fd].flags & O_WRONLY) == O_WRONLY) rw = 1;
    if ((fdesc[fd].flags & O_RDWR) == O_RDWR) {
        if (rw == 0) rw = 2;
    }

    if (fdesc[fd].f->type == DNODE || rw == 0) { //eisdir or ebadf
        return -1;
    }

    if (count + fdesc[fd].offset >= LONG_MAX)
        return -1;

    if (count + fdesc[fd].offset > fdesc[fd].f->size) {
        fdesc[fd].f->content = realloc(fdesc[fd].f->content, count + fdesc[fd].offset);

        if (fdesc[fd].offset > fdesc[fd].f->size) {
            memset((char *) fdesc[fd].f->content + fdesc[fd].f->size, 0, count + fdesc[fd].offset - fdesc[fd].f->size);
        }
        fdesc[fd].f->size = count + fdesc[fd].offset;
    }

    memcpy((char *) fdesc[fd].f->content + fdesc[fd].offset, buf, count);
    fdesc[fd].offset += count;
    return count;
}

ssize_t rread(int fd, void *buf, size_t count) {
    if (fd < 0 || fdesc[fd].used == false || fdesc[fd].f->type == DNODE) { //ebadf or eisdir
        return -1;
    }

    int rw = 0; //rdonly 0,wronly 1,rdwr 2
    if ((fdesc[fd].flags & O_RDONLY) == O_RDONLY) rw = 0;
    if ((fdesc[fd].flags & O_WRONLY) == O_WRONLY) rw = 1;
    if ((fdesc[fd].flags & O_RDWR) == O_RDWR) {
        if (rw == 0) rw = 2;
    }

    if (fdesc[fd].f->type == DNODE || rw == 1) { //eisdir or ebadf
        return -1;
    }

    if (count + fdesc[fd].offset >= LONG_MAX)
        return -1;

    if (fdesc[fd].offset >= fdesc[fd].f->size) {
        return 0;
    } else if (count + fdesc[fd].offset <= fdesc[fd].f->size) {
        memcpy(buf, (char *) fdesc[fd].f->content + fdesc[fd].offset, count);
        fdesc[fd].offset += count;
    } else {
        count = fdesc[fd].f->size - fdesc[fd].offset;
        memcpy(buf, (char *) fdesc[fd].f->content + fdesc[fd].offset, count);
        fdesc[fd].offset = fdesc[fd].f->size;
    }
    return count;
}

off_t rseek(int fd, off_t offset, int whence) {
    if (fd < 0 || fd >= NRFD)return -1;
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
    if (pathname == NULL) return -1;

    char *pathname_simple = malloc(MAX_LEN * sizeof(char)); //reduce slashes
    reduce_slashes(pathname, pathname_simple);

    for (int i = 0;pathname_simple[i] != 0; ++i) { //einval
        if (isalnum(pathname_simple[i]) == 0 && pathname_simple[i] != '.' && pathname_simple[i] != '/') {
            free(pathname_simple);
            status = ENOENT;
            return -1;
        }
    }

    char *directions[MAX_LEN];
    int count = 0;
    bool is_slash_end = false;
    split_path(directions,pathname_simple,&count,&is_slash_end);

    if (is_slash_end || strlen(directions[count - 1]) > 32) { //basename
        for (int i = 0; i < count; ++i) free(directions[i]);
        free(pathname_simple);
        status = ENOENT;
        return -1;
    }

    node *temp = malloc(sizeof(node));
    temp->type = DNODE;
    temp->dirents = NULL;
    temp->content = NULL;
    temp->nrde = 0;
    temp->name = strdup(directions[count - 1]);
    temp->size = 0;

    if (count == 1) { //below root
        root->nrde++;
        root->dirents = realloc(root->dirents, root->nrde * sizeof(node));
        root->dirents[root->nrde - 1] = temp;
        temp->upper = root;

    } else {
        char *up_dir_name = malloc(1 + strlen(pathname_simple));
        strcpy(up_dir_name, pathname_simple);
        up_dir_name[strlen(pathname_simple) - strlen(directions[count - 1]) - 1] = 0;
        node *up_dir = find(up_dir_name, true);

        if (up_dir == NULL || up_dir->type == FNODE) {//enoent or enotdir
            if (up_dir != NULL && up_dir->type == FNODE)status = ENOTDIR;
            for (int i = 0; i < count; ++i) free(directions[i]);
            free(up_dir_name);
            free(pathname_simple);
            free(temp->name);
            free(temp);
            return -1;
        }

        up_dir->nrde++;
        up_dir->dirents = realloc(up_dir->dirents, up_dir->nrde * sizeof(node));
        up_dir->dirents[up_dir->nrde - 1] = temp;
        temp->upper = up_dir;

        free(up_dir_name);
    }

    for (int i = 0; i < count; ++i) free(directions[i]);
    free(pathname_simple);

    return 0;
}

int rrmdir(const char *pathname) {
    if (pathname == NULL)return -1;

    node *temp = find(pathname, false);
    if (temp == NULL) { //enoent
        return -1;
    } else if (temp->type == FNODE || temp->nrde != 0) { //enotdir or enotempty
        return -1;
    } else if (strcmp("/", pathname) == 0) {//eacces
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
    for (int i = location; i < upper->nrde - 1; ++i) {//move
        upper->dirents[i] = upper->dirents[i + 1];
    }
    upper->nrde--;
    return 0;
}

int runlink(const char *pathname) {
    if (pathname == NULL) return -1;

    node *temp = find(pathname, false);
    if (temp == NULL) {//enoent
        return -1;
    } else if (temp->type == DNODE) {//eisdir
        return -1;
    }

    node *upper = temp->upper;
    int location = 0;
    for (int i = 0; i < upper->nrde; ++i) {
        if (strcmp(upper->dirents[i]->name, temp->name) == 0) {
            free(temp->name);
            if (temp->content != NULL) {
                free(temp->content);
            }
            free(upper->dirents[i]);
            location = i;
            break;
        }
    }
    for (int i = location; i < upper->nrde - 1; ++i) {//move
        upper->dirents[i] = upper->dirents[i + 1];
    }
    upper->nrde--;
    return 0;
}

void init_ramfs() {
    //root
    node *temp = malloc(sizeof(node));
    if (temp == NULL) return;

    temp->type = DNODE;
    temp->dirents = NULL;
    temp->content = NULL;
    temp->upper = NULL;
    temp->nrde = 0;
    temp->name = malloc(2 * sizeof(char));
    strcpy(temp->name, "/");
    temp->size = 0;
    root = temp;

    //FD
    for (int i = 0; i < NRFD; ++i) {
        fdesc[i].offset = 0;
        fdesc[i].used = false;
        fdesc[i].f = NULL;
        fdesc[i].flags = 0;
    }
}

void free_node(node *nd) {
    if (nd == NULL)return;

    if (nd->type == FNODE) {
        if (nd->content != NULL)free(nd->content);
        free(nd->name);
        free(nd);
    } else {
        for (int i = 0; i < nd->nrde; ++i) {
            free_node(nd->dirents[i]);
        }
        free(nd->name);
        free(nd);
    }
}

void close_ramfs() {
    if (root == NULL)return;
    free_node(root);
    root = NULL;
}