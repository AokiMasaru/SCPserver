#include <gtest/gtest.h>
#include "scp_server.hpp"
#include <libssh/libssh.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace {
// テスト用の設定とヘルパー関数
const int TEST_PORT = 22222;
const std::string TEST_DATA_DIR = "test_data";

// テスト用のファイルを作成
void create_test_file(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    f << content;
    f.close();
}

// libsshクライアントのセットアップ
ssh_session setup_ssh_client() {
    ssh_session session = ssh_new();
    EXPECT_NE(session, nullptr);

    ssh_options_set(session, SSH_OPTIONS_HOST, "localhost");
    ssh_options_set(session, SSH_OPTIONS_PORT, &TEST_PORT);
    ssh_options_set(session, SSH_OPTIONS_USER, "testuser");
    
    // 未知のホストを自動的に受け入れる（テスト用）
    int state = ssh_is_server_known(session);
    ssh_write_knownhost(session);

    return session;
}
}  // namespace

class SCPServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // テストデータディレクトリの準備
        std::filesystem::create_directory(TEST_DATA_DIR);
        
        // テスト用のホスト鍵を生成
        std::string key_path = TEST_DATA_DIR + "/ssh_host_rsa_key";
        if (!std::filesystem::exists(key_path)) {
            if (system(("ssh-keygen -t rsa -f " + key_path + " -N \"\"").c_str()) != 0) {
                FAIL() << "ホスト鍵の生成に失敗しました";
            }
        }
        
        // サーバーの起動
        server_ = std::make_unique<scp::SCPServer>();
        server_->set_port(TEST_PORT);
        server_->set_host_key(key_path);
        server_->set_allow_password(true);

        server_thread_ = std::thread([this]() {
            server_->run();
        });

        // サーバーの起動を待つ
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        if (server_) {
            server_->shutdown();
        }
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }

    std::unique_ptr<scp::SCPServer> server_;
    std::thread server_thread_;
};

// 基本的な接続テスト（30秒でタイムアウト）
TEST_F(SCPServerTest, BasicConnection) {
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(30);
    
    ssh_session session = setup_ssh_client();
    ASSERT_NE(session, nullptr);

    int rc = ssh_connect(session);
    EXPECT_EQ(rc, SSH_OK);

    // タイムアウトチェック
    if (std::chrono::steady_clock::now() - start_time > timeout) {
        FAIL() << "テストがタイムアウトしました";
    }

    ssh_disconnect(session);
    ssh_free(session);
}

// パスワード認証テスト（30秒でタイムアウト）
TEST_F(SCPServerTest, PasswordAuth) {
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(30);
    
    ssh_session session = setup_ssh_client();
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(ssh_connect(session), SSH_OK);

    // パスワード認証を試行
    EXPECT_EQ(ssh_userauth_password(session, nullptr, "testpass"), SSH_AUTH_SUCCESS);

    // タイムアウトチェック
    if (std::chrono::steady_clock::now() - start_time > timeout) {
        FAIL() << "テストがタイムアウトしました";
    }

    ssh_disconnect(session);
    ssh_free(session);
}

// 公開鍵認証テスト（30秒でタイムアウト）
TEST_F(SCPServerTest, PublicKeyAuth) {
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::seconds(30);
    
    // クライアント鍵の生成
    std::string key_path = TEST_DATA_DIR + "/test_client_key";
    if (!std::filesystem::exists(key_path)) {
        if (system(("ssh-keygen -t rsa -f " + key_path + " -N \"\"").c_str()) != 0) {
            FAIL() << "クライアント鍵の生成に失敗しました";
        }
    }
    
    ssh_session session = setup_ssh_client();
    ASSERT_NE(session, nullptr);

    ASSERT_EQ(ssh_connect(session), SSH_OK);

    // 公開鍵の読み込みと認証
    ssh_key private_key;
    std::string key_path = TEST_DATA_DIR + std::string("/test_client_key");
    ASSERT_EQ(ssh_pki_import_privkey_file(
        key_path.c_str(),
        nullptr,
        nullptr,
        nullptr,
        &private_key
    ), SSH_OK);    
    
    EXPECT_EQ(ssh_userauth_publickey(session, nullptr, private_key), SSH_AUTH_SUCCESS);

    // タイムアウトチェック
    if (std::chrono::steady_clock::now() - start_time > timeout) {
        FAIL() << "テストがタイムアウトしました";
    }

    ssh_key_free(private_key);
    ssh_disconnect(session);
    ssh_free(session);
}

// セッションチャネルテスト（30秒でタイムアウト）
TEST_F(SCPServerTest, SessionChannel) {
    // Note: GTEST_TIMEOUT環境変数で制御可能
    
    ssh_session session = setup_ssh_client();
    ASSERT_NE(session, nullptr);

    // 接続タイムアウトの設定（5秒）
    int timeout = 5;
    ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &timeout);
    
    ASSERT_EQ(ssh_connect(session), SSH_OK);
    ASSERT_EQ(ssh_userauth_password(session, nullptr, "testpass"), SSH_AUTH_SUCCESS);

    // チャネルの作成とオープン（タイムアウトも設定）
    ssh_channel channel = ssh_channel_new(session);
    ASSERT_NE(channel, nullptr);
    
    // チャネルのブロッキングモードを設定
    ssh_channel_set_blocking(channel, 1);
    
    EXPECT_EQ(ssh_channel_open_session(channel), SSH_OK);

    // テストコマンドの実行
    EXPECT_EQ(ssh_channel_request_exec(channel, "echo pass-ok"), SSH_OK);

    // 結果の読み取り（タイムアウト付き）
    char buffer[256];
    int nbytes = 0;
    auto start_time = std::chrono::steady_clock::now();
    auto timeout_duration = std::chrono::seconds(5);  // 5秒でタイムアウト

    while (nbytes <= 0) {
        nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
        if (nbytes > 0) break;
        
        // タイムアウトチェック
        auto current_time = std::chrono::steady_clock::now();
        if (current_time - start_time > timeout_duration) {
            FAIL() << "チャネルからの読み取りがタイムアウトしました";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ASSERT_GT(nbytes, 0) << "データの読み取りに失敗しました";
    buffer[nbytes] = '\0';
    EXPECT_STREQ(buffer, "pass-ok\n");

    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}