function js_libs_function_handle(name) {
    return name;
}

function hello_world() {
    io.puts("Hello World!");
}

function add(a, b) {
    return a + b;
}

function sub(a, b) {
    return a - b;
}

io.call_void_func(js_libs_function_handle("hello_world"));
io.call_sophisticated_func(js_libs_function_handle("add"), 2, 3, '+');
io.call_sophisticated_func(js_libs_function_handle("sub"), 42, 1337, '-');
