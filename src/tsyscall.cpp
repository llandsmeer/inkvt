#include <fcntl.h>
#include "tsyscall.hpp"

int main(int argc, const char ** argv) {
    assert(argc > 1);
    pid_t pid = atoi(argv[1]);
    struct tsyscall t;
    t.begin(pid);
    int fd = 0, flags;
    while ((flags = t.fcntl(fd, F_GETFL, 0)) >= 0) {
        printf("[%d]: %d\n", fd, flags);
        fd += 1;
    }
    t.end();
}
