from riot_libgen.generators.wasm import WasmLibGen
from riot_libgen.generators.js import JsLibGen

"""
potentials for optimizations:
- add safety & sanity checks for almost anything
- add duplicate warnings/errors
- derive function handle and pointer handles from parameters and use polymorphism instead of string references (currently, function and pointer handles are more or less just a configuration representation)
- extract lots of decision making from templates into Python code (e.g. in WASM the big if-else-constructs could be done in Python instead, would make readability of templates a lot better)
- allow function handles as return types
- split module template files into multiple files (use includes, import macros)
- prefix all local variables in generated code with "libgen_" (e.g. args, results, func, name, global_object, res_value,
  value_<param>, length_<param>, <param>_ptr, <param>_is_null), and reject parameter names starting with "libgen_" in
  the config loader, so that parameter names cannot collide with generated names (so far only the result variable of
  native function wrappers is called libgen_res)
"""