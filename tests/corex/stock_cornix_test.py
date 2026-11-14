"""Stock 49-key layout and isolation from the CoreX trackball hardware."""
import json
import re
from pathlib import Path
from keymap_checks import validate
root = Path(__file__).resolve().parents[2]
s = (root/'config/Cornix.keymap').read_text()
assert len(validate(s, positions=49)) == 9
board_keymap = (root/'boards/jzf/cornix/cornix.keymap').read_text()
assert board_keymap.startswith('#if defined(CORNIX_STOCK_RIGHT)')
assert '#include "../../../config/Cornix.keymap"' in board_keymap
assert 'corex_listener' not in s and 'mouse_gesture' not in s
assert 'zip_temp_layer' not in s and 'paw3222' not in s
layout = (root/'boards/jzf/cornix/cornix-stock-layouts.dtsi').read_text()
assert len(re.findall(r'RC\(', layout.split('map = <')[1].split('>;')[0])) == 49
assert len(re.findall(r'&key_physical_attrs', layout)) == 49
assert not re.search(r'RC\(3,\s*10\)', layout)
assert 'display-name = "LAYOUT_49"' in layout
keys = json.loads((root/'config/Cornix.json').read_text())['layouts']['LAYOUT_49']['layout']
assert len(keys) == 49 and keys[46]['col'] == 11
assert not any(k['row'] == 3 and k['col'] == 10 for k in keys)
board = (root/'boards/jzf/cornix/cornix_right.dts').read_text()
assert '#define CORNIX_STOCK_RIGHT 1' in board
assert 'col-offset = <7>' in board and '&right_encoder' in board
assert 'control-gpios = <&gpio0 24 GPIO_ACTIVE_HIGH>' in board
print('Stock Cornix: 49 keys, layout metadata, right wiring and no CoreX input processors passed')
