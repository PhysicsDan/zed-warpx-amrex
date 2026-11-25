#!/bin/sh
# Only needed after editing the grammar in tree-sitter-warpx/ (grammar.js or
# src/scanner.c). Highlighting colours and rules live in languages/warpx/ and
# need no rebuild: use "Rebuild" on the extension in Zed's Extensions page.
#
# 1. regenerate the parser:  cd tree-sitter-warpx && tree-sitter generate --abi 14
# 2. run:                    sh rebuild-grammar.sh
# 3. in Zed's Extensions page, click "Rebuild" on WarpX
set -e
cd "$(dirname "$0")"
root="$(pwd -P)"
url="file://$root/tree-sitter-warpx"

git -C tree-sitter-warpx add -A
if ! git -C tree-sitter-warpx diff --cached --quiet; then
  git -C tree-sitter-warpx -c user.name="WarpX user" -c user.email="warpx@localhost" \
    -c commit.gpgsign=false commit -q --no-verify -m "Update grammar"
fi
rev="$(git -C tree-sitter-warpx rev-parse HEAD)"

# Point extension.toml at the local grammar repository and its last commit
sed -e "s|^repository = .*|repository = \"$url\"|" -e "s|^rev = .*|rev = \"$rev\"|" \
  extension.toml > extension.toml.tmp && mv extension.toml.tmp extension.toml

# Zed refuses to reuse grammars/warpx unless its origin is that same URL
if [ -d grammars/warpx/.git ]; then
  git -C grammars/warpx remote set-url origin "$url"
fi
echo "Grammar at $rev. Now click Rebuild on WarpX in Zed's Extensions page."
