def pytest_addoption(parser):
    group = parser.getgroup('libgen', 'LibGen test applications (tests/apps)')
    group.addoption('--board', default='native',
                    help='RIOT board to run the test applications on, e.g. native, esp32-wroom-32 or '
                         'nucleo-f429zi (default: native)')
    group.addoption('--no-asan', action='store_true',
                    help='do not build the test applications with AddressSanitizer on native boards')
    group.addoption('--port', default=None,
                    help='serial port of the board (passed as PORT to RIOT, if given)')
