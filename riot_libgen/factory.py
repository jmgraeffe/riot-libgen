from riot_libgen.context import Context
from riot_libgen.function import Function
from riot_libgen.function_handle import FunctionHandle
from riot_libgen.pointer_handle import PointerHandle
from riot_libgen.library import Library
from riot_libgen.parameter import Parameter


class Factory:
    def __init__(self, context: Context):
        self.context = context

    def create_library(self, name: str, config: dict):
        return Library(name, config, self)

    def create_function(self, name: str, config: dict, library: Library):
        return Function(name, config, self, library)

    def create_parameter(self, name: str, config: dict, library: Library):
        return Parameter(name, config, self, library)

    def create_function_handle(self, name: str, config: dict, library: Library):
        return FunctionHandle(name, config, self, library)

    def create_pointer_handle(self, name: str, config: dict, library: Library):
        return PointerHandle(name, config, self, library)
