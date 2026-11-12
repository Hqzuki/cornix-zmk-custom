"""Run the actual BLE observer/controller against a clock and connection harness."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
DRIVER = ROOT / "drivers/keymap/auto_os.c"
STUBS = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#define BIT(x) (1u << (x))
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define ARG_UNUSED(x) (void)(x)
#define CONFIG_BT_MAX_CONN 6
#define CONFIG_CORNIX_AUTO_OS_SETTLE_MS 1000
#define CONFIG_ZMK_LOG_LEVEL 0
#define CONFIG_APPLICATION_INIT_PRIORITY 90
#define LOG_MODULE_DECLARE(...)
#define LOG_ERR(...)
#define LOG_INF(...)
#define SYS_INIT(...)
#define K_NO_WAIT 0
#define K_MSEC(x) (x)
#define ZMK_EV_EVENT_BUBBLE 0
#define ZMK_LISTENER(...)
#define ZMK_SUBSCRIPTION(...)
static int64_t now;
static int64_t k_uptime_get(void) { return now; }
struct k_work {};
struct k_work_delayable { void (*fn)(struct k_work *); bool pending; int64_t due; };
#define K_WORK_DELAYABLE_DEFINE(name, fn) struct k_work_delayable name={fn,false,0}
static int k_work_reschedule(struct k_work_delayable *w, int64_t ms) {
 w->pending=true; w->due=now+ms; return 0;
}
static int k_work_schedule(struct k_work_delayable *w, int64_t ms) {
 if (!w->pending) { w->pending=true; w->due=now+ms; } return 0;
}
struct k_spinlock { bool held; };
typedef int k_spinlock_key_t;
static k_spinlock_key_t k_spin_lock(struct k_spinlock *l) {
 assert(!l->held); l->held=true; return 0;
}
static void k_spin_unlock(struct k_spinlock *l, k_spinlock_key_t k) { (void)k; l->held=false; }
#define BT_CONN_TYPE_LE 1
#define BT_CONN_ROLE_PERIPHERAL 1
#define BT_CONN_ROLE_CENTRAL 0
struct bt_conn { uint8_t index; int role, type, refs; bool connected; };
struct bt_conn_info { int role, type; };
static int bt_conn_get_info(struct bt_conn *c, struct bt_conn_info *i) {
 i->role=c->role; i->type=c->type; return 0;
}
static uint8_t bt_conn_index(struct bt_conn *c) { return c->index; }
static struct bt_conn *bt_conn_ref(struct bt_conn *c) { assert(c->refs>0); c->refs++; return c; }
static void bt_conn_unref(struct bt_conn *c) { assert(c->refs>1); c->refs--; }
static struct bt_conn *active_conn;
static struct bt_conn *zmk_ble_active_profile_conn(void) {
 return active_conn && active_conn->connected ? bt_conn_ref(active_conn) : NULL;
}
static const int uuid_map=1, uuid_info=2, uuid_pnp=3, uuid_appearance=4, uuid_battery=5;
#define BT_UUID_HIDS_REPORT_MAP (&uuid_map)
#define BT_UUID_HIDS_INFO (&uuid_info)
#define BT_UUID_DIS_PNP_ID (&uuid_pnp)
#define BT_UUID_GAP_APPEARANCE (&uuid_appearance)
static int bt_uuid_cmp(const int *a, const int *b) { return *a-*b; }
struct bt_gatt_attr { const int *uuid; };
struct bt_gatt_authorization_cb { bool (*read_authorize)(struct bt_conn *, const struct bt_gatt_attr *); };
static const struct bt_gatt_authorization_cb *registered;
static int registration_error;
static int bt_gatt_authorization_cb_register(const struct bt_gatt_authorization_cb *c) {
 registered=c; return registration_error;
}
struct bt_conn_cb { void (*connected)(struct bt_conn *, uint8_t); void (*disconnected)(struct bt_conn *, uint8_t); };
#define BT_CONN_CB_DEFINE(name) struct bt_conn_cb name
enum { ZMK_TRANSPORT_NONE, ZMK_TRANSPORT_USB, ZMK_TRANSPORT_BLE };
struct zmk_endpoint_instance { int transport; };
static struct zmk_endpoint_instance selected;
static struct zmk_endpoint_instance zmk_endpoint_get_selected(void) { return selected; }
static bool mac_active, num_active=true, inject_event;
static unsigned layer_calls;
static void request_update(void);
static bool zmk_keymap_layer_active(int id) { assert(id==1); return mac_active; }
static int change_layer(int id, bool mac) {
 assert(id==1); mac_active=mac; layer_calls++;
 if (inject_event) { inject_event=false; request_update(); }
 return 0;
}
static int zmk_keymap_layer_activate(int id, bool locking) { assert(!locking); return change_layer(id,true); }
static int zmk_keymap_layer_deactivate(int id, bool locking) { assert(!locking); return change_layer(id,false); }
typedef struct {} zmk_event_t;
'''
HARNESS = r'''
static void tick(int64_t t) {
 now=t;
 int count=0;
 while (update_work.pending && update_work.due<=now) {
  assert(++count<10); update_work.pending=false; update_work.fn(NULL);
 }
}
static void read_attr(struct bt_conn *c, const int *uuid) {
 assert(registered->read_authorize(c,&(struct bt_gatt_attr){uuid}));
 tick(now);
}
static void connect(struct bt_conn *c) {
 c->connected=true; auto_os_connections.connected(c,0); tick(now);
}
static void disconnect(struct bt_conn *c) {
 c->connected=false; auto_os_connections.disconnected(c,0); tick(now);
}
int main(void) {
 assert(auto_os_init()==0 && registered);
 registration_error=-EALREADY; assert(auto_os_init()==-EALREADY); registration_error=0;
 selected.transport=ZMK_TRANSPORT_BLE;
 struct bt_conn apple={0,BT_CONN_ROLE_PERIPHERAL,BT_CONN_TYPE_LE,1,false};
 struct bt_conn win={1,BT_CONN_ROLE_PERIPHERAL,BT_CONN_TYPE_LE,1,false};
 struct bt_conn split={2,BT_CONN_ROLE_CENTRAL,BT_CONN_TYPE_LE,1,false};
 active_conn=&apple; connect(&apple); assert(!mac_active && apple.refs==2);
 connect(&split); read_attr(&split,BT_UUID_HIDS_REPORT_MAP);
 tick(2000); assert(!mac_active && split.refs==1); /* split never classifies */

 /* No read evidence: remain WIN indefinitely. Polling battery changes nothing. */
 read_attr(&apple,&uuid_battery); tick(5000); assert(!mac_active);
 read_attr(&apple,BT_UUID_HIDS_REPORT_MAP);
 tick(5500); read_attr(&apple,BT_UUID_HIDS_REPORT_MAP); /* duplicate does not postpone */
 read_attr(&apple,&uuid_battery);
 tick(5999); assert(!mac_active);
 inject_event=true; tick(6000); assert(mac_active && num_active);
 unsigned calls=layer_calls;
 tick(7000); read_attr(&apple,BT_UUID_HIDS_INFO); /* late discovery re-arms */
 assert(mac_active && layer_calls==calls); /* retain prior layout while waiting */
 tick(7999); assert(mac_active);
 tick(8000); assert(!mac_active && layer_calls==calls+1);

 /* Same radio object reconnects to Windows: discard Apple evidence immediately. */
 disconnect(&apple); assert(!mac_active && apple.refs==1);
 connect(&apple); read_attr(&apple,BT_UUID_HIDS_REPORT_MAP);
 tick(8500); read_attr(&apple,BT_UUID_DIS_PNP_ID);
 tick(8900); read_attr(&apple,BT_UUID_GAP_APPEARANCE);
 tick(9000); assert(!mac_active); /* no temporary MAC during the discovery burst */
 tick(9900); assert(!mac_active && classify(sessions[0].reads)==OS_WIN);

 /* Simultaneously connected hosts: classify each separately; follow active only. */
 connect(&win); read_attr(&win,BT_UUID_HIDS_REPORT_MAP); tick(10900);
 assert(!mac_active && classify(sessions[1].reads)==OS_APPLE);
 active_conn=&win; host_changed(&(zmk_event_t){}); tick(now); assert(mac_active);
 active_conn=&apple; host_changed(&(zmk_event_t){}); tick(now); assert(!mac_active);
 active_conn=&win; host_changed(&(zmk_event_t){}); tick(now); assert(mac_active);
 disconnect(&apple); assert(mac_active); /* inactive disconnect cannot reset current */

 /* USB output uses WIN and never consumes BLE classification; BLE resumes it. */
 selected.transport=ZMK_TRANSPORT_USB; host_changed(&(zmk_event_t){}); tick(now);
 assert(!mac_active);
 selected.transport=ZMK_TRANSPORT_BLE; host_changed(&(zmk_event_t){}); tick(now);
 assert(mac_active);
 disconnect(&win); assert(!mac_active && win.refs==1);
 active_conn=&win; connect(&win); tick(20000); assert(!mac_active); /* no saved guess */

 /* Recorded Mac trace: DIS PnP ID only, no HIDS access in the capture window. */
 read_attr(&win,BT_UUID_DIS_PNP_ID); tick(21000);
 assert(mac_active && classify(sessions[1].reads)==OS_APPLE);
 /* New evidence long after a guess must still refine it. */
 tick(121000); read_attr(&win,BT_UUID_GAP_APPEARANCE);
 assert(mac_active); tick(121999); assert(mac_active);
 tick(122000); assert(!mac_active && classify(sessions[1].reads)==OS_WIN);
 /* Appearance alone has no Apple evidence; Android maps to WIN. */
 disconnect(&win); connect(&win); read_attr(&win,BT_UUID_GAP_APPEARANCE);
 tick(123000); assert(!mac_active && classify(sessions[1].reads)==OS_UNKNOWN);
 disconnect(&win); connect(&win);
 read_attr(&win,BT_UUID_HIDS_REPORT_MAP); read_attr(&win,BT_UUID_HIDS_INFO);
 tick(124000); assert(!mac_active && classify(sessions[1].reads)==OS_WIN);

 /* Disconnect before settling, then reuse slot: no stale delayed Apple result. */
 disconnect(&win); connect(&win); read_attr(&win,BT_UUID_HIDS_REPORT_MAP);
 tick(124500); disconnect(&win); connect(&win);
 tick(126000); assert(!mac_active && !sessions[1].settled);
 disconnect(&win); disconnect(&split);
 assert(apple.refs==1 && win.refs==1 && split.refs==1 && num_active);
 puts("Auto OS: settle timing, Apple/WIN/unknown, reconnect, host isolation, split exclusion, USB and reference lifecycle passed");
}
'''

with tempfile.TemporaryDirectory(prefix="cornix-auto-os-") as directory:
    tmp = Path(directory)
    for header in re.findall(r"#include <([^>]+)>", DRIVER.read_text()):
        if header.startswith(("zephyr/", "zmk/")):
            stub = tmp / header
            stub.parent.mkdir(parents=True, exist_ok=True)
            stub.write_text("")
    source = tmp / "test.c"
    source.write_text(STUBS + '\n#include "' + str(DRIVER) + '"\n' + HARNESS)
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I" + str(tmp), str(source), "-o", str(tmp / "test"),
    ], check=True)
    subprocess.run([str(tmp / "test")], check=True)

# Migration/configuration: neither keymap is allowed to override auto detection.
for name in ("cornix_right", "corex_right"):
    keymap = (ROOT / "config" / (name + ".keymap")).read_text()
    assert "&df" not in keymap and "default_layer.dtsi" not in keymap
    for slot in range(5):
        assert f"bindings = <&bt BT_SEL {slot}>;" in keymap
assert "settings_" not in DRIVER.read_text()
