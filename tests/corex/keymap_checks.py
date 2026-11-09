"""Formatting-independent checks; user-editable key assignments are not snapshots.

This is a small structural check, not a DTS compiler. Firmware CI validates
preprocessor expressions, binding parameter counts and behavior definitions.
"""
import re
import unittest

# Preserve quoted strings while removing both styles of DTS comments.
COMMENTS = re.compile(r'("(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\n]*)', re.S)
INTEGER = r'(?:0[xX][0-9a-fA-F]+|\d+)'
NUMBER = rf'(?:{INTEGER}|\(\s*{INTEGER}\s*\))'


def clean(source):
    return COMMENTS.sub(lambda m: m[0] if m[0].startswith('"') else ' ', source)


def number(value):
    value = value.strip('() \t\r\n')
    return int(value, 16 if value.lower().startswith('0x') else 10)


def body(source, opening):
    depth = 1
    for match in re.finditer(r'"(?:\\.|[^"\\])*"|[{}]', source[opening + 1:]):
        if match[0] == '{':
            depth += 1
        elif match[0] == '}':
            depth -= 1
            if depth == 0:
                return source[opening + 1:opening + 1 + match.start()]
    raise AssertionError('Unclosed keymap node')


def validate(source, positions=49):
    source = clean(source)
    node = re.search(r'\bkeymap\s*\{', source)
    assert node, 'Missing keymap node'
    keymap = body(source, node.end() - 1)
    layers = []
    offset = 0
    while match := re.search(r'\b([\w-]+)\s*\{', keymap[offset:]):
        opening = offset + match.end() - 1
        layer = body(keymap, opening)
        bindings = re.search(r'\bbindings\s*=\s*((?:<[^>]*>\s*,?\s*)+);', layer)
        if bindings:
            count = len(re.findall(r'&[\w-]+', bindings[1]))
            assert count == positions, f'{match[1]}: {count} bindings; expected {positions}'
            layers.append(match[1])
        offset = opening + len(layer) + 2
    assert layers, 'No keymap layers found'
    # Only literal indices are checked here; expressions are handled by DTS CI.
    for match in re.finditer(rf'&(?:mo|to|tog|sl|lt)\s+({NUMBER})(?=\s|[>;])', keymap):
        index = number(match[1])
        assert index < len(layers), f'Layer reference {index} exceeds {len(layers)} layers'
    for match in re.finditer(r'\b(?:layers|layer|mac-layer)\s*=\s*<([^>]+)>', source):
        values = re.findall(NUMBER, match[1])
        if re.fullmatch(rf'\s*(?:{NUMBER}\s*)+', match[1]):
            for value in values:
                index = number(value)
                assert index < len(layers), f'Processor layer {index} exceeds {len(layers)} layers'
    return layers


class EditorCompatibilityTests(unittest.TestCase):
    source = '''/ { keymap { base { bindings = <&kp SPACE &lt 1 A>; };
        extra { bindings = <&trans &kp B>; }; }; };
        &listener { layers = <1>; };'''

    def test_editor_formatting(self):
        variants = [self.source,
                    self.source.replace(' ', '\n\t'),
                    self.source.replace('1', '( 1 )'),
                    self.source.replace('1', '0x1'),
                    self.source.replace('bindings', '/* } < &fake */ bindings'),
                    self.source.replace('&kp SPACE', '&mt LSHIFT SPACE'),
                    self.source.replace('base', 'renamed'),
                    self.source.replace('<&trans &kp B>', '<&kp C>, <&trans>')]
        for source in variants:
            with self.subTest(source=source):
                self.assertEqual(len(validate(source, positions=2)), 2)

    def test_missing_key_is_rejected(self):
        with self.assertRaisesRegex(AssertionError, 'bindings'):
            validate(self.source.replace('&trans ', ''), positions=2)

    def test_bad_layer_is_rejected(self):
        for source in [self.source.replace('&lt 1', '&lt (2)'),
                       self.source.replace('layers = <1>', 'layers = <(2)>')]:
            with self.assertRaisesRegex(AssertionError, 'exceeds'):
                validate(source, positions=2)


if __name__ == '__main__':
    unittest.main()
