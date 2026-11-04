from pathlib import Path

from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.factory import Factory
from riot_libgen.function import Function
from riot_libgen.library import Library
from riot_libgen.function_handle import FunctionHandle
from riot_libgen.libgen import LibGen
from riot_libgen.parameter import Parameter
from riot_libgen.struct import Struct
from riot_libgen.helpers import FUNCTION_HANDLE_TYPE, POINTER_HANDLE_TYPE


"""
TODO:
- extract sanity checks (parameter types etc.) into Python code instead of having it in template code
- optimize field names (I32/i32) into function so the template code is shorter
"""


# from https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md
NATIVE_TYPES_MAPPING = {
    # named main types (from https://en.wikipedia.org/wiki/C_data_types#Main_types)

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

    'long': 'i',
    'long int': 'i',
    'signed long': 'i',
    'signed long int': 'i',

    'unsigned long': 'i',
    'unsigned long int': 'i',

    'long long': 'I',
    'long long int': 'I',
    'signed long long': 'I',
    'signed long long int': 'I',

    'unsigned long long': 'I',
    'unsigned long long int': 'I',

    'float': 'f',

    'double': 'F', #TODO verify correctness
    'long double': 'F', #TODO verify correctness

    # stddef types (incomplete)

    'size_t': 'i', #TODO this is a problem, platform specific
    'ssize_t': 'I', #TODO this is a problem, platform specific

    # stdint types (from https://www.gnu.org/software/libc/manual/html_node/Integers.html)

    'int8_t': 'i',
    'int16_t': 'i',
    'int32_t': 'i',
    'int64_t': 'I',
    'uint8_t': 'i',
    'uint16_t': 'i',
    'uint32_t': 'i',
    'uint64_t': 'I',
    
    'int_least8_t': 'i',
    'int_least16_t': 'i',
    'int_least32_t': 'i',
    'int_least64_t': 'I',
    'uint_least8_t': 'i',
    'uint_least16_t': 'i',
    'uint_least32_t': 'i',
    'uint_least64_t': 'I',

    'int_fast8_t': 'i',
    'int_fast16_t': 'i',
    'int_fast32_t': 'i',
    'int_fast64_t': 'I',
    'uint_fast8_t': 'i',
    'uint_fast16_t': 'i',
    'uint_fast32_t': 'i',
    'uint_fast64_t': 'I',

    'intmax_t': 'I',
    'uintmax_t': 'I',

    # special types

    'bool': 'i',
    'void': '',
    'const char*': '$',
    'const uint8_t*': 'i', #TODO this might become variable in the future
    'uint8_t*': 'i' #TODO this might become variable in the future
}

POINTER_TYPE = 'i' #TODO this might become variable in the future
STRING_TYPE = 'const char*'

# Types as they are stored in the memory of the WASM application (wasm32): the C type that is used for copying a value
# between the WASM memory and the host, and its size (which is also its alignment in wasm32). Sizes may differ from the
# host (e.g. long and size_t on 64 bit hosts), which is why structs and arrays are converted field by field.
GUEST_TYPES = {
    'char': ('int8_t', 1),
    'signed char': ('int8_t', 1),
    'unsigned char': ('uint8_t', 1),

    'short': ('int16_t', 2),
    'short int': ('int16_t', 2),
    'signed short': ('int16_t', 2),
    'signed short int': ('int16_t', 2),
    'unsigned short': ('uint16_t', 2),
    'unsigned short int': ('uint16_t', 2),

    'int': ('int32_t', 4),
    'signed': ('int32_t', 4),
    'signed int': ('int32_t', 4),
    'unsigned': ('uint32_t', 4),
    'unsigned int': ('uint32_t', 4),

    'long': ('int32_t', 4),
    'long int': ('int32_t', 4),
    'signed long': ('int32_t', 4),
    'signed long int': ('int32_t', 4),
    'unsigned long': ('uint32_t', 4),
    'unsigned long int': ('uint32_t', 4),

    'long long': ('int64_t', 8),
    'long long int': ('int64_t', 8),
    'signed long long': ('int64_t', 8),
    'signed long long int': ('int64_t', 8),
    'unsigned long long': ('uint64_t', 8),
    'unsigned long long int': ('uint64_t', 8),

    'float': ('float', 4),
    'double': ('double', 8),

    'size_t': ('uint32_t', 4),
    # see wasm_libs_runtime.h.tmpl
    'ssize_t': ('int64_t', 8),

    'int8_t': ('int8_t', 1),
    'int16_t': ('int16_t', 2),
    'int32_t': ('int32_t', 4),
    'int64_t': ('int64_t', 8),
    'uint8_t': ('uint8_t', 1),
    'uint16_t': ('uint16_t', 2),
    'uint32_t': ('uint32_t', 4),
    'uint64_t': ('uint64_t', 8),

    'int_least8_t': ('int8_t', 1),
    'int_least16_t': ('int16_t', 2),
    'int_least32_t': ('int32_t', 4),
    'int_least64_t': ('int64_t', 8),
    'uint_least8_t': ('uint8_t', 1),
    'uint_least16_t': ('uint16_t', 2),
    'uint_least32_t': ('uint32_t', 4),
    'uint_least64_t': ('uint64_t', 8),

    'int_fast8_t': ('int8_t', 1),
    'int_fast16_t': ('int16_t', 2),
    'int_fast32_t': ('int32_t', 4),
    'int_fast64_t': ('int64_t', 8),
    'uint_fast8_t': ('uint8_t', 1),
    'uint_fast16_t': ('uint16_t', 2),
    'uint_fast32_t': ('uint32_t', 4),
    'uint_fast64_t': ('uint64_t', 8),

    'intmax_t': ('int64_t', 8),
    'uintmax_t': ('uint64_t', 8),

    'bool': ('bool', 1),
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
            if parameter.is_array() or parameter.is_struct():
                signature += POINTER_TYPE
            elif parameter.is_output():
                if parameter.type == STRING_TYPE:
                    raise LibGenConfigException(
                        'WASM generator does not support output strings yet (parameter \'{}\')'.format(parameter.name))
                # pointer into the WASM memory
                signature += POINTER_TYPE
            elif parameter.is_function_handle():
                signature += NATIVE_TYPES_MAPPING[FUNCTION_HANDLE_TYPE]
            elif parameter.is_pointer_handle():
                signature += NATIVE_TYPES_MAPPING[POINTER_HANDLE_TYPE]
            elif parameter.type == STRING_TYPE:
                # passed as pointer and converted by the generated code, so that NULL can be passed as well
                signature += POINTER_TYPE
            elif parameter.type in NATIVE_TYPES_MAPPING:
                signature += NATIVE_TYPES_MAPPING[parameter.type]
            else:
                raise LibGenConfigException(
                    'WASM generator does not support parameter type \'{}\' yet'.format(parameter.type))
        signature += ')'

        # add return type
        if self.returns_struct():
            signature += POINTER_TYPE
        elif self.return_type in NATIVE_TYPES_MAPPING:
            signature += NATIVE_TYPES_MAPPING[self.return_type]
        elif self.return_type in self._library.pointer_handles:
            signature += NATIVE_TYPES_MAPPING[POINTER_HANDLE_TYPE]
        else:
            raise LibGenConfigException(
                'WASM generator does not support return type \'{}\' yet'.format(self.return_type))

        return signature


class WasmParameter(Parameter):
    def get_wasm_runtime_signature(self) -> str | None:
        if self.type in NATIVE_TYPES_MAPPING:
            return NATIVE_TYPES_MAPPING[self.type]
        return None

    def is_integer(self) -> bool:
        sig = self.get_wasm_runtime_signature()
        if sig.lower() == 'i':
            return True
        return False

    def guest_size(self) -> int:
        """size in the memory of the WASM application"""
        if self.is_array():
            return self._element_parameter.guest_size() * self.array_size
        if self.is_struct():
            return self._library.structs[self.struct].guest_size
        return self._guest_type()[1]

    def guest_align(self) -> int:
        """alignment in the memory of the WASM application"""
        if self.is_array():
            return self._element_parameter.guest_align()
        if self.is_struct():
            return self._library.structs[self.struct].guest_align
        return self._guest_type()[1]

    def guest_transfer_type(self) -> str:
        """C type that is used to copy the value between the WASM memory and the host"""
        return self._guest_type()[0]

    def _guest_type(self) -> tuple[str, int]:
        if self.is_pointer_handle():
            return POINTER_HANDLE_TYPE, 8
        if self._type in GUEST_TYPES:
            return GUEST_TYPES[self._type]
        raise LibGenConfigException(
            'WASM generator does not support \'{}\' (\'{}\') in structs and arrays yet'.format(self._type, self.name))


class WasmStruct(Struct):
    def guest_layout(self) -> tuple[dict[str, int], int, int]:
        """offsets of the fields, size and alignment of the struct in the memory of the WASM application (natural
        alignment as in wasm32)"""
        offsets = {}
        offset = 0
        align = 1
        for name, field in self.fields.items():
            field_align = field.guest_align()
            offset = (offset + field_align - 1) // field_align * field_align
            offsets[name] = offset
            offset += field.guest_size()
            align = max(align, field_align)
        size = (offset + align - 1) // align * align
        return offsets, size, align

    @property
    def guest_offsets(self) -> dict[str, int]:
        return self.guest_layout()[0]

    @property
    def guest_size(self) -> int:
        return self.guest_layout()[1]

    @property
    def guest_align(self) -> int:
        return self.guest_layout()[2]


class WasmFunctionHandle(FunctionHandle):
    def get_wasm_runtime_signature_for_return_type(self) -> str | None:
        if self.return_type in self._library.structs:
            return POINTER_TYPE
        elif self.return_type in NATIVE_TYPES_MAPPING:
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

    def create_struct(self, name: str, config: dict, library: Library):
        return WasmStruct(name, config, self, library)


class WasmLibGen(LibGen):
    def __init__(self):
        super().__init__()
        self._factory = WasmFactory(self._context)
        self._generator_template_path = Path(__file__).resolve().parent.joinpath('templates/')
