// THE SURVIVAL GAME — core/platform_iface.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from prelude.c lines 1-74 in the Nov 2026 fine-grained refactor; code unchanged.


#include <stdint.h>
#include <stddef.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int32_t  i32;

// ---------------------------------------------------------------------------
// Platform interface. The game core calls these; each platform provides them.
// ---------------------------------------------------------------------------
// audio: note event (channel, midi note, velocity, wave/instrument id)
void plat_note(int ch, int note, int vel, int instr);
// storage: persist / load the save block (NUL-terminated text)
void plat_save(const char *data, int len);
int  plat_load(char *buf, int cap);            // returns length or -1
// per-slot run storage: slot 1..3 (the global blob above is slot 0)
void plat_save_slot(int slot, const char *data, int len);
int  plat_load_slot(int slot, char *buf, int cap);
// "TheSurvivalGame_save.txt" for slot 0, "..._save1.txt" .. for slots
static void save_slot_name(char *out, int slot){
    const char *base = "TheSurvivalGame_save";
    int i=0; while (base[i]){ out[i]=base[i]; i++; }
    if (slot>=1 && slot<=3) out[i++] = (char)('0'+slot);
    const char *ext = ".txt"; int j=0; while (ext[j]) out[i++]=ext[j++];
    out[i]=0;
}
// network: mode 0=off 1=host 2=client. Packets are opaque byte blobs.
int  plat_net_start(int mode, const char *addr); // 0 ok
void plat_net_stop(void);
int  plat_net_send(const u8 *data, int len, int dest); // dest: -1 broadcast (host), 0 host (client)
int  plat_net_recv(u8 *buf, int cap, int *src);        // returns len or 0
// time in seconds (monotonic) and a "quit" flag for native builds
double plat_now(void);
// the local civil date as a day number (days since 1970-01-01, local
// time) — the WEEKLY CHALLENGE's only clock (v1.25 feat 6). plat_now
// is monotonic and never learns the date; this one does nothing else.
int plat_daynum(void);
// the local time of day as minutes since midnight (0..1440, seconds
// folded in as a fraction) — the LIVING SKY's only clock (v1.25 feat
// 12). Cosmetic only: nothing but colour ever reads it.
float plat_minutes(void);
// where friends point a browser to join this machine's game
// ("192.168.1.42:8379"), or 0 on builds that cannot host
const char *plat_join_addr(void);
// Whales Only: open a purchase page in the player's browser, and pick
// up an unlock code the player dropped off (web: ?whale= link / native:
// a whale-code.txt file beside the saves). poll returns length or 0.
void plat_open_url(const char *url);
int  plat_whale_poll(char *buf, int cap);
// native builds: start the built-in web server that serves the game
// page and relays browser guests into this host (called once at launch)
void web_server_start(void);
// ---------------------------------------------------------------------------
// THE SOURCE ROOM (v1.26): the mod overlay + the host recompile.
// plat_mod_* persist one edited source file each, independent of the
// game saves: wasm keeps them in localStorage (tsgmod:<path>), the
// native builds keep files under the save home's tsg-modsrc/ folder,
// mirroring the src-relative path. load returns length or -1.
// ---------------------------------------------------------------------------
int  plat_mod_load(const char *path, char *buf, int cap);
void plat_mod_save(const char *path, const char *data, int len);
void plat_mod_clear(void);
int  plat_mod_exists(const char *path);
// the native host's recompile pipeline (src/platform/srcbuild.c;
// wasm stubs report unavailable — BUILD there needs the host app).
// The core stages EVERY blob file's current text (edited or stock)
// into <save home>/tsg-build/src/, then runs the compiler async;
// the result is a rebuilt web page the server swaps in for the stock
// one. A failed build never replaces the live page.
int  plat_srcbuild_available(void);   // 1 when a compiler was found
void plat_srcbuild_begin(void);       // wipe/create the staging tree
void plat_srcbuild_file(const char *path, const char *data, int len);
void plat_srcbuild_run_async(void);   // kick the compile on a worker
int  plat_srcbuild_poll(void);        // 0 running, 1 done-ok, -1 failed
int  plat_srcbuild_state(void);       // 0 idle, 1 building, 2 done-ok, 3 failed
int  plat_srcbuild_log(char *buf, int cap);   // compiler log tail; returns len
int  plat_srcbuild_active(void);      // 1 while a custom page is live
void plat_srcbuild_clear(void);       // drop the custom page (stock served again)
unsigned char *plat_srcbuild_page(int *len);  // malloc'd custom page for the server, or 0
const unsigned char *tsg_stock_page(int *len);   // server.c: the stock page as served (hosted flag in)
void plat_save_home_dir(char *out, int cap);     // the platform's stable save home (per-platform)
static void sfx(int id);   // sound effects, implemented with the audio section
static void try_buy(int pi, int sel);  // store purchases, implemented below
static void station_action(int pi, int station, int row);   // mall stations, below
static void draw_active_panel(void);                        // the open panel, below
static void ui_vignette(int x0, int y0, int w, int h);      // glow-up shading, below
static void bowl_update(int pi, float dt);                  // Bug Bowl tick, below
static void draw_bowling(void);                             // Bug Bowl view, below
static int act2_down(int a);                                // pad-2 held state, below
static void push_ev(int type, float x, float y); // net event queue (host)
static float client_aim=0; static int client_weapon=0, client_want_wall=0;
static int client_joined=0;   // client has sent its 'J' join packet
static float wall_send_t=0;   // host wall-broadcast timer (net section)

