"""Compile the actual default-layer driver with host mocks (no Zephyr SDK)."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
STUBS = r'''
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#define CONFIG_ZMK_LOG_LEVEL 0
#define CONFIG_ZMK_DEFAULT_LAYER_MIN_INDEX 0
#define CONFIG_ZMK_DEFAULT_LAYER_MAX_INDEX 1
#define ZMK_KEYMAP_LAYERS_LEN 9
#define ZMK_ENDPOINT_COUNT 7
#define ZMK_BLE_PROFILE_COUNT 5
#define ZMK_ENDPOINT_STR_LEN 16
#define CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE 60000
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define K_MSEC(x) (x)
#define IS_ENABLED(x) 0
#define DT_HAS_COMPAT_STATUS_OKAY(x) 1
#define LOG_MODULE_DECLARE(...)
#define LOG_WRN(...)
#define LOG_ERR(...)
#define LOG_INF(...)
#define SYS_INIT(...)
#define BEHAVIOR_DT_INST_DEFINE(...)
#define ZMK_LISTENER(...)
#define ZMK_SUBSCRIPTION(...)
#define ZMK_BEHAVIOR_OPAQUE 0
#define ZMK_EV_EVENT_BUBBLE 0
struct device {};
struct k_work {};
struct k_work_delayable { void (*handler)(struct k_work *); int delay; };
static void k_work_init_delayable(struct k_work_delayable *w, void (*h)(struct k_work *)) { w->handler=h; }
/* Pending writes disappear at power-off; do not silently execute them here. */
static int k_work_reschedule(struct k_work_delayable *w, int ms) { w->delay=ms; return 0; }
typedef int zmk_keymap_layer_id_t;
enum { ZMK_TRANSPORT_NONE, ZMK_TRANSPORT_USB, ZMK_TRANSPORT_BLE };
struct zmk_endpoint_instance { int transport; struct { int profile_index; } ble; };
static struct zmk_endpoint_instance selected;
static int active_profile;
static bool mac_active;
static unsigned char saved[ZMK_ENDPOINT_COUNT];
static bool saved_present;
static int save_error, save_count;
static struct zmk_endpoint_instance zmk_endpoint_get_selected(void) { return selected; }
static int zmk_ble_active_profile_index(void) { return active_profile; }
static int zmk_endpoint_instance_to_index(struct zmk_endpoint_instance e) { return e.transport==ZMK_TRANSPORT_USB?1:2+e.ble.profile_index; }
static bool zmk_endpoint_instance_eq(struct zmk_endpoint_instance a, struct zmk_endpoint_instance b) { return a.transport==b.transport && (a.transport!=ZMK_TRANSPORT_BLE || a.ble.profile_index==b.ble.profile_index); }
static void zmk_endpoint_instance_to_str(struct zmk_endpoint_instance e,char *s,size_t n) { snprintf(s,n,"%d",e.transport); }
static int zmk_keymap_layer_default(void) { return 0; }
static int zmk_keymap_layer_index_to_id(int i) { return i; }
static int zmk_keymap_layer_deactivate(int i,bool locking) { if(i==1)mac_active=false; return 0; }
static int zmk_keymap_layer_activate(int i,bool locking) { if(i==1)mac_active=true; return 0; }
typedef int (*settings_read_cb)(void *,void *,size_t);
struct settings_handler { const char *name; int (*h_set)(const char *,size_t,settings_read_cb,void *); };
static int settings_save_one(const char *name,const void *p,size_t n) {
    assert(n==sizeof(saved));
    if (save_error) { return save_error; }
    memcpy(saved,p,n); saved_present=true; save_count++; return 0;
}
static int settings_subsys_init(void) { return 0; }
static struct settings_handler *registered;
static int settings_register(struct settings_handler *s) { registered=s; return 0; }
static int read_saved_settings(void *arg,void *out,size_t len) { memcpy(out,saved,len); return len; }
static int settings_load_subtree(const char *s) {
    return saved_present ? registered->h_set("settings",sizeof(saved),read_saved_settings,NULL) : 0;
}
static bool settings_name_steq(const char *s,const char *key,const char **next) { *next=NULL; return strcmp(s,key)==0; }
struct zmk_behavior_binding { uint32_t param1,param2; };
struct zmk_behavior_binding_event {};
struct behavior_driver_api { int (*binding_pressed)(struct zmk_behavior_binding *,struct zmk_behavior_binding_event); int (*binding_released)(struct zmk_behavior_binding *,struct zmk_behavior_binding_event); };
typedef struct { int type; } zmk_event_t;
static const void *as_zmk_ble_active_profile_changed(const zmk_event_t *e) { return e->type==1?e:NULL; }
static const void *as_zmk_endpoint_changed(const zmk_event_t *e) { return e->type==2?e:NULL; }
'''
HARNESS = r'''
static void press(int command,int layer) {
    struct zmk_behavior_binding binding={.param1=command,.param2=layer};
    assert(on_keymap_binding_pressed(&binding,(struct zmk_behavior_binding_event){})==0);
}
static int read_saved(void *arg,void *out,size_t len) { memcpy(out,saved,len); return len; }
int main(void) {
    default_layer_init();
    /* Actual BLE output despite USB being preferred: save profile 0, not USB. */
    selected=(struct zmk_endpoint_instance){.transport=ZMK_TRANSPORT_BLE};
    active_profile=0; press(DEFAULT_LAYER_CMD_SELECT,1);
    assert(saved[2]==1 && saved[1]==0 && mac_active);
    /* Power-cycle immediately: no delayed work has had time to run. */
    memset(&default_layers,0,sizeof(default_layers)); mac_active=false;
    assert(default_layer_init()==0);
    assert(mac_active && default_layers.endpoint_defaults[2]==1);
    /* Re-selecting the same layout restores it without wearing flash. */
    int writes=save_count;
    mac_active=false; press(DEFAULT_LAYER_CMD_SELECT,1);
    assert(mac_active && save_count==writes);
    /* Failed flash writes must not masquerade as a successful selection. */
    save_error=-EIO;
    assert(zmk_default_layer_set(selected,0)==-EIO);
    assert(mac_active && saved[2]==1 && default_layers.endpoint_defaults[2]==1);
    save_error=0;
    /* BT_SEL can change before the endpoint's old profile is updated. */
    active_profile=2; press(DEFAULT_LAYER_CMD_SELECT_BLE,1);
    assert(saved[4]==1 && saved[2]==1);
    /* USB output is independent of the selected BLE profile. */
    selected.transport=ZMK_TRANSPORT_USB;
    endpoint_changed_cb(&(zmk_event_t){.type=2}); assert(!mac_active);
    active_profile=1; press(DEFAULT_LAYER_CMD_SELECT_BLE,1);
    assert(saved[3]==1 && saved[1]==0 && !mac_active);
    press(DEFAULT_LAYER_CMD_SELECT,1); assert(saved[1]==1 && mac_active);
    /* A new, unpaired slot is editable before any connection exists. */
    selected.transport=ZMK_TRANSPORT_NONE;
    active_profile=4; press(DEFAULT_LAYER_CMD_SELECT_BLE,1);
    assert(saved[6]==1 && mac_active);
    /* Reboot restores storage and reconnect/profile events select its slot. */
    memset(&default_layers,0,sizeof(default_layers));
    assert(default_layer_set("settings",sizeof(saved),read_saved,NULL)==0);
    active_profile=3; endpoint_changed_cb(&(zmk_event_t){.type=1}); assert(!mac_active);
    active_profile=0; endpoint_changed_cb(&(zmk_event_t){.type=1}); assert(mac_active);
    selected.transport=ZMK_TRANSPORT_USB;
    endpoint_changed_cb(&(zmk_event_t){.type=2}); assert(mac_active);
    puts("Default-layer driver: immediate power-cycle persistence, save failure, dedup, USB isolation and reconnect passed");
}
'''

with tempfile.TemporaryDirectory(prefix="corex-default-layer-") as directory:
    tmp = Path(directory)
    source = ROOT / "drivers/keymap/default_layer.c"
    import re
    for header in re.findall(r"#include <([^>]+)>", source.read_text()):
        if not header.startswith(("drivers/", "zephyr/", "zmk/")):
            continue
        stub = tmp / header
        stub.parent.mkdir(parents=True, exist_ok=True)
        stub.write_text("")
    harness = tmp / "test.c"
    harness.write_text(STUBS + '\n#include "' + str(source) + '"\n' + HARNESS)
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                    "-Wno-unused-function", "-Wno-unused-parameter", "-Wno-unused-const-variable",
                    "-Wno-sign-compare", "-I"+str(tmp), "-I"+str(ROOT/"include"),
                    str(harness), "-o", str(tmp/"test")], check=True)
    subprocess.run([str(tmp/"test")], check=True)
