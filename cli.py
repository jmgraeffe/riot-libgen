from pathlib import Path
from argparse import ArgumentParser

import riot_libgen

#TODO error handling (config exists etc.)

if __name__ == '__main__':
	parser = ArgumentParser(description="Command-Line Interface of the RIOT OS Library Generator")
	parser.add_argument('-c', '--config', type=str, required=True, nargs='+', help='Path(s) of the YAML specification file')
	parser.add_argument('-d', '--dest', type=str, required=True, help='Path to generate the RIOT OS module to')
	parser.add_argument('-g', '--generator', choices=['wasm', 'js'], required=True, help='Generator to use (currently supported: "wasm", "js")')
	args = parser.parse_args()

	libgen = None
	if args.generator == 'wasm':
		libgen = riot_libgen.WasmLibGen()
	elif args.generator == 'js':
		libgen = riot_libgen.JsLibGen()
	else:
		print('"{}" is not a supported generator. Currently supported: "wasm", "js"')
		exit(1)
	for config in args.config:
		print(config)
		libgen.load_config_from_file(config)
	libgen.render(Path(args.dest))
