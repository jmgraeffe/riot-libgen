from riot_libgen.context import Context
from riot_libgen.function import Function
from riot_libgen.function_handle import FunctionHandle
from riot_libgen.library import Library
from riot_libgen.parameter import Parameter


class Factory:
    def __init__(self, context: Context):
        self.context = context

    def create_library(self, name: str, config: dict = None):
        return Library(name, config, self)

    def create_function(self, name: str, config: dict = None):
        return Function(name, config, self)

    def create_parameter(self, name: str, config: dict = None):
        return Parameter(name, config, self)

    def create_function_handle(self, name: str, config: dict = None):
        return FunctionHandle(name, config, self)
