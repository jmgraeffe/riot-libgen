from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.helpers import NATIVE_TYPES, BYTE_ARRAY_TYPES
from riot_libgen.library import Library

class Parameter:
    def __init__(self, name, config: dict, factory, library: Library):
        self.type: str | None = None
        self.length_parameter : str | None = None

        self.name = name
        self._factory = factory
        self._context = factory.context
        self._library = library

        self.load_config_from_dict(config)

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
            raise LibGenConfigException('could not read config')

        if 'type' not in dict_:
            raise LibGenConfigException('missing parameter type for \'{}\''.format(self.name))
        elif dict_['type'] not in NATIVE_TYPES and dict_['type'] not in self._library.function_handles:
            raise LibGenConfigException('unknown parameter type \'{}\' for \'{}\''.format(dict_['type'], self.name))
        elif dict_['type'] in BYTE_ARRAY_TYPES and 'length_parameter' not in dict_:
            raise LibGenConfigException('parameter that holds length of byte array needs to be set for parameter \'{}\''.format(self.name))
        self.type = dict_['type']

        if 'length_parameter' in dict_:
            if not isinstance(dict_['length_parameter'], str):
                raise LibGenConfigException('parameter length of \'{}\' is not a string, so it cannot be a parameter name'.format(self.name))
            #TODO it would be best to also do a check if the specified length parameter actually exists, but that requires doing that after adding all parameters...
            self.length_parameter = dict_['length_parameter']

    def is_function_handle(self) -> bool:
        if self.type in self._library.function_handles:
            return True
        return False

    def is_byte_array(self) -> bool:
        if self.type in BYTE_ARRAY_TYPES:
            return True
        return False

    def is_const(self) -> bool:
        return self.type.startswith('const')
