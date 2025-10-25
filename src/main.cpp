#include "scp_server.hpp"

#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

static std::unique_ptr<scp::SCPServer> g_server;

// SIGINT (Ctrl-C) ハンドラ: サーバにシャットダウンを要求する
void handle_sigint(int) {
    if (g_server) {
        std::cerr << "シャットダウン中...\n";
        g_server->shutdown();
    }
}

int main(int argc, char** argv) {
    unsigned short port = 2222;
    std::string host_key;
    bool allow_password = false;

    // 引数: [port] [host_key] [allow_password]
    if (argc > 1) port = static_cast<unsigned short>(std::stoi(argv[1]));
    if (argc > 2) host_key = argv[2];
    if (argc > 3) allow_password = (std::string(argv[3]) == "1" || std::string(argv[3]) == "true");

    g_server = std::make_unique<scp::SCPServer>(port, host_key, allow_password);

    std::signal(SIGINT, handle_sigint);

    std::cout << "SCPserver テンプレートをポート " << port << " で起動します\n";
    if (!host_key.empty()) std::cout << "ホスト鍵: " << host_key << " を使用します\n";
    g_server->run();

    return 0;
}
