from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.helpers import POINTER_HANDLE_MAX_ENTRIES_DEFAULT
from riot_libgen.library import Library


class Struct:
    def __init__(self, name, config: dict, factory, library: Library):
        self.fields = {}
        self.name = name
        self.type: str | None = None

        # instances of allocatable structs can be created and freed by the runtime, and live on the host in between
        self.allocatable = False
        self.new_function = 'new_' + name
        self.free_function = 'free_' + name
        self.max_entries = POINTER_HANDLE_MAX_ENTRIES_DEFAULT

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
                if self.fields[name].is_reference():
                    raise LibGenConfigException('pointers to structs are not supported as struct fields yet (\'{}\' of \'{}\')'.format(name, self.name))
        self.type = dict_['type']

        if 'allocatable' in dict_:
            allocatable = dict_['allocatable']
            if isinstance(allocatable, dict):
                self.allocatable = True
                self.new_function = allocatable.get('new', self.new_function)
                self.free_function = allocatable.get('free', self.free_function)
                self.max_entries = int(allocatable.get('max_entries', self.max_entries))
            elif isinstance(allocatable, bool):
                self.allocatable = allocatable
            else:
                raise LibGenConfigException('allocatable of struct \'{}\' needs to be a boolean or an object'.format(self.name))