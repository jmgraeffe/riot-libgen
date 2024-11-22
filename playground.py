from pathlib import Path

import riot_libgen

libgen = riot_libgen.WasmLibGen()
libgen.load_config_from_file('examples/wasm/playground/wasm_playground.libgen-config.yml')
libgen.render(Path('examples/wasm/playground/external_modules/wasm_libs'))

#libgen = riot_libgen.JsLibGen()
#libgen.load_config_from_file('examples/js/playground/js_playground.libgen-config.yml')
#libgen.render(Path('examples/js/playground/external_modules/js_libs'))
