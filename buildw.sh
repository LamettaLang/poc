#!/bin/bash

set -eo pipefail
cd "$(dirname "$(realpath -e "${BASH_SOURCE[0]}")")"

# i can't remember that command line, so here goes another wrapper

if [[ -n "$DOCKER" ]]; then
  echo -e "\033[91mDo not run this file inside docker\033[0m"
  exit 1
fi

env UID="$(id -u)" GID="$(id -g)" docker compose up