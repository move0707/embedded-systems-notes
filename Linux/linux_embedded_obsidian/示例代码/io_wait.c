/*
 * 学习示例：阻塞 read / 非阻塞 read / poll
 * 根据本次对话中提供的 io_wait.c 整理；纠正误导性注释并补充零字节分支。
 * 运行时需要一个参数：block 或 nonblock。
 * 在交互式终端中通常要输入一行并按回车，标准输入才可读。
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char buffer[128];
    ssize_t n;

    if (argc != 2) {
        printf("Usage: %s block|nonblock\n", argv[0]);
        return 1;
    }

    // 读取标准输入 fd=0 的原有文件状态标志；fd=0 只是编号，不存放输入数据
    int flags = fcntl(STDIN_FILENO, F_GETFL);
    if (flags == -1) {
        perror("fcntl");
        return 1;
    }

    if (strcmp(argv[1], "block") == 0) {
        printf("阻塞模式：等待输入数据...\n");
        fflush(stdout);

        // 终端无可读的一整行时，阻塞模式下当前线程通常会等待
        n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("收到：%s", buffer);
        } else if (n == 0) {
            printf("输入结束\n");
        } else {
            perror("read");
        }

    } else if (strcmp(argv[1], "nonblock") == 0) {
        // 让 read() 在暂时无数据时直接返回；并非跳过 read() 调用
        if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl");
            return 1;
        }

        printf("非阻塞模式：立即尝试读取...\n");
        n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            printf("没有数据，read立即返回\n");
        } else if (n > 0) {
            buffer[n] = '\0';
            printf("收到：%s", buffer);
        } else if (n == 0) {
            printf("输入结束\n");
        } else {
            perror("read");
        }

        // pfd 只存放监听的 fd、关心的事件和返回的事件标志，不存放 Hello 等原始数据
        struct pollfd pfd = {
            .fd = STDIN_FILENO, // 0：标准输入编号；当前终端中通常关联键盘输入
            .events = POLLIN   // 希望监听“可读”事件，不代表现在一定可读
        };

        printf("poll等待输入，最多5秒...\n");
        fflush(stdout);

        // Linux 内核负责等待；数据存在终端输入缓冲区，不存在 pfd 里
        int ret = poll(&pfd, 1, 5000);
        if (ret > 0 && (pfd.revents & POLLIN)) {
            n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                printf("收到：%s", buffer);
            } else if (n == 0) {
                printf("输入结束\n");
            } else {
                perror("read");
            }
        } else if (ret == 0) {
            printf("等待超时，没有收到数据\n");
        } else if (ret == -1) {
            perror("poll");
        } else {
            printf("输入设备发生其他事件\n");
        }

        // 恢复此前的状态标志
        if (fcntl(STDIN_FILENO, F_SETFL, flags) == -1) {
            perror("restore fcntl");
            return 1;
        }
    } else {
        printf("参数必须是 block 或 nonblock\n");
        return 1;
    }

    return 0;
}
