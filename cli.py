from pathlib import Path
from argparse import ArgumentParser

import riot_libgen

#TODO error handling (config exists etc.)

if __name__ == '__main__':
	parser = ArgumentParser(description="Command-Line Interface of the RIOT OS Library Generator")
	parser.add_argument('-c', '--config', type=str, required=True, nargs='+', help='Path(s) of the YAML specification file')
	parser.add_argument('-d', '--destination', type=str, required=False, help='Path to generate the RIOT OS module to')
	parser.add_argument('-g', '--generator', choices=['wasm', 'js', None], required=False, default=None, help='Generator to use (currently supported: "wasm", "js")')
	parser.add_argument('-a', '--validate', action='store_true', required=False, help='Wether to validate configuration files beforehand (or only validate configs if no generator and destination is given)')
	args = parser.parse_args()

	schema = None
	if args.validate is True:
		import json
		import jsonschema
		import yaml
		try:
		    from yaml import CLoader as YamlLoader, CDumper as YamlDumper
		except ImportError:
		    from yaml import YamlLoader, YamlDumper

		with open('riot-libgen.schema.json') as file:
			schema = json.load(file)

	libgen = None
	if args.generator is not None:
		if args.destination is None:
			print('You need to specify a destination for the generated code with -d "<path>" or "--destination <path>".')
			exit(1)
		if args.generator == 'wasm':
			libgen = riot_libgen.WasmLibGen()
		elif args.generator == 'js':
			libgen = riot_libgen.JsLibGen()
		else:
			print('"{}" is not a supported generator. Currently supported: "wasm", "js"')
			exit(1)
	elif args.destination is not None and args.validate is False:
		print('You need to specify a generator with "-g <generator>" or "--generator <generator>". Currently supported: "wasm", "js"')
		exit(1)

	for config in args.config:
		print('loading configuration file "{}"...'.format(config))

		if args.validate is True:
			instance = None
			with open(config, 'r') as file:
				instance = yaml.load(file, Loader=YamlLoader)
			jsonschema.validate(instance=instance, schema=schema)
			print('validation successful')

		if libgen is not None:
			libgen.load_config_from_file(config)
			print('loaded successfully')

	if libgen is not None:
		print('generating code...')
		libgen.render(Path(args.destination))
		print('generation done')
