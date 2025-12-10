#!/bin/bash
set -eo pipefail
cd "$(dirname "$(realpath -e "${BASH_SOURCE[0]}")")"

# wrapper to fix permissions of mounted volumes

if [[ -z "$DOCKER" || ! -d /home/mybcp || ! -d /usr/mybcp ]]; then
  echo -e "\033[91mDon't run this file manually\033[0m"
  exit 1
fi

# 🤷
[[ -z "$USER" ]] && USER="$(whoami)"
echo -e "\033[94mchown-ing symlinked volumes...\033[0m"
sudo chown -R "${USER?:USER is unset}:${USER}" /home/mybcp /usr/mybcp

if [[ "$DOCKER" == "build" ]]; then
  # this is a manual invocation, build
  echo -e "\033[94mbuiling in container...\033[0m"
  ./build.sh "$@"
fi
# otherwise we ran in a dev container
echo -e "\033[94m[ DEV CONTAINER READY ]\033[0m"
while true; do cat; done