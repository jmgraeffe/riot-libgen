"""Tests for the files rendered by the generators (without compiling them, see test_apps.py for that)."""
import re
from pathlib import Path

import pytest
import yaml

import riot_libgen
from riot_libgen.exceptions import LibGenConfigException

REPO_DIR = Path(__file__).resolve().parents[1]

GENERATORS = {'wasm': riot_libgen.WasmLibGen, 'js': riot_libgen.JsLibGen}


def render(generator: str, libraries: dict, path: Path) -> Path:
    libgen = GENERATORS[generator]()
    libgen.load_config_from_dict({'libraries': libraries})
    libgen.render(path)
    return path


def read(path: Path, generator: str, name: str) -> str:
    return (path / name.format(g=generator)).read_text()


@pytest.fixture(params=GENERATORS)
def generator(request):
    return request.param


# configurations used by the examples (as in their gen.sh), and by the test applications
EXAMPLE_CONFIGS = {
    'wasm': {
        'wasm/playground': ['external/easyio/easyio.libgen-config.yml',
                            'examples/wasm/playground/wasm_playground.libgen-config.yml'],
        'wasm/saul_libgen': ['examples/wasm/saul_libgen/saul_libgen.libgen-config.yml',
                             'external/easysaul/easysaul.libgen-config.yml'],
        'wasm/coap_libgen': ['examples/wasm/coap_libgen/coap_libgen.libgen-config.yml',
                             'external/easycoap/easycoap.libgen-config.yml'],
        'tests/apps/wasm': ['tests/apps/modules/libgen_tests/libgen_tests.libgen-config.yml',
                            'tests/apps/modules/libgen_tests/libgen_tests_wasm.libgen-config.yml'],
    },
    'js': {
        'js/playground': ['examples/js/playground/js_playground.libgen-config.yml'],
        'js/saul_libgen': ['external/math.libgen-config.yml', 'external/easysaul/easysaul.libgen-config.yml'],
        'js/coap_libgen': ['external/easycoap/easycoap.libgen-config.yml'],
        'tests/apps/js': ['tests/apps/modules/libgen_tests/libgen_tests.libgen-config.yml'],
    },
}


def _example_params():
    for generator, examples in EXAMPLE_CONFIGS.items():
        for name, configs in examples.items():
            yield pytest.param(generator, configs, id=f'{generator}-{name}')


@pytest.mark.parametrize('generator,configs', list(_example_params()))
def test_examples_render_cleanly(tmp_path, generator, configs):
    libgen = GENERATORS[generator]()
    for config in configs:
        libgen.load_config_from_file(str(REPO_DIR / config))
    libgen.render(tmp_path)

    for path in tmp_path.rglob('*'):
        if path.is_file():
            content = path.read_text()
            assert content.endswith('\n'), f'{path.name}: missing newline at end of file'
            assert '\t' not in content or path.name.startswith('Makefile'), f'{path.name}: contains tabs'
            for number, line in enumerate(content.splitlines(), 1):
                assert line == line.rstrip(), f'{path.name}:{number}: trailing whitespace'
            assert '\n\n\n' not in content, f'{path.name}: multiple consecutive blank lines'


def test_module_files(tmp_path, generator):
    render(generator, {'lib': {'functions': {'f': {}}}}, tmp_path)
    names = {str(p.relative_to(tmp_path)) for p in tmp_path.rglob('*') if p.is_file()}
    expected = {f'{generator}_libs.c', f'include/{generator}_libs.h', 'Makefile', 'Makefile.dep',
                'Makefile.include'}
    if generator == 'wasm':
        expected.add('include/wasm_libs_runtime.h')
    assert names == expected


def test_makefile_dep(tmp_path, generator):
    render(generator, {
        'a': {'modules': ['mod_a', 'shared'], 'packages': ['pkg_a']},
        'b': {'modules': ['shared', 'mod_b']},
    }, tmp_path)
    makefile_dep = read(tmp_path, generator, 'Makefile.dep')
    runtime_package = {'wasm': 'wamr', 'js': 'jerryscript'}[generator]
    assert f'ifneq (,$(filter {generator}_libs,$(USEMODULE)))' in makefile_dep
    assert re.findall(r'USEMODULE \+= (\S+)', makefile_dep) == ['mod_a', 'shared', 'mod_b']
    assert re.findall(r'USEPKG \+= (\S+)', makefile_dep) == [runtime_package, 'pkg_a']


def test_header(tmp_path, generator):
    render(generator, {
        'lib': {
            'includes': ['first.h', 'second.h'],
            'functions': {
                'f': {'original_name': 'native_f', 'parameters': {'a': 'int', 's': 'const char*'},
                      'return_type': 'double'},
                'g': {'parameters': {'cb': 'handler'}},
                'h': {'return_type': 'ptr'},
            },
            'function_handles': {'handler': {'parameters': {'x': 'int'}, 'return_type': 'int'}},
            'pointer_handles': {'ptr': 'thing_t*'},
        },
    }, tmp_path)
    header = read(tmp_path, generator, 'include/{g}_libs.h')
    runtime_header = {'wasm': 'wasm_export.h', 'js': 'jerryscript.h'}[generator]
    assert re.findall(r'#include "(\S+)"', header) == [runtime_header, 'first.h', 'second.h']
    assert f'int {generator}_libs_register_lib_natives(' in header
    assert f'int {generator}_libs_release_lib_ptr_pointer_handle(void* pointer);' in header
    # forward declarations of the native functions
    assert 'double native_f(int a, const char* s);' in header
    assert 'void g(int (*cb)(int x));' in header
    assert 'thing_t* h(void);' in header


def test_wasm_runtime_header(tmp_path):
    render('wasm', {
        'lib': {
            'constants': {'answer': 42, 'greeting': 'hello'},
            'functions': {
                'f': {'parameters': {'a': 'int', 'cb': 'handler', 's': 'data', 'arr': 'int[3]'},
                      'return_type': 'ptr'},
                'g': {'return_type': 'data'},
            },
            'function_handles': {'handler': {}},
            'pointer_handles': {'ptr': 'thing_t*'},
            'structs': {'data': {'type': 'data_t', 'fields': {'val': 'int16_t[3]', 'unit': 'uint8_t'}}},
        },
        'other': {'prefix': 'pfx', 'constants': {'value': 7}, 'functions': {'h': {}}},
    }, tmp_path)
    header = (tmp_path / 'include' / 'wasm_libs_runtime.h').read_text()
    assert '#define LIB_ANSWER (42)' in header
    assert '#define LIB_GREETING ("hello")' in header
    assert '#define PFX_VALUE (7)' in header
    assert 'typedef ssize_t lib_ptr_t;' in header
    assert re.search(r'typedef struct \{\s+int16_t val\[3\];\s+uint8_t unit;\s+\} lib_data_t;', header)
    assert 'int64_t lib_f(int a, wasm_libs_function_handle_t cb, lib_data_t* s, int arr[3]);' in header
    assert 'lib_data_t* lib_g();' in header
    assert 'void pfx_h();' in header


def test_wasm_native_symbols(tmp_path):
    render('wasm', {
        'lib': {
            'functions': {
                'f': {'parameters': {'a': 'int', 'b': 'int64_t', 'c': 'float', 'd': 'double', 's': 'const char*',
                                     'cb': 'handler', 'p': 'ptr', 'arr': 'int[2]', 'st': 'data'},
                      'return_type': 'double'},
                'g': {'return_type': 'ptr'},
                'h': {'return_type': 'data'},
            },
            'function_handles': {'handler': {}},
            'pointer_handles': {'ptr': 'thing_t*'},
            'structs': {'data': {'type': 'data_t'}},
        },
    }, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    signatures = dict(re.findall(r'"lib_(\w+)",\s*NULL,[^\n]*\n\s*"([^"]*)"', source))
    assert signatures == {'f': '(iIfF$$Iii)F', 'g': '()I', 'h': '()i'}


@pytest.mark.parametrize('generator,type_', [
    pytest.param('wasm', 'bool', marks=pytest.mark.xfail(
        raises=LibGenConfigException, reason='bool is listed as supported, but not mapped by the WASM generator')),
    pytest.param('js', 'bool', marks=pytest.mark.xfail(
        raises=LibGenConfigException, reason='bool is listed as supported, but not mapped by the JS generator')),
])
def test_bool(tmp_path, generator, type_):
    render(generator, {'lib': {'functions': {'f': {'parameters': {'b': type_}, 'return_type': type_}}}}, tmp_path)


def test_js_string_return_not_supported(tmp_path):
    with pytest.raises(LibGenConfigException):
        render('js', {'lib': {'functions': {'f': {'return_type': 'const char*'}}}}, tmp_path)


# Consistency checks of generated code for constructs that do not compile because of known bugs (and thus
# cannot be part of the test applications yet).

def _called_but_undefined(source: str, prefix: str) -> set[str]:
    called = set(re.findall(rf'\b({prefix}\w*)\(', source))
    defined = set(re.findall(rf'^\S[^\n;]*\b({prefix}\w*)\([^;]*\{{\s*$', source, re.MULTILINE))
    return called - defined


def test_function_handle_with_pointer_handle_parameter(tmp_path, generator):
    render(generator, {'lib': {
        'functions': {'f': {'parameters': {'cb': 'handler'}}},
        'function_handles': {'handler': {'parameters': {'p': 'ptr'}}},
        'pointer_handles': {'ptr': 'thing_t*'},
    }}, tmp_path)
    source = read(tmp_path, generator, '{g}_libs.c')
    assert _called_but_undefined(source, 'pointer_handles_get_or_add') == set()


def test_wasm_function_handle_with_float_parameters(tmp_path):
    render('wasm', {'lib': {
        'functions': {'f': {'parameters': {'cb': 'handler'}}},
        'function_handles': {'handler': {'parameters': {'f': 'float', 'd': 'double'}}},
    }}, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    assert set(re.findall(r'\.kind = (\w+);', source)) == {'WASM_F32', 'WASM_F64'}


def test_wasm_function_handle_with_string_parameter(tmp_path):
    render('wasm', {'lib': {
        'functions': {'f': {'parameters': {'cb': 'handler'}}},
        'function_handles': {'handler': {'parameters': {'s': 'const char*'}}},
    }}, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    # the name of the parameter is used, the terminating '\0' is copied as well, and NULL is passed as 0
    assert 's != NULL ? (int32_t) wasm_runtime_module_dup_data(wasm_module_inst, (const char*) s, ' \
           'strlen((const char*) s) + 1) : 0;' in source


@pytest.mark.parametrize('element_type', ['uint8_t', 'int', 'double'])
def test_wasm_array_sizes(tmp_path, element_type):
    """Arrays are checked and copied with the size of their elements, not the size of a pointer to them."""
    render('wasm', {'lib': {
        'functions': {'f': {'parameters': {'values': f'{element_type}[3]'}},
                      'g': {'parameters': {'cb': 'handler'}}},
        'function_handles': {'handler': {'parameters': {'values': f'{element_type}[3]'}}},
    }}, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    assert 'sizeof(&' not in source
    # range check of the array passed to f(), and copy of the array passed to the function handle
    assert source.count('sizeof(values[0]) * (3)') == 2


def test_wasm_struct_size(tmp_path):
    """Struct parameters are checked with the size of the struct, not the size of a pointer to it."""
    render('wasm', {'lib': {
        'functions': {'f': {'parameters': {'m': 'measurement'}}},
        'structs': {'measurement': {'type': 'measurement_t', 'fields': {'val': 'int16_t[3]'}}},
    }}, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    assert 'wasm_runtime_validate_app_addr(wasm_module_inst, (uintptr_t) m, sizeof(measurement_t))' in source


def _byte_array_function(generator: str, tmp_path: Path) -> str:
    render(generator, {'lib': {'functions': {'f': {'parameters': {
        'data': {'type': 'const uint8_t*', 'length_parameter': 'len'},
        'len': 'size_t',
    }}}}}, tmp_path)
    return read(tmp_path, generator, '{g}_libs.c')


def test_wasm_byte_array_range(tmp_path):
    """The whole range of a byte array (as given by the length parameter) is checked to be in the WASM memory."""
    source = _byte_array_function('wasm', tmp_path)
    assert 'wasm_runtime_validate_app_addr(wasm_module_inst, (uintptr_t) data, len)' in source


def test_js_byte_array_length(tmp_path):
    """The length parameter must not be larger than the byte array (which is copied from a JS string)."""
    source = _byte_array_function('js', tmp_path)
    assert re.search(r'if \(\(size_t\) len > length_data\) \{\s+core_panic\(', source)


def test_js_pointer_handle_release_function(tmp_path):
    render('js', {'lib': {'pointer_handles': {'ptr': 'thing_t*'}}}, tmp_path)
    source = (tmp_path / 'js_libs.c').read_text()
    assert re.search(r'^int js_libs_release_lib_ptr_pointer_handle\(void\* pointer\) \{', source, re.MULTILINE)


def test_config_files_are_valid_yaml():
    for generator_configs in EXAMPLE_CONFIGS.values():
        for configs in generator_configs.values():
            for config in configs:
                with open(REPO_DIR / config) as file:
                    assert 'libraries' in yaml.safe_load(file), config
