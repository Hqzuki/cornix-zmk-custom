"""Exercise the actual processor with host stubs and check layer references."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    for name in ['drivers/input_processor.h', 'zmk/keymap.h']:
        path = tmp / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('')
    source = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define INPUT_EV_REL 2
#define INPUT_REL_HWHEEL 6
#define INPUT_REL_WHEEL 8
#define ZMK_INPUT_PROC_CONTINUE 0
#define DT_INST_FOREACH_STATUS_OKAY(fn)
struct device { const void *config; };
struct input_event { int type, code, value; };
struct zmk_input_processor_state { int unused; };
struct zmk_input_processor_driver_api {
 int (*handle_event)(const struct device *, struct input_event *, uint32_t,
                     uint32_t, struct zmk_input_processor_state *);
};
static bool mac;
static int zmk_keymap_layer_index_to_id(int index) { return index + 10; }
static bool zmk_keymap_layer_active(int id) { assert(id == 11); return mac; }
'''
    source += '\n#include "' + str(root / 'drivers/keymap/pan_direction.c') + '"\n'
    source += r'''
int main(void) {
 const uint8_t layer = 1;
 struct device dev = {.config = &layer};
 for (int os = 0; os < 2; os++) {
  mac = os;
  for (int value = -12; value <= 12; value++) {
   for (int axis = 0; axis < 3; axis++) {
    struct input_event e = {INPUT_EV_REL,
      axis == 0 ? INPUT_REL_HWHEEL : axis == 1 ? INPUT_REL_WHEEL : 0, value};
    assert(pan_direction_api.handle_event(&dev, &e, 0, 0, NULL) == 0);
    assert(e.value == (os && axis == 0 ? -value : value));
   }
  }
 }
 struct input_event e = {1, INPUT_REL_HWHEEL, 5};
 pan_direction_api.handle_event(&dev, &e, 0, 0, NULL);
 assert(e.value == 5);
}
'''
    (tmp / 'test.c').write_text(source)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-parameter', '-I', str(tmp), str(tmp / 'test.c'),
                    '-o', str(tmp / 'test')], check=True)
    subprocess.run([str(tmp / 'test')], check=True)

s = (root / 'config/corex_right.keymap').read_text()
keymap = s.split('    keymap {')[1]
assert re.findall(r'^        (\w+) \{', keymap, re.M) == [
    'WIN', 'MAC', 'NUM', 'FN', 'PAN', 'GESTURE', 'MOUSE', 'BLUETOOTH', 'WINDOW']
assert s.count('&lt 4 LANG2') == 2
assert s.count('&mo 5') == 2 and s.count('&lt 7 ') == 2
assert '<&zip_temp_layer 6 0>' in s
assert s.count('layers = <5>') == 1
assert 'layers = <5 8>' in s
assert s.count('layer = <4>') == 1
assert s.index('&pan_direction') < s.index('&scroll_inertia')
assert 'PAN_MAC' not in s and 'PAN_WIN' not in s
print('PAN: OS direction, vertical/pointer passthrough and layer references passed')

# WINDOW is appended so existing layer IDs and saved base layers stay valid.
assert s.count('&space_window_win') == 1
assert s.count('&space_window_mac') == 1
assert 'bindings = <&lt 8 SPACE>, <&kp LG(UP)>' in s
assert 'bindings = <&lt 8 SPACE>, <&kp LG(ENTER)>' in s
assert s.count('mods = <(MOD_LSFT | MOD_RSFT)>') == 2
assert 'space_taps_' not in s and 'zmk,behavior-tap-dance' not in s
assert s.index('window_layer_gesture {') < s.index('pan_scroll {')
window = s.split('window_gesture: window_gesture {')[1].split('    pan_direction:')[0]
for pattern, win, mac in [
    ('LEFT', 'LG(LEFT)', 'LG(LEFT)'),
    ('RIGHT', 'LG(RIGHT)', 'LG(RIGHT)'),
    ('UP', 'LG(LS(LEFT))', 'LG(UP)'),
    ('DOWN', 'LG(LS(RIGHT))', 'LG(DOWN)'),
]:
    gesture = re.search(r'pattern = <GESTURE_' + pattern + r'>;(.*?)\n        };', window, re.S).group(1)
    assert 'win-bindings = <&kp ' + win + '>' in gesture
    assert 'mac-bindings = <&kp ' + mac + '>' in gesture
assert 'window_minimize' not in s and 'window_maximize' not in s
blocks = re.findall(r'^        (\w+) \{.*?bindings = <(.*?)>;', keymap, re.M | re.S)
for name, bindings in blocks:
    assert len(re.findall(r'&\w+', bindings)) == 49, name
print('WINDOW: Space hold, snap/monitor shortcuts, Mac Command arrows and 49-key layers passed')
