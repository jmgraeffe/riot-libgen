from riot_libgen.exceptions import LibGenConfigException
from riot_libgen.helpers import POINTER_HANDLE_MAX_ENTRIES_DEFAULT


class Allocatable:
    """
    Structs and named arrays can be allocatable: instances can be created and freed by the runtime, and live on the host
    in between.
    """

    # used in messages, e.g. "struct" or "array"
    kind = None

    def _init_allocatable(self, name: str):
        self.allocatable = False
        self.new_function = 'new_' + name
        self.free_function = 'free_' + name
        self.max_entries = POINTER_HANDLE_MAX_ENTRIES_DEFAULT

    def _load_allocatable_from_dict(self, dict_: dict):
        if 'allocatable' not in dict_:
            return

        allocatable = dict_['allocatable']
        if isinstance(allocatable, dict):
            self.allocatable = True
            self.new_function = allocatable.get('new', self.new_function)
            self.free_function = allocatable.get('free', self.free_function)
            self.max_entries = int(allocatable.get('max_entries', self.max_entries))
        elif isinstance(allocatable, bool):
            self.allocatable = allocatable
        else:
            raise LibGenConfigException('allocatable of {} \'{}\' needs to be a boolean or an object'.format(self.kind, self.name))

    def is_struct(self) -> bool:
        return self.kind == 'struct'

    def is_array(self) -> bool:
        return self.kind == 'array'
