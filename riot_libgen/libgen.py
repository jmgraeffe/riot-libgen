from pathlib import Path
from jinja2 import Environment, FileSystemLoader
import yaml

from riot_libgen.context import Context
from riot_libgen.exceptions import LibGenConfigException, RaiseExtension
from riot_libgen.factory import Factory

try:
    from yaml import CLoader as YamlLoader, CDumper as YamlDumper
except ImportError:
    from yaml import YamlLoader, YamlDumper


class LibGen:
    def __init__(self, config: dict = None):
        self._context = Context()
        self._factory = Factory(self._context)
        self._template_path: Path|None = None

        if config is not None:
            self.load_config_from_dict(config)

    def load_config_from_file(self, file_path: str):
        config = None
        with open(file_path, 'r') as stream:
            config = yaml.load(stream, Loader=YamlLoader)
        if config is None:
            raise LibGenConfigException('could not read config file')

        self.load_config_from_dict(config)

    def load_config_from_dict(self, dict_: dict):
        if 'function_handles' in dict_:
            for name, config in dict_['function_handles'].items():
                self._context.function_handles[name] = self._factory.create_function_handle(name, config)

        if 'libraries' not in dict_:
            raise LibGenConfigException('invalid config file format (no \'libraries\' tag)')

        for name, config in dict_['libraries'].items():
            self._context.libraries[name] = self._factory.create_library(name, config)

    def render(self, output_path: Path):
        self._render_all_templates_and_copy_remaining_files_in_directory(output_path, self._context.to_dict())

    def _render_all_templates_and_copy_remaining_files_in_directory(self, output_path: Path, variables: dict):
        output_path.mkdir(exist_ok=True, parents=True)

        module_template_path = self._template_path.joinpath(Path('module'))

        env = Environment(loader=FileSystemLoader(self._template_path), extensions=[RaiseExtension])

        for entity_path in module_template_path.rglob('*'):
            rel_path = entity_path.relative_to(module_template_path)
            out_path = output_path.joinpath(rel_path)

            # copy directory if not existing
            if entity_path.is_dir():
                out_path.mkdir(exist_ok=True)  # TODO check permissions
                continue
            elif entity_path.is_file() and entity_path.suffix != '.tmpl':
                out_path.write_bytes(entity_path.read_bytes())
                continue

            template = env.get_template('module/' + str(rel_path))
            bytes_ = template.render(variables).encode('utf-8')
            out_path.with_suffix('').write_bytes(bytes_)
