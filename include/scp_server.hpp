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

    // パラメータ設定メソッド
    void set_port(unsigned short port) { port_ = port; }
    void set_host_key(const std::string& key) { host_key_ = key; }
    void set_allow_password(bool allow) { allow_password_ = allow; }

    // パラメータ取得メソッド
    unsigned short get_port() const { return port_; }
    const std::string& get_host_key() const { return host_key_; }
    bool get_allow_password() const { return allow_password_; }

private:
    unsigned short port_;
    bool running_ = false;
    std::string host_key_;
    bool allow_password_ = false;
};

} // namespace scp
