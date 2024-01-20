#include "ramfs.h"
#include "shell.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

const char *content = "export PATH=/usr/bin/\n";
const char *ct = "export PATH=/home:$PATH";
int main() {
    init_ramfs();

    assert(rmkdir("/home") == 0);
    assert(rmkdir("//home") == -1);
    assert(rmkdir("/test/1") == -1);
    assert(rmkdir("/home/ubuntu") == 0);
    assert(rmkdir("/usr") == 0);
    assert(rmkdir("/usr/bin") == 0);
    assert(rmkdir("/usr/bin/1") == 0);
    assert(rmkdir("/usr/bin/1/1") == 0);
    assert(rmkdir("/usr/bin/1/1/1") == 0);
    assert(rmkdir("/usr/bin/1/1/1/1") == 0);
    assert(rmkdir("/usr/bin/1/1/1/2") == 0);
    assert(rmkdir("/usr/bin/1/1/1/3") == 0);
    assert(rmkdir("/usr/bin/1/1/1/4") == 0);
    assert(rmkdir("/usr/bin////1/1/1/5/") == 0);
    assert(rmkdir("/usr/bin/1/1/1/6") == 0);
    assert(rmkdir("/usr/bin/1/1/1/7") == 0);
    assert(rmkdir("/usr/bin/1/1/1////7") == -1);
    assert(rmkdir("/usr/bin/1/1/1/3/qdwadsd/") == 0);
    assert(rmkdir("/usr/bin/1/1/1/1") == -1);
    assert(rmkdir("/usr/bin/1/1/11/1") == -1);
    assert(rmkdir("/usr/bin/1/1/21/1") == -1);
    assert(rmkdir("/usr/bin/1/1/31/1") == -1);
    assert(rmkdir("/usr/bin/1/1/41/1") == -1);
    assert(rmkdir("/usr/bin/1/1/51/1") == -1);
    assert(rmkdir("/usr/bin/1/2/61/1") == -1);
    assert(rmkdir("/usr/bin/1/2/2") == -1);
    assert(rmkdir("/usr/bin/1") == -1);
    assert(rmkdir("/usr/bin/1/1/2") == 0);
    assert(rwrite(ropen("/home///ubuntu//.bashrc", O_CREAT | O_WRONLY), content, strlen(content)) == strlen(content));

    int fds = ropen("/home///ubuntu/", O_CREAT | O_WRONLY);
    assert(rclose(fds) == -1);
    fds = ropen("/usr/bin/1/1/1/3/qdwads", O_CREAT | O_WRONLY);
    assert(rclose(fds) == 0);
    fds = ropen("/usr/bin/1/1/1/3/qdwadsd/sojfasf", O_CREAT | O_RDONLY);
    assert(rclose(fds) == 0);
    fds = ropen("/usr/bin/1/1/1/1", O_RDONLY);
    assert(rclose(fds) == 0);

    int fd = ropen("/home/ubuntu/.bashrc", O_RDONLY);
    char buf[105] = {0};

    assert(rread(fd, buf, 100) == strlen(content));
    assert(!strcmp(buf, content));
    assert(rwrite(ropen("/home////ubuntu//.bashrc", O_WRONLY | O_APPEND), ct, strlen(ct)) == strlen(ct));
    memset(buf, 0, sizeof(buf));
    assert(rread(fd, buf, 100) == strlen(ct));
    assert(!strcmp(buf, ct));
    assert(rseek(fd, 0, SEEK_SET) == 0);
    memset(buf, 0, sizeof(buf));
    assert(rread(fd, buf, 100) == strlen(content) + strlen(ct));
    char ans[205] = {0};
    strcat(ans, content);
    strcat(ans, ct);
    assert(!strcmp(buf, ans));

    init_shell();

    assert(scat("/home/ubuntu/.bashrc") == 0);
    assert(stouch("/home/ls") == 0);
    assert(stouch("/home///ls/") == 0);
    assert(smkdir("/home///ls/") == 1);
    assert(smkdir("/home///ls/s") == 1);
    assert(smkdir("/home///l/") == 0);
    assert(smkdir("/home///l/sss") == 0);
    assert(swhich("ls") == 0);
    assert(stouch("/usr/bin/ls") == 0);
    assert(swhich("ls") == 0);
    assert(secho("hello world\\n") == 0);
    assert(secho("\\$PATH is $PATH") == 0);
    assert(sls("/usr/bin/1/1/1/3/qdwadsd/sojfasf") == 0);

    close_shell();
    close_ramfs();
}
