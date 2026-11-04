#!/bin/sh
cd "$(dirname "$0")"
python3 ../../../cli.py -g js -c ../modules/libgen_tests/libgen_tests.libgen-config.yml -d external_modules/js_libs
