from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.helpers import NATIVE_TYPES, BYTE_ARRAY_TYPES, POINTER_HANDLE_TYPE
from riot_libgen.library import Library


class Function:
    def __init__(self, name, config: dict, factory, library: Library):
        self.parameters = {}
        self.original_name = name

        self.name = name
        self._factory = factory
        self._context = factory.context
        self._library = library
        self._return_type = 'void'

        self.load_config_from_dict(config)

    @property
    def return_type(self):
        if self._return_type in self._library.pointer_handles:
            return POINTER_HANDLE_TYPE
        else:
            return self._return_type

    @property
    def returned_pointer_handle(self) -> str:
        return self._return_type

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
            raise LibGenConfigException('could not read config')

        if 'original_name' in dict_:
            self.original_name = dict_['original_name']

        if 'return_type' in dict_:
            if dict_['return_type'] not in NATIVE_TYPES and dict_['return_type'] not in self._library.pointer_handles:
                raise LibGenConfigException('unknown return type \'{}\' for \'{}\''.format(dict_['return_type'], self.name))
            self._return_type = dict_['return_type']

        if 'parameters' in dict_:
            for name, config in dict_['parameters'].items():
                # shortcut for parameter type
                if isinstance(config, str):
                    config = {'type': config}
                self.parameters[name] = self._factory.create_parameter(name, config, self._library)

        if 'includes' in dict_:
            self._context.add_includes(dict_['includes'])

        if 'modules' in dict_:
            self._context.add_modules(dict_['modules'])

        if 'packages' in dict_:
            self._context.add_packages(dict_['packages'])

    def returns_byte_array(self) -> bool:
        if self._return_type in BYTE_ARRAY_TYPES:
            return True
        return False

    def returns_pointer_handle(self) -> bool:
        if self._return_type in self._library.pointer_handles:
            return True
        return False