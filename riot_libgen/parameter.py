import re
from riot_libgen.exceptions import LibGenConfigException, LibGenRuntimeException
from riot_libgen.helpers import NATIVE_TYPES, BYTE_ARRAY_TYPES, FUNCTION_HANDLE_TYPE, POINTER_HANDLE_TYPE, ARRAY_PATTERN, \
    REFERENCE_PATTERN
from riot_libgen.library import Library

# input parameters are passed from the runtime to the host, output parameters are filled by the host
DIRECTIONS = ['in', 'out']


class Parameter:
    def __init__(self, name, config: dict, factory, library: Library):
        self.length_parameter : str | None = None
        self.direction = 'in'
        self.name = name

        self._factory = factory
        self._context = factory.context
        self._library = library
        self._type: str | None = None
        self._element_parameter: Parameter | None = None
        # pointers to structs: name of the struct, and whether the pointer is const
        self._reference: str | None = None
        self._reference_const = False

        self.load_config_from_dict(config)

    @property
    def type(self):
        if self._type in self._library.function_handles:
            return FUNCTION_HANDLE_TYPE
        elif self._type in self._library.pointer_handles:
            return POINTER_HANDLE_TYPE
        else:
            return self._type

    @property
    def array_size(self):
        match = re.match(ARRAY_PATTERN, self._type)
        if not match:
            raise LibGenRuntimeException('array size is requested from a parameter that is not an array')
        return int(match.group(2))

    @property
    def struct(self):
        if self._type not in self._library.structs:
            raise LibGenRuntimeException('struct name is requested from a parameter that is not a struct')
        return self._type

    @property
    def function_handle(self):
        if self._type not in self._library.function_handles:
            raise LibGenRuntimeException('function handle name is requested from a parameter that is not a function handle')
        return self._type

    @property
    def pointer_handle(self):
        if self._type not in self._library.pointer_handles:
            raise LibGenRuntimeException('pointer handle name is requested from a parameter that is not a pointer handle')
        return self._type

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
            raise LibGenConfigException('could not read config')

        if 'type' not in dict_:
            raise LibGenConfigException('missing parameter type for \'{}\''.format(self.name))
        elif dict_['type'] not in NATIVE_TYPES and dict_['type'] not in self._library.function_handles and dict_['type'] not in self._library.pointer_handles and dict_['type'] not in self._library.structs:
            match = re.match(ARRAY_PATTERN, dict_['type'])
            reference_match = re.match(REFERENCE_PATTERN, dict_['type'])
            if reference_match and reference_match.group(2) in self._library.structs:
                self._reference = reference_match.group(2)
                self._reference_const = reference_match.group(1) is not None
                if not self._library.structs[self._reference].allocatable and not self._reference_const:
                    raise LibGenConfigException('pointer to struct \'{}\' needs to be const for parameter \'{}\', as the struct is not allocatable (it is passed as temporary copy)'.format(self._reference, self.name))
            elif match:
                type_ = match.group(1)
                if type_ in NATIVE_TYPES or type_ in self._library.function_handles or type_ in self._library.pointer_handles or type_ in self._library.structs:
                    self._element_parameter = self._factory.create_parameter(self.name + '_elem', {'type': type_}, self._library)
                else:
                    raise LibGenConfigException('unknown element type \'{}\' for parameter \'{}\''.format(type_, self.name))
            else:
                raise LibGenConfigException('unknown parameter type \'{}\' for \'{}\''.format(dict_['type'], self.name))
        elif dict_['type'] in BYTE_ARRAY_TYPES and 'length_parameter' not in dict_:
            raise LibGenConfigException('parameter that holds length of byte array needs to be set for parameter \'{}\''.format(self.name))
        self._type = dict_['type']

        if 'length_parameter' in dict_:
            if not isinstance(dict_['length_parameter'], str):
                raise LibGenConfigException('parameter length of \'{}\' is not a string, so it cannot be a parameter name'.format(self.name))
            #TODO it would be best to also do a check if the specified length parameter actually exists, but that requires doing that after adding all parameters...
            self.length_parameter = dict_['length_parameter']

        if 'direction' in dict_:
            if dict_['direction'] not in DIRECTIONS:
                raise LibGenConfigException('invalid direction \'{}\' for parameter \'{}\' (possible: {})'.format(dict_['direction'], self.name, ', '.join(DIRECTIONS)))
            self.direction = dict_['direction']
            if self.is_output() and self.is_function_handle():
                raise LibGenConfigException('function handle \'{}\' cannot be an output parameter'.format(self.name))
            if self.is_output() and self.is_reference():
                raise LibGenConfigException('pointer to struct \'{}\' cannot be an output parameter (yet)'.format(self.name))
            if self.is_output() and self.is_byte_array() and self.is_const():
                raise LibGenConfigException('byte array \'{}\' cannot be an output parameter, as it is const'.format(self.name))

    def is_output(self) -> bool:
        return self.direction == 'out'

    def is_reference(self) -> bool:
        """pointer to a struct"""
        return self._reference is not None

    def is_const_reference(self) -> bool:
        return self._reference_const

    def is_allocated_reference(self) -> bool:
        """pointer to an instance of an allocatable struct, which is passed by handle"""
        return self.is_reference() and self._library.structs[self._reference].allocatable

    @property
    def referenced_struct(self) -> str:
        if self._reference is None:
            raise LibGenRuntimeException('referenced struct is requested from a parameter that is not a pointer to a struct')
        return self._reference

    def is_struct(self) -> bool:
        if self._type in self._library.structs:
            return True
        return False

    def is_array(self) -> bool:
        if self._element_parameter is not None:
            return True
        return False

    def is_function_handle(self) -> bool:
        if self._type in self._library.function_handles:
            return True
        return False

    def is_pointer_handle(self) -> bool:
        if self._type in self._library.pointer_handles:
            return True
        return False

    def is_byte_array(self) -> bool:
        if self._type in BYTE_ARRAY_TYPES:
            return True
        return False

    def is_const(self) -> bool:
        return self._type.startswith('const')
