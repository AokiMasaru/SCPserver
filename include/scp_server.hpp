// SCP サーバのインターフェース（プロジェクトテンプレート）
#pragma once

#include <string>

namespace scp {

class SCPServer {
public:
    // コンストラクタ: デフォルトポートは 2222
    explicit SCPServer(unsigned short port = 2222) : port_(port) {}
    ~SCPServer();

    // サーバを起動する（ブロッキング）
    void run();

    // 別スレッドから優雅にシャットダウンを要求する
    void shutdown();

private:
    unsigned short port_;
    bool running_ = false;
};

} // namespace scp
