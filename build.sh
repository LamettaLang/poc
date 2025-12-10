#!/bin/bash

# work in script dir
cd "$(dirname "${BASH_SOURCE[0]}")" || exit 1

# make venv, activate/deactivate
if ! [[ -d venv ]]; then
    python3 -m venv venv || exit 1
fi
source venv/bin/activate
trap deactivate EXIT

# prep tools
if ! which conan >/dev/null; then
    pip install cmake conan || exit 1
    conan profile detect --force
fi

if ! which g++ >/dev/null; then
    echo missing build tools
    exit 1
elif g++ -std=c++23 2>&1 | grep -q unrecognized; then
    echo g++ does not support c++23
    exit 1
fi

# update deps and build
python3 build_py "$@"
