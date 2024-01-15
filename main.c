#include "ramfs.h"
#include "shell.h"
#include <assert.h>
#include <string.h>

extern node *root;
const char *content = "export PATH=$PATH:/usr/bin/\n";
const char *ct = "export PATH=/home:$PATH";

int main() {
    init_ramfs();

    assert(rmkdir("/home") == 0);
    assert(rmkdir("//home") == -1);
    assert(rmkdir("/test/1") == -1);
    assert(rmkdir("/home/ubuntu") == 0);
    assert(rmkdir("/usr") == 0);
    assert(rmkdir("/usr/bin") == 0);
    assert(ropen("/home///ubuntu//.bashrc", O_CREAT | O_WRONLY) == 0);
    int fd = ropen("/home/ubuntu/.bashrc", O_RDONLY);
    stouch("/home/ubuntu/.bashrc");
    rwrite(ropen("/home/ubuntu/.bashrc", O_WRONLY), content, strlen(content));
    rwrite(ropen("/home/ubuntu/.bashrc", O_WRONLY | O_APPEND), ct, strlen(ct));
    scat("/home/ubuntu/.bashrc");

    init_shell();
    swhich("ls");
    stouch("/usr/bin/ls");
    swhich("ls");
    stouch("/home/ls");
    swhich("ls");
    secho("hello world");
    secho("The Environment Variable PATH is:\\$PATH");
    close_ramfs();
    close_shell();
    assert(root == NULL);
}
