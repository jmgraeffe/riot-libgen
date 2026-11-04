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
    for directory in ['examples', 'external', 'tests']
    for path in (REPO_DIR / directory).rglob('*.libgen-config.yml')
    if 'RIOT' not in path.parts
)

KNOWN_INVALID = {
    'tests/apps/modules/libgen_tests/libgen_tests.libgen-config.yml':
        'BUG: the schema does not allow the object form of pointer handles (with max_entries)',
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


@pytest.mark.xfail(reason='BUG: the schema does not allow the object form of pointer handles (with max_entries)')
def test_pointer_handle_object():
    assert errors({'libraries': {'mylib': {'pointer_handles': {
        'my_ptr': {'type': 'thing_t*', 'max_entries': 3},
    }}}}) == []


@pytest.mark.xfail(reason='the schema does not describe structs yet (they are accepted without being checked)')
def test_struct_needs_type():
    assert errors({'libraries': {'mylib': {'structs': {
        'my_struct': {'fields': {'xyz': 'int'}},
    }}}}) != []
