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
    std::cout << "libssh サポート有効 — SSH リスナーを初期化します。\n";

    // libssh を用いた最小限の受け入れ実装。
    // - host_key_ にホスト鍵のパスが必要
    // - 公開鍵認証は受け入れる（テスト用に任意の公開鍵を許可）
    // - allow_password_ が true の場合は固定ユーザ/パスワードを受け入れる

    if (host_key_.empty()) {
        std::cerr << "ERROR: libssh を有効にする場合はホスト鍵ファイルのパスを指定してください。\n";
        return;
    }

    ssh_bind sshbind = ssh_bind_new();
    if (!sshbind) {
        std::cerr << "ssh_bind_new failed\n";
        return;
    }

    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDADDR, "0.0.0.0");
    std::string portstr = std::to_string(port_);
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, portstr.c_str());
    // ホスト鍵ファイルを設定
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_HOSTKEY, host_key_.c_str());

    if (ssh_bind_listen(sshbind) != SSH_OK) {
        std::cerr << "ssh_bind_listen failed: " << ssh_get_error(sshbind) << "\n";
        ssh_bind_free(sshbind);
        return;
    }

    std::cout << "libssh リスナーが開始されました。接続待ち...\n";

    while (running_) {
        ssh_session session = ssh_new();
        if (!session) break;

        // 接続受け入れ（ブロック）
        int rc = ssh_bind_accept(sshbind, session);
        if (rc != SSH_OK) {
            std::cerr << "ssh_bind_accept failed: " << ssh_get_error(sshbind) << "\n";
            ssh_free(session);
            continue;
        }

        if (ssh_handle_key_exchange(session) != SSH_OK) {
            std::cerr << "key exchange failed: " << ssh_get_error(session) << "\n";
            ssh_disconnect(session);
            ssh_free(session);
            continue;
        }

        std::cout << "接続確立: クライアント認証を待ちます...\n";

        // 認証処理（簡易）
        bool authenticated = false;
        while (true) {
            ssh_message msg = ssh_message_get(session);
            if (!msg) break;

            if (ssh_message_type(msg) == SSH_REQUEST_AUTH) {
                // パスワード認証
                if (ssh_message_subtype(msg) == SSH_AUTH_METHOD_PASSWORD) {
                    if (allow_password_) {
                        const char* user = ssh_message_auth_user(msg);
                        const char* password = ssh_message_auth_password(msg);
                        // テスト用: ユーザ 'testuser' / パスワード 'testpass' を受け入れる
                        if (user && password && std::string(user) == "testuser" && std::string(password) == "testpass") {
                            ssh_message_auth_reply_success(msg, 0);
                            authenticated = true;
                        } else {
                            ssh_message_auth_set_methods(msg, SSH_AUTH_METHOD_PASSWORD);
                            ssh_message_reply_default(msg);
                        }
                    } else {
                        ssh_message_reply_default(msg);
                    }
                }
                // 公開鍵認証: テスト用に任意の公開鍵を受け入れる
                else if (ssh_message_subtype(msg) == SSH_AUTH_METHOD_PUBLICKEY) {
                    // ここではテストのために公開鍵を受け入れる
                    ssh_message_auth_reply_success(msg, 0);
                    authenticated = true;
                } else {
                    ssh_message_reply_default(msg);
                }
            } else {
                ssh_message_reply_default(msg);
            }
            ssh_message_free(msg);
            if (authenticated) break;
        }

        if (!authenticated) {
            std::cerr << "認証に失敗しました。接続を終了します。\n";
            ssh_disconnect(session);
            ssh_free(session);
            continue;
        }

        std::cout << "認証成功。チャネルを待ちます...\n";

        // 単純にチャネルを受け取って挨拶する（SCP サブシステムの完全実装は別途）
        ssh_channel channel = ssh_channel_new(session);
        if (!channel) {
            std::cerr << "ssh_channel_new failed\n";
            ssh_disconnect(session);
            ssh_free(session);
            continue;
        }

        // ここではセッションを閉じる前に少し待つ
        std::this_thread::sleep_for(std::chrono::seconds(1));

        ssh_channel_free(channel);
        ssh_disconnect(session);
        ssh_free(session);
        std::cout << "クライアント処理完了。\n";
    }

    ssh_bind_free(sshbind);
#endif
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
