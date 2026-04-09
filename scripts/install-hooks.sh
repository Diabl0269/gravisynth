#!/bin/bash
set -e

HOOK_DIR="$(git rev-parse --show-toplevel)/.git/hooks"
SCRIPT_DIR="$(git rev-parse --show-toplevel)/scripts"

# Handle worktrees — .git may be a file pointing to the real git dir
GIT_PATH="$(git rev-parse --show-toplevel)/.git"
if [ -f "$GIT_PATH" ]; then
    HOOK_DIR="$(cat "$GIT_PATH" | sed 's/gitdir: //')/hooks"
fi

mkdir -p "$HOOK_DIR"

cat > "$HOOK_DIR/pre-push" << 'EOF'
#!/bin/bash
exec "$(git rev-parse --show-toplevel)/scripts/pre-push-release-test.sh"
EOF

chmod +x "$HOOK_DIR/pre-push"

echo "Installed pre-push hook."
