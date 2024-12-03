from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.helpers import NATIVE_TYPES
from riot_libgen.library import Library


class Function:
    def __init__(self, name, config: dict, factory, library: Library):
        self.parameters = {}
        self.return_type = 'void'
        self.original_name: str | None = None

        self.name = name
        self._factory = factory
        self._context = factory.context
        self._library = library

        self.load_config_from_dict(config)

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
            raise LibGenConfigException('could not read config')

        if 'original_name' not in dict_:
            raise LibGenConfigException('missing original function name for \'{}\''.format(self.name))
        self.original_name = dict_['original_name']

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

        if 'includes' in dict_:
            self._context.add_includes(dict_['includes'])

        if 'modules' in dict_:
            self._context.add_modules(dict_['modules'])

        if 'packages' in dict_:
            self._context.add_packages(dict_['packages'])
