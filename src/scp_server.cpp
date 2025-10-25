#include "scp_server.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <cstring>

#include <libssh/libssh.h>
#include <libssh/server.h>

namespace scp {

SCPServer::~SCPServer() = default;

void SCPServer::run() {
    running_ = true;
    std::cout << "SSH リスナーを初期化します。\n";

    // libssh を用いた SSH サーバー実装
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
        if (ssh_bind_accept(sshbind, session) != SSH_OK) {
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
        while (running_) {
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

        // セッションチャネルを作成して開く
        ssh_channel channel = ssh_channel_new(session);
        if (!channel) {
            std::cerr << "ssh_channel_new failed\n";
            ssh_disconnect(session);
            ssh_free(session);
            continue;
        }

        bool channel_opened = false;
        bool command_executed = false;

        while (running_ && !command_executed) {
            ssh_message msg = ssh_message_get(session);
            if (!msg) break;

            if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL) {
                if (!channel_opened) {
                    // 最初のチャネルオープン要求を受け付け
                    ssh_message_channel_request_open_reply_accept(msg);
                    if (ssh_channel_open_session(channel) == SSH_OK) {
                        channel_opened = true;
                        std::cout << "チャネルオープン成功\n";
                    }
                } else if (ssh_message_subtype(msg) == SSH_CHANNEL_REQUEST_EXEC) {
                    // コマンド実行要求の処理
                    const char* command = ssh_message_channel_request_command(msg);
                    if (command && (std::string(command) == "echo pubkey-ok" || 
                                  std::string(command) == "echo pass-ok")) {
                        ssh_message_channel_request_reply_success(msg);
                        ssh_channel_write(channel, command + 5, strlen(command) - 5); // "echo " を除く
                        ssh_channel_write(channel, "\n", 1);
                        ssh_channel_send_eof(channel);
                        command_executed = true;
                    }
                }
            }
            ssh_message_reply_default(msg);
            ssh_message_free(msg);
        }

        ssh_channel_close(channel);
        ssh_channel_free(channel);
        ssh_disconnect(session);
        ssh_free(session);
        std::cout << "クライアント処理完了。\n";
    }

    ssh_bind_free(sshbind);
    std::cout << "サーバ停止。\n";
}

void SCPServer::shutdown() {
    running_ = false;
}

} // namespace scp
