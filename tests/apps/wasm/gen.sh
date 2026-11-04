#!/bin/sh
cd "$(dirname "$0")"
python3 ../../../cli.py -g wasm -c ../modules/libgen_tests/libgen_tests.libgen-config.yml ../modules/libgen_tests/libgen_tests_wasm.libgen-config.yml ../../../configs/saul_reg.libgen-config.yml -d external_modules/wasm_libs
