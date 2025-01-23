from pathlib import Path

from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.factory import Factory
from riot_libgen.function import Function
from riot_libgen.library import Library
from riot_libgen.function_handle import FunctionHandle
from riot_libgen.libgen import LibGen
from riot_libgen.parameter import Parameter

NATIVE_TYPES_MAPPING = {
    # from https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md

    'char': 'i',
    'signed char': 'i',
    'unsigned char': 'i',

    'short': 'i',
    'short int': 'i',
    'signed short': 'i',
    'signed short int': 'i',

    'unsigned short': 'i',
    'unsigned short int': 'i',

    'int': 'i',
    'signed': 'i',
    'signed int': 'i',

    'unsigned': 'i',
    'unsigned int': 'i',

    'long': 'i',  # i32
    'long int': 'i',  # i32
    'signed long': 'i',  # i32
    'signed long int': 'i',  # i32

    'unsigned long': 'i',  # i32
    'unsigned long int': 'i',  # i32

    'long long': 'I',  # i64
    'long long int': 'I',  # i64
    'signed long long': 'I',  # i64
    'signed long long int': 'I',  # i64

    'unsigned long long': 'I',  # i64
    'unsigned long long int': 'I',  # i64

    'float': 'f',  # f32

    'double': 'F',  # f64

    # TODO still unsure what those types correspond in WASM runtime
    # 'long double',
    # 'bool',

    'void': '',
    'const char*': '$'
}


class WasmLibrary(Library):
    def __init__(self, name, config: dict, factory):
        super().__init__(name, config, factory)

        if self.prefix is not None:
            self.prefix = self.prefix + '_'
        else:
            self.prefix = self.name + '_'


class WasmFunction(Function):
    def get_wasm_runtime_signature(self) -> str:
        # add parameters
        signature = '('
        for parameter in self.parameters.values():
            if parameter.type in self._library.function_handles:
                signature += NATIVE_TYPES_MAPPING['const char*']
            elif parameter.type in NATIVE_TYPES_MAPPING:
                signature += NATIVE_TYPES_MAPPING[parameter.type]
            else:
                raise LibGenConfigException(
                    'WASM generator does not support parameter type \'{}\' yet'.format(parameter.type))
        signature += ')'

        # add return type
        if self.return_type not in NATIVE_TYPES_MAPPING:
            raise LibGenConfigException(
                'WASM generator does not support return type \'{}\' yet'.format(self.return_type))
        signature += NATIVE_TYPES_MAPPING[self.return_type]

        return signature


class WasmParameter(Parameter):
    def get_wasm_runtime_signature(self) -> str | None:
        if self.type in NATIVE_TYPES_MAPPING:
            return NATIVE_TYPES_MAPPING[self.type]
        return None


class WasmFunctionHandle(FunctionHandle):
    def get_wasm_runtime_signature_for_return_type(self) -> str | None:
        if self.return_type in NATIVE_TYPES_MAPPING:
            return NATIVE_TYPES_MAPPING[self.return_type]
        return None


class WasmFactory(Factory):
    def create_library(self, name: str, config: dict):
        return WasmLibrary(name, config, self)

    def create_function(self, name: str, config: dict, library: Library):
        return WasmFunction(name, config, self, library)

    def create_parameter(self, name: str, config: dict, library: Library):
        return WasmParameter(name, config, self, library)

    def create_function_handle(self, name: str, config: dict, library: Library):
        return WasmFunctionHandle(name, config, self, library)


class WasmLibGen(LibGen):
    def __init__(self):
        super().__init__()
        self._factory = WasmFactory(self._context)
        self._template_path = Path(__file__).resolve().parent.joinpath('templates/')
