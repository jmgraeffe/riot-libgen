from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.helpers import NATIVE_TYPES


class Parameter:
    def __init__(self, name, config: dict, factory):
        self.type: str | None = None

        self.name = name
        self._factory = factory
        self._context = factory.context

        self.load_config_from_dict(config)

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
            raise LibGenConfigException('could not read config')

        if 'type' not in dict_:
            raise LibGenConfigException('missing parameter type for \'{}\''.format(self.name))
        if dict_['type'] not in NATIVE_TYPES and dict_['type'] not in self._factory.context.function_handles:
            raise LibGenConfigException('unknown parameter type \'{}\' for \'{}\''.format(dict_['type'], self.name))
        self.type = dict_['type']

    def is_function_handle(self) -> bool:
        if self.type in self._context.function_handles:
            return True
        return False
