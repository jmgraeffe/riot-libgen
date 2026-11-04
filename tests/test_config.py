"""Tests for loading LibGen configurations into the internal representation (no code generation)."""
import pytest

import riot_libgen
from riot_libgen.exceptions import LibGenConfigException, LibGenRuntimeException
from riot_libgen.helpers import FUNCTION_HANDLE_TYPE, POINTER_HANDLE_TYPE, POINTER_HANDLE_MAX_ENTRIES_DEFAULT

GENERATORS = [riot_libgen.WasmLibGen, riot_libgen.JsLibGen]


@pytest.fixture(params=GENERATORS, ids=['wasm', 'js'])
def libgen(request):
    return request.param()


def load(libgen, libraries: dict):
    libgen.load_config_from_dict({'libraries': libraries})
    return libgen._context


def load_library(libgen, library: dict, name='lib'):
    return load(libgen, {name: library}).libraries[name]


def test_missing_libraries(libgen):
    with pytest.raises(LibGenConfigException):
        libgen.load_config_from_dict({})


def test_empty_library(libgen):
    with pytest.raises(LibGenConfigException):
        load(libgen, {'lib': None})


def test_multiple_libraries_and_configs(libgen):
    load(libgen, {'a': {'functions': {'f': {}}}, 'b': {'functions': {'g': {}}}})
    context = load(libgen, {'c': {'functions': {'h': {}}}})
    assert list(context.libraries) == ['a', 'b', 'c']


class TestFunctions:
    def test_defaults(self, libgen):
        function = load_library(libgen, {'functions': {'f': {}}}).functions['f']
        assert function.name == 'f'
        assert function.original_name == 'f'
        assert function.return_type == 'void'
        assert function.parameters == {}

    @pytest.mark.xfail(reason='decided to change: the string shortcut of functions is the return type (as for '
                              'parameters, fields and pointer handles), but currently it is the original name')
    def test_shortcut_is_return_type(self, libgen):
        function = load_library(libgen, {'functions': {'f': 'int'}}).functions['f']
        assert function.return_type == 'int'
        assert function.original_name == 'f'
        assert function.parameters == {}

    @pytest.mark.xfail(raises=LibGenConfigException,
                       reason='BUG: functions without value (allowed by the schema) are rejected')
    def test_null_shortcut(self, libgen):
        function = load_library(libgen, {'functions': {'f': None}}).functions['f']
        assert function.return_type == 'void'
        assert function.original_name == 'f'
        assert function.parameters == {}

    def test_original_name(self, libgen):
        function = load_library(libgen, {'functions': {'f': {'original_name': 'native_f'}}}).functions['f']
        assert function.name == 'f'
        assert function.original_name == 'native_f'

    def test_parameters_and_return_type(self, libgen):
        function = load_library(libgen, {'functions': {'f': {
            'parameters': {'a': 'int', 'b': {'type': 'double'}},
            'return_type': 'uint8_t',
        }}}).functions['f']
        assert list(function.parameters) == ['a', 'b']
        assert function.parameters['a'].type == 'int'
        assert function.parameters['b'].type == 'double'
        assert function.return_type == 'uint8_t'

    def test_unknown_parameter_type(self, libgen):
        with pytest.raises(LibGenConfigException):
            load_library(libgen, {'functions': {'f': {'parameters': {'a': 'unknown_t'}}}})

    def test_unknown_return_type(self, libgen):
        with pytest.raises(LibGenConfigException):
            load_library(libgen, {'functions': {'f': {'return_type': 'unknown_t'}}})

    def test_array_return_type_not_supported(self, libgen):
        with pytest.raises(LibGenConfigException):
            load_library(libgen, {'functions': {'f': {'return_type': 'int[3]'}}})

    def test_includes_modules_packages(self, libgen):
        context = load(libgen, {'lib': {'functions': {'f': {
            'includes': ['f.h'], 'modules': ['f_mod'], 'packages': ['f_pkg'],
        }}}})
        assert context.includes == ['f.h']
        assert context.modules == ['f_mod']
        assert context.packages == ['f_pkg']


class TestParameters:
    def test_byte_array_needs_length_parameter(self, libgen):
        with pytest.raises(LibGenConfigException):
            load_library(libgen, {'functions': {'f': {'parameters': {'data': 'const uint8_t*'}}}})

    def test_byte_array(self, libgen):
        function = load_library(libgen, {'functions': {'f': {'parameters': {
            'data': {'type': 'uint8_t*', 'length_parameter': 'len'},
            'len': 'size_t',
        }}}}).functions['f']
        assert function.parameters['data'].is_byte_array()
        assert function.parameters['data'].length_parameter == 'len'
        assert not function.parameters['data'].is_const()

    def test_length_parameter_needs_to_be_a_name(self, libgen):
        with pytest.raises(LibGenConfigException):
            load_library(libgen, {'functions': {'f': {'parameters': {
                'data': {'type': 'const uint8_t*', 'length_parameter': 3},
            }}}})

    def test_array(self, libgen):
        parameter = load_library(libgen, {'functions': {'f': {'parameters': {'a': 'int16_t[4]'}}}}) \
            .functions['f'].parameters['a']
        assert parameter.is_array()
        assert parameter.array_size == 4
        assert parameter._element_parameter.type == 'int16_t'

    def test_array_of_unknown_type(self, libgen):
        with pytest.raises(LibGenConfigException):
            load_library(libgen, {'functions': {'f': {'parameters': {'a': 'unknown_t[4]'}}}})

    @pytest.mark.xfail(raises=NameError, reason='BUG: LibGenRuntimeException is not imported in parameter.py')
    def test_array_size_of_non_array(self, libgen):
        parameter = load_library(libgen, {'functions': {'f': {'parameters': {'a': 'int'}}}}) \
            .functions['f'].parameters['a']
        with pytest.raises(LibGenRuntimeException):
            parameter.array_size


class TestFunctionHandles:
    def test_parameter(self, libgen):
        library = load_library(libgen, {
            'functions': {'f': {'parameters': {'cb': 'handler'}}},
            'function_handles': {'handler': {'parameters': {'a': 'int'}, 'return_type': 'int'}},
        })
        parameter = library.functions['f'].parameters['cb']
        assert parameter.is_function_handle()
        assert parameter.function_handle == 'handler'
        assert parameter.type == FUNCTION_HANDLE_TYPE
        handle = library.function_handles['handler']
        assert list(handle.parameters) == ['a']
        assert handle.return_type == 'int'

    def test_empty(self, libgen):
        handle = load_library(libgen, {'function_handles': {'handler': None}}).function_handles['handler']
        assert handle.parameters == {}
        assert handle.return_type == 'void'

    def test_returns_struct(self, libgen):
        handle = load_library(libgen, {
            'function_handles': {'handler': {'return_type': 's'}},
            'structs': {'s': {'type': 's_t'}},
        }).function_handles['handler']
        assert handle.returns_struct()

    @pytest.mark.xfail(reason='BUG: pointer handles are rejected as return type of function handles '
                              '(and POINTER_HANDLE_TYPE is not imported in function_handle.py)')
    def test_returns_pointer_handle(self, libgen):
        handle = load_library(libgen, {
            'function_handles': {'handler': {'return_type': 'ptr'}},
            'pointer_handles': {'ptr': 'void*'},
        }).function_handles['handler']
        assert handle.returns_pointer_handle()
        assert handle.return_type == POINTER_HANDLE_TYPE


class TestPointerHandles:
    def test_shortcut(self, libgen):
        library = load_library(libgen, {'pointer_handles': {'ptr': 'thing_t*'}})
        assert library.pointer_handles['ptr'].type == 'thing_t*'
        assert library.pointer_handles['ptr'].max_entries == POINTER_HANDLE_MAX_ENTRIES_DEFAULT

    def test_max_entries(self, libgen):
        library = load_library(libgen, {'pointer_handles': {'ptr': {'type': 'thing_t*', 'max_entries': 3}}})
        assert library.pointer_handles['ptr'].max_entries == 3

    def test_missing_type(self, libgen):
        with pytest.raises(LibGenConfigException):
            load_library(libgen, {'pointer_handles': {'ptr': {'max_entries': 3}}})

    def test_parameter_and_return_type(self, libgen):
        library = load_library(libgen, {
            'functions': {'f': {'parameters': {'p': 'ptr'}, 'return_type': 'ptr'}},
            'pointer_handles': {'ptr': 'thing_t*'},
        })
        function = library.functions['f']
        assert function.parameters['p'].is_pointer_handle()
        assert function.parameters['p'].type == POINTER_HANDLE_TYPE
        assert function.returns_pointer_handle()
        assert function.return_type == POINTER_HANDLE_TYPE


class TestStructs:
    def test_fields(self, libgen):
        library = load_library(libgen, {
            'functions': {'f': {'parameters': {'s': 'data'}, 'return_type': 'data'}},
            'structs': {'data': {'type': 'data_t', 'fields': {'val': 'int16_t[3]', 'unit': 'uint8_t'}}},
        })
        struct = library.structs['data']
        assert struct.type == 'data_t'
        assert list(struct.fields) == ['val', 'unit']
        assert struct.fields['val'].is_array()
        assert library.functions['f'].parameters['s'].is_struct()
        assert library.functions['f'].returns_struct()

    @pytest.mark.xfail(raises=NameError, reason='BUG: LibGenConfigException is not imported in struct.py')
    def test_missing_type(self, libgen):
        with pytest.raises(LibGenConfigException):
            load_library(libgen, {'structs': {'data': {'fields': {'x': 'int'}}}})


class TestLibraries:
    def test_constants(self, libgen):
        library = load_library(libgen, {'constants': {'answer': 42, 'greeting': 'hello'}})
        assert library.constants == {'answer': 42, 'greeting': 'hello'}

    def test_includes_modules_packages_are_deduplicated(self, libgen):
        context = load(libgen, {
            'a': {'includes': ['x.h', 'y.h'], 'modules': ['m'], 'packages': ['p']},
            'b': {'includes': ['y.h', 'z.h'], 'modules': ['m', 'n'], 'packages': ['p']},
        })
        assert context.includes == ['x.h', 'y.h', 'z.h']
        assert context.modules == ['m', 'n']
        assert context.packages == ['p']
