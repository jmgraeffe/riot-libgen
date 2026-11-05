"""Tests for the JSON schema of configuration files (riot-libgen.schema.json), as used by cli.py --validate."""
import json
from pathlib import Path

import jsonschema
import pytest
import yaml

REPO_DIR = Path(__file__).resolve().parents[1]

with open(REPO_DIR / 'riot-libgen.schema.json') as file:
    SCHEMA = json.load(file)

CONFIG_FILES = sorted(
    path.relative_to(REPO_DIR)
    for directory in ['configs', 'examples', 'external', 'tests']
    for path in (REPO_DIR / directory).rglob('*.libgen-config.yml')
    if 'RIOT' not in path.parts
)

KNOWN_INVALID = {
}


def errors(instance) -> list[str]:
    return [error.message for error in jsonschema.Draft202012Validator(SCHEMA).iter_errors(instance)]


def _config_params():
    for path in CONFIG_FILES:
        marks = [pytest.mark.xfail(reason=KNOWN_INVALID[str(path)])] if str(path) in KNOWN_INVALID else []
        yield pytest.param(path, id=str(path), marks=marks)


def test_schema_is_valid():
    jsonschema.Draft202012Validator.check_schema(SCHEMA)


@pytest.mark.parametrize('path', list(_config_params()))
def test_config_files_are_valid(path):
    with open(REPO_DIR / path) as file:
        assert errors(yaml.safe_load(file)) == []


def test_function_shortcuts():
    assert errors({'libraries': {'mylib': {'functions': {
        'with_return_type': 'int',
        'without_anything': None,
    }}}}) == []


def test_function_shortcut_needs_to_be_a_return_type():
    assert errors({'libraries': {'mylib': {'functions': {'my_func': 42}}}}) != []


def test_pointer_handle_object():
    assert errors({'libraries': {'mylib': {'pointer_handles': {
        'my_ptr': {'type': 'thing_t*', 'max_entries': 3},
    }}}}) == []


@pytest.mark.parametrize('pointer_handle', [
    {'max_entries': 3},                                  # missing type
    {'type': 'thing_t*', 'max_entries': 0},              # at least one entry
    {'type': 'thing_t*', 'max_entries': '3'},            # not an integer
    {'type': 'thing_t*', 'unknown': 1},                  # unknown property
])
def test_pointer_handle_object_invalid(pointer_handle):
    assert errors({'libraries': {'mylib': {'pointer_handles': {'my_ptr': pointer_handle}}}}) != []


def test_struct():
    assert errors({'libraries': {'mylib': {'structs': {
        'my_struct': {'type': 'my_struct_t', 'fields': {'val': 'int16_t[3]', 'unit': 'uint8_t', 'x': 'double'}},
        'empty_struct': {'type': 'empty_t'},
    }}}}) == []


@pytest.mark.parametrize('struct', [
    {'fields': {'xyz': 'int'}},                          # missing type
    {'type': 'my_struct_t', 'fields': {'xyz': 42}},      # field type is not a string
    {'type': 'my_struct_t', 'fields': {'Xyz': 'int'}},   # invalid field name
    {'type': 'my_struct_t', 'unknown': 1},               # unknown property
])
def test_struct_invalid(struct):
    assert errors({'libraries': {'mylib': {'structs': {'my_struct': struct}}}}) != []


def test_library_prefix():
    assert errors({'libraries': {'mylib': {'prefix': 'my', 'functions': {'my_func': None}}}}) == []


def test_library_unknown_property():
    assert errors({'libraries': {'mylib': {'struct': {}}}}) != []


def test_constants():
    assert errors({'libraries': {'mylib': {'constants': {
        'answer': 42,
        'pi_ish': 3.14,
        'greeting': 'hello',
    }}}}) == []


def test_parameter_direction():
    assert errors({'libraries': {'mylib': {'functions': {'my_func': {'parameters': {
        'aaa': {'type': 'int', 'direction': 'in'},
        'bbb': {'type': 'int', 'direction': 'out'},
    }}}}}}) == []


def test_parameter_direction_invalid():
    assert errors({'libraries': {'mylib': {'functions': {'my_func': {'parameters': {
        'aaa': {'type': 'int', 'direction': 'up'},
    }}}}}}) != []


def test_allocatable_structs():
    assert errors({'libraries': {'mylib': {'structs': {
        'aaa': {'type': 'a_t', 'allocatable': True},
        'bbb': {'type': 'b_t', 'allocatable': {'new': 'create_b', 'free': 'destroy_b', 'max_entries': 3}},
    }}}}) == []


@pytest.mark.parametrize('allocatable', ['yes', {'new': 3}, {'max_entries': 0}, {'unknown': 1}])
def test_allocatable_structs_invalid(allocatable):
    assert errors({'libraries': {'mylib': {'structs': {'aaa': {'type': 'a_t', 'allocatable': allocatable}}}}}) != []


def test_arrays():
    assert errors({'libraries': {'mylib': {'arrays': {
        'aaa': 'uint8_t[4]',
        'bbb': {'type': 'int[3]'},
        'ccc': {'type': 'uint8_t[64]', 'allocatable': {'new': 'create_c', 'free': 'destroy_c', 'max_entries': 3}},
    }}}}) == []


@pytest.mark.parametrize('array', ['uint8_t', 'uint8_t[]', {'allocatable': True}, {'type': 'int[3]', 'unknown': 1},
                                   {'type': 'int[3]', 'allocatable': {'max_entries': 0}}])
def test_arrays_invalid(array):
    assert errors({'libraries': {'mylib': {'arrays': {'aaa': array}}}}) != []
