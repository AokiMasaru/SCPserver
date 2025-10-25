# SCPserver — C++ SCP server template

このリポジトリは C++ で SCP サーバを実装するためのプロジェクトテンプレートです。

主な点:
- CMake ビルド（C++17）
- オプションで libssh を使用して本格的な SSH/SCP を実装可能（libssh が見つからない場合は TCP のフォールバックスケルトンを起動します）

依存関係（Debian/Ubuntu の例）:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libssl-dev libssh-dev
依存関係（Debian/Ubuntu の例）:
```bash
# 省略可: リポジトリ付属のスクリプトで一括インストール
./scripts/install-deps-debian.sh
```
mkdir -p build
cmake -S . -B build -DUSE_LIBSSH=ON
cmake --build build -- -j
```

テスト（簡易）:
ビルド手順:
```bash
mkdir -p build
cmake -S . -B build -DUSE_LIBSSH=ON
cmake --build build -- -j$(nproc)
```

```bash
./build/scpserver 2222
# Fallback 実装では接続を受けるだけの簡易サーバです。
```

次のステップ:
- libssh の API を用いて SSH バインド（ssh_bind）の初期化、鍵の読み込み、セッション受け入れ、SCP サブシステムのハンドリングを実装してください。

Windows 用の依存インストール（補足）:

このリポジトリは主に Linux を想定していますが、Windows でもビルド可能です。推奨は `vcpkg` を使う方法です（Visual Studio または Ninja + MSVC を使用）。代替として MSYS2 環境でのビルド手順も記載します。

- 推奨: vcpkg を使う（Visual Studio / MSVC 向け）

```powershell
# 1) vcpkg を取得
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 2) 必要なライブラリをインストール（x64 例）
.\vcpkg install libssh:x64-windows openssl:x64-windows

# 3) CMake を vcpkg ツールチェーン経由で呼び出してビルド
cd ..\SCPserver
mkdir build
cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake -DUSE_LIBSSH=ON
cmake --build build --config Release
```

注: `-A x64` や `--config Release` は Visual Studio のジェネレータを利用する場合の例です。Ninja を使う場合は `-G Ninja` を併用してください。

- 代替: MSYS2 環境を使う（MinGW-w64）

```sh
# MSYS2 をインストールして MSYS2 MinGW 64-bit シェルを開く
# パッケージ更新
pacman -Syu
# 必要パッケージ
pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-libssh mingw-w64-x86_64-openssl mingw-w64-x86_64-cmake pkg-config

# ビルド（MSYS2 MinGW 64-bit シェル内で）
mkdir -p build
cmake -S . -B build -G "MSYS Makefiles" -DUSE_LIBSSH=ON
cmake --build build -- -j$(nproc)
```

注意点:
- Windows 環境ではライブラリの名前やアーキテクチャ（x86/x64）に注意してください。
- `vcpkg` を使う場合は `-DCMAKE_TOOLCHAIN_FILE` を指定することで CMake が自動的に vcpkg のライブラリを検出します。
- 管理者権限やパスの設定が必要になる場合があります。

