#! /usr/bin/env bash

set -o errexit  # abort on nonzero exit status
set -o nounset  # abort on unbound variable
set -o pipefail # don't hide errors within pipes

CURRENT_DIR="$(cd -P -- "$(dirname -- "$0")"; pwd -P)"

exit_error() {
  printf "Error: %s.  Please try to exec '.${CURRENT_DIR}/run.sh --help'.\n" "${1}" 1>&2
  exit 1
}

check_cmake_install() {
  if [ ! -x /usr/bin/cmake ]; then
    sudo apt-get update --fix-missing
    sudo apt-get install -y cmake
  fi
}

build_debug() {
  check_cmake_install
  # Clean
  [[ -d "${CURRENT_DIR}/cmake-build-debug" ]] && /usr/bin/cmake --build "${CURRENT_DIR}/cmake-build-debug" --target clean -- -j 12
  # Generate Makefile
  /usr/bin/cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" -S "${CURRENT_DIR}" -B "${CURRENT_DIR}/cmake-build-debug"
  # Build
  /usr/bin/cmake --build "${CURRENT_DIR}/cmake-build-debug" --target all -- -j 12
}

build_release() {
  check_cmake_install
  # Clean
  [[ -d "${CURRENT_DIR}/cmake-build-release" ]] && /usr/bin/cmake --build "${CURRENT_DIR}/cmake-build-release" --target clean -- -j 12
  # Generate Makefile
  /usr/bin/cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" -S "${CURRENT_DIR}" -B "${CURRENT_DIR}/cmake-build-release"
  # Build
  /usr/bin/cmake --build "${CURRENT_DIR}/cmake-build-release" --target all -- -j 12
}

uninstall() {
  if [ -f /usr/lib/systemd/system/adc_monitor.service ]; then
    if [ "$(systemctl is-active adc_monitor.service)" = "enabled" ]; then
      systemctl stop adc_monitor.service
    fi
    if [ "$(systemctl is-enabled adc_monitor.service)" = "enabled" ]; then
      systemctl disable adc_monitor.service
    fi
    rm /usr/lib/systemd/system/adc_monitor.service
  fi
  if [ -L /usr/bin/adc_monitor ]; then
    rm /usr/bin/adc_monitor
  fi
  if [ -d /root/bin ]; then
    rm -rf /root/bin/*
  fi
  if [ -d /root/logs ]; then
    rm -rf /root/logs/*
  fi
}

help() {
  cat <<EOF
  Usage:
       --help                       Displays this help
       --build-debug                Build ADC Monitor with debug flag
       --build-release              Build ADC Monitor with release flag, then install it to your system
       --uninstall                  Uninstall ADC Monitor
EOF
}

main() {
  if [ "$#" -ne 1 ]; then
    exit_error "Argument error"
  fi
  case "${1}" in
  --build-debug)
    build_debug
    ;;
  --build-release)
    build_release
    ;;
  --uninstall)
    uninstall
    ;;
  --help)
    help
    ;;
  *)
    exit_error "Unknown argument"
    ;;
  esac
}

main "$@"
exit 0
