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
                             'external/math.libgen-config.yml', 'configs/saul_reg.libgen-config.yml'],
        'wasm/coap_libgen': ['examples/wasm/coap_libgen/coap_libgen.libgen-config.yml',
                             'external/easycoap/easycoap.libgen-config.yml'],
        'tests/apps/wasm': ['tests/apps/modules/libgen_tests/libgen_tests.libgen-config.yml',
                            'tests/apps/modules/libgen_tests/libgen_tests_wasm.libgen-config.yml',
                            'configs/saul_reg.libgen-config.yml'],
    },
    'js': {
        'js/playground': ['examples/js/playground/js_playground.libgen-config.yml'],
        'js/saul_libgen': ['external/math.libgen-config.yml', 'configs/saul_reg.libgen-config.yml'],
        'js/coap_libgen': ['external/easycoap/easycoap.libgen-config.yml'],
        'tests/apps/js': ['tests/apps/modules/libgen_tests/libgen_tests.libgen-config.yml',
                          'tests/apps/modules/libgen_tests/libgen_tests_js.libgen-config.yml',
                          'configs/saul_reg.libgen-config.yml'],
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


def test_js_constants(tmp_path):
    """Constants are properties of the library object, so they do not use the prefix of the library."""
    render('js', {
        'lib': {'constants': {'answer': 42}},
        'other': {'prefix': 'pfx', 'constants': {'value': 7}},
    }, tmp_path)
    source = (tmp_path / 'js_libs.c').read_text()
    names = re.findall(r'property_name_constant = jerry_create_string\(\(const jerry_char_t\*\) \("(\w+)"\)\)', source)
    assert names == ['ANSWER', 'VALUE']


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
    # strings are passed as i32 and converted by the generated code (so that NULL can be passed), function
    # handles (names of guest functions) are converted by WAMR ($)
    assert signatures == {'f': '(iIfFi$Iii)F', 'g': '()I', 'h': '()i'}


def test_bool(tmp_path, generator):
    render(generator, {'lib': {'functions': {'f': {'parameters': {'b': 'bool'}, 'return_type': 'bool'}}}}, tmp_path)
    source = read(tmp_path, generator, '{g}_libs.c')
    if generator == 'wasm':
        # bool is passed as i32
        assert re.search(r'"lib_f",\s*NULL,[^\n]*\n\s*"\(i\)i"', source)
    else:
        # bool is a JS boolean
        assert 'bool b = jerry_value_to_boolean(argv[0]);' in source
        assert 'return jerry_create_boolean(libgen_res);' in source


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


@pytest.mark.parametrize('element_type,guest_size', [('uint8_t', 3), ('int', 12), ('double', 24), ('size_t', 12)])
def test_wasm_array_sizes(tmp_path, element_type, guest_size):
    """Arrays are checked and copied with their size in the WASM memory (which may differ from the host)."""
    render('wasm', {'lib': {
        'functions': {'f': {'parameters': {'values': f'{element_type}[3]'}},
                      'g': {'parameters': {'cb': 'handler'}}},
        'function_handles': {'handler': {'parameters': {'values': f'{element_type}[3]'}}},
    }}, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    assert 'sizeof(&' not in source
    # range check of the array passed to f(), and memory allocated for the array passed to the function handle
    assert f'wasm_runtime_validate_app_addr(wasm_module_inst, (uintptr_t) values, {guest_size})' in source
    assert f'wasm_runtime_module_malloc(wasm_module_inst, {guest_size}, ' in source


def test_wasm_struct_size(tmp_path):
    """Struct parameters are checked with the size of the struct in the WASM memory."""
    render('wasm', {'lib': {
        'functions': {'f': {'parameters': {'m': 'measurement'}}},
        'structs': {'measurement': {'type': 'measurement_t', 'fields': {'val': 'int16_t[3]'}}},
    }}, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    assert 'wasm_runtime_validate_app_addr(wasm_module_inst, (uintptr_t) m, 6)' in source


def test_wasm_struct_layout(tmp_path):
    """Structs are laid out like in wasm32 (natural alignment), which is checked when compiling the WASM application."""
    render('wasm', {'lib': {
        'functions': {'f': {'parameters': {'e': 'envelope'}}},
        'pointer_handles': {'ptr': 'thing_t*'},
        'structs': {
            'measurement': {'type': 'measurement_t', 'fields': {'val': 'int16_t[3]', 'unit': 'uint8_t', 'scale': 'int8_t'}},
            'envelope': {'type': 'envelope_t', 'fields': {
                'tag': 'uint8_t', 'm': 'measurement', 'weight': 'double', 'count': 'size_t', 'p': 'ptr', 'l': 'long',
            }},
        },
    }}, tmp_path)
    header = (tmp_path / 'include' / 'wasm_libs_runtime.h').read_text()
    assert re.search(r'typedef struct \{\s+uint8_t tag;\s+lib_measurement_t m;\s+double weight;\s+size_t count;\s+'
                     r'int64_t p;\s+long l;\s+\} lib_envelope_t;', header)
    assert '_Static_assert(sizeof(lib_envelope_t) == 48, ' in header
    offsets = dict(re.findall(r'__builtin_offsetof\(lib_envelope_t, (\w+)\) == (\d+)', header))
    assert offsets == {'tag': '0', 'm': '2', 'weight': '16', 'count': '24', 'p': '32', 'l': '40'}


def test_wasm_partial_struct(tmp_path):
    """Only declared fields are part of the struct in the WASM application, and converted by the host."""
    render('wasm', {'lib': {
        'functions': {'f': {'parameters': {'p': 'partial'}}},
        'structs': {'partial': {'type': 'partial_t', 'fields': {'a': 'int', 'b': 'int16_t'}}},
    }}, tmp_path)
    header = (tmp_path / 'include' / 'wasm_libs_runtime.h').read_text()
    assert re.search(r'typedef struct \{\s+int a;\s+int16_t b;\s+\} lib_partial_t;', header)
    source = (tmp_path / 'wasm_libs.c').read_text()
    assert 'partial_t p_struct = { 0 };' in source
    assert set(re.findall(r'p_struct\.(\w+) =', source)) == {'a', 'b'}


@pytest.mark.parametrize('field_type', ['const char*', 'handler', 'long double'])
def test_wasm_unsupported_struct_fields(tmp_path, field_type):
    with pytest.raises(LibGenConfigException):
        render('wasm', {'lib': {
            'functions': {'f': {'parameters': {'s': 'data'}}},
            'function_handles': {'handler': None},
            'structs': {'data': {'type': 'data_t', 'fields': {'x': field_type}}},
        }}, tmp_path)


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


OUTPUT_LIBRARY = {'lib': {
    'functions': {'f': {'parameters': {
        'count': {'type': 'int', 'direction': 'out'},
        'p': {'type': 'ptr', 'direction': 'out'},
        'm': {'type': 'data', 'direction': 'out'},
        'arr': {'type': 'int[3]', 'direction': 'out'},
        'buf': {'type': 'uint8_t*', 'length_parameter': 'len', 'direction': 'out'},
        'len': 'size_t',
    }, 'return_type': 'int'}},
    'pointer_handles': {'ptr': 'thing_t*'},
    'structs': {'data': {'type': 'data_t', 'fields': {'x': 'int'}}},
}}


def test_output_forward_declarations(tmp_path, generator):
    render(generator, OUTPUT_LIBRARY, tmp_path)
    header = read(tmp_path, generator, 'include/{g}_libs.h')
    assert 'int f(int* count, thing_t** p, data_t* m, int arr[3], uint8_t* buf, size_t len);' in header


def test_wasm_output_parameters(tmp_path):
    render('wasm', OUTPUT_LIBRARY, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    # output parameters are pointers into the WASM memory
    signatures = dict(re.findall(r'"lib_(\w+)",\s*NULL,[^\n]*\n\s*"([^"]*)"', source))
    assert signatures == {'f': '(iiiiii)i'}
    runtime_header = (tmp_path / 'include' / 'wasm_libs_runtime.h').read_text()
    assert 'int lib_f(int* count, int64_t* p, lib_data_t* m, int arr[3], uint8_t* buf, size_t len);' in runtime_header


def test_wasm_output_strings_not_supported(tmp_path):
    with pytest.raises(LibGenConfigException):
        render('wasm', {'lib': {'functions': {'f': {'parameters': {
            's': {'type': 'const char*', 'direction': 'out'},
        }}}}}, tmp_path)


def test_js_output_strings(tmp_path):
    render('js', {'lib': {'functions': {'f': {'parameters': {
        's': {'type': 'const char*', 'direction': 'out'},
    }}}}}, tmp_path)
    header = (tmp_path / 'include' / 'js_libs.h').read_text()
    assert 'void f(const char** s);' in header


ALLOCATABLE_LIBRARY = {'lib': {
    'functions': {
        'add': {'parameters': {'acc': 'accumulator*', 'value': 'int'}},
        'sum': {'parameters': {'acc': 'const accumulator*'}, 'return_type': 'int'},
        'checksum': {'parameters': {'m': 'const measurement*'}, 'return_type': 'int'},
    },
    'structs': {
        'accumulator': {'type': 'accumulator_t', 'allocatable': True, 'fields': {'sum': 'int'}},
        'opaque': {'type': 'opaque_t', 'allocatable': {'new': 'create_opaque', 'free': 'destroy_opaque'}},
        'measurement': {'type': 'measurement_t', 'fields': {'val': 'int16_t[3]'}},
    },
}}


def test_pointer_forward_declarations(tmp_path, generator):
    render(generator, ALLOCATABLE_LIBRARY, tmp_path)
    header = read(tmp_path, generator, 'include/{g}_libs.h')
    assert 'void add(accumulator_t* acc, int value);' in header
    assert 'int sum(const accumulator_t* acc);' in header
    assert 'int checksum(const measurement_t* m);' in header


def test_wasm_allocatable_structs(tmp_path):
    render('wasm', ALLOCATABLE_LIBRARY, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    signatures = dict(re.findall(r'"lib_(\w+)",\s*NULL,[^\n]*\n\s*"([^"]*)"', source))
    # instances are passed as handles, pointers to structs that are not allocatable as pointer to a copy
    assert signatures == {'add': '(Ii)', 'sum': '(I)i', 'checksum': '(i)i', 'new_accumulator': '(i)I',
                          'free_accumulator': '(I)i', 'create_opaque': '(i)I', 'destroy_opaque': '(I)i'}
    header = (tmp_path / 'include' / 'wasm_libs_runtime.h').read_text()
    assert 'int64_t lib_new_accumulator(const lib_accumulator_t* init);' in header
    assert 'int lib_free_accumulator(int64_t instance);' in header
    assert 'int64_t lib_create_opaque(const lib_opaque_t* init);' in header
    assert 'void lib_add(int64_t acc, int value);' in header
    assert 'int lib_checksum(const lib_measurement_t* m);' in header


def test_js_allocatable_structs(tmp_path):
    render('js', ALLOCATABLE_LIBRARY, tmp_path)
    source = (tmp_path / 'js_libs.c').read_text()
    names = re.findall(r'jerry_create_string\(\(const jerry_char_t\*\) \("(\w+)"\)\);\s+jerry_value_t property_value_function', source)
    assert names == ['add', 'sum', 'checksum', 'new_accumulator', 'free_accumulator', 'create_opaque', 'destroy_opaque']


ARRAYS_LIBRARY = {'lib': {
    'functions': {
        'fill': {'parameters': {'buf': 'buffer*', 'value': 'uint8_t'}},
        'sum': {'parameters': {'buf': 'const buffer*'}, 'return_type': 'int'},
        'samples_sum': {'parameters': {'s': 'const samples*'}, 'return_type': 'int'},
        'checksum': {'parameters': {'ms': 'const measurements*'}, 'return_type': 'int'},
    },
    'structs': {
        'measurement': {'type': 'measurement_t', 'fields': {'val': 'int16_t[3]'}},
    },
    'arrays': {
        'buffer': {'type': 'uint8_t[4]', 'allocatable': True},
        'samples': 'int32_t[3]',
        'measurements': {'type': 'measurement[2]', 'allocatable': {'new': 'create_measurements', 'free': 'destroy_measurements'}},
    },
}}


def test_array_pointer_forward_declarations(tmp_path, generator):
    render(generator, ARRAYS_LIBRARY, tmp_path)
    header = read(tmp_path, generator, 'include/{g}_libs.h')
    assert 'void fill(uint8_t* buf, uint8_t value);' in header
    assert 'int sum(const uint8_t* buf);' in header
    assert 'int samples_sum(const int32_t* s);' in header
    assert 'int checksum(const measurement_t* ms);' in header


def test_wasm_named_arrays(tmp_path):
    render('wasm', ARRAYS_LIBRARY, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    signatures = dict(re.findall(r'"lib_(\w+)",\s*NULL,[^\n]*\n\s*"([^"]*)"', source))
    assert signatures == {'fill': '(Ii)', 'sum': '(I)i', 'samples_sum': '(i)i', 'checksum': '(I)i',
                          'new_buffer': '(i)I', 'free_buffer': '(I)i',
                          'create_measurements': '(i)I', 'destroy_measurements': '(I)i'}
    header = (tmp_path / 'include' / 'wasm_libs_runtime.h').read_text()
    assert 'int64_t lib_new_buffer(const uint8_t init[4]);' in header
    assert 'int lib_free_buffer(int64_t instance);' in header
    assert 'int64_t lib_create_measurements(const lib_measurement_t init[2]);' in header
    assert 'void lib_fill(int64_t buf, uint8_t value);' in header
    assert 'int lib_samples_sum(const int32_t s[3]);' in header


def test_js_named_arrays(tmp_path):
    render('js', ARRAYS_LIBRARY, tmp_path)
    source = (tmp_path / 'js_libs.c').read_text()
    names = re.findall(r'jerry_create_string\(\(const jerry_char_t\*\) \("(\w+)"\)\);\s+jerry_value_t property_value_function', source)
    assert names == ['fill', 'sum', 'samples_sum', 'checksum', 'new_buffer', 'free_buffer',
                     'create_measurements', 'destroy_measurements']


POINTER_FIELDS_LIBRARY = {'lib': {
    'functions': {
        'apply': {'parameters': {'s': 'scaler', 'value': 'int'}},
        'make': {'parameters': {'factor': 'int'}, 'return_type': 'scaler'},
    },
    'structs': {
        'accumulator': {'type': 'accumulator_t', 'allocatable': True},
        'scaler': {'type': 'scaler_t', 'fields': {'factor': 'int', 'acc': 'accumulator*', 'data': 'const buffer*'}},
    },
    'arrays': {
        'buffer': {'type': 'uint8_t[4]', 'allocatable': True},
    },
}}


def test_wasm_pointer_fields(tmp_path):
    render('wasm', POINTER_FIELDS_LIBRARY, tmp_path)
    header = (tmp_path / 'include' / 'wasm_libs_runtime.h').read_text()
    # instances are referenced by handles
    assert '    int64_t acc;' in header
    assert '    int64_t data;' in header
    assert '_Static_assert(sizeof(lib_scaler_t) == 24, ' in header
    assert '_Static_assert(__builtin_offsetof(lib_scaler_t, acc) == 8, ' in header
    source = (tmp_path / 'wasm_libs.c').read_text()
    assert 's_struct.acc = (accumulator_t*) allocated_get_lib_accumulator(libgen_handle);' in source
    assert 's_struct.data = (uint8_t*) allocated_get_lib_buffer(libgen_handle);' in source
    assert 'allocated_find_lib_accumulator((const void*) libgen_res.acc);' in source


def test_js_pointer_fields(tmp_path):
    render('js', POINTER_FIELDS_LIBRARY, tmp_path)
    source = (tmp_path / 'js_libs.c').read_text()
    assert 's.acc = (accumulator_t*) allocated_get_lib_accumulator(' in source
    assert 's.data = (uint8_t*) allocated_get_lib_buffer(' in source
    assert 'jerry_create_number(allocated_find_lib_accumulator((const void*) libgen_res.acc));' in source


VOID_POINTER_LIBRARY = {'lib': {
    'functions': {
        'send': {'parameters': {'data': {'type': 'const void*', 'length_parameter': 'len'}, 'len': 'size_t'}},
        'recv': {'parameters': {'buf': {'type': 'void*', 'length_parameter': 'len'}, 'len': 'size_t'},
                 'return_type': 'ssize_t'},
    },
}}


def test_void_pointer_forward_declarations(tmp_path, generator):
    # declarations need to match the host functions exactly, otherwise they conflict with the original headers
    render(generator, VOID_POINTER_LIBRARY, tmp_path)
    header = read(tmp_path, generator, 'include/{g}_libs.h')
    assert 'void send(const void* data, size_t len);' in header
    assert 'ssize_t recv(void* buf, size_t len);' in header


def test_wasm_void_pointers(tmp_path):
    render('wasm', VOID_POINTER_LIBRARY, tmp_path)
    source = (tmp_path / 'wasm_libs.c').read_text()
    signatures = dict(re.findall(r'"lib_(\w+)",\s*NULL,[^\n]*\n\s*"([^"]*)"', source))
    assert signatures == {'send': '(ii)', 'recv': '(ii)I'}
    header = (tmp_path / 'include' / 'wasm_libs_runtime.h').read_text()
    assert 'void lib_send(const void* data, size_t len);' in header
    assert 'ssize_t lib_recv(void* buf, size_t len);' in header



def test_declare(tmp_path, generator):
    render(generator, {'lib': {'functions': {
        'recv': {'declare': False, 'parameters': {'buf': 'buffer*'}, 'return_type': 'ssize_t'},
        'other': {'parameters': {'buf': 'buffer*'}},
    }, 'arrays': {'buffer': {'type': 'uint8_t[4]', 'allocatable': True}}}}, tmp_path)
    header = read(tmp_path, generator, 'include/{g}_libs.h')
    # the included headers declare the function (e.g. as ssize_t recv(void* buf))
    assert 'recv(' not in header
    assert 'void other(uint8_t* buf);' in header
    # it is called anyway
    source = read(tmp_path, generator, '{g}_libs.c')
    assert 'ssize_t libgen_res = recv(' in source
