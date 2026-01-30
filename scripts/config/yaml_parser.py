#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0
#
# Lightweight YAML Parser
#
# A minimal YAML parser for Codezero container manifests.
# No external dependencies required.
#
# Supports:
#   - Key-value pairs
#   - Nested dictionaries (indentation-based)
#   - Lists (with - prefix)
#   - Comments (# prefix)
#   - Strings (quoted or unquoted)
#   - Integers (decimal and hex)
#   - Booleans (true/false/yes/no)
#

import re
from typing import Any, Dict, List, Union


class YAMLParseError(Exception):
    """YAML parsing error with line context."""

    def __init__(self, message: str, line_num: int = 0, line: str = ""):
        self.line_num = line_num
        self.line = line
        full_msg = f"Line {line_num}: {message}"
        if line:
            full_msg += f"\n  {line}"
        super().__init__(full_msg)


def parse_value(value_str: str) -> Any:
    """Parse a YAML value string into Python type."""
    value_str = value_str.strip()

    if not value_str:
        return None

    # Remove inline comments
    if " #" in value_str:
        # But not inside quotes
        if not (value_str.startswith('"') or value_str.startswith("'")):
            value_str = value_str.split(" #")[0].strip()

    # Empty inline list
    if value_str == "[]":
        return []

    # Empty inline dict
    if value_str == "{}":
        return {}

    # Inline list: [a, b, c]
    if value_str.startswith("[") and value_str.endswith("]"):
        inner = value_str[1:-1].strip()
        if not inner:
            return []
        # Simple inline list parsing (no nested structures)
        items = []
        for item in inner.split(","):
            items.append(parse_value(item.strip()))
        return items

    # Quoted string
    if (value_str.startswith('"') and value_str.endswith('"')) or (
        value_str.startswith("'") and value_str.endswith("'")
    ):
        return value_str[1:-1]

    # Boolean
    lower = value_str.lower()
    if lower in ("true", "yes", "on"):
        return True
    if lower in ("false", "no", "off"):
        return False

    # Null
    if lower in ("null", "~", ""):
        return None

    # Hex integer
    if value_str.startswith("0x") or value_str.startswith("0X"):
        try:
            return int(value_str, 16)
        except ValueError:
            pass

    # Decimal integer
    try:
        return int(value_str)
    except ValueError:
        pass

    # Float
    try:
        return float(value_str)
    except ValueError:
        pass

    # Unquoted string
    return value_str


def get_indent(line: str) -> int:
    """Get indentation level (number of leading spaces)."""
    return len(line) - len(line.lstrip())


def parse_yaml(content: str) -> Dict[str, Any]:
    """
    Parse YAML content into a Python dict.

    Args:
        content: YAML string content

    Returns:
        Parsed dictionary

    Raises:
        YAMLParseError: On parsing failure
    """
    lines = content.split("\n")
    return _parse_block(lines, 0, 0)[0]


def _parse_block(lines: List[str], start_idx: int, base_indent: int) -> tuple:
    """
    Parse a YAML block at given indentation level.

    Returns (parsed_value, next_line_index)
    """
    result = {}
    current_list = None
    current_key = None
    i = start_idx

    while i < len(lines):
        line = lines[i]
        line_num = i + 1

        # Skip empty lines and comments
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            i += 1
            continue

        indent = get_indent(line)

        # End of this block
        if indent < base_indent:
            break

        # List item
        if stripped.startswith("- "):
            if current_list is None:
                current_list = []
                if current_key:
                    result[current_key] = current_list

            item_content = stripped[2:].strip()

            # Check if item is a nested dict
            if ":" in item_content:
                # Inline dict start: "- key: value"
                item_dict = {}
                key, _, val = item_content.partition(":")
                key = key.strip()
                val = val.strip()

                if val:
                    item_dict[key] = parse_value(val)
                else:
                    # Nested block under list item
                    nested, next_i = _parse_block(lines, i + 1, indent + 2)
                    item_dict[key] = nested
                    i = next_i
                    current_list.append(item_dict)
                    continue

                # Check for more keys at same level
                j = i + 1
                while j < len(lines):
                    next_line = lines[j]
                    next_stripped = next_line.strip()
                    if not next_stripped or next_stripped.startswith("#"):
                        j += 1
                        continue
                    next_indent = get_indent(next_line)
                    if next_indent <= indent:
                        break
                    if ":" in next_stripped and not next_stripped.startswith("- "):
                        k, _, v = next_stripped.partition(":")
                        k = k.strip()
                        v = v.strip()
                        if v:
                            item_dict[k] = parse_value(v)
                        else:
                            nested, next_j = _parse_block(lines, j + 1, next_indent + 2)
                            item_dict[k] = nested
                            j = next_j
                            continue
                    j += 1
                current_list.append(item_dict)
                i = j
                continue
            else:
                # Simple list item
                current_list.append(parse_value(item_content))
            i += 1
            continue

        # Key-value pair
        if ":" in stripped:
            current_list = None
            key, _, value = stripped.partition(":")
            key = key.strip()
            value = value.strip()

            if value:
                # Inline value
                result[key] = parse_value(value)
                current_key = key
            else:
                # Check next line for nested content
                current_key = key
                if i + 1 < len(lines):
                    next_line = lines[i + 1]
                    next_stripped = next_line.strip()
                    if next_stripped and not next_stripped.startswith("#"):
                        next_indent = get_indent(next_line)
                        if next_indent > indent:
                            if next_stripped.startswith("- "):
                                # List follows
                                current_list = []
                                result[key] = current_list
                            else:
                                # Nested dict follows
                                nested, next_i = _parse_block(lines, i + 1, next_indent)
                                result[key] = nested
                                i = next_i
                                continue
                        else:
                            result[key] = None
                    else:
                        result[key] = None
                else:
                    result[key] = None

            i += 1
            continue

        # Unhandled line
        raise YAMLParseError(f"Unexpected content", line_num, stripped)

    return result, i


def load(file_path: str) -> Dict[str, Any]:
    """
    Load and parse a YAML file.

    Args:
        file_path: Path to YAML file

    Returns:
        Parsed dictionary
    """
    with open(file_path, "r") as f:
        content = f.read()
    return parse_yaml(content)


def loads(content: str) -> Dict[str, Any]:
    """
    Parse YAML string.

    Args:
        content: YAML string

    Returns:
        Parsed dictionary
    """
    return parse_yaml(content)


# Compatibility alias
safe_load = loads


if __name__ == "__main__":
    # Test with sample YAML
    test_yaml = """
# Test YAML
schemaVersion: 1
kconfig: pb926

containers:
  - id: 0
    name: "hello_world0"
    type: baremetal
    project: hello_world

    pager:
      lma: 0x100000
      vma: 0xa0000000

    memory:
      physical:
        - start: 0x100000
          end: 0xe00000

      virtual:
        - start: 0xa0000000
          end: 0xb0000000

    capabilities:
      pools:
        thread:
          enabled: true
          size: 64
        space:
          enabled: true
          size: 64
        mutex:
          enabled: true
          size: 100
        map:
          enabled: true
          size: 800
        cap:
          enabled: true
          size: 32

      tctrl:
        enabled: true
        target: current_container

    devices: []
"""

    import json

    result = parse_yaml(test_yaml)
    print(json.dumps(result, indent=2))
