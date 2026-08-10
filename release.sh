#!/usr/bin/env bash
#
# Cuts a new release: bumps the version everywhere it's tracked (meson.build,
# .env, the AppStream changelog + pinned screenshot URL), commits, and tags.
# Pushing is opt-in via --push so a bare run is fully local and reversible.
#
# Usage:
#   ./release.sh <version> "<changelog description>" [--push]
#
# Example:
#   ./release.sh 0.1.12-Beta "Fix X and Y" --push

set -euo pipefail

log() { printf '\n\033[1m==> %s\033[0m\n' "$1"; }
die() { printf 'error: %s\n' "$1" >&2; exit 1; }

[ -f meson.build ] || die "run this from the repo root (meson.build not found)"

VERSION="${1:-}"
DESCRIPTION="${2:-}"
PUSH=0
for arg in "${@:3}"; do
  case "$arg" in
    --push) PUSH=1 ;;
    *) die "unknown argument: $arg" ;;
  esac
done

[ -n "$VERSION" ] || die "usage: ./release.sh <version> \"<changelog description>\" [--push]"
[ -n "$DESCRIPTION" ] || die "a changelog description is required"

TAG="v${VERSION%%-*}"
METAINFO="data/io.github.chiddekel.Notepad.metainfo.xml.in"
TODAY="$(date +%F)"

git diff --cached --quiet || die "there are already staged changes -- commit or unstage them first"
git rev-parse "$TAG" >/dev/null 2>&1 && die "tag $TAG already exists locally"
git ls-remote --exit-code --tags origin "$TAG" >/dev/null 2>&1 && die "tag $TAG already exists on origin"

OLD_TAG="v$(sed -n "s/^VERSION=\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/p" .env)"

log "Bumping to $VERSION (tag $TAG)"

sed -i "s/^VERSION=.*/VERSION=${VERSION}/" .env

sed -i "0,/^  version: '.*',$/s//  version: '${VERSION}',/" meson.build

sed -i "s#/gnome-notepad/${OLD_TAG}/#/gnome-notepad/${TAG}/#" "$METAINFO"

python3 - "$METAINFO" "$VERSION" "$TODAY" "$DESCRIPTION" <<'PYEOF'
import sys

path, version, today, description = sys.argv[1:5]
with open(path, encoding="utf-8") as f:
    content = f.read()

marker = "  <releases>\n"
idx = content.index(marker) + len(marker)
entry = (
    f'    <release version="{version}" date="{today}">\n'
    f"      <description>\n"
    f"        <p>{description}</p>\n"
    f"      </description>\n"
    f"    </release>\n"
)
content = content[:idx] + entry + content[idx:]

with open(path, "w", encoding="utf-8") as f:
    f.write(content)
PYEOF

git add .env meson.build "$METAINFO"
git commit -m "Bump to ${VERSION}: ${DESCRIPTION}"
git tag "$TAG"

log "Committed and tagged $TAG"
echo "Files changed: .env, meson.build, $METAINFO"

if [ "$PUSH" -eq 1 ]; then
  log "Pushing"
  git push origin main
  git push origin "$TAG"
else
  log "Not pushed (pass --push to also push)"
  echo "  git push origin main"
  echo "  git push origin $TAG"
fi
