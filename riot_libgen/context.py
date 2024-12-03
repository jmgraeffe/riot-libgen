class Context:
    def __init__(self):
        self.libraries = {}
        self.includes = []
        self.modules = []
        self.packages = []

    def add_includes(self, includes):
        for include in includes:
            if include not in self.includes:
                self.includes.append(include)

    def add_modules(self, modules):
        for module in modules:
            if module not in self.modules:
                self.modules.append(module)

    def add_packages(self, packages):
        for package in packages:
            if package not in self.packages:
                self.packages.append(package)

    def to_dict(self):
        return {
            'libraries': self.libraries,
            'includes': self.includes,
            'modules': self.modules,
            'packages': self.packages
        }