#! /usr/bin/env bash

set -o errexit  # abort on nonzero exit status
set -o nounset  # abort on unbound variable
set -o pipefail # don't hide errors within pipes

CURRENT_DIR=$(dirname "$0")
ADC_MONITOR_TCP_API_PORT=3000

main() {
  echo "WebSocket Gateway: Wait for port ${ADC_MONITOR_TCP_API_PORT} is open"
  while netstat -lnt | awk -v port=":${ADC_MONITOR_TCP_API_PORT}$" '$4 ~ port {exit 1}'; do sleep 10; done
  echo "WebSocket Gateway: Port ${ADC_MONITOR_TCP_API_PORT} is now open"
  npm run --prefix "${CURRENT_DIR}/websocket-gateway/" start
}

main
exit 0