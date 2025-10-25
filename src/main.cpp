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
    if (argc > 1) port = static_cast<unsigned short>(std::stoi(argv[1]));

    g_server = std::make_unique<scp::SCPServer>(port);

    std::signal(SIGINT, handle_sigint);

    std::cout << "SCPserver テンプレートをポート " << port << " で起動します\n";
    g_server->run();

    return 0;
}
