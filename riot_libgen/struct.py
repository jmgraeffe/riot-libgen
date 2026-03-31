from riot_libgen.library import Library


class Struct:
    def __init__(self, name, config: dict, factory, library: Library):
        self.fields = {}
        self.name = name
        self.type: str | None = None

        self._factory = factory
        self._library = library

        self.load_config_from_dict(config)

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
           return

        if 'type' not in dict_:
            raise LibGenConfigException('missing parameter type for \'{}\''.format(self.name))
        if 'fields' in dict_:
            for name, config in dict_['fields'].items():
                # shortcut for parameter type
                if isinstance(config, str):
                    config = {'type': config}
                self.fields[name] = self._factory.create_parameter(name, config, self._library)
        self.type = dict_['type']