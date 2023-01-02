#! /usr/bin/env bash

set -o errexit # abort on nonzero exit status
set -o nounset # abort on unbound variable
#set -o pipefail # don't hide errors within pipes

if [ "$#" -ne 2 ]; then
  printf "Arguments are missing" 1>&2
  exit 1
fi

BUILD_TYPE=$(echo "$1" | awk '{print tolower($0)}')
VERSION="$2"
BUILD_DIR=$(pwd)
BIN="adc_monitor"

if [ "${BUILD_TYPE}" = "release" ]; then
  if [ ! -d /root/bin ]; then
    mkdir /root/bin
  fi
  cp "${BUILD_DIR}/${BIN}" "/root/bin/${BIN}-${VERSION}.bin"
  chmod +x "/root/bin/${BIN}-${VERSION}.bin"
  ln -sfn "/root/bin/${BIN}-${VERSION}.bin" /usr/bin/adc_monitor

  # Create the service to launch automatically adc_monitor at startup
  if [ ! -x /usr/lib/systemd/system/adc_monitor.service ]; then
    cat >/usr/lib/systemd/system/adc_monitor.service <<'EOF'
[Unit]
Description=ADC Monitor Service
Before=websocket-gateway.service
Wants=multi-user.target

[Service]
Type=simple
ExecStart=/usr/bin/adc_monitor
ExecStop=/bin/kill -15 $MAINPID
Restart=on-failure
RestartSec=10
#TimeoutStartSec=
TimeoutStopSec=10

[Install]
WantedBy=multi-user.target
EOF
    chmod 644 /usr/lib/systemd/system/adc_monitor.service
    systemctl enable adc_monitor.service
  fi
fi

echo "Post build finished"
