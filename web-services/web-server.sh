#! /usr/bin/env bash

set -o errexit  # abort on nonzero exit status
set -o nounset  # abort on unbound variable
set -o pipefail # don't hide errors within pipes

CURRENT_DIR=$(dirname "$0")
WEBSOCKET_GATEWAY_PORT=8888

main() {
  echo "Web Server: Wait for port ${WEBSOCKET_GATEWAY_PORT} is open"
  while netstat -lnt | awk -v port=":${WEBSOCKET_GATEWAY_PORT}$" '$4 ~ port {exit 1}'; do sleep 10; done
  echo "Web Server: Port ${WEBSOCKET_GATEWAY_PORT} is now open"
  npm run --prefix "${CURRENT_DIR}/web-server/" start
}

main
exit 0