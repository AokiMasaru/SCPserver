# テストのビルド設定
enable_testing()

# Google Test の検出と設定
find_package(GTest REQUIRED)
include(GoogleTest)

# テストソースのコンパイル設定
add_executable(test_scp_server
    test_scp_server.cpp
)

# テストターゲットのリンク設定
target_link_libraries(test_scp_server
    PRIVATE
    scpserver_lib
    GTest::GTest
    GTest::Main
    ssh
)

# テストケースの登録
gtest_discover_tests(test_scp_server)

# テストデータディレクトリの作成
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)

# テストキーの生成スクリプトをコピー
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_test_keys.sh
    ${CMAKE_CURRENT_BINARY_DIR}/generate_test_keys.sh
    COPYONLY
)