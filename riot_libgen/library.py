from riot_libgen.exceptions import LibGenConfigException


class Library:
    def __init__(self, name, config: dict, factory):
        self.functions = {}

        self.name = name
        self._factory = factory
        self._context = factory.context

        self.load_config_from_dict(config)

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
            raise LibGenConfigException('could not read config')

        if 'function_handles' in dict_:
            for name, config in dict_['function_handles'].items():
                if name in self._context.function_handles:
                    raise LibGenConfigException('function handle with duplicate name found: \'{}\''.format(name))
                self._context.function_handles[name] = self._factory.create_function_handle(name, config)

        if 'functions' in dict_:
            for name, config in dict_['functions'].items():
                # shortcut for original name
                if isinstance(config, str):
                    config = {'original_name': config}
                self.functions[name] = self._factory.create_function(name, config)

        if 'includes' in dict_:
            self._context.add_includes(dict_['includes'])

        if 'modules' in dict_:
            self._context.add_modules(dict_['modules'])

        if 'packages' in dict_:
            self._context.add_packages(dict_['packages'])
