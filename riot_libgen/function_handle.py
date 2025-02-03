from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.helpers import NATIVE_TYPES, BYTE_ARRAY_TYPES
from riot_libgen.library import Library


class FunctionHandle:
    def __init__(self, name, config: dict, factory, library: Library):
        self.parameters = {}
        self.return_type = 'void'

        self.name = name
        self._factory = factory
        self._library = library

        self.load_config_from_dict(config)

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
           return

        if 'return_type' in dict_:
            if dict_['return_type'] not in NATIVE_TYPES:
                raise LibGenConfigException('unknown parameter type \'{}\' for \'{}\''.format(dict_['return_type'], self.name))
            self.return_type = dict_['return_type']

        if 'parameters' in dict_:
            for name, config in dict_['parameters'].items():
                # shortcut for parameter type
                if isinstance(config, str):
                    config = {'type': config}
                self.parameters[name] = self._factory.create_parameter(name, config, self._library)

    def returns_byte_array(self) -> bool:
        if self.return_type in BYTE_ARRAY_TYPES:
            return True
        return False
