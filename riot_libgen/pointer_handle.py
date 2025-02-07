from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.helpers import POINTER_HANDLE_MAX_ENTRIES_DEFAULT
from riot_libgen.library import Library


class PointerHandle:
    def __init__(self, name, config: dict, factory, library: Library):
        self.type: str | None = None
        self.max_entries = POINTER_HANDLE_MAX_ENTRIES_DEFAULT
        
        self.name = name
        self._factory = factory
        self._library = library

        self.load_config_from_dict(config)

    def load_config_from_dict(self, dict_: dict):
        if dict_ is None:
           return

        if 'type' not in dict_:
            raise LibGenConfigException('missing pointer handle type for \'{}\''.format(self.name))
        self.type = dict_['type']

        if 'max_entries' in dict_:
            self.max_entries = int(dict_['max_entries'])
