#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // 设置 LD_PRELOAD 环境变量，预加载 /usr/lib/aarch64-linux-gnu/mmap_redirect.so
    setenv("LD_PRELOAD", "/usr/lib/aarch64-linux-gnu/libmmap_hook.so", 1);

    // 调用 exec，使用提供的参数
    if (argc > 1) {
        execvp(argv[1], argv + 1);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return 0;
}