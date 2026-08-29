#!/usr/bin/env bash
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$repo_root"

full=false
if [[ "${1:-}" == "--full" ]]; then full=true; fi

echo "[quality] checking whitespace"
# The override avoids treating a Windows checkout's CRLF worktree as a
# repository-wide change when this script is launched through WSL.
if [[ -n "${GITHUB_BASE_REF:-}" ]]; then
  git -c core.autocrlf=true diff --check "origin/$GITHUB_BASE_REF...HEAD"
elif [[ "${GITHUB_ACTIONS:-}" == "true" ]] && git rev-parse HEAD^ >/dev/null 2>&1; then
  git -c core.autocrlf=true diff --check HEAD^ HEAD
else
  git -c core.autocrlf=true diff --check
fi

echo "[quality] rejecting newly added generated or secret files"
if [[ -n "${GITHUB_BASE_REF:-}" ]]; then
  added_files=$(git diff --name-only --diff-filter=A "origin/$GITHUB_BASE_REF...HEAD")
elif [[ "${GITHUB_ACTIONS:-}" == "true" ]] && git rev-parse HEAD^ >/dev/null 2>&1; then
  added_files=$(git diff --name-only --diff-filter=A HEAD^ HEAD)
else
  added_files=$(git diff --cached --name-only --diff-filter=A)
fi
tracked_bad=$(printf '%s\n' "$added_files" | grep -E '(^|/)(build|build-wsl|build-ci|\.run|vcpkg_installed)/|\.(db|db-shm|db-wal|pyc)$|(^|/)\.env($|\.)' || true)
if [[ -n "$tracked_bad" ]]; then
  printf 'New generated/secret files must not be committed:\n%s\n' "$tracked_bad" >&2
  exit 1
fi

echo "[quality] checking JavaScript syntax"
while IFS= read -r file; do node --check "$file"; done < <(find frontend -maxdepth 1 -name '*.js' -print | sort)

echo "[quality] checking Python syntax"
python3 -m py_compile tests/e2e_test.py tests/load_test.py

echo "[quality] running dependency-free C++ unit tests"
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT
g++ -std=c++20 -Wall -Wextra -Werror -Iservices/ApiGateway/include tests/delivery_quote_test.cpp -o "$tmp_dir/delivery_quote_test"
g++ -std=c++20 -Wall -Wextra -Werror -Iservices/OrderService/include tests/payment_order_status_test.cpp -o "$tmp_dir/payment_order_status_test"
g++ -std=c++20 -Wall -Wextra -Werror -Iservices/ApiGateway/include tests/http_result_test.cpp -o "$tmp_dir/http_result_test"
"$tmp_dir/delivery_quote_test"
"$tmp_dir/payment_order_status_test"
"$tmp_dir/http_result_test"

if [[ "$full" == false ]]; then
  echo "[quality] fast checks passed"
  exit 0
fi

: "${VCPKG_ROOT:?Set VCPKG_ROOT to a bootstrapped vcpkg directory}"
build_dir=${CI_BUILD_DIR:-build-ci}

echo "[build] configuring clean CMake build"
cmake -S . -B "$build_dir" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

echo "[build] compiling all services"
cmake --build "$build_dir" --parallel 2

echo "[test] running CTest"
ctest --test-dir "$build_dir" --output-on-failure
