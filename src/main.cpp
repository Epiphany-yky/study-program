#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <csignal>
#include <sys/select.h>
#include <errno.h>
#include <cstring>
#include "thread_pool.h"

#define LOG_INFO(fmt, ...)  printf("[INFO] %s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] %s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

bool g_running = true;

void sig_handler(int sig) {
    LOG_INFO("收到退出信号，正在关闭服务器...");
    g_running = false;
}

int main() {
    signal(SIGINT, sig_handler);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        LOG_ERROR("创建socket失败: %s", strerror(errno));
        return 1;
    }
    LOG_INFO("socket 创建成功，专属文件描述符 fd = %d", listen_fd);

    int reuse_opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_opt, sizeof(reuse_opt));

    const int PORT = 8090;
    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    int bind_ret = bind(listen_fd, (sockaddr *)&serv_addr, sizeof(serv_addr));
    if (bind_ret < 0) {
        LOG_ERROR("绑定端口失败: %s", strerror(errno));
        close(listen_fd);
        return 1;
    }
    LOG_INFO("端口绑定成功！正在监听端口：%d", PORT);

    int listen_ret = listen(listen_fd, 5);
    if (listen_ret < 0) {
        LOG_ERROR("监听失败: %s", strerror(errno));
        close(listen_fd);
        return 1;
    }
    LOG_INFO("服务器已启动，正在等待客户端连接...");

    ThreadPool pool(8);

    while(g_running)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(listen_fd, &read_fds);

        struct timespec timeout = {.tv_sec = 1, .tv_nsec = 0};

        int ret = pselect(listen_fd + 1, &read_fds, NULL, NULL, &timeout, NULL);

        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("pselect失败: %s", strerror(errno));
            break;
        } else if (ret == 0) {
            continue;
        }

        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (sockaddr *)&client_addr, &client_len);

        if (client_fd < 0) {
            LOG_ERROR("接受客户端连接失败: %s", strerror(errno));
            continue;
        }

        LOG_INFO("新客户端连接成功！客户端fd = %d", client_fd);

        pool.add_task([client_fd](){
            // 1. 先读取客户端发来的 HTTP 请求（虽然我们不解析，但必须读出来，否则客户端会卡住）
            char buf[1024] = {0};
            read(client_fd, buf, sizeof(buf) - 1);
            LOG_INFO("收到 HTTP 请求，客户端fd = %d", client_fd);

            // 2. 构造标准的 HTTP 响应
            const char *http_response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html; charset=utf-8\r\n"
                "Connection: close\r\n"
                "\r\n"
                "<html>"
                "<head><title>我的多线程 Web 服务器</title></head>"
                "<body>"
                "<h1>🎉 恭喜！你的多线程 Web 服务器跑通了！</h1>"
                "<p>这是一个由 C++ 线程池驱动的高并发服务器。</p>"
                "</body>"
                "</html>";

            // 3. 发送 HTTP 响应
            ssize_t send_n = write(client_fd, http_response, strlen(http_response));
            if (send_n < 0) {
                LOG_ERROR("发送 HTTP 响应失败: %s", strerror(errno));
            } else {
                LOG_INFO("已成功发送 HTTP 响应（%zd 字节），客户端fd = %d", send_n, client_fd);
            }

            // 4. 关闭连接
            close(client_fd);
            LOG_INFO("客户端连接已关闭，客户端fd = %d", client_fd);
        });
    }

    close(listen_fd);
    LOG_INFO("服务器已正常关闭");
    return 0;
}