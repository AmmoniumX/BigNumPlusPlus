#!/usr/bin/env bash
set -e # Exit immediately if any compilation or test fails

for f in build/debug build/release build/gcc; do 
  cmake --build build/gcc
  ctest --test-dir build/gcc
done
