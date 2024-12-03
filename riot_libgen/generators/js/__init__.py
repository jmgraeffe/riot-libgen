from enum import Enum
from pathlib import Path

from riot_libgen.factory import Factory
from riot_libgen.function import Function
from riot_libgen.function_handle import FunctionHandle
from riot_libgen.library import Library
from riot_libgen.libgen import LibGen
from riot_libgen.parameter import Parameter


class JsType(Enum):
    UNDEFINED = 0
    NUMBER = 1
    STRING = 2
    UNKNOWN = 100


NATIVE_TYPES_MAPPING = {
    'char': JsType.NUMBER,
    'signed char': JsType.NUMBER,
    'unsigned char': JsType.NUMBER,

    'short': JsType.NUMBER,
    'short int': JsType.NUMBER,
    'signed short': JsType.NUMBER,
    'signed short int': JsType.NUMBER,

    'unsigned short': JsType.NUMBER,
    'unsigned short int': JsType.NUMBER,

    'int': JsType.NUMBER,
    'signed': JsType.NUMBER,
    'signed int': JsType.NUMBER,

    'unsigned': JsType.NUMBER,
    'unsigned int': JsType.NUMBER,

    'long': JsType.NUMBER,
    'long int': JsType.NUMBER,
    'signed long': JsType.NUMBER,
    'signed long int': JsType.NUMBER,

    'unsigned long': JsType.NUMBER,
    'unsigned long int': JsType.NUMBER,

    'long long': JsType.NUMBER,
    'long long int': JsType.NUMBER,
    'signed long long': JsType.NUMBER,
    'signed long long int': JsType.NUMBER,

    'unsigned long long': JsType.NUMBER,
    'unsigned long long int': JsType.NUMBER,

    'float': JsType.NUMBER,

    'double': JsType.NUMBER,
    'long double': JsType.NUMBER,

    # TODO still unsure what those types correspond in JS runtime
    # 'bool',

    'void': JsType.UNDEFINED,
    'const char*': JsType.STRING
}


class JsFunction(Function):
    def returns_number(self) -> bool:
        if self.return_type in NATIVE_TYPES_MAPPING and NATIVE_TYPES_MAPPING[self.return_type] == JsType.NUMBER:
            return True
        return False


class JsParameter(Parameter):
    def is_number(self) -> bool:
        if self.type in NATIVE_TYPES_MAPPING and NATIVE_TYPES_MAPPING[self.type] == JsType.NUMBER:
            return True
        return False

    def is_string(self) -> bool:
        if self.type in NATIVE_TYPES_MAPPING and NATIVE_TYPES_MAPPING[self.type] == JsType.STRING:
            return True
        return False


class JsFunctionHandle(FunctionHandle):
    def returns_number(self) -> bool:
        if self.return_type in NATIVE_TYPES_MAPPING and NATIVE_TYPES_MAPPING[self.return_type] == JsType.NUMBER:
            return True
        return False


class JsFactory(Factory):
    def create_function(self, name: str, config: dict, library: Library):
        return JsFunction(name, config, self, library)

    def create_parameter(self, name: str, config: dict, library: Library):
        return JsParameter(name, config, self, library)

    def create_function_handle(self, name: str, config: dict, library: Library):
        return JsFunctionHandle(name, config, self, library)


class JsLibGen(LibGen):
    def __init__(self):
        super().__init__()
        self._factory = JsFactory(self._context)
        self._template_path = Path(__file__).resolve().parent.joinpath('templates/')
