/*
 * 文件 IO 复习示例：与之前练习的知识点对应。
 * 注意：这是单独整理的复习示例，并不声称与你电脑里的 file_io.c 逐字相同。
 * 功能：追加日志，然后重新打开并读出全文。
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    const char *path = "device.log";
    const char msg[] = "Device started\n";
    char buffer[128];

    // 只写：不存在就创建；存在就追加到末尾
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open for write");
        return 1;
    }

    // write() 返回实际写入字节数，生产代码要考虑部分写入
    ssize_t written = write(fd, msg, strlen(msg));
    if (written == -1 || (size_t)written != strlen(msg)) {
        if (written == -1) perror("write");
        else fprintf(stderr, "write: incomplete write\n");
        close(fd);
        return 1;
    }

    if (close(fd) == -1) {
        perror("close after write");
        return 1;
    }

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open for read");
        return 1;
    }

    ssize_t n;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        // 使用 fwrite() 按长度输出字节，不假设 read() 自带 '\0'
        if (fwrite(buffer, 1, (size_t)n, stdout) != (size_t)n) {
            perror("fwrite");
            close(fd);
            return 1;
        }
    }
    if (n == -1) perror("read");
    if (close(fd) == -1) perror("close after read");

    return (n == -1) ? 1 : 0;
}
