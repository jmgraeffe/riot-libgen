import re
from riot_libgen.allocatable import Allocatable
from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.helpers import ARRAY_PATTERN
from riot_libgen.library import Library


class NamedArray(Allocatable):
    """
    Array with a name, a homogeneous element type and a static size (e.g. a buffer), which can be referenced by
    parameters as pointer (e.g. "buffer*" or "const buffer*").
    """

    kind = 'array'

    def __init__(self, name, config: dict, factory, library: Library):
        self.name = name
        self.type: str | None = None
        # parameter with the type of the array (e.g. "uint8_t[64]"), used for conversions
        self.parameter = None

        self._init_allocatable(name)

        self._factory = factory
        self._library = library

        self.load_config_from_dict(config)

    @property
    def size(self) -> int:
        return self.parameter.array_size

    @property
    def element(self):
        return self.parameter._element_parameter

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
            raise LibGenConfigException('could not read config of array \'{}\''.format(self.name))

        if 'type' not in dict_:
            raise LibGenConfigException('missing type for array \'{}\''.format(self.name))
        if not isinstance(dict_['type'], str) or not re.match(ARRAY_PATTERN, dict_['type']):
            raise LibGenConfigException('type of array \'{}\' needs to be an array type with static size (e.g. uint8_t[64])'.format(self.name))
        self.type = dict_['type']
        self.parameter = self._factory.create_parameter(self.name, {'type': self.type}, self._library)
        # elements live on the host, so they cannot refer to functions of the runtime
        if self.element.is_function_handle():
            raise LibGenConfigException('function handles are not supported as elements of array \'{}\''.format(self.name))

        self._load_allocatable_from_dict(dict_)
