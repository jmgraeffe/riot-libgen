from riot_libgen.allocatable import Allocatable
from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.library import Library


class Struct(Allocatable):
    kind = 'struct'

    def __init__(self, name, config: dict, factory, library: Library):
        self.fields = {}
        self.name = name
        self.type: str | None = None

        self._init_allocatable(name)

        self._factory = factory
        self._library = library

        self.load_config_from_dict(config)

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
           return

        if 'type' not in dict_:
            raise LibGenConfigException('missing type for struct \'{}\''.format(self.name))
        if 'fields' in dict_:
            for name, config in dict_['fields'].items():
                # shortcut for parameter type
                if isinstance(config, str):
                    config = {'type': config}
                self.fields[name] = self._factory.create_parameter(name, config, self._library)
                if self.fields[name].is_output():
                    raise LibGenConfigException('struct fields cannot be output parameters (\'{}\' of \'{}\')'.format(name, self.name))
        self.type = dict_['type']

        self._load_allocatable_from_dict(dict_)

    def check_fields(self):
        """checks that need all structs and arrays to be loaded"""
        for name, field in self.fields.items():
            if field.is_reference():
                field.check_reference()
                # instances are referenced by handles, a temporary copy would not outlive the call
                if not field.is_allocated_reference():
                    raise LibGenConfigException('pointer field \'{}\' of struct \'{}\' needs to point to an allocatable struct or array'.format(name, self.name))
