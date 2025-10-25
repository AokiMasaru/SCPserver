#!/bin/bash

# テスト用のSSH鍵を生成するスクリプト
KEY_DIR="test_data"
mkdir -p "$KEY_DIR"

# サーバーのホスト鍵を生成
ssh-keygen -t rsa -b 2048 -f "$KEY_DIR/ssh_host_rsa_key" -N "" -C "test-host-key"

# クライアントの認証用鍵を生成
ssh-keygen -t rsa -b 2048 -f "$KEY_DIR/test_client_key" -N "" -C "test-client-key"

# 権限を適切に設定
chmod 600 "$KEY_DIR/ssh_host_rsa_key"
chmod 644 "$KEY_DIR/ssh_host_rsa_key.pub"
chmod 600 "$KEY_DIR/test_client_key"
chmod 644 "$KEY_DIR/test_client_key.pub"