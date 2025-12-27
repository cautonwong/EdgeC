#!/usr/bin/env bash
set -e

ROOT=$(pwd)
TP=third_party
BUILD=build
INSTALL_BASE=".local"

mkdir -p "$TP" "$BUILD"

# --------------------------------------------------
# 工具函数
# --------------------------------------------------
get_latest_release_tag() {
  local repo=$1
  local tag=$(curl -s "https://api.github.com/repos/$repo/releases/latest" | jq -r '.tag_name')
  if [[ -z "$tag" || "$tag" == "null" ]]; then
    tag=$(curl -s "https://api.github.com/repos/$repo/tags" | jq -r '.[0].name')
  fi
  echo "$tag"
}

add_submodule_release() {
  local name=$1
  local repo=$2

  echo "==> $name"
  if [ -f .gitmodules ] && grep -q "path = $TP/$name" .gitmodules; then
    echo "    ✅ already in .gitmodules"
    return
  fi

  tag=$(get_latest_release_tag "$repo")

  if [[ -z "$tag" || "$tag" == "null" ]]; then
    echo "    ❌ no release or tag found"
    exit 1
  fi

  git submodule add --depth 1 "https://github.com/$repo.git" "$TP/$name"
  (cd "$TP/$name" && git fetch --tags && git checkout "$tag")
  echo "    ✅ $tag"
}

# --------------------------------------------------
# 1. Git submodules (latest release)
# --------------------------------------------------
add_submodule_release cmocka     coreboot/cmocka
add_submodule_release libuv      libuv/libuv
add_submodule_release libmodbus  stephane/libmodbus
add_submodule_release cJSON      DaveGamble/cJSON

# --------------------------------------------------
# 2. SQLite amalgamation
# --------------------------------------------------
echo "==> SQLite amalgamation"

SQLITE_DIR="$TP/sqlite"
mkdir -p "$SQLITE_DIR"
TMP=$(mktemp -d)

# Fetch the relative path of the amalgamation zip
REL_PATH=$(curl -s https://www.sqlite.org/download.html | grep -oE '[0-9]+/sqlite-amalgamation-[0-9]+\.zip' | head -n1)
ZIP=$(basename "$REL_PATH")
CODE=$(echo "$ZIP" | grep -oE '[0-9]{7}' | head -n1)

curl -fL "https://www.sqlite.org/$REL_PATH" -o "$TMP/$ZIP"
unzip -q "$TMP/$ZIP" -d "$TMP"

cp "$TMP/sqlite-amalgamation-${CODE}/sqlite3.c" "$SQLITE_DIR"
cp "$TMP/sqlite-amalgamation-${CODE}/sqlite3.h" "$SQLITE_DIR"

cat > "$SQLITE_DIR/VERSION.txt" <<EOF
SQLite Code $CODE
EOF

rm -rf "$TMP"

# --------------------------------------------------
# 3. Build configs
# --------------------------------------------------
build_target() {
  local arch=$1
  local cc=$2
  local prefix="$INSTALL_BASE/$arch"

  echo "========================================"
  echo "Building for $arch"
  echo "Install prefix: $prefix"
  echo "========================================"

  mkdir -p "$BUILD/$arch"
  mkdir -p "$prefix"

  export CC=$cc
  export CFLAGS="-O2 -fPIC"
  export LDFLAGS=""

  # ---------- SQLite ----------
  echo "==> sqlite"
  mkdir -p "$BUILD/$arch/sqlite"
  $CC -c "$SQLITE_DIR/sqlite3.c" -o "$BUILD/$arch/sqlite/sqlite3.o"
  ar rcs "$BUILD/$arch/sqlite/libsqlite3.a" "$BUILD/$arch/sqlite/sqlite3.o"
  install -Dm644 "$BUILD/$arch/sqlite/libsqlite3.a" "$prefix/lib/libsqlite3.a"
  install -Dm644 "$SQLITE_DIR/sqlite3.h" "$prefix/include/sqlite3.h"

  # ---------- cJSON ----------
  echo "==> cJSON"
  cmake -S "$TP/cJSON" -B "$BUILD/$arch/cjson" \
    -DCMAKE_C_COMPILER=$cc \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DBUILD_SHARED_LIBS=OFF
  cmake --build "$BUILD/$arch/cjson"
  cmake --install "$BUILD/$arch/cjson"

  # ---------- libuv ----------
  echo "==> libuv"
  cmake -S "$TP/libuv" -B "$BUILD/$arch/libuv" \
    -DCMAKE_C_COMPILER=$cc \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DBUILD_SHARED_LIBS=OFF
  cmake --build "$BUILD/$arch/libuv"
  cmake --install "$BUILD/$arch/libuv"

  # ---------- libmodbus ----------
  echo "==> libmodbus"

  # 确保 configure 存在
  if [ ! -f "$TP/libmodbus/configure" ]; then
    (cd "$TP/libmodbus" && ./autogen.sh)
  fi

  BUILD_TRIPLET=$(gcc -dumpmachine)
  HOST_TRIPLET=$($cc -dumpmachine)

  CONFIGURE_OPTS=(
    CC=$cc
    --prefix="$prefix"
    --enable-static
    --disable-shared
  )

  # 只有交叉编译才加 --host
  if [[ "$BUILD_TRIPLET" != "$HOST_TRIPLET" ]]; then
    CONFIGURE_OPTS+=(--host="$HOST_TRIPLET")

    # 可选：消除运行期测试
    CONFIGURE_OPTS+=(
      ac_cv_func_malloc_0_nonnull=yes
      ac_cv_func_realloc_0_nonnull=yes
    )
  fi

  mkdir -p "$BUILD/$arch/libmodbus"
  (
    cd "$BUILD/$arch/libmodbus"
    "$ROOT/$TP/libmodbus/configure" "${CONFIGURE_OPTS[@]}"
  )

  make -C "$BUILD/$arch/libmodbus"
  make -C "$BUILD/$arch/libmodbus" install

  # ---------- cmocka ----------
  echo "==> cmocka"
  cmake -S "$TP/cmocka" -B "$BUILD/$arch/cmocka" \
    -DCMAKE_C_COMPILER=$cc \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DBUILD_SHARED_LIBS=OFF
  cmake --build "$BUILD/$arch/cmocka"
  cmake --install "$BUILD/$arch/cmocka"
}

# --------------------------------------------------
# 4. Build both targets
# --------------------------------------------------
build_target linux-x86_64   gcc
build_target linux-aarch64  aarch64-linux-gnu-gcc

echo
echo "✅ ALL DONE"
echo "Installed under:"
echo "  ~/.local/linux-x86_64"
echo "  ~/.local/linux-aarch64"
