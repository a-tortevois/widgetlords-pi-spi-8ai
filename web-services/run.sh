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
  if [ ! -x /usr/bin/node ]; then
    curl -sL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
    sudo apt-get update --fix-missing
    sudo apt-get install -y nodejs
    npm install npm@latest --location=global
  fi
  if [ ! -d /root/www ]; then
    mkdir /root/www
  else
    rm -rf /root/www/*
  fi
  cp -r "${CURRENT_DIR}/web-server" /root/www/web-server
  cp -r "${CURRENT_DIR}/websocket-gateway" /root/www/websocket-gateway
  echo "## Install web-server dependencies"
  cd /root/www/web-server && npm install
  echo "## Install websocket-gateway dependencies"
  cd /root/www/websocket-gateway && npm install
  cd "${CURRENT_DIR}"
  cp "${CURRENT_DIR}/web-server.sh" /root/www/web-server.sh
  cp "${CURRENT_DIR}/websocket-gateway.sh" /root/www/websocket-gateway.sh
  chmod +x /root/www/*.sh

  if [ ! -x /usr/lib/systemd/system/adc_monitor_web_server.service ]; then
    cat >/usr/lib/systemd/system/adc_monitor_web_server.service <<'EOF'
[Unit]
Description=ADC Monitor Web Server Service
After=adc_monitor_websocket_gateway.service
Wants=multi-user.target

[Service]
Type=forking
ExecStart=/root/www/web-server.sh
Restart=on-failure
RestartSec=10
#TimeoutStartSec=
TimeoutStopSec=10

[Install]
WantedBy=multi-user.target
EOF
    chmod 644 /usr/lib/systemd/system/adc_monitor_web_server.service
    systemctl enable adc_monitor_web_server.service
  fi

  if [ ! -x /usr/lib/systemd/system/adc_monitor_websocket_gateway.service ]; then
    cat >/usr/lib/systemd/system/adc_monitor_websocket_gateway.service <<'EOF'
[Unit]
Description=ADC Monitor WebSocket Gateway Service
Before=adc_monitor_web_server.service
After=adc_monitor.service
Wants=multi-user.target

[Service]
Type=forking
ExecStart=/root/www/websocket-gateway.sh
Restart=on-failure
RestartSec=10
#TimeoutStartSec=
TimeoutStopSec=10

[Install]
WantedBy=multi-user.target
EOF
    chmod 644 /usr/lib/systemd/system/adc_monitor_websocket_gateway.service
    systemctl enable adc_monitor_websocket_gateway.service
  fi
}

uninstall() {
  if [ -f /usr/lib/systemd/system/adc_monitor_websocket_gateway.service ]; then
    if [ "$(systemctl is-active adc_monitor_websocket_gateway.service)" = "enabled" ]; then
      systemctl stop adc_monitor_websocket_gateway.service
    fi
    if [ "$(systemctl is-enabled adc_monitor_websocket_gateway.service)" = "enabled" ]; then
      systemctl disable adc_monitor_websocket_gateway.service
    fi
    rm /usr/lib/systemd/system/adc_monitor_websocket_gateway.service
  fi

  if [ -f /usr/lib/systemd/system/adc_monitor_web_server.service ]; then
    if [ "$(systemctl is-active adc_monitor_web_server.service)" = "enabled" ]; then
      systemctl stop adc_monitor_web_server.service
    fi
    if [ "$(systemctl is-enabled adc_monitor_web_server.service)" = "enabled" ]; then
      systemctl disable adc_monitor_web_server.service
    fi
    rm /usr/lib/systemd/system/adc_monitor_web_server.service
  fi
  rm -rf /root/www
}

help() {
  cat <<EOF
  Usage:
       --help                       Displays this help
       --install                    Install ADC Monitor WebSocket Gateway & WebServer services
       --uninstall                  Uninstall ADC Monitor WebSocket Gateway & WebServer services
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
