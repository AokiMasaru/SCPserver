## 目的
このリポジトリは C++ による SCP サーバ実装のためのテンプレートです。AI エージェントはまずここを読み、ビルド／実装箇所／注意点を把握してから作業に入ってください。

## 必読ファイル（まずこれを読む）
- `CMakeLists.txt` — ビルド設定。libssh は必須です。
- `src/` — 実装（`src/main.cpp`, `src/scp_server.cpp`）。
- `include/` — 公開ヘッダ（`include/scp_server.hpp`）。
- `README.md` — 依存とビルド手順の短いまとめ。

## すぐにやること（環境準備とビルド）
1. 依存をインストール（Debian/Ubuntu の例）:

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libssl-dev libssh-dev
```

2. ビルド手順:

```bash
mkdir -p build
cmake -S . -B build
cmake --build build -- -j
```

3. 実行例:

```bash
./build/scpserver 2222
```

## 実装上の重要ポイント（このリポジトリ固有）
- SCP サーバは libssh を使って SSH セッションとチャネルを処理します。
- libssh 実装での主な手順: `ssh_bind` の作成→鍵オプション設定→`ssh_bind_listen`→接続受け入れ→`ssh_new`/`ssh_handle_key_exchange`→認証→`ssh_channel_open_session`→SCP サブシステムの開始／ハンドリング。
- セキュリティ: 認証、鍵管理、権限制御は慎重に。テスト用の鍵以外をコミットしないでください。

## テストと CI
- テストは `ctest` (バージョン 3.22.1以上) を使用します。以下のような項目をテストに含めてください：
  - ユニットテスト: libssh セッション管理、認証処理
  - 統合テスト: SCP コマンドの実行、ファイル転送
  - セキュリティテスト: 不正な認証試行、権限チェック
- テストの実行方法:
  ```bash
  cd build && ctest --output-on-failure
  ```
- 新しいテストを追加する場合は、`tests/` ディレクトリ以下に配置し、`CMakeLists.txt` に追加してください。
- CI を追加する場合は、ビルドステップに `cmake -S . -B build` と `cmake --build build -- -j`、そしてテストステップとして `ctest --test-dir build --output-on-failure` を入れてください。

## 変更・PR の方針（短く実務的）
- 小さな論理単位で PR を作る。必ずビルドを通す（`cmake`→`cmake --build`）。
- 新しい依存を追加する場合は `README.md` に記載する。

### コードコメントの言語
- このリポジトリに追加するソースコード内のコメントは、常に日本語で記述してください（説明・TODO・注釈などすべて）。
	エージェントが生成・編集するコメントも同様です。

### コミットメッセージの言語
- このリポジトリへのコミットメッセージは日本語で記述してください。短く要点をまとめた日本語の一行目（概要）と、必要に応じて空行の後に詳細を日本語で追記するスタイルを推奨します。

例:

```
feat: libssh 初期バインド処理を追加

libssh による ssh_bind の初期化と簡易ハンドラを追加しました。\
この変更は開発用の最小実装であり、本番向けの認証強化は別 PR で対応します。
```

## 追加作業の提案（必要なら私がやります）
- SCP プロトコルハンドリングの改善を実装できます。
- 開発用 Dockerfile と GitHub Actions のワークフローを作成できます。

不足・追記したい箇所があれば、対象ファイル名と具体的要望を教えてください。
