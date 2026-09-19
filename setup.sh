#!/usr/bin/env bash
#
# setup.sh — prepare the local environment and build + run the uzu tests.
#
# Mirrors the workflow documented in README.md:
#   * ensures the build prerequisites are present (CMake and a C++20 compiler),
#   * configures the CMake project,
#   * builds everything including the tests,
#   * runs the test suite with ctest.
#
# uzu is header-only and depends on nothing outside the standard library, so there is nothing to
# fetch and nothing to link against. Most of the build time is the test suite: every assertion in
# tests/ is a static_assert, so the compiler evaluates each fit rather than running it.
#
# Configuration via environment variables:
#   BUILD_DIR=build       Build directory.
#   BUILD_TYPE=Release    CMake build type.
#   JOBS=<nproc>          Parallel build jobs.
#   INSTALL_DEPS=auto     auto | yes | no  (system package installation).
#
# Usage:
#   ./setup.sh [--debug|--release] [--build-type=<Type>] [--benchmarks]
#
# CLI flags take precedence over BUILD_TYPE.
#
set -euo pipefail

# --- Resolve the repository root (directory containing this script) ----------
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"
INSTALL_DEPS="${INSTALL_DEPS:-auto}"
BENCHMARKS="OFF"

log()  { printf '\033[1;34m[setup]\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[setup]\033[0m %s\n' "$*" >&2; }
err()  { printf '\033[1;31m[setup]\033[0m %s\n' "$*" >&2; }
have() { command -v "$1" >/dev/null 2>&1; }

usage() {
  cat <<'USAGE'
Usage: ./setup.sh [--debug|--release] [--build-type=<Type>] [--benchmarks]

Options:
  --debug               Shortcut for --build-type=Debug
  --release             Shortcut for --build-type=Release
  --build-type=<Type>   Explicit CMake build type (Debug, Release, RelWithDebInfo, MinSizeRel)
  --benchmarks          Also configure the compile-time benchmarks (off by default)
  -h, --help            Show this help message and exit
USAGE
}

parse_args() {
  while [ "$#" -gt 0 ]; do
    case "$1" in
      --debug) BUILD_TYPE="Debug" ;;
      --release) BUILD_TYPE="Release" ;;
      --build-type=*) BUILD_TYPE="${1#*=}" ;;
      --benchmarks) BENCHMARKS="ON" ;;
      -h|--help) usage; exit 0 ;;
      *) err "Unknown argument: $1"; usage; exit 2 ;;
    esac
    shift
  done
}

# --- 1. Dependencies ---------------------------------------------------------
maybe_sudo() {
  if [ "$(id -u)" -eq 0 ]; then "$@"; else sudo "$@"; fi
}

install_apt_deps() {
  local pkgs=(build-essential cmake)
  log "Installing dependencies via apt: ${pkgs[*]}"
  maybe_sudo apt-get update -y
  maybe_sudo apt-get install -y "${pkgs[@]}"
}

install_termux_deps() {
  local pkgs=(cmake clang)
  log "Installing dependencies via pkg: ${pkgs[*]}"
  pkg install -y "${pkgs[@]}"
}

ensure_deps() {
  case "$INSTALL_DEPS" in
    no)
      log "Skipping dependency installation (INSTALL_DEPS=no)."
      ;;
    yes)
      if [ -n "${PREFIX:-}" ] && [[ "$PREFIX" == *com.termux* ]]; then install_termux_deps
      elif have apt-get; then install_apt_deps
      else err "No supported package manager found (apt-get or Termux's pkg); please install the prerequisites manually."; fi
      ;;
    auto)
      if have cmake && { have g++ || have clang++; }; then
        log "Core build tools already present."
      elif [ -n "${PREFIX:-}" ] && [[ "$PREFIX" == *com.termux* ]]; then
        install_termux_deps
      elif have apt-get; then
        install_apt_deps
      else
        warn "Missing build tools, and no supported package manager available."
        warn "Please install: cmake (>=3.24) and a C++20 compiler (see README.md)."
      fi
      ;;
    *)
      err "Invalid INSTALL_DEPS='$INSTALL_DEPS' (expected auto|yes|no)."; exit 2
      ;;
  esac
}

check_tools() {
  local missing=0
  have cmake || { err "Required tool missing: cmake"; missing=1; }
  if ! have g++ && ! have clang++; then
    err "No C++ compiler found (need g++ or clang++ with C++20 support)."
    missing=1
  fi
  if [ "$missing" -ne 0 ]; then
    err "Install the missing prerequisites and re-run (see README.md)."
    exit 1
  fi
}

# --- 2. Configure / build / test ---------------------------------------------
configure() {
  log "Configuring '$BUILD_TYPE' build in ./$BUILD_DIR"
  cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DUZU_BUILD_TESTS=ON \
    -DUZU_BUILD_BENCHMARKS="$BENCHMARKS"
}

build() {
  log "Building (jobs: $JOBS) — the compile-time fits are evaluated here, so this is the slow part"
  cmake --build "$BUILD_DIR" -j "$JOBS"
}

run_tests() {
  log "Running tests"
  ctest --test-dir "$BUILD_DIR" --output-on-failure
}

main() {
  parse_args "$@"
  ensure_deps
  check_tools
  configure
  build
  run_tests
  log "All done — environment ready, build and tests passed."
}

main "$@"
