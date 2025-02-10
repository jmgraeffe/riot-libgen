NATIVE_TYPES = [
    # named main types (from https://en.wikipedia.org/w/index.php?title=C_data_types&oldid=1220806182)

    'char',
    'signed char',
    'unsigned char',

    'short',
    'short int',
    'signed short',
    'signed short int',

    'unsigned short',
    'unsigned short int',

    'int',
    'signed',
    'signed int',

    'unsigned',
    'unsigned int',

    'long',
    'long int',
    'signed long',
    'signed long int',

    'unsigned long',
    'unsigned long int',

    'long long',
    'long long int',
    'signed long long',
    'signed long long int',

    'unsigned long long',
    'unsigned long long int',

    'float',

    'double',
    'long double',

    # stddef types (incomplete)

    'size_t',
    'ssize_t',

    # stdint types (from https://www.gnu.org/software/libc/manual/html_node/Integers.html)

    'int8_t',
    'int16_t',
    'int32_t',
    'int64_t',
    'uint8_t',
    'uint16_t',
    'uint32_t',
    'uint64_t',
    
    'int_least8_t',
    'int_least16_t',
    'int_least32_t',
    'int_least64_t',
    'uint_least8_t',
    'uint_least16_t',
    'uint_least32_t',
    'uint_least64_t',

    'int_fast8_t',
    'int_fast16_t',
    'int_fast32_t',
    'int_fast64_t',
    'uint_fast8_t',
    'uint_fast16_t',
    'uint_fast32_t',
    'uint_fast64_t',

    'intmax_t',
    'uintmax_t',

    # special types

    'bool',
    'void',
    'const char*',
    'const uint8_t*',
    'uint8_t*'
]

BYTE_ARRAY_TYPES = [
    'const uint8_t*',
    'uint8_t*'
]

# function handles
FUNCTION_HANDLE_TYPE = 'const char*'

# pointer handles
POINTER_HANDLE_TYPE = 'int64_t'
POINTER_HANDLE_MAX_ENTRIES_DEFAULT = 10