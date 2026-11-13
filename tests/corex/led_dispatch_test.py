"""Exercise the actual LED controller's event scheduling on both split roles."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
DRIVER = ROOT / "drivers/status/controller.c"
STUBS = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#define IS_ENABLED(x) (x)
#define CONFIG_ZMK_BATTERY_REPORTING_FETCH_MODE_STATE_OF_CHARGE 0
#define DT_HAS_CHOSEN(x) 0
#define CONFIG_ZMK_LOG_LEVEL 0
#define CONFIG_APPLICATION_INIT_PRIORITY 90
#define LOG_MODULE_REGISTER(...)
#define LOG_ERR(...)
#define SYS_INIT(...)
#define ARG_UNUSED(x) (void)(x)
#define K_FOREVER 0
#define K_NO_WAIT 0
#define K_MSEC(x) (x)
#define ZMK_ACTIVITY_ACTIVE 0
#define ZMK_EV_EVENT_BUBBLE 0
#define POWER_USBREGSTATUS_VBUSDETECT_Msk 1
static struct { int USBREGSTATUS; } power_reg;
#define NRF_POWER (&power_reg)
static int64_t now;
static int activity;
static int64_t k_uptime_get(void) { return now; }
static int zmk_activity_get_state(void) { return activity; }
#define K_MUTEX_DEFINE(name) int name
static void k_mutex_lock(int *m, int t) { (void)m; (void)t; }
static void k_mutex_unlock(int *m) { (void)m; }
typedef int atomic_t;
static bool atomic_cas(atomic_t *v, int old, int n) {
 if (*v != old) return false; *v=n; return true;
}
static void atomic_set(atomic_t *v, int n) { *v=n; }
struct k_work {};
struct k_work_delayable { int due; bool pending; };
#define K_WORK_DELAYABLE_DEFINE(name, fn) struct k_work_delayable name
static int k_work_reschedule(struct k_work_delayable *w, int ms) {
 w->due=ms; w->pending=true; return 0;
}
static int k_work_schedule(struct k_work_delayable *w, int ms) {
 if (!w->pending) { w->due=ms; w->pending=true; } return 0;
}
typedef struct { int type; } zmk_event_t;
struct zmk_battery_state_changed { int state_of_charge; };
struct zmk_split_peripheral_status_changed { bool connected; };
static const struct zmk_battery_state_changed *as_zmk_battery_state_changed(const zmk_event_t *e) {
 (void)e; return NULL;
}
static const struct zmk_split_peripheral_status_changed *as_zmk_split_peripheral_status_changed(const zmk_event_t *e) {
 (void)e; return NULL;
}
#define ZMK_LISTENER(name, fn)
/* Keep subscriptions callable so the test dispatches only subscribed events. */
#define ZMK_SUBSCRIPTION(name, event) \
 static void dispatch_##event(void) { status_event(&(zmk_event_t){0}); }
struct bt_conn {};
struct bt_conn_info { int role; };
#define BT_CONN_ROLE_CENTRAL 0
static int bt_conn_get_info(struct bt_conn *c, struct bt_conn_info *i) {
 (void)c; i->role=0; return 0;
}
struct conn_cb { void (*connected)(struct bt_conn *, uint8_t); void (*disconnected)(struct bt_conn *, uint8_t); };
#define BT_CONN_CB_DEFINE(name) struct conn_cb name
static int layer;
static int zmk_ble_active_profile_index(void) { return 0; }
static bool zmk_ble_active_profile_is_connected(void) { return true; }
static bool zmk_ble_active_profile_is_open(void) { return false; }
static int zmk_keymap_highest_layer_active(void) { return layer; }
static int zmk_keymap_layer_index_to_id(int i) { return i; }
struct zmk_behavior_binding { const char *behavior_dev; uint32_t param1, param2; };
struct zmk_behavior_binding_event { int64_t timestamp; };
static int sent, written, order, sent_order, write_order;
static uint32_t sent_flags;
static int zmk_split_central_invoke_behavior(int p, struct zmk_behavior_binding *b,
 struct zmk_behavior_binding_event e, bool pressed) {
 (void)p; (void)e; (void)pressed; sent++; sent_flags=b->param2;
 sent_order=++order; return 0;
}
'''
HARNESS = r'''
static bool event_during_write;
static struct status_frame displayed;
int status_backend_init(void) { return 0; }
int status_backend_write(struct status_frame frame, int64_t t) {
 (void)t; displayed=frame; written++; write_order=++order;
 if (event_during_write) { event_during_write=false; wake_render(); }
 return 0;
}
static void run_render(void) { render_work.pending=false; render(NULL); }
int main(void) {
 status_init(); now=1000;
#if CONFIG_ZMK_SPLIT_ROLE_CENTRAL
 state.peer_connected=true;
 run_render(); assert(sent==1);
 layer=2; dispatch_zmk_layer_state_changed();
 assert(render_work.pending && render_work.due==0);
 run_render(); assert(sent==2 && ((sent_flags>>8)&255)==2);
 assert(sent_order < write_order); /* Local rail wake must not delay sync. */
 layer=0; dispatch_zmk_layer_state_changed();
 assert(render_work.due==0); run_render();
 assert(sent==3 && ((sent_flags>>8)&255)==0);
 dispatch_zmk_ble_active_profile_changed(); assert(render_work.due==0);
#else
 /* Already-active receiver, including the one-second idle timer. */
 state.remote_active=true;
 render_work.due=1000; render_work.pending=true;
 assert(corex_status_receive(0, STATUS_SYNC_LAYER | STATUS_SYNC_ACTIVE | (2u<<8))==0);
 assert(render_work.due==0);
 run_render(); assert(displayed.outer.g==13);
 assert(corex_status_receive(0, STATUS_SYNC_LAYER | STATUS_SYNC_ACTIVE)==0);
 assert(render_work.due==0); run_render(); assert(displayed.outer.r==13);
 assert(corex_status_receive(0, 1u<<16)==-EINVAL);
 assert(render_work.due==RENDER_FAST_MS);
#endif
 /* An event received during SPI output must survive the periodic re-arm. */
 event_during_write=true;
 run_render(); assert(render_work.pending && render_work.due==0);
 return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="cornix-led-dispatch-") as directory:
    tmp = Path(directory)
    for header in re.findall(r"#include <([^>]+)>", DRIVER.read_text()):
        if header.startswith(("zephyr/", "zmk/", "hal/")):
            stub = tmp / header
            stub.parent.mkdir(parents=True, exist_ok=True)
            stub.write_text("")
    source = tmp / "test.c"
    source.write_text(STUBS + '\n#include "' + str(DRIVER) + '"\n' + HARNESS)
    for central in (0, 1):
        binary = tmp / f"test-{central}"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-Wno-unused-function", "-Wno-unused-variable", "-Wno-misleading-indentation",
            f"-DCONFIG_ZMK_SPLIT_ROLE_CENTRAL={central}", "-I" + str(tmp),
            str(source), "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True)
print("LED dispatch: layer press/release, immediate receive, sync ordering and event race passed")
