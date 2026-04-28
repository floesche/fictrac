#!/bin/sh
set -e

echo
echo "+------------------------------+"
echo "|    FicTrac install (pixi)    |"
echo "+------------------------------+"
echo

cd "$(dirname "$0")"

# 1. Bootstrap pixi if missing (no sudo required; installs to ~/.pixi)
if ! command -v pixi >/dev/null 2>&1; then
    echo "+-- Installing pixi -----------+"
    curl -fsSL https://pixi.sh/install.sh | sh
    export PATH="$HOME/.pixi/bin:$PATH"
    if ! command -v pixi >/dev/null 2>&1; then
        echo "Failed to install pixi. Add ~/.pixi/bin to PATH and retry."
        exit 1
    fi
fi

echo
echo "+-- Updating pixi -------------+"
pixi self-update

# 2. Resolve dependencies into .pixi/envs/default
echo
echo "+-- Resolving dependencies ----+"
pixi install

# 3. Configure + build
echo
echo "+-- Building FicTrac ----------+"
pixi run build

# 4. Smoke check
echo
if [ -x ./build/fictrac ]; then
    echo "FicTrac built successfully -> ./build/fictrac"
else
    echo "Build failed: ./build/fictrac not found"
    exit 1
fi
