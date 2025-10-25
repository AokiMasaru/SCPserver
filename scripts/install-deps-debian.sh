#!/usr/bin/env bash
# Debian/Ubuntu 用の依存自動インストールスクリプト
set -euo pipefail

echo "Updating apt and installing build dependencies (requires sudo)"
sudo apt update
sudo apt install -y build-essential cmake pkg-config libssl-dev libssh-dev netcat

echo "Dependencies installed. You can now run: mkdir -p build && cmake -S . -B build -DUSE_LIBSSH=ON && cmake --build build -- -j$(nproc)"
