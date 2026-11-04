"""
Builds and runs the RIOT test applications in tests/apps (one for JerryScript, one for WAMR) and reports
every test case printed by them as a separate test.

The applications are regular RIOT applications, so they can also be run manually, e.g.:

    tests/apps/js/gen.sh && make -C tests/apps/js BOARD=nucleo-f429zi flash test

By default, they run on the native board (with AddressSanitizer). Use --board to run them on a
microcontroller, e.g. "pytest -m apps --board esp32-wroom-32".
"""
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

import pytest

pytestmark = pytest.mark.apps

APPS_DIR = Path(__file__).resolve().parent / 'apps'

JS = 'js'
WASM = 'wasm'
BOTH = (JS, WASM)

# Test cases printed by the test applications, and the runtimes they are expected for. Cases that crash
# because of known bugs are disabled in the applications (see BUG comments there) and missing here.
CASES = {
    'primitives.int': BOTH,
    'primitives.int_negative': BOTH,
    'primitives.double': BOTH,
    'primitives.float': BOTH,
    'primitives.int64': BOTH,
    'primitives.char': BOTH,
    'primitives.unsigned': BOTH,
    'primitives.size_t': BOTH,
    'primitives.no_parameters': BOTH,
    'primitives.void': BOTH,
    'primitives.original_name': BOTH,
    'constants.number': BOTH,
    'constants.string': BOTH,
    'strings.length': BOTH,
    'strings.content': BOTH,
    'strings.utf8': BOTH,
    'strings.empty': BOTH,
    'strings.null': (JS,),
    'byte_arrays.content': BOTH,
    'byte_arrays.length_parameter': BOTH,
    'byte_arrays.writable': BOTH,
    'function_handles.int': BOTH,
    'function_handles.string_parameter': BOTH,
    'function_handles.string_parameter_terminated': BOTH,
    'function_handles.string_parameter_null': BOTH,
    'function_handles.byte_array_parameter': BOTH,
    'function_handles.void': BOTH,
    'function_handles.array_parameter': (JS,),  # WASM: disabled, see BUG in apps/wasm/guest/main.c
    'function_handles.struct_parameter': BOTH,
    'function_handles.struct_return': BOTH,
    'function_handles.string_return': (WASM,),  # not supported by the JS generator
    'pointer_handles.roundtrip': BOTH,
    'pointer_handles.distinct': BOTH,
    'pointer_handles.same_pointer_same_handle': BOTH,
    'pointer_handles.null_to_runtime': BOTH,
    'pointer_handles.null_from_runtime': BOTH,
    'pointer_handles.release_and_reuse': BOTH,
    'arrays.int': BOTH,
    'arrays.double': BOTH,
    'structs.parameter': BOTH,
    'structs.parameter_negative': BOTH,
    'structs.return': BOTH,
    'structs.double_fields': BOTH,
}

# Cases that fail because of known bugs, by case name and runtime.
KNOWN_FAILURES = {
}

RESULT_PATTERN = re.compile(r'\b(PASS|FAIL) ([\w.]+)(?::\s*(.*))?$')
# the marker may be prefixed by the terminal program (e.g. timestamps), but has to end the line
DONE_PATTERN = re.compile(r'(^|\s)LIBGEN_TESTS_DONE$')


@dataclass
class AppRun:
    error: str | None = None
    output: str = ''
    results: dict[str, tuple[str, str]] = field(default_factory=dict)  # case -> (PASS|FAIL, details)
    done: bool = False


def _make(app_dir: Path, *args, env: dict) -> subprocess.CompletedProcess:
    return subprocess.run(['make', '-C', str(app_dir), *args], env=env, capture_output=True, text=True)


def _run_app(runtime: str, config: pytest.Config) -> AppRun:
    app_dir = APPS_DIR / runtime
    board = config.getoption('--board')
    native = board.startswith('native')

    env = dict(os.environ)
    # gen.sh and RIOT's testrunner need the Python environment pytest runs in
    env['PATH'] = str(Path(sys.executable).parent) + os.pathsep + env.get('PATH', '')
    env['BOARD'] = board
    env['ASAN_OPTIONS'] = 'detect_leaks=0'
    if config.getoption('--port'):
        env['PORT'] = config.getoption('--port')

    res = subprocess.run([str(app_dir / 'gen.sh')], env=env, capture_output=True, text=True)
    if res.returncode != 0:
        return AppRun(error=f'generating the module failed:\n{res.stdout}\n{res.stderr}')

    target = 'all-asan' if native and not config.getoption('--no-asan') else 'all'
    res = _make(app_dir, f'-j{os.cpu_count()}', target, env=env)
    if res.returncode != 0:
        return AppRun(error=f'building the application failed:\n{res.stdout[-10000:]}\n{res.stderr[-10000:]}')

    if not native:
        res = _make(app_dir, 'flash-only', env=env)
        if res.returncode != 0:
            return AppRun(error=f'flashing the application failed:\n{res.stdout}\n{res.stderr}')

    # the exit code of "make test" is not used, as it also fails for known failures
    res = _make(app_dir, 'test', env=env)
    run = AppRun(output=res.stdout + res.stderr)
    for line in res.stdout.splitlines():
        if DONE_PATTERN.search(line.rstrip()):
            run.done = True
        match = RESULT_PATTERN.search(line)
        if match:
            run.results[match.group(2)] = (match.group(1), match.group(3) or '')
    return run


_app_runs: dict[str, AppRun] = {}


@pytest.fixture
def app_run(request) -> callable:
    def get(runtime: str) -> AppRun:
        if runtime not in _app_runs:
            _app_runs[runtime] = _run_app(runtime, request.config)
        run = _app_runs[runtime]
        if run.error:
            pytest.fail(run.error, pytrace=False)
        return run
    return get


def _case_params():
    for name, runtimes in CASES.items():
        for runtime in runtimes:
            marks = []
            if (name, runtime) in KNOWN_FAILURES:
                marks.append(pytest.mark.xfail(reason=KNOWN_FAILURES[(name, runtime)]))
            yield pytest.param(runtime, name, id=f'{runtime}-{name}', marks=marks)


@pytest.mark.parametrize('runtime', BOTH)
def test_app_completes(app_run, runtime):
    """The application runs all cases without crashing, and reports exactly the expected cases."""
    run = app_run(runtime)
    assert run.done, f'the application did not finish (crash or panic?):\n{run.output[-10000:]}'
    expected = {name for name, runtimes in CASES.items() if runtime in runtimes}
    assert set(run.results) == expected, 'cases reported by the application differ from CASES'


@pytest.mark.parametrize('runtime,name', list(_case_params()))
def test_case(app_run, runtime, name):
    run = app_run(runtime)
    if name not in run.results:
        pytest.fail(f'case was not reported by the application (crash or panic?):\n{run.output[-5000:]}',
                    pytrace=False)
    status, details = run.results[name]
    assert status == 'PASS', details
