#include "scp_server.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#if defined(HAVE_LIBSSH)
// 注意: 実装時は libssh の API を用いて SSH セッションを受け付ける必要があります。
// ここではプレースホルダとして libssh ヘッダをインクルードしています。
#include <libssh/libssh.h>
#endif

#if !defined(HAVE_LIBSSH)
// フォールバック: シンプルな TCP リスナー（本物の SCP/SSH 実装ではありません）
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#endif

namespace scp {

SCPServer::~SCPServer() = default;

void SCPServer::run() {
    running_ = true;
#if defined(HAVE_LIBSSH)
    std::cout << "libssh サポート有効 — ここで SSH リスナーを初期化します。\n";
    // 本格実装: ssh_bind の作成→オプション設定→listen→accept→セッション処理（SCP サブシステム）
    // このテンプレートは最小限の骨組みのみを示します。libssh-dev をインストールして実装してください。
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
#else
    std::cout << "libssh 非検出 — ポート " << port_ << " で TCP フォールバックを起動します\n";
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) {
        perror("socket");
        return;
    }

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(srv);
        return;
    }

    if (listen(srv, 1) < 0) {
        perror("listen");
        close(srv);
        return;
    }

    std::cout << "接続待ち... (Ctrl-C で停止)\n";
    while (running_) {
        sockaddr_in client{};
        socklen_t clilen = sizeof(client);
        int cl = accept(srv, (sockaddr*)&client, &clilen);
        if (cl < 0) {
            if (running_) perror("accept");
            break;
        }

        char buf[256];
        int n = read(cl, buf, sizeof(buf)-1);
        if (n > 0) {
            buf[n] = '\0';
            std::cout << "受信（フォールバック）: " << buf << "\n";
            const char* msg = "SCP-server-skeleton: received\n";
            write(cl, msg, strlen(msg));
        }
        close(cl);
    }

    close(srv);
#endif
    std::cout << "サーバ停止。\n";
}

void SCPServer::shutdown() {
    running_ = false;
}

} // namespace scp
