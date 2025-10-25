#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PORT=2222
HOST_KEY=tests/host_rsa
USER_KEY=tests/user_rsa
USER=testuser
PASS=testpass

mkdir -p tests

echo "=== テスト鍵の作成（既存がなければ） ==="
if [ ! -f "$HOST_KEY" ]; then
  ssh-keygen -t rsa -f "$HOST_KEY" -N "" -q
fi
if [ ! -f "$USER_KEY" ]; then
  ssh-keygen -t rsa -f "$USER_KEY" -N "" -q
fi

echo "=== サーバをバックグラウンドで起動（libssh 必須） ==="
if [ ! -x build/scpserver ]; then
  echo "ビルドされたバイナリが見つかりません: build/scpserver" >&2
  exit 1
fi

# ホスト鍵とパスワード認証を有効にして起動
stdbuf -oL ./build/scpserver $PORT "$HOST_KEY" 1 > /tmp/scp_integration.log 2>&1 &
PID=$!
trap 'kill $PID || true; wait $PID 2>/dev/null || true' EXIT

sleep 1

echo "=== 公開鍵認証のテスト ==="
PUB_OUT=$(ssh -i "$USER_KEY" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p $PORT $USER@localhost 'echo pubkey-ok' 2>/dev/null || true)
if [ "$PUB_OUT" = "pubkey-ok" ]; then
  echo "公開鍵認証: OK"
else
  echo "公開鍵認証: 失敗（出力: $PUB_OUT）" >&2
fi

echo "=== パスワード認証のテスト ==="
if command -v sshpass >/dev/null 2>&1; then
  PASS_OUT=$(sshpass -p "$PASS" ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p $PORT $USER@localhost 'echo pass-ok' 2>/dev/null || true)
  if [ "$PASS_OUT" = "pass-ok" ]; then
    echo "パスワード認証: OK"
  else
    echo "パスワード認証: 失敗（出力: $PASS_OUT）" >&2
  fi
else
  echo "sshpass が見つかりません。パスワード認証テストをスキップしました。（インストール: sudo apt install sshpass）"
fi

echo "=== テストログ（/tmp/scp_integration.log） ==="
tail -n 200 /tmp/scp_integration.log || true

echo "テスト完了。サーバを停止します。"
kill $PID || true
sleep 0.2
