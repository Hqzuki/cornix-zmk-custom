"""Exercise OS key selection and matching key release in the actual driver."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    for name in ['drivers/behavior.h', 'zmk/behavior.h', 'zmk/keymap.h']:
        p = tmp / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('')
    source = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define DT_INST_FOREACH_STATUS_OKAY(fn)
#define DT_NODELABEL(n) n
#define DEVICE_DT_NAME(n) "key_press"
struct device { const void *config; void *data; };
struct zmk_behavior_binding { const char *behavior_dev; uint32_t param1, param2; };
struct zmk_behavior_binding_event { int position; };
struct behavior_driver_api {
 int (*binding_pressed)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
 int (*binding_released)(struct zmk_behavior_binding *, struct zmk_behavior_binding_event);
};
static bool mac;
static const struct device *current;
static uint32_t sent;
static bool pressed;
static const struct device *zmk_behavior_get_binding(const char *name) {
 (void)name; return current;
}
static int zmk_keymap_layer_index_to_id(int layer) { return layer + 10; }
static bool zmk_keymap_layer_active(int layer) { assert(layer == 11); return mac; }
static int zmk_behavior_invoke_binding(struct zmk_behavior_binding *binding,
 struct zmk_behavior_binding_event event, bool state) {
 assert(!strcmp(binding->behavior_dev, "key_press"));
 assert(event.position == 19);
 sent = binding->param1; pressed = state; return 0;
}
'''
    source += '\n#include "' + str(root / 'drivers/keymap/os_key.c') + '"\n'
    source += r'''
int main(void) {
 const struct os_key_config cfg = {101, 202, 1};
 struct os_key_data data = {0};
 struct device dev = {&cfg, &data};
 current = &dev;
 struct zmk_behavior_binding binding = {"window_left", 0, 0};
 struct zmk_behavior_binding_event event = {19};
 for (int os = 0; os < 2; os++) {
  mac = os;
  assert(os_key_api.binding_pressed(&binding, event) == 0);
  assert(pressed && sent == (os ? 202 : 101));
  mac = !os;
  assert(os_key_api.binding_released(&binding, event) == 0);
  assert(!pressed && sent == (os ? 202 : 101));
 }
}
'''
    (tmp / 'test.c').write_text(source)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-I', str(tmp),
                    str(tmp / 'test.c'), '-o', str(tmp / 'test')], check=True)
    subprocess.run([str(tmp / 'test')], check=True)
print('OS keys: WIN/MAC selection and matching release across base-layer changes passed')
