# SCPserver — C++ SCP server template

このリポジトリは C++ で SCP サーバを実装するためのプロジェクトテンプレートです。

主な点:
- CMake ビルド（C++17）
- オプションで libssh を使用して本格的な SSH/SCP を実装可能（libssh が見つからない場合は TCP のフォールバックスケルトンを起動します）

依存関係（Debian/Ubuntu の例）:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libssl-dev libssh-dev
```

ビルド手順:

```bash
mkdir -p build
cmake -S . -B build -DUSE_LIBSSH=ON
cmake --build build -- -j
```

テスト（簡易）:

```bash
# 今は単体テストは置いていません。将来的に ctest を追加してください。
```

実行例:

```bash
./build/scpserver 2222
# Fallback 実装では接続を受けるだけの簡易サーバです。
```

次のステップ:
- libssh の API を用いて SSH バインド（ssh_bind）の初期化、鍵の読み込み、セッション受け入れ、SCP サブシステムのハンドリングを実装してください。
