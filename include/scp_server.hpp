// SCP サーバのインターフェース（プロジェクトテンプレート）
#pragma once

#include <string>

namespace scp {

class SCPServer {
public:
    // コンストラクタ: デフォルトポートは 2222
    // host_key: SSH サーバのホスト鍵ファイルパス（libssh 使用時に必要）
    // allow_password: パスワード認証を受け付けるかどうか
    explicit SCPServer(unsigned short port = 2222, const std::string& host_key = "", bool allow_password = false)
        : port_(port), host_key_(host_key), allow_password_(allow_password) {}
    ~SCPServer();

    // サーバを起動する（ブロッキング）
    void run();

    // 別スレッドから優雅にシャットダウンを要求する
    void shutdown();

private:
    unsigned short port_;
    bool running_ = false;
    std::string host_key_;
    bool allow_password_ = false;
};

} // namespace scp
