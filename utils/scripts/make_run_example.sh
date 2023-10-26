#!/bin/bash

set -e
. ./utils/scripts/functions.sh

echo_and_run_cmd "cd build"

# Build and run the main example of the project
echo_and_run_cmd "make"

echo_and_run_cmd "./examples/main"
