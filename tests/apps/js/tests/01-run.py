#!/usr/bin/env python3
"""Waits until all test cases ran and fails if any of them failed (see main.js)."""
import sys

from testrunner import run


def testfunc(child):
    child.expect_exact('LIBGEN_TESTS_DONE', timeout=120)
    if 'FAIL ' in child.before:
        raise RuntimeError('some test cases failed')


if __name__ == '__main__':
    sys.exit(run(testfunc))
