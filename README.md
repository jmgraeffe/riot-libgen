# riot-libgen

## Tests

Install the dependencies (e.g. in a virtual environment) with `pip install -r requirements-dev.txt`, then run
`pytest` in the repository root:

* `tests/test_config.py` and `tests/test_rendering.py` test loading configurations and rendering modules,
* `tests/test_apps.py` builds and runs the RIOT test applications in `tests/apps` (one for JerryScript, one for
  WAMR), which exercise all supported features of the generated code. Each test case they print is reported
  as a separate test.

The test applications run on the native board (with AddressSanitizer) by default. To run them on a
microcontroller, pass the board (and optionally the serial port):

    pytest -m apps --board nucleo-f429zi
    pytest -m apps --board esp32-wroom-32 --port /dev/ttyUSB0

They are regular RIOT applications, so they can also be run manually, e.g.
`tests/apps/js/gen.sh && make -C tests/apps/js BOARD=nucleo-f429zi flash test`.

Test cases that fail because of known bugs are marked as expected failures, so fixing a bug turns them into
unexpected passes (which fail the test run until the marker is removed).
