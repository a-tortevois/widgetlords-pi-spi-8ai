#! /usr/bin/env bash

set -o errexit  # abort on nonzero exit status
set -o nounset  # abort on unbound variable
set -o pipefail # don't hide errors within pipes

CURRENT_DIR="$(cd -P -- "$(dirname -- "$0")"; pwd -P)"

exit_error() {
  printf "Error: %s.  Please try to exec '.${CURRENT_DIR}/run.sh --help'.\n" "${1}" 1>&2
  exit 1
}

install_libwidgetlords() {
  if [ "$(ldconfig -p | grep -c "libwidgetlords")" -eq 0 ]; then
    if [ "$(getconf LONG_BIT)" -eq 64 ]; then
      wget https://github.com/widgetlords/libwidgetlords/releases/download/v2.1.1/libwidgetlords_2.1.1_arm64.deb
      sudo dpkg -i libwidgetlords_2.1.1_arm64.deb
    else
      wget https://github.com/widgetlords/libwidgetlords/releases/download/v2.1.1/libwidgetlords_2.1.1_armhf.deb
      sudo dpkg -i libwidgetlords_2.1.1_armhf.deb
    fi
  fi
}

update_config_txt() {
  update=0
  if [ "$(grep -c "dtparam=spi=on" /boot/config.txt)" -ne 1 ]; then
    echo "dtparam=spi=on" >>/boot/config.txt
    update=1
  fi
  if [ "$(grep -c "dtoverlay=pi-spi" /boot/config.txt)" -ne 1 ]; then
    echo "dtoverlay=pi-spi:extra_cs=true" >>/boot/config.txt
    update=1
  fi
  if [ "${update}" -eq 1 ]; then
    echo "Please reboot at the end of the installation process"
  fi
}

install() {
  install_libwidgetlords
  update_config_txt
  chmod +x ./adc-monitor/run.sh
  ./adc-monitor/run.sh --build-release
  chmod +x ./web-services/run.sh
  ./web-services/run.sh --install
}

uninstall() {
  # Stop services
  if [ "$(systemctl list-units --type=service --state=active | grep -c adc_monitor)" -ne 0 ]; then
    systemctl stop adc_monitor*
  fi
  # Uninstall libwidgetlords
  if [ "$(ldconfig -p | grep -c "libwidgetlords")" -ne 0 ]; then
    sudo apt remove libwidgetlords
  fi
  # Remove pi-spi overlay from config.txt
  if [ "$(grep -c "dtoverlay=pi-spi" /boot/config.txt)" -ne 0 ]; then
    sed -i '/dtoverlay=pi-spi:extra_cs=true/d' /boot/config.txt
  fi
  # Uninstall services
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
