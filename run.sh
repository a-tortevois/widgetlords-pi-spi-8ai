#! /usr/bin/env bash

set -o errexit  # abort on nonzero exit status
set -o nounset  # abort on unbound variable
set -o pipefail # don't hide errors within pipes

CURRENT_DIR="$(cd -P -- "$(dirname -- "$0")"; pwd -P)"

exit_error() {
  printf "Error: %s.  Please try to exec '.${CURRENT_DIR}/run.sh --help'.\n" "${1}" 1>&2
  exit 1
}

install() {
  chmod +x ./adc-monitor/run.sh
  ./adc-monitor/run.sh --build-release
  chmod +x ./web-services/run.sh
  ./web-services/run.sh --install
}

uninstall() {
  ./adc-monitor/run.sh --uninstall
  ./web-services/run.sh --uninstall
}

help() {
  cat <<EOF
  Usage:
       --help                       Displays this help
       --install                    Install ADC Monitor
       --uninstall                  Uninstall ADC Monitor
EOF
}

main() {
  if [ "$#" -ne 1 ]; then
    exit_error "Argument error"
  fi
  case "${1}" in
  --install)
    install
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
