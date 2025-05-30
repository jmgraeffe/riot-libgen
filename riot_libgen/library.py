from riot_libgen.exceptions import LibGenConfigException


class Library:
    def __init__(self, name, config: dict, factory):
        self.functions = {}
        self.function_handles = {}
        self.constants = {}
        self.pointer_handles = {}

        self.name = name
        self.prefix = None
        self._factory = factory
        self._context = factory.context

        self.load_config_from_dict(config)

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
            raise LibGenConfigException('could not read config')

        if 'prefix' in dict_:
            self.prefix = dict_['prefix']

        if 'function_handles' in dict_:
            for name, config in dict_['function_handles'].items():
                self.function_handles[name] = self._factory.create_function_handle(name, config, self)

        if 'pointer_handles' in dict_:
            for name, config in dict_['pointer_handles'].items():
                # shortcut for type
                if isinstance(config, str):
                    config = {'type': config}
                self.pointer_handles[name] = self._factory.create_pointer_handle(name, config, self)

        if 'functions' in dict_:
            for name, config in dict_['functions'].items():
                # shortcut for original name
                if isinstance(config, str):
                    config = {'original_name': config}
                self.functions[name] = self._factory.create_function(name, config, self)

        if 'constants' in dict_:
            for name, value in dict_['constants'].items():
                self.constants[name] = value

        if 'includes' in dict_:
            self._context.add_includes(dict_['includes'])

        if 'modules' in dict_:
            self._context.add_modules(dict_['modules'])

        if 'packages' in dict_:
            self._context.add_packages(dict_['packages'])

    def get_constant_identifier(self, name: str) -> str:
        if self.prefix is not None:
            return self.prefix.upper() + name.upper()
        else:
            return name.upper()
