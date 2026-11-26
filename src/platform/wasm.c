// THE SURVIVAL GAME — platform/wasm.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from platform.c lines 1-1077 in the Nov 2026 fine-grained refactor; code unchanged.

// ===========================================================================
// PLATFORM: WebAssembly — JS shell provides everything below.
// ===========================================================================
#ifdef PLAT_WASM
__attribute__((import_module("env"), import_name("js_now")))   double js_now(void);
__attribute__((import_module("env"), import_name("js_daynum"))) int js_daynum(void);
__attribute__((import_module("env"), import_name("js_minutes"))) double js_minutes(void);
__attribute__((import_module("env"), import_name("js_note")))  void js_note(int,int,int,int);
__attribute__((import_module("env"), import_name("js_save")))  void js_save(int, const char*, int);
__attribute__((import_module("env"), import_name("js_load")))  int js_load(int, char*, int);
__attribute__((import_module("env"), import_name("js_net_start"))) int js_net_start(int, const char*);
__attribute__((import_module("env"), import_name("js_net_stop")))  void js_net_stop(void);
__attribute__((import_module("env"), import_name("js_net_send")))  void js_net_send(const unsigned char*, int, int);
__attribute__((import_module("env"), import_name("js_net_recv")))  int js_net_recv(unsigned char*, int);
__attribute__((import_module("env"), import_name("js_net_src")))   int js_net_src(void);

double plat_now(void){ return js_now(); }
int plat_daynum(void){ return js_daynum(); }
float plat_minutes(void){ return (float)js_minutes(); }
void plat_note(int ch, int note, int vel, int instr){ js_note(ch, note, vel, instr); }
void plat_save(const char *d, int len){ js_save(0, d, len); }
int plat_load(char *buf, int cap){ return js_load(0, buf, cap); }
void plat_save_slot(int slot, const char *d, int len){ js_save(slot, d, len); }
int plat_load_slot(int slot, char *buf, int cap){ return js_load(slot, buf, cap); }
// THE SOURCE ROOM (v1.26): the edit overlay in localStorage, and the
// host-build stubs — the web build cannot compile; BUILD here reports
// that it needs the host app (the edits above are saved regardless).
__attribute__((import_module("env"), import_name("js_mod_save")))  void js_mod_save(const char*, const char*, int);
__attribute__((import_module("env"), import_name("js_mod_load")))  int js_mod_load(const char*, char*, int);
__attribute__((import_module("env"), import_name("js_mod_clear"))) void js_mod_clear(void);
__attribute__((import_module("env"), import_name("js_mod_exists"))) int js_mod_exists(const char*);
int plat_mod_load(const char *path, char *buf, int cap){ return js_mod_load(path, buf, cap); }
void plat_mod_save(const char *path, const char *data, int len){ js_mod_save(path, data, len); }
void plat_mod_clear(void){ js_mod_clear(); }
int plat_mod_exists(const char *path){ return js_mod_exists(path); }
int plat_srcbuild_available(void){ return 0; }
void plat_srcbuild_begin(void){}
void plat_srcbuild_file(const char *path, const char *data, int len){ (void)path; (void)data; (void)len; }
void plat_srcbuild_run_async(void){}
int plat_srcbuild_poll(void){ return 1; }
int plat_srcbuild_state(void){ return 0; }
int plat_srcbuild_log(char *buf, int cap){ const char *m = "BUILD NEEDS THE HOST APP"; int n = slen(m); if (n>cap-1) n = cap-1; smemcpy(buf, m, n); buf[n]=0; return n; }
int plat_srcbuild_active(void){ return 0; }
void plat_srcbuild_clear(void){}
unsigned char *plat_srcbuild_page(int *len){ (void)len; return 0; }
const unsigned char *tsg_stock_page(int *len){ if (len) *len=0; return 0; }
void plat_save_home_dir(char *out, int cap){ if (cap>0) out[0]=0; }
int plat_net_start(int mode, const char *addr){ return js_net_start(mode, addr); }
void plat_net_stop(void){ js_net_stop(); }
int plat_net_send(const u8 *d, int len, int dest){ js_net_send(d, len, dest); return 0; }
int plat_net_recv(u8 *buf, int cap, int *src){ int n=js_net_recv(buf, cap); *src=js_net_src(); return n; }
__attribute__((import_module("env"), import_name("js_open_url"))) void js_open_url(const char*);
__attribute__((import_module("env"), import_name("js_whale_poll"))) int js_whale_poll(char*, int);
void plat_open_url(const char *url){   // only ever a buy.stripe.com link from the table above
    if (url && url[0]=='h' && url[1]=='t' && url[2]=='t' && url[3]=='p' && url[4]=='s') js_open_url(url);
}
int plat_whale_poll(char *buf, int cap){ return js_whale_poll(buf, cap); }

// compiler may emit these for struct copies in freestanding builds
void *memcpy(void *d, const void *s, unsigned long n){ return smemcpy(d, s, (int)n); }
void *memset(void *d, int v, unsigned long n){ return smemset(d, v, (int)n); }
void *memmove(void *d, const void *s, unsigned long n){
    u8 *a=(u8*)d; const u8 *b=(const u8*)s;
    if (a<b) for (unsigned long i=0;i<n;i++) a[i]=b[i];
    else for (unsigned long i=n;i>0;i--) a[i-1]=b[i-1];
    return d;
}

// exports for the JS shell
__attribute__((export_name("fb"))) u32 *exp_fb(void){ return fb; }
/* debug/test hooks */
__attribute__((export_name("dbg_aim"))) int exp_dbg_aim(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)(G.p[id].aim*1000); }
__attribute__((export_name("dbg_plaza"))) int exp_dbg_plaza(void){
    int n=0; for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
        float dx=G.bugs[i].x, dy=G.bugs[i].y; if (dx*dx+dy*dy < 170*170) n++; }
    return n; }
/* frame() itself is exported via the linker flag --export=frame */
__attribute__((export_name("set_raw"))) void exp_set_raw(int lo, int hi){
    IN.raw = (u64)(u32)lo | ((u64)(u32)hi<<32);   // held physical keys (PK_*)
}
__attribute__((export_name("pulse_key"))) void exp_pulse_key(int pk){
    if (pk>=0 && pk<64) IN.pulse |= (1ULL<<pk);    // one-frame tap (touch ✕ button)
}
__attribute__((export_name("press"))) void exp_press(int pk){
    if (pk>=0 && pk<64) IN.edges_in |= (1ULL<<pk); // a real keydown event
}
__attribute__((export_name("act"))) void exp_act(int a, int down){
    if (a<0 || a>=ACT_COUNT) return;
    if (down){ if (!((IN.act_held>>a)&1)) IN.act_edges |= (1u<<a); IN.act_held |= (1u<<a); }
    else IN.act_held &= ~(1u<<a);
}
__attribute__((export_name("act2"))) void exp_act2(int a, int down){
    if (a<0 || a>=ACT_COUNT) return;
    if (down){ if (!((IN.act2_held>>a)&1)) IN.act2_edges |= (1u<<a); IN.act2_held |= (1u<<a); }
    else IN.act2_held &= ~(1u<<a);
}
__attribute__((export_name("pads"))) void exp_pads(int n){ IN.pad_count = n; }
__attribute__((export_name("pad_axes"))) void exp_pad_axes(float lx, float ly, float rx, float ry){
    IN.pad_ax[0]=lx; IN.pad_ax[1]=ly; IN.pad_ax[2]=rx; IN.pad_ax[3]=ry;   // web Gamepad axes: +Y is down
}
__attribute__((export_name("pad2_axes"))) void exp_pad2_axes(float lx, float ly, float rx, float ry){
    IN.pad2_ax[0]=lx; IN.pad2_ax[1]=ly; IN.pad2_ax[2]=rx; IN.pad2_ax[3]=ry;   // pad 2, same convention (v1.25 feat 13)
}
__attribute__((export_name("text_mode"))) int exp_text_mode(void){
    // nonzero while a text field owns typing: 1/2 = the menu's name/IP
    // fields, 3 = the Whales Only code entry. The web shell watches
    // this to raise the soft keyboard on touch devices, where no
    // physical keyboard exists to type with (Daniel, Nov 4).
    // the menu field only owns typing while the menu is actually up:
    // edit_field is assigned in menu_input() and nowhere else, so
    // without this gate a field selected before starting a run kept
    // "owning" typing for the whole run and the shell raised the soft
    // keyboard on every gameplay tap (Daniel, Nov 9).
    if (G.state==GS_MENU && G.edit_field) return G.edit_field;
    if (whale_code_mode[G.my_id]) return 3;
    if (BWS[G.my_id].craft_menu && BWS[G.my_id].craft_search) return 4;   // the craft menu's search field
    // v1.27: 5 = the Source Room editor / LIVE TUNING on a touch
    // device (srcterm_text_wanted carries the essential mobile gate:
    // on a desktop this field must never take focus, or the shell's
    // keydown path would eat the editor's Ctrl+S / Ctrl+T chords)
    if (srcterm_text_wanted()) return 5;
    return 0;
}
__attribute__((export_name("dbg_peek"))) void exp_dbg_peek(void){ peek_slots(); }
__attribute__((export_name("dbg_pkbuddy"))) int exp_dbg_pkbuddy(int s){
    return (s>=1 && s<=3)? pk_bspecies[s] : -99;
}
__attribute__((export_name("dbg_pkbname"))) int exp_dbg_pkbname(int s){
    if (s<1 || s>3 || !pk_bname[s][0]) return -1;
    for (int i=0;i<BUD_NAMES_N;i++) if (scmp(pk_bname[s], BUD_ELF[i])==0) return i;
    return -1;
}
__attribute__((export_name("dbg_myname"))) char *exp_dbg_myname(void){ return G.my_name; }
__attribute__((export_name("evt_char"))) void exp_evt_char(int c){
    if (IN.typed_n<16) IN.typed[IN.typed_n++]=(char)c;
}
__attribute__((export_name("set_touch"))) void exp_set_touch(int x, int y, int down){
    IN.touch_x=x; IN.touch_y=y; IN.touch_down=down; if (down){ IN.touch_seen=1; IN.touch_last=IN.dev_t+0.001f; }
}
__attribute__((export_name("evt_tap"))) void exp_evt_tap(int x, int y){
    IN.tap_x=x; IN.tap_y=y; IN.tapped=1; IN.touch_seen=1; IN.touch_last=IN.dev_t+0.001f;
}
__attribute__((export_name("set_mobile"))) void exp_set_mobile(int on){
    IN.mobile_boot = on? 1:0;   // touch-first device: hide key-binding UI
}
__attribute__((export_name("touch_point"))) void exp_touch_point(int id, int x, int y, int phase){
    core_touch_point(id, x, y, phase);   // v1.31: the body lives in the core now (input/touch.c) — the native backends feed their mouse through the same function
}
__attribute__((export_name("autostart"))) void exp_autostart(void){
    // This page was served by a native host's built-in web server: skip
    // the menu and join that host's game directly. The host is whoever
    // served the page, so no address or mode choice is needed. Deferred
    // to the first frame: game_init() has not run yet at shell boot.
    want_autostart = 1;
}
const char *plat_join_addr(void){ return 0; }   // the web build never hosts
__attribute__((export_name("dbg_state"))) int exp_dbg_state(void){ return G.state; }
__attribute__((export_name("dbg_piece"))) int exp_dbg_piece(void){ return mus_piece(); }
// ADAPTIVE MUSIC INTENSITY (v1.25 feat 15): the latched level, the
// raw danger signal, and the context — observed, never steered (the
// danger itself is staged with the game's own seams: dbg_spawnbug,
// dbg_tp, dbg_bwhackhere, dbg_bnforce). Nothing here is ever saved.
__attribute__((export_name("dbg_muslvl"))) int exp_dbg_muslvl(void){ return mus_dbg_lvl(); }
__attribute__((export_name("dbg_mushot"))) int exp_dbg_mushot(void){ return mus_dbg_hot(); }
__attribute__((export_name("dbg_musctx"))) int exp_dbg_musctx(void){ return mus_dbg_ctx(); }
__attribute__((export_name("dbg_whale"))) int exp_dbg_whale(void){ return G.whale_own; }
__attribute__((export_name("dbg_whaleon"))) int exp_dbg_whaleon(void){ return G.whale_on; }
__attribute__((export_name("dbg_pwon"))) int exp_dbg_pwon(void){ int id = G.my_id>=0? G.my_id : 0; return G.p[id].whale_on; }   // v1.32: the LOCAL PLAYER's on-mask — the kill gate reads this one, not G's
__attribute__((export_name("dbg_pown"))) int exp_dbg_pown(void){ int id = G.my_id>=0? G.my_id : 0; return G.p[id].whale_own; }
extern int g_party_bursts;
__attribute__((export_name("dbg_party"))) int exp_dbg_party(void){ return g_party_bursts; }   // v1.32: CONFETTI SQUASH bursts this session
static char dbg_code_scratch[200];
__attribute__((export_name("dbg_scratch"))) char *exp_dbg_scratch(void){ return dbg_code_scratch; }
__attribute__((export_name("dbg_redeem"))) int exp_dbg_redeem(void){
    int s = whale_redeem(dbg_code_scratch); if (s>=0) whale_grant(s); return s; }
__attribute__((export_name("dbg_kills"))) int exp_dbg_kills(void){ return G.total_kills; }
__attribute__((export_name("dbg_fs"))) int exp_dbg_fs(void){ return G.fs_state*10 + G.fs_step; }
__attribute__((export_name("dbg_fspool"))) int exp_dbg_fspool(void){ int id = G.my_id>=0? G.my_id : 0; return G.fs_pool[id]; }
__attribute__((export_name("dbg_comfort"))) int exp_dbg_comfort(void){ return G.vol_music*10000 + G.vol_sfx*100 + G.mute_all*10 + G.shake_on; }
__attribute__((export_name("dbg_shake"))) int exp_dbg_shake(void){ return (int)(G.shake*100); }
__attribute__((export_name("dbg_fxshake"))) int exp_dbg_fxshake(void){ fx_shake(3.0f); return (int)(G.shake*100); }
__attribute__((export_name("dbg_shkoff"))) int exp_dbg_shkoff(void){ return (int)(shk_off_x*100); }
__attribute__((export_name("dbg_bugs"))) int exp_dbg_bugs(void){
    int n=0; for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive) n++; return n; }
__attribute__((export_name("dbg_bugavgw"))) int exp_dbg_bugavgw(void){
    float sum=0; int n=0; for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){ sum+=G.bugs[i].w; n++; }
    return n? (int)(sum/n*10.0f) : 0; }
__attribute__((export_name("dbg_bugbig"))) int exp_dbg_bugbig(void){
    int n=0; for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive && G.bugs[i].w>60.0f) n++; return n; }
__attribute__((export_name("dbg_carried"))) int exp_dbg_carried(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)(G.p[id].carried*10); }
__attribute__((export_name("dbg_hp"))) int exp_dbg_hp(void){
    int id = G.my_id>=0? G.my_id : 0; return G.p[id].hp; }
__attribute__((export_name("dbg_maxbug"))) int exp_dbg_maxbug(void){
    float m=0; for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive && G.bugs[i].w>m) m=G.bugs[i].w; return (int)(m*10); }
__attribute__((export_name("dbg_walls"))) int exp_dbg_walls(void){
    int n=0; for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1) n++; return n; }
__attribute__((export_name("dbg_storesel"))) int exp_dbg_storesel(void){ return G.store_sel; }
__attribute__((export_name("dbg_pads"))) int exp_dbg_pads(void){ return IN.pad_count; }
__attribute__((export_name("dbg_dev"))) int exp_dbg_dev(void){ return dev_local(); }
__attribute__((export_name("dbg_devp2"))) int exp_dbg_devp2(void){ return G.p2_slot>=0? dev_of(G.p2_slot) : -1; }
__attribute__((export_name("dbg_lbl"))) char *exp_dbg_lbl(int act){
    static char b[16]; scpy(b, lbl2(dev_local(), act), 16); return b; }
__attribute__((export_name("dbg_lblp2"))) char *exp_dbg_lblp2(int act){
    static char b[16]; scpy(b, G.p2_slot>=0? lbl2(dev_of(G.p2_slot), act) : "", 16); return b; }
__attribute__((export_name("dbg_menun"))) int exp_dbg_menun(void){ return menu_count(); }
__attribute__((export_name("dbg_mdiff"))) int exp_dbg_mdiff(void){ return G.difficulty; }
__attribute__((export_name("dbg_mzen"))) int exp_dbg_mzen(void){ return G.zen; }
__attribute__((export_name("dbg_meddie"))) int exp_dbg_meddie(void){ return G.eddie; }
__attribute__((export_name("dbg_coop"))) int exp_dbg_coop(void){ return G.coop_pref; }
__attribute__((export_name("dbg_p2"))) int exp_dbg_p2(void){ return G.p2_slot; }
__attribute__((export_name("dbg_p2x"))) int exp_dbg_p2x(void){ return G.p2_slot>=0? (int)G.p[G.p2_slot].x : -99999; }
__attribute__((export_name("dbg_p2y"))) int exp_dbg_p2y(void){ return G.p2_slot>=0? (int)G.p[G.p2_slot].y : -99999; }
__attribute__((export_name("dbg_p2hp"))) int exp_dbg_p2hp(void){ return G.p2_slot>=0? G.p[G.p2_slot].hp : -1; }
__attribute__((export_name("dbg_split"))) int exp_dbg_split(void){ return vsplit; }
__attribute__((export_name("dbg_orient"))) int exp_dbg_orient(void){ return vorient; }
__attribute__((export_name("dbg_bowl_tx"))) int exp_dbg_bowl_tx(int pi){ return (int)G.bowl[pi&3].thrower_x; }
__attribute__((export_name("dbg_kill2"))) int exp_dbg_kill2(void){ if (G.p2_slot>=0) hurt_player(G.p2_slot, 100000.0f); return 0; }
__attribute__((export_name("dbg_bowl_ph"))) int exp_dbg_bowl_ph(int pi){ Bowl *b=&G.bowl[pi&3]; return b->active? b->phase : -1; }
__attribute__((export_name("dbg_splitf"))) int exp_dbg_splitf(void){ return (int)(vsplit_f*100); }
__attribute__((export_name("dbg_store2"))) int exp_dbg_store2(void){ return lstore[1]; }
__attribute__((export_name("dbg_store1"))) int exp_dbg_store1(void){ return lstore[0]; }
__attribute__((export_name("dbg_storesel2"))) int exp_dbg_storesel2(void){ return G.store_sel2; }
__attribute__((export_name("dbg_give2"))) void exp_dbg_give2(int w){
    if (G.p2_slot>=0 && w>=0 && w<NWEAP) G.p[G.p2_slot].owned[w]++;
}
__attribute__((export_name("dbg_zen"))) int exp_dbg_zen(void){ return G.zen_run; }
__attribute__((export_name("dbg_projs"))) int exp_dbg_projs(void){
    int n=0; for (int i=0;i<MAXPROJ;i++) if (G.projs[i].alive) n++; return n; }
__attribute__((export_name("dbg_upg"))) int exp_dbg_upg(int u){
    int id = G.my_id>=0? G.my_id : 0; return (u>=0 && u<NUPG)? G.p[id].upg[u] : -1; }
__attribute__((export_name("dbg_setupg"))) void exp_dbg_setupg(int u, int lv){
    int id = G.my_id>=0? G.my_id : 0; if (u>=0 && u<NUPG && lv>=0) G.p[id].upg[u]=lv; }
__attribute__((export_name("dbg_wof"))) int exp_dbg_wof(void){
    int id = G.my_id>=0? G.my_id : 0; return G.p[id].wof>0? 1 : 0; }
__attribute__((export_name("dbg_speed"))) int exp_dbg_speed(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)(player_speed(&G.p[id])*10.0f); }
__attribute__((export_name("dbg_shieldmax"))) int exp_dbg_shieldmax(void){
    int id = G.my_id>=0? G.my_id : 0; return G.p[id].shield_max; }
__attribute__((export_name("dbg_spawnbug"))) void exp_dbg_spawnbug(float x, float y){ spawn_bug(x, y); }
__attribute__((export_name("dbg_aggro"))) int exp_dbg_aggro(void){
    int n=0; for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive && G.bugs[i].aggro>=0) n++;
    return n; }
__attribute__((export_name("dbg_bugidx"))) int exp_dbg_bugidx(float x, float y){
    int best=-1; float bd=1e18f;
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
        float dx=G.bugs[i].x-x, dy=G.bugs[i].y-y, d2=dx*dx+dy*dy;
        if (d2<bd){ bd=d2; best=i; }
    }
    return best; }
__attribute__((export_name("dbg_bugx"))) int exp_dbg_bugx(int i){
    return (i>=0 && i<MAXBUGS && G.bugs[i].alive)? (int)G.bugs[i].x : -99999; }
__attribute__((export_name("dbg_bugy"))) int exp_dbg_bugy(int i){
    return (i>=0 && i<MAXBUGS && G.bugs[i].alive)? (int)G.bugs[i].y : -99999; }
__attribute__((export_name("dbg_bugaggro"))) int exp_dbg_bugaggro(int i){
    return (i>=0 && i<MAXBUGS && G.bugs[i].alive)? G.bugs[i].aggro : -99999; }
__attribute__((export_name("dbg_px"))) int exp_dbg_px(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)G.p[id].x; }
__attribute__((export_name("dbg_py"))) int exp_dbg_py(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)G.p[id].y; }
__attribute__((export_name("dbg_budy"))) int exp_dbg_budy(void){
    int id = G.my_id>=0? G.my_id : 0; return G.bud[id].active? (int)G.bud[id].y : -99999; }
__attribute__((export_name("dbg_rollback"))) int exp_dbg_rollback(void){
    int id = G.my_id>=0? G.my_id : 0; return G.p[id].rollback; }
__attribute__((export_name("dbg_touchctl"))) int exp_dbg_touchctl(int v){
    (void)v; G.touch_ctl=1; return G.touch_ctl; }   // v1.33: the scheme is pinned — the export stays so STICK suites keep their setter, and a 0 is coerced exactly like an old save's
__attribute__((export_name("dbg_beans"))) int exp_dbg_beans(int v){
    int id = G.my_id>=0? G.my_id : 0;
    if (v>=0) G.p[id].beans=v; return G.p[id].beans; }
__attribute__((export_name("dbg_beant"))) int exp_dbg_beant(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)(G.p[id].bean_t*100); }
__attribute__((export_name("dbg_batkcd"))) int exp_dbg_batkcd(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)(G.bud[id].atk_cd*100); }
__attribute__((export_name("dbg_givestick"))) int exp_dbg_givestick(void){
    int id = G.my_id>=0? G.my_id : 0; BWS[id].ent_sticks += 3; return BWS[id].ent_sticks; }
__attribute__((export_name("dbg_threat"))) int exp_dbg_threat(void){
    compute_threat(); return (int)(G.threat*100); }
__attribute__((export_name("dbg_region"))) int exp_dbg_region(void){
    int id = G.my_id>=0? G.my_id : 0;
    float d = fsqrt(G.p[id].x*G.p[id].x + G.p[id].y*G.p[id].y);
    return region_at(d); }
__attribute__((export_name("dbg_beamlen"))) int exp_dbg_beamlen(void){
    int id = G.my_id>=0? G.my_id : 0; return beam_len[id]; }
__attribute__((export_name("dbg_zoom"))) int exp_dbg_zoom(void){
    return (int)(G.zoom*1000); }
__attribute__((export_name("dbg_beams"))) int exp_dbg_beams(void){
    int id = G.my_id>=0? G.my_id : 0; return beam_n[id]; }
__attribute__((export_name("dbg_beamang"))) int exp_dbg_beamang(int k){
    int id = G.my_id>=0? G.my_id : 0; return (k>=0 && k<MAXCOPIES)? (int)(beam_ang[id][k]*1000) : -9999; }
__attribute__((export_name("dbg_weapon"))) int exp_dbg_weapon(void){
    int id = G.my_id>=0? G.my_id : 0; return G.p[id].weapon; }
__attribute__((export_name("dbg_copies"))) int exp_dbg_copies(int w){
    int id = G.my_id>=0? G.my_id : 0; return (w>=0 && w<5)? G.p[id].owned[w] : -1; }
__attribute__((export_name("dbg_feed"))) int exp_dbg_feed(void){
    int id = G.my_id>=0? G.my_id : 0;
    for (int i=0;i<5;i++) hat_push(&G.p[id], 2.0f);
    return G.p[id].stack_n; }
__attribute__((export_name("dbg_hurt"))) int exp_dbg_hurt(int dmg){
    int id = G.my_id>=0? G.my_id : 0; hurt_player(id, (float)dmg); return G.p[id].hp; }
__attribute__((export_name("dbg_bowl_new"))) void exp_dbg_bowl_new(void){ bowl_new_game(G.my_id); }
__attribute__((export_name("dbg_bowl_roll"))) int exp_dbg_bowl_roll(int k){
    Bowl *bw=&G.bowl[G.my_id];
    if (!bw->active) bowl_new_game(G.my_id);
    int up = bowl_pins_up(bw);
    if (k>up) k=up; if (k<0) k=0;
    bw->pins_before = up;
    int togo = k;   // pin order is irrelevant to the scoring being tested
    for (int i=0;i<10 && togo>0;i++) if (bw->standing[i]){ bw->standing[i]=0; togo--; }
    bowl_record(bw, k);
    return bw->nrolls;
}
__attribute__((export_name("dbg_bowl_score"))) int exp_dbg_bowl_score(void){
    Bowl *bw=&G.bowl[G.my_id];
    if (bw->game_over) return bw->final;
    int t=-1; for (int f=0;f<10;f++){ int c=bowl_cum(bw, f); if (c<0) break; t=c; }
    return t;
}
__attribute__((export_name("dbg_bowl_frame"))) int exp_dbg_bowl_frame(void){ return G.bowl[G.my_id].frame*10 + G.bowl[G.my_id].ball; }
__attribute__((export_name("dbg_bowl_msgt"))) int exp_dbg_bowl_msgt(void){ return (int)(G.bowl[G.my_id].msg_t*1000); }
__attribute__((export_name("dbg_bowl_over"))) int exp_dbg_bowl_over(void){ return G.bowl[G.my_id].game_over; }
__attribute__((export_name("dbg_bowl_pins"))) int exp_dbg_bowl_pins(void){ return bowl_pins_up(&G.bowl[G.my_id]); }
// BUG BOWL LEAGUE (v1.25 feat 10): the league book of the local
// player, one field per id (0 games, 1 best, 2 pins, 3 strikes,
// 4 season, 5 night, 6 spins, 7 trophies, 8 last_score, 9 last_place,
// 10 fin_place, 11 last_night, 12 fin_season, 13 xp-pool note via
// dbg_lgxp below), plus the rival functions the draw itself reads —
// so the suite asserts the game's own arithmetic, not a copy.
__attribute__((export_name("dbg_lg"))) int exp_dbg_lg(int f){
    BowlLeague *L=&G.blg[G.my_id];
    switch (f){
        case 0: return L->games; case 1: return L->best; case 2: return L->pins;
        case 3: return L->strikes; case 4: return L->season; case 5: return L->night;
        case 6: return L->spins; case 7: return L->trophies; case 8: return L->last_score;
        case 9: return L->last_place; case 10: return L->fin_place; case 11: return L->last_night;
        case 12: return L->fin_season; default: return -1;
    }
}
__attribute__((export_name("dbg_lgrival"))) int exp_dbg_lgrival(int season, int night, int r){ return lg_rival_score(season, night, r); }
__attribute__((export_name("dbg_lgtotal"))) int exp_dbg_lgtotal(int season, int r, int nights){ return lg_rival_total(season, r, nights); }
__attribute__((export_name("dbg_lgname"))) char *exp_dbg_lgname(int r){ return (char*)LG_RIVAL_NAME[r<0? 0 : r>2? 2 : r]; }
__attribute__((export_name("dbg_track"))) int exp_dbg_track(void){ return mus_piece(); }
__attribute__((export_name("dbg_setthreat"))) void exp_dbg_setthreat(int t){ threat_override = t<0? -1 : t*0.1f; }
__attribute__((export_name("dbg_diff"))) int exp_dbg_diff(void){ return G.diff_run; }
__attribute__((export_name("dbg_setdiff"))) void exp_dbg_setdiff(int d){ if (d>=0 && d<4) G.diff_run=d; }
__attribute__((export_name("dbg_bank"))) int exp_dbg_bank(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)G.p[id].banked; }
__attribute__((export_name("dbg_hatstand"))) int exp_dbg_hatstand(void){
    int id = G.my_id>=0? G.my_id : 0; return G.p[id].hatstand; }
__attribute__((export_name("dbg_mall"))) int exp_dbg_mall(void){
    int id = G.my_id>=0? G.my_id : 0; return G.p[id].in_mall; }
__attribute__((export_name("dbg_station"))) int exp_dbg_station(void){ return G.store_station; }
__attribute__((export_name("dbg_onwall"))) int exp_dbg_onwall(void){
    int id = G.my_id>=0? G.my_id : 0; return G.p[id].on_wall>=0? 1:0; }
__attribute__((export_name("dbg_grpmax"))) int exp_dbg_grpmax(void){
    int m=0; for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive){
        int s = G.walls[i].grp>=0? grp_n[G.walls[i].grp] : 1;
        if (s>m) m=s; }
    return m; }
__attribute__((export_name("dbg_pool"))) int exp_dbg_pool(void){
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive && G.walls[i].grp>=0)
        return (int)grp_pool[G.walls[i].grp];
    return 0; }
__attribute__((export_name("dbg_buddy"))) int exp_dbg_buddy(void){
    int id = G.my_id>=0? G.my_id : 0; return G.bud[id].active? G.bud[id].species+1 : 0; }
__attribute__((export_name("dbg_buddyxp"))) int exp_dbg_buddyxp(void){
    int id = G.my_id>=0? G.my_id : 0; return G.bud[id].xp; }
__attribute__((export_name("dbg_buddyhp"))) int exp_dbg_buddyhp(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].hp; }
__attribute__((export_name("dbg_budname"))) int exp_dbg_budname(void){
    int id = G.my_id>=0? G.my_id : 0; return G.bud[id].name_idx; }
__attribute__((export_name("dbg_budcolor"))) int exp_dbg_budcolor(void){
    int id = G.my_id>=0? G.my_id : 0; return G.bud[id].color; }
__attribute__((export_name("dbg_setbudxp"))) void exp_dbg_setbudxp(int v){
    int id = G.my_id>=0? G.my_id : 0; G.bud[id].xp = v; }
__attribute__((export_name("dbg_budfaint"))) void exp_dbg_budfaint(void){
    int id = G.my_id>=0? G.my_id : 0; hurt_buddy(id, 99999.0f); }
__attribute__((export_name("dbg_budups"))) int exp_dbg_budups(void){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id];
    return b->up_hp*1000 + b->up_atk*100 + b->up_def*10 + b->skills; }
__attribute__((export_name("dbg_bwstate"))) int exp_dbg_bwstate(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].in_bw? 1 : 0; }
__attribute__((export_name("dbg_entroom"))) int exp_dbg_entroom(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_room; }
__attribute__((export_name("dbg_entdoor"))) int exp_dbg_entdoor(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_sub? BWS[id].ent_door_sub : BWS[id].ent_door_upto; }
__attribute__((export_name("dbg_entkey"))) int exp_dbg_entkey(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_key; }
__attribute__((export_name("dbg_entbits"))) int exp_dbg_entbits(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_bits; }
__attribute__((export_name("dbg_entev"))) int exp_dbg_entev(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_ev*10 + BWS[id].ent_phase; }
__attribute__((export_name("dbg_entsub"))) int exp_dbg_entsub(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_sub; }
__attribute__((export_name("dbg_entsubb"))) int exp_dbg_entsubb(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].ent_sub_best; }
__attribute__((export_name("dbg_enttrig"))) int exp_dbg_enttrig(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id]; S->ent_ev = 7; S->ent_phase = 1; S->ent_t = 0.45f; S->ent_passes = 1; return 7; }
__attribute__((export_name("dbg_entfuse"))) int exp_dbg_entfuse(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_fuse_mask; }
__attribute__((export_name("dbg_entlib"))) int exp_dbg_entlib(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_libdone; }
__attribute__((export_name("dbg_entrec"))) int exp_dbg_entrec(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].ent_best + G.bud[id].ent_done*10000; }
__attribute__((export_name("dbg_entsticks"))) int exp_dbg_entsticks(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_sticks; }
__attribute__((export_name("dbg_entchase"))) int exp_dbg_entchase(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_chase; }
__attribute__((export_name("dbg_entx"))) int exp_dbg_entx(void){ int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].x; }
__attribute__((export_name("dbg_bwent2"))) void exp_dbg_bwent2(void){ int id = G.my_id>=0? G.my_id : 0; mall_enter(id); bw_enter_mode(id, 2); }
__attribute__((export_name("dbg_inarcade"))) int exp_dbg_inarcade(void){ int id = G.my_id>=0? G.my_id : 0; return G.p[id].in_arcade; }
__attribute__((export_name("dbg_arcade"))) void exp_dbg_arcade(void){ int id = G.my_id>=0? G.my_id : 0; if (!G.p[id].in_mall) mall_enter(id); arcade_enter(id); }
__attribute__((export_name("dbg_cabinet"))) int exp_dbg_cabinet(void){ int id = G.my_id>=0? G.my_id : 0; return cabinet_at(&G.p[id]); }
__attribute__((export_name("dbg_casroom"))) int exp_dbg_casroom(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].cas_room; }
__attribute__((export_name("dbg_casscore"))) int exp_dbg_casscore(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].cas_score; }
__attribute__((export_name("dbg_castreasures"))) int exp_dbg_castreasures(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].cas_treasures; }
__attribute__((export_name("dbg_casitems"))) int exp_dbg_casitems(void){ int id = G.my_id>=0? G.my_id : 0; return (BWS[id].cas_sword? 1 : 0) + (BWS[id].cas_key? 2 : 0) + (BWS[id].cas_won? 4 : 0); }
__attribute__((export_name("dbg_casgive"))) int exp_dbg_casgive(int what){ int id = G.my_id>=0? G.my_id : 0; if (what==1) BWS[id].cas_sword = 1; else if (what==2) BWS[id].cas_key = 1; return BWS[id].cas_sword + BWS[id].cas_key*2; }
__attribute__((export_name("dbg_casmon"))) int exp_dbg_casmon(void){ int id = G.my_id>=0? G.my_id : 0; int n = 0; for (int i=0;i<15;i++) if (BWS[id].cas_malive[i] && BWS[id].cas_mroom[i]==BWS[id].cas_room) n++; return n; }
__attribute__((export_name("dbg_casbest"))) int exp_dbg_casbest(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].cas_best; }
__attribute__((export_name("dbg_casshuffle"))) int exp_dbg_casshuffle(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].cas_shuffle; }
__attribute__((export_name("dbg_casseed"))) int exp_dbg_casseed(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].cas_seed; }
__attribute__((export_name("dbg_cassetseed"))) int exp_dbg_cassetseed(int n){ g_cas_seed_override = n; return n; }
__attribute__((export_name("dbg_casitemroom"))) int exp_dbg_casitemroom(int it){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].cas_iroom[it]; }
__attribute__((export_name("dbg_casitemx"))) int exp_dbg_casitemx(int it){ int id = G.my_id>=0? G.my_id : 0; return (int)(BWS[id].cas_ix[it] - 20000.0f); }
__attribute__((export_name("dbg_casitemy"))) int exp_dbg_casitemy(int it){ int id = G.my_id>=0? G.my_id : 0; return (int)(BWS[id].cas_iy[it] - 20000.0f); }
__attribute__((export_name("dbg_casmonroom"))) int exp_dbg_casmonroom(int m){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].cas_mroom[m]; }
__attribute__((export_name("dbg_casmontype"))) int exp_dbg_casmontype(int m){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].cas_mtype[m]; }
__attribute__((export_name("dbg_casmonx"))) int exp_dbg_casmonx(int m){ int id = G.my_id>=0? G.my_id : 0; return (int)(BWS[id].cas_mx[m] - 20000.0f); }
__attribute__((export_name("dbg_casmony"))) int exp_dbg_casmony(int m){ int id = G.my_id>=0? G.my_id : 0; return (int)(BWS[id].cas_my[m] - 20000.0f); }
// TROPHY CASE (v1.25 feat 4): the case panel's own words, read back
// through the very helpers the panel draws with — the suite asserts
// what the player sees, not a re-implementation of it.
__attribute__((export_name("dbg_caserows"))) int exp_dbg_caserows(void){ int id = G.my_id>=0? G.my_id : 0; return station_rows(5, &G.p[id]); }
__attribute__((export_name("dbg_casename"))) char *exp_dbg_casename(int row){ static char b[48]; int id = G.my_id>=0? G.my_id : 0; const char *s = station_row_name(5, row, &G.p[id], b); if (s!=b) scpy(b, s, 48); return b; }
__attribute__((export_name("dbg_caseright"))) char *exp_dbg_caseright(int row){ static char b[32]; int id = G.my_id>=0? G.my_id : 0; const char *s = station_row_right(5, row, &G.p[id], b); if (s!=b) scpy(b, s, 32); return b; }
__attribute__((export_name("dbg_caseblurb"))) char *exp_dbg_caseblurb(int row){ int id = G.my_id>=0? G.my_id : 0; return (char*)station_blurb(5, row, &G.p[id]); }
// WEEKLY CHALLENGE (v1.25 feat 6): the board's own words through the
// panel helpers, plus the week seam — dbg_wksetday pins the perceived
// day (a day number; <0 hands the clock back to the device) so the
// suite can live in a known week and roll weeks on demand.
__attribute__((export_name("dbg_wksetday"))) int exp_dbg_wksetday(int d){ wk_test_set_day(d); return wk_week_now(); }
__attribute__((export_name("dbg_wkweek"))) int exp_dbg_wkweek(void){ return wk_week_now(); }
__attribute__((export_name("dbg_wk"))) int exp_dbg_wk(void){ return G.wk_week; }
__attribute__((export_name("dbg_wktype"))) int exp_dbg_wktype(void){ if (!G.wk_week) return -1; int t,tg; wk_challenge(G.wk_week, &t, &tg); return t; }
__attribute__((export_name("dbg_wktarget"))) int exp_dbg_wktarget(void){ if (!G.wk_week) return -1; int t,tg; wk_challenge(G.wk_week, &t, &tg); return tg; }
__attribute__((export_name("dbg_wkprog"))) int exp_dbg_wkprog(void){ return G.wk_week? wk_progress() : -1; }
__attribute__((export_name("dbg_wkdone"))) int exp_dbg_wkdone(void){ return (G.wk_week && G.wk_done_week==G.wk_week)? 1 : 0; }
__attribute__((export_name("dbg_wktotal"))) int exp_dbg_wktotal(void){ return G.wk_total; }
__attribute__((export_name("dbg_wkname"))) char *exp_dbg_wkname(void){ static char b[24]; if (!G.wk_week){ scpy(b, "", 24); return b; } int t,tg; wk_challenge(G.wk_week, &t, &tg); scpy(b, WK_NAME[t], 24); return b; }
__attribute__((export_name("dbg_wkrows"))) int exp_dbg_wkrows(void){ int id = G.my_id>=0? G.my_id : 0; return station_rows(6, &G.p[id]); }
__attribute__((export_name("dbg_wkrowname"))) char *exp_dbg_wkrowname(int row){ static char b[48]; int id = G.my_id>=0? G.my_id : 0; const char *s = station_row_name(6, row, &G.p[id], b); if (s!=b) scpy(b, s, 48); return b; }
__attribute__((export_name("dbg_wkrowright"))) char *exp_dbg_wkrowright(int row){ static char b[32]; int id = G.my_id>=0? G.my_id : 0; const char *s = station_row_right(6, row, &G.p[id], b); if (s!=b) scpy(b, s, 32); return b; }
// THE LIVING SKY (v1.25 feat 12): the clock seam and the pure
// functions, observed. dbg_todset pins the perceived minutes of day
// (tenths; <0 hands the clock back to the device) and dbg_todday
// pins the perceived daynum for the sky/weather only — the Weekly
// Challenge's own seam is untouched. Nothing here is ever saved.
__attribute__((export_name("dbg_todset"))) int exp_dbg_todset(int min_x10){ tod_test_set_min(min_x10 < 0? -1.0f : min_x10/10.0f); return (int)(tod_minutes()*10.0f); }
__attribute__((export_name("dbg_todday"))) int exp_dbg_todday(int d){ tod_test_set_day(d); return tod_daynum(); }
__attribute__((export_name("dbg_todmin"))) int exp_dbg_todmin(void){ return (int)(tod_minutes()*10.0f); }
__attribute__((export_name("dbg_toddk"))) int exp_dbg_toddk(void){ return (int)(tod_darkf()*1000.0f); }
__attribute__((export_name("dbg_todwarm"))) int exp_dbg_todwarm(void){ return (int)(tod_warmf()*1000.0f); }
__attribute__((export_name("dbg_todsky"))) int exp_dbg_todsky(int r, int g, int b){ u32 c = tod_sky(COL(r, g, b)); return (COL_R(c)<<16) | (COL_G(c)<<8) | COL_B(c); }
__attribute__((export_name("dbg_wxkind"))) int exp_dbg_wxkind(int region){ return (region < 0 || region >= NREGIONS)? -1 : WX_KIND[region]; }
__attribute__((export_name("dbg_wxepoch"))) int exp_dbg_wxepoch(void){ return wx_epoch_now(); }
__attribute__((export_name("dbg_wx"))) int exp_dbg_wx(int region){ return wx_state(region, wx_epoch_now()); }
__attribute__((export_name("dbg_wxat"))) int exp_dbg_wxat(int region, int epoch){ return wx_state(region, epoch); }
// NAMED ELITES (v1.25 feature 8): the bounty board's state, the name
// derivation, the live elite, and the force seam — dbg_bnforce arms
// the elevation for the NEXT spawn_bug, which still has to pass the
// in-spawn gate (cooldown, no live elite, not a client), exactly the
// way the spawner's own roll does.
__attribute__((export_name("dbg_bnserial"))) int exp_dbg_bnserial(void){ return G.bn_serial; }
__attribute__((export_name("dbg_bndone"))) int exp_dbg_bndone(void){ return G.bn_done; }
__attribute__((export_name("dbg_bncool"))) int exp_dbg_bncool(void){ return (int)(G.bn_cool+0.5f); }
__attribute__((export_name("dbg_bnname"))) char *exp_dbg_bnname(void){ static char b[24]; scpy(b, ELITE_NAME[bn_name_idx(G.bn_serial)], 24); return b; }
__attribute__((export_name("dbg_bntitle"))) char *exp_dbg_bntitle(void){ static char b[48]; scpy(b, ELITE_TITLE[bn_name_idx(G.bn_serial)], 48); return b; }
__attribute__((export_name("dbg_bnkind"))) int exp_dbg_bnkind(void){ return bn_kind_of(G.bn_serial); }
__attribute__((export_name("dbg_bnforce"))) int exp_dbg_bnforce(void){ bn_force = 1; return 1; }
__attribute__((export_name("dbg_bnlive"))) int exp_dbg_bnlive(void){ return G.bn_live; }
__attribute__((export_name("dbg_bnelite"))) int exp_dbg_bnelite(void){ int li = bn_any_live(); return li<0? -1 : G.bugs[li].elite-1; }
__attribute__((export_name("dbg_bnelitew"))) int exp_dbg_bnelitew(void){ int li = bn_any_live(); return li<0? -1 : (int)(G.bugs[li].w+0.5f); }
__attribute__((export_name("dbg_bnx"))) int exp_dbg_bnx(void){ int li = bn_any_live(); return li<0? -99999 : (int)G.bugs[li].x; }
__attribute__((export_name("dbg_bny"))) int exp_dbg_bny(void){ int li = bn_any_live(); return li<0? -99999 : (int)G.bugs[li].y; }
__attribute__((export_name("dbg_bnsetcool"))) int exp_dbg_bnsetcool(int s){ G.bn_cool = (float)s; return (int)G.bn_cool; }
__attribute__((export_name("dbg_bnrows"))) int exp_dbg_bnrows(void){ int id = G.my_id>=0? G.my_id : 0; return station_rows(7, &G.p[id]); }
__attribute__((export_name("dbg_bnrowname"))) char *exp_dbg_bnrowname(int row){ static char b[48]; int id = G.my_id>=0? G.my_id : 0; const char *s = station_row_name(7, row, &G.p[id], b); if (s!=b) scpy(b, s, 48); return b; }
__attribute__((export_name("dbg_bnrowright"))) char *exp_dbg_bnrowright(int row){ static char b[32]; int id = G.my_id>=0? G.my_id : 0; const char *s = station_row_right(7, row, &G.p[id], b); if (s!=b) scpy(b, s, 32); return b; }
__attribute__((export_name("dbg_bnblurb"))) char *exp_dbg_bnblurb(int row){ int id = G.my_id>=0? G.my_id : 0; return (char*)station_blurb(7, row, &G.p[id]); }
__attribute__((export_name("dbg_topearned"))) int exp_dbg_topearned(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].top_earned; }
__attribute__((export_name("dbg_topworn"))) int exp_dbg_topworn(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].top_worn; }
__attribute__((export_name("dbg_topwornp"))) int exp_dbg_topwornp(void){ int id = G.my_id>=0? G.my_id : 0; return G.p[id].top_worn; }
__attribute__((export_name("dbg_topname"))) char *exp_dbg_topname(int t){ static char b[20]; if (t<0||t>TOP_N) t=0; scpy(b, TOP_NAME[t], 20); return b; }
__attribute__((export_name("dbg_topheavy"))) int exp_dbg_topheavy(int kg){ int id = G.my_id>=0? G.my_id : 0; G.p[id].heaviest = (float)kg; return (int)G.p[id].heaviest; }
__attribute__((export_name("dbg_budrows"))) int exp_dbg_budrows(void){ int id = G.my_id>=0? G.my_id : 0; return station_rows(G.p[id].in_store? G.p[id].station : 3, &G.p[id]); }
__attribute__((export_name("dbg_budrowname"))) char *exp_dbg_budrowname(int row){ static char b[48]; int id = G.my_id>=0? G.my_id : 0; const char *s = station_row_name(G.p[id].in_store? G.p[id].station : 3, row, &G.p[id], b); if (s!=b) scpy(b, s, 48); return b; }
// HAT PORTRAIT BOOTH (v1.25 feat 9): the booth's state, read-only —
// suites enter and pose through real input and observe through these
__attribute__((export_name("dbg_portstage"))) int exp_dbg_portstage(void){ int id = G.my_id>=0? G.my_id : 0;
    // feat 9's numbering, kept stable across the MALL ENLARGEMENT: 5 = the
    // booth's rows are open, 6 = the gallery, 0 = neither. The booth is
    // station 8 now and its stage lives in booth_stage.
    if (booth_stage[id]) return 6;
    if (G.p[id].in_store && G.p[id].station==8) return 5;
    return bud_stage[id]; }
__attribute__((export_name("dbg_portbd"))) int exp_dbg_portbd(void){ int id = G.my_id>=0? G.my_id : 0; return port_bd[id]; }
__attribute__((export_name("dbg_porthat"))) int exp_dbg_porthat(void){ int id = G.my_id>=0? G.my_id : 0; return port_hat[id]; }
__attribute__((export_name("dbg_porthas"))) int exp_dbg_porthas(void){ int id = G.my_id>=0? G.my_id : 0; return portrait[id].has; }
__attribute__((export_name("dbg_portkg"))) int exp_dbg_portkg(void){ int id = G.my_id>=0? G.my_id : 0; return portrait[id].kg; }
__attribute__((export_name("dbg_porttop"))) int exp_dbg_porttop(void){ int id = G.my_id>=0? G.my_id : 0; return portrait[id].top_worn; }
__attribute__((export_name("dbg_portn"))) int exp_dbg_portn(void){ int id = G.my_id>=0? G.my_id : 0; return portrait[id].stack_n; }
__attribute__((export_name("dbg_portbk"))) int exp_dbg_portbk(void){ int id = G.my_id>=0? G.my_id : 0; return portrait[id].backdrop; }
__attribute__((export_name("dbg_porthaton"))) int exp_dbg_porthaton(void){ int id = G.my_id>=0? G.my_id : 0; return portrait[id].hat_on; }
__attribute__((export_name("dbg_portkind"))) int exp_dbg_portkind(int i){ int id = G.my_id>=0? G.my_id : 0; if (i<0 || i>=MAXSTACK) return -1; return portrait[id].kinds[i]; }
__attribute__((export_name("dbg_portserial"))) int exp_dbg_portserial(void){ return G.port_req_serial; }
__attribute__((export_name("dbg_budrowright"))) char *exp_dbg_budrowright(int row){ static char b[32]; int id = G.my_id>=0? G.my_id : 0; const char *s = station_row_right(G.p[id].in_store? G.p[id].station : 3, row, &G.p[id], b); if (s!=b) scpy(b, s, 32); return b; }
__attribute__((export_name("dbg_casx"))) int exp_dbg_casx(void){ int id = G.my_id>=0? G.my_id : 0; return (int)(G.bud[id].x - 20000.0f); }
__attribute__((export_name("dbg_casy"))) int exp_dbg_casy(void){ int id = G.my_id>=0? G.my_id : 0; return (int)(G.bud[id].y - 20000.0f); }
__attribute__((export_name("dbg_crtstate"))) int exp_dbg_crtstate(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].crt_state; }
__attribute__((export_name("dbg_crtrace"))) int exp_dbg_crtrace(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].crt_race; }
__attribute__((export_name("dbg_crttrack"))) int exp_dbg_crttrack(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].crt_track; }
__attribute__((export_name("dbg_crtpts"))) int exp_dbg_crtpts(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].crt_pts[0]; }
__attribute__((export_name("dbg_crtcup"))) int exp_dbg_crtcup(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].crt_cup; }
__attribute__((export_name("dbg_crtcupbest"))) int exp_dbg_crtcupbest(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].crt_cupbest; }
__attribute__((export_name("dbg_crtgprank"))) int exp_dbg_crtgprank(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].crt_gprank[0]; }
__attribute__((export_name("dbg_crtduo"))) int exp_dbg_crtduo(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].crt_duo; }
__attribute__((export_name("dbg_crtpos2"))) int exp_dbg_crtpos2(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].crt_place[1]; }
__attribute__((export_name("dbg_crtprog2"))) int exp_dbg_crtprog2(void){ int id = G.my_id>=0? G.my_id : 0; return (int)BWS[id].crt_prog[1]; }
__attribute__((export_name("dbg_crtbest2"))) int exp_dbg_crtbest2(void){ return (G.p2_slot>=0)? G.bud[G.p2_slot].crt_best_pos : -1; }
__attribute__((export_name("dbg_crtskip2"))) int exp_dbg_crtskip2(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (G.bud[id].bw_mode != 7 || S->crt_duo < 0) return -1;
    S->crt_state = 1; S->crt_count = 0;
    S->crt_prog[1] = 3.0f * CRT_LEN - 130.0f;
    { float x, y, hx, hy; crt_point_at(CRT_OFF + S->crt_prog[1], 0, &x, &y, &hx, &hy);
      S->crt_x[1] = x; S->crt_y[1] = y; S->crt_h[1] = fatan2(hy, hx); S->crt_v[1] = 120.0f;
      S->crt_seg[1] = crt_nearest_full(x, y); }
    return (int)S->crt_prog[1]; }
__attribute__((export_name("dbg_crtpos"))) int exp_dbg_crtpos(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].crt_place[0]; }
__attribute__((export_name("dbg_crtlap"))) int exp_dbg_crtlap(void){ int id = G.my_id>=0? G.my_id : 0; int l = (int)(BWS[id].crt_prog[0] / CRT_LEN) + 1; if (l<1) l=1; if (l>3) l=3; return l; }
__attribute__((export_name("dbg_crtprog"))) int exp_dbg_crtprog(void){ int id = G.my_id>=0? G.my_id : 0; return (int)BWS[id].crt_prog[0]; }
__attribute__((export_name("dbg_crttime"))) int exp_dbg_crttime(void){ int id = G.my_id>=0? G.my_id : 0; return (int)(BWS[id].crt_t * 10.0f); }
__attribute__((export_name("dbg_crtbest"))) int exp_dbg_crtbest(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].crt_best_pos; }
__attribute__((export_name("dbg_crtbesttime"))) int exp_dbg_crtbesttime(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].crt_best_time; }
// MEDAL TIMES (v1.25 feat 3): read the per-track best + derived medal
// through the same table the game renders with; set the live race clock
// so a test can land a finish at a chosen time via the real record path
__attribute__((export_name("dbg_crttbest"))) int exp_dbg_crttbest(int track){ int id = G.my_id>=0? G.my_id : 0; if (track<0 || track>7) return -1; return G.bud[id].crt_tbest[track]; }
__attribute__((export_name("dbg_crtmedal"))) int exp_dbg_crtmedal(int track){ int id = G.my_id>=0? G.my_id : 0; if (track<0 || track>7) return -1; return G.bud[id].crt_medal_best[track]; }   // v1.27: the EARNED medal (podium rule), not a derivation from the best time
__attribute__((export_name("dbg_crtsettime"))) int exp_dbg_crtsettime(int ds){ int id = G.my_id>=0? G.my_id : 0; BWS[id].crt_t = ds * 0.1f; return (int)(BWS[id].crt_t * 10.0f); }
__attribute__((export_name("dbg_crtskip"))) int exp_dbg_crtskip(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (G.bud[id].bw_mode != 7) return -1;
    // park the player a short straight from home on the final lap
    S->crt_state = 1; S->crt_count = 0;
    S->crt_prog[0] = 3.0f * CRT_LEN - 130.0f;
    { float x, y, hx, hy; crt_point_at(CRT_OFF + S->crt_prog[0], 0, &x, &y, &hx, &hy);
      S->crt_x[0] = x; S->crt_y[0] = y; S->crt_h[0] = fatan2(hy, hx); S->crt_v[0] = 120.0f;
      S->crt_seg[0] = crt_nearest_full(x, y); }
    return (int)S->crt_prog[0]; }
__attribute__((export_name("dbg_crtfin"))) int exp_dbg_crtfin(int i){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (G.bud[id].bw_mode != 7 || i<1 || i>5) return -1;
    // v1.27 test staging (the medal podium boundary): park rival kart
    // i just PAST the finish line — the race's own finish check then
    // registers its place on the next frame, exactly as if it had
    // driven home. Observation/staging only, like dbg_crtskip.
    S->crt_prog[i] = 3.0f * CRT_LEN + 50.0f;
    { float x, y, hx, hy; crt_point_at(CRT_OFF + S->crt_prog[i], 0, &x, &y, &hx, &hy);
      S->crt_x[i] = x; S->crt_y[i] = y; S->crt_h[i] = fatan2(hy, hx); S->crt_v[i] = 120.0f;
      S->crt_seg[i] = crt_nearest_full(x, y); }
    return (int)S->crt_prog[i]; }
__attribute__((export_name("dbg_arcgate"))) int exp_dbg_arcgate(void){ int id = G.my_id>=0? G.my_id : 0; return arcgate[id]; }
__attribute__((export_name("dbg_jet"))) int exp_dbg_jet(void){ int id = G.my_id>=0? G.my_id : 0; return G.p[id].bean_jet>0? 1 : (G.p[id].jet_fall>0? 2 : 0); }
__attribute__((export_name("dbg_givebean"))) void exp_dbg_givebean(void){ int id = G.my_id>=0? G.my_id : 0; G.p[id].beans++; }
__attribute__((export_name("dbg_bwentm"))) void exp_dbg_bwentm(int m){ int id = G.my_id>=0? G.my_id : 0; mall_enter(id); bw_enter_mode(id, m); }
// WAR! (bw_mode 8) test hooks. Packings: dbg_warunits(side,type)
// counts alive units (side 0 buddies / 1 bugs, type -1 = any);
// dbg_warunitxy / dbg_warstructxy pack arena-local coords as
// x*10000+y; dbg_warunitpos packs the unit's on-screen (fb) coords
// the same way; struct 0 is always the Compiler Core, hives follow.
__attribute__((export_name("dbg_warstate"))) int exp_dbg_warstate(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].bw_mode==8? BWS[id].war_state : -1; }
__attribute__((export_name("dbg_warskirmish"))) int exp_dbg_warskirmish(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_skirmish; }   // v1.19: 1 while the live WAR! battle is an ad-hoc skirmish
__attribute__((export_name("dbg_warbits"))) int exp_dbg_warbits(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_bits; }
__attribute__((export_name("dbg_warconq"))) int exp_dbg_warconq(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].war_prog; }
__attribute__((export_name("dbg_warhives"))) int exp_dbg_warhives(void){ int id = G.my_id>=0? G.my_id : 0; return war_hives_alive(&BWS[id]); }
__attribute__((export_name("dbg_warhbits"))) int exp_dbg_warhbits(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; for (int s=0;s<WAR_MAXS;s++) if (S->war_shp[s]>0 && S->war_stype[s]==WAR_S_HIVE) return S->war_hbits[s]; return -1; }   // v1.21: the first living hive's purse (-1 = no hive)
__attribute__((export_name("dbg_warunits"))) int exp_dbg_warunits(int side, int type){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; int n=0;
    for (int i=0;i<WAR_MAXU;i++){ if (S->war_uhp[i]<=0) continue; if (side>=0 && war_u_side(S->war_utype[i])!=side) continue; if (type>=0 && S->war_utype[i]!=type) continue; n++; } return n; }
__attribute__((export_name("dbg_warstructs"))) int exp_dbg_warstructs(int type){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; int n=0;
    for (int i=0;i<WAR_MAXS;i++) if (S->war_shp[i]>0 && S->war_stype[i]==type) n++; return n; }
__attribute__((export_name("dbg_warsel"))) int exp_dbg_warsel(void){ int id = G.my_id>=0? G.my_id : 0; return war_sel_count(&BWS[id]); }
__attribute__((export_name("dbg_warselstruct"))) int exp_dbg_warselstruct(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_selstruct; }
__attribute__((export_name("dbg_warenter"))) int exp_dbg_warenter(int r){ int id = G.my_id>=0? G.my_id : 0; if (G.bud[id].bw_mode!=8) return -1; war_start_battle(id, r); return BWS[id].war_state; }
__attribute__((export_name("dbg_warseed"))) int exp_dbg_warseed(int v){ int id = G.my_id>=0? G.my_id : 0; BWS[id].war_seedforce = v; return BWS[id].war_seedforce; }   // test-only: pin the v1.18 per-battle novelty seed (0 = off, the shipped game never calls this)
__attribute__((export_name("dbg_waraddbits"))) int exp_dbg_waraddbits(int n){ int id = G.my_id>=0? G.my_id : 0; BWS[id].war_bits += n; return BWS[id].war_bits; }
__attribute__((export_name("dbg_warcam"))) int exp_dbg_warcam(int x, int y){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; S->war_camx = WAR_OX + x; S->war_camy = WAR_OY + y; war_clamp_cam(S); return 1; }
__attribute__((export_name("dbg_warfind"))) int exp_dbg_warfind(int type){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0 && S->war_utype[i]==type) return i; return -1; }
__attribute__((export_name("dbg_warunitxy"))) int exp_dbg_warunitxy(int i){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (i<0 || i>=WAR_MAXU || S->war_uhp[i]<=0) return -1; return (int)(S->war_ux[i]-WAR_OX)*10000 + (int)(S->war_uy[i]-WAR_OY); }
__attribute__((export_name("dbg_warstructxy"))) int exp_dbg_warstructxy(int i){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (i<0 || i>=WAR_MAXS || S->war_shp[i]<=0) return -1; return (int)(S->war_sx[i]-WAR_OX)*10000 + (int)(S->war_sy[i]-WAR_OY); }
__attribute__((export_name("dbg_warstructhp"))) int exp_dbg_warstructhp(int i){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (i<0 || i>=WAR_MAXS || S->war_shp[i]<=0) return -1; return (int)S->war_shp[i]; }   // v1.19 addendum: a structure's current HP (the patcher checks read repairs through this)
__attribute__((export_name("dbg_warunitpos"))) int exp_dbg_warunitpos(int i){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (i<0 || i>=WAR_MAXU || S->war_uhp[i]<=0) return -1;
    int vx, vy, vw2, vh2; p1_view_rect(&vx, &vy, &vw2, &vh2);
    int sx = vx + vw2/2 + (int)(S->war_ux[i] - S->war_camx);
    int sy = vy + vh2/2 + (int)(S->war_uy[i] - S->war_camy - WAR_BAR_H/2);
    if (sx<0) sx=0; if (sy<0) sy=0; return sx + sy*10000; }
__attribute__((export_name("dbg_wartapunit"))) int exp_dbg_wartapunit(int i){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (i<0 || i>=WAR_MAXU || S->war_uhp[i]<=0) return -1; war_select_unit(S, i, 0); return war_sel_count(S); }
__attribute__((export_name("dbg_warbuildat"))) int exp_dbg_warbuildat(int kind){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (G.bud[id].bw_mode!=8 || S->war_state!=1 || (kind<1 || kind>3) && kind!=5 && kind!=6) return -1;
    int hq = war_find_hq(S); if (hq<0) return -1;
    int cost = WAR_BUILD_COST[kind]; if (S->war_bits < cost) return -1;
    static const float RING[3] = { 78.0f, 108.0f, 138.0f };
    for (int r=0;r<3;r++) for (int a=0;a<8;a++){
        float x = S->war_sx[hq] + fcos((float)a * (PI/4)) * RING[r];
        float y = S->war_sy[hq] + fsin((float)a * (PI/4)) * RING[r];
        if (war_place_space(S, kind, x, y)){ S->war_bits -= cost; return war_spawn_struct(S, kind, 0, x, y); }   // dbg bypass: physical room only, never the property rule
    }
    return -1; }
__attribute__((export_name("dbg_warbuildxy"))) int exp_dbg_warbuildxy(int kind, int x, int y){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (G.bud[id].bw_mode!=8 || S->war_state!=1 || (kind<1 || kind>3) && kind!=5 && kind!=6) return -1;
    int cost = WAR_BUILD_COST[kind]; if (S->war_bits < cost) return -1;
    float wx = WAR_OX + (float)x, wy = WAR_OY + (float)y;
    if (kind==WAR_S_WALL || kind==WAR_S_GATE){ wx = war_snap24(wx, WAR_OX); wy = war_snap24(wy, WAR_OY); }   // same grid snap as a real placement
    if (!war_place_space(S, kind, wx, wy)) return -1;   // property bypassed, room still required
    S->war_bits -= cost;
    return war_spawn_struct(S, kind, 0, wx, wy); }
__attribute__((export_name("dbg_wartrain"))) int exp_dbg_wartrain(int kind){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (G.bud[id].bw_mode!=8 || S->war_state!=1 || (kind<0 || kind>2) && kind!=5 && kind!=6) return -1;
    int want = kind==0? WAR_S_HQ : kind<=2? WAR_S_PATCH : WAR_S_ASM;
    int s = -1;
    for (int i=0;i<WAR_MAXS;i++) if (S->war_shp[i]>0 && S->war_stype[i]==want){ s=i; break; }
    if (s<0) return -1;
    int cost = WAR_TRAIN_COST[kind]; if (S->war_bits < cost) return -1;
    if (war_count_units(S, 0) >= 24) return -1;
    S->war_bits -= cost;
    float a = (float)((s*47) % 6) * 1.1f;
    war_spawn_unit(S, kind, S->war_sx[s] + fcos(a)*(WAR_SRAD[S->war_stype[s]]+16), S->war_sy[s] + fsin(a)*(WAR_SRAD[S->war_stype[s]]+16));
    return war_count_type(S, kind); }
__attribute__((export_name("dbg_warplace"))) int exp_dbg_warplace(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_place; }
__attribute__((export_name("dbg_warcur"))) int exp_dbg_warcur(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (!S->war_curon) return -1; return (int)(S->war_curx-WAR_OX)*10000 + (int)(S->war_cury-WAR_OY); }   // pad cursor: -1 off, else arena-local x*10000+y (dbg_warunitxy packing)
__attribute__((export_name("dbg_warbarhl"))) int exp_dbg_warbarhl(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_barhl; }
__attribute__((export_name("dbg_warbarcap"))) int exp_dbg_warbarcap(void){ int id = G.my_id>=0? G.my_id : 0; if (!BWS[id].war_barcap) return 0; return BWS[id].war_barhl_row? 2 : 1; }   // v1.20: bar capture — 0 none, 1 captured on the options/list row, 2 captured on the category row
__attribute__((export_name("dbg_warpause"))) int exp_dbg_warpause(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_pause; }   // v1.17: the battle pause menu is up
__attribute__((export_name("dbg_warpsel"))) int exp_dbg_warpsel(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_pause_sel; }   // v1.31: the battle pause's keyboard selection (audit P7 checks)
__attribute__((export_name("dbg_warregion"))) int exp_dbg_warregion(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_region; }   // v1.17: the region the current battle is for
__attribute__((export_name("dbg_warsnap"))) int exp_dbg_warsnap(void){ int id = G.my_id>=0? G.my_id : 0; return war_snap_on[id]; }   // v1.17: a battle snapshot is parked for this player
__attribute__((export_name("dbg_warsksnap"))) int exp_dbg_warsksnap(void){ int id = G.my_id>=0? G.my_id : 0; return war_snap_sk_on[id]; }   // v1.29: a SKIRMISH snapshot is parked for this player
__attribute__((export_name("dbg_warskpanel"))) int exp_dbg_warskpanel(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_sk_panel; }   // v1.29: the RESUME / NEW SKIRMISH entry panel is up
__attribute__((export_name("dbg_warbcat"))) int exp_dbg_warbcat(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_bcat; }   // v1.17: open bar category (-1 top level, 0 TRAIN, 1 BUILD, 2 DEFENSE)
__attribute__((export_name("dbg_warnodes"))) int exp_dbg_warnodes(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; int sum=0;
    for (int n=0;n<S->war_nn;n++) sum += S->war_namt[n]; return S->war_nn*100000 + sum; }   // v1.17: node count *100000 + total bits left on the field
__attribute__((export_name("dbg_warnodexy"))) int exp_dbg_warnodexy(int n){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (n<0 || n>=S->war_nn || S->war_namt[n]<=0) return -1; return (int)(S->war_nx[n]-WAR_OX)*10000 + (int)(S->war_ny[n]-WAR_OY); }   // v1.27: node n's ARENA-RELATIVE coords, packed like dbg_warunitxy (every war seam reports relative — featxy/structxy/unitxy all subtract WAR_OX/WAR_OY; observation only — the wall checks scan placement clearance, and v1.27's node jitter means they must read node spots live)
__attribute__((export_name("dbg_warcarry"))) int exp_dbg_warcarry(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; int m=-1;
    for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0 && S->war_utype[i]==WAR_U_HARV && S->war_ucarry[i]>m) m = S->war_ucarry[i]; return m; }   // v1.17: the fullest harvester's load
__attribute__((export_name("dbg_warqueue"))) int exp_dbg_warqueue(int s){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (s<0 || s>=WAR_MAXS || S->war_shp[s]<=0) return -1; return S->war_qlen[s]; }   // v1.17: a building's training-queue depth
__attribute__((export_name("dbg_warrally"))) int exp_dbg_warrally(int s){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (s<0 || s>=WAR_MAXS || S->war_shp[s]<=0 || S->war_rallyx[s]==0) return -1;
    return (int)(S->war_rallyx[s]-WAR_OX)*10000 + (int)(S->war_rallyy[s]-WAR_OY); }   // v1.17: a building's rally point, dbg_warunitxy packing, -1 = unset
__attribute__((export_name("dbg_warterrain"))) int exp_dbg_warterrain(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_tn; }
// v1.33 skirmish hooks: the setup picks, the live clan count, per-unit
// and per-struct clan tags, the retaliation latch, the wall-drag
// state, miner counts per team, and unit HP (no HP export existed).
__attribute__((export_name("dbg_warclans"))) int exp_dbg_warclans(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_clans; }
__attribute__((export_name("dbg_warsize"))) int exp_dbg_warsize(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_size; }
__attribute__((export_name("dbg_wardiff"))) int exp_dbg_wardiff(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_diff; }
__attribute__((export_name("dbg_warclanpick"))) int exp_dbg_warclanpick(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_clanpick; }
__attribute__((export_name("dbg_waruclan"))) int exp_dbg_waruclan(int i){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (i<0 || i>=WAR_MAXU || S->war_uhp[i]<=0) return -1; return S->war_uclan[i]; }
__attribute__((export_name("dbg_warsclan"))) int exp_dbg_warsclan(int s){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (s<0 || s>=WAR_MAXS || S->war_shp[s]<=0) return -1; return S->war_sclan[s]; }
__attribute__((export_name("dbg_waraggro"))) int exp_dbg_waraggro(int i){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (i<0 || i>=WAR_MAXU || S->war_uhp[i]<=0) return 0; return S->war_uaggro[i]; }
__attribute__((export_name("dbg_warwdrag"))) int exp_dbg_warwdrag(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_wdrag; }
__attribute__((export_name("dbg_warminers"))) int exp_dbg_warminers(int team){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; int n=0;
    for (int s=0;s<WAR_MAXS;s++) if (S->war_shp[s]>0 && S->war_stype[s]==WAR_S_MINER && S->war_sclan[s]==team) n++; return n; }
__attribute__((export_name("dbg_warunithp"))) int exp_dbg_warunithp(int i){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (i<0 || i>=WAR_MAXU) return -1; return (int)S->war_uhp[i]; }
__attribute__((export_name("dbg_warutype"))) int exp_dbg_warutype(int i){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (i<0 || i>=WAR_MAXU || S->war_uhp[i]<=0) return -1; return S->war_utype[i]; }
__attribute__((export_name("dbg_warstype"))) int exp_dbg_warstype(int s){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (s<0 || s>=WAR_MAXS || S->war_shp[s]<=0) return -1; return S->war_stype[s]; }
__attribute__((export_name("dbg_warfeatn"))) int exp_dbg_warfeatn(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; int n=0;
    for (int t=0;t<S->war_tn;t++) if (S->war_thp[t]>0) n++; return n; }   // debris still standing
__attribute__((export_name("dbg_warfeat"))) int exp_dbg_warfeat(int t){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (t<0 || t>=S->war_tn) return -1; return (int)S->war_thp[t]*100 + S->war_ttype[t]; }   // hp*100 + kind (0 tree / 1 rock / 2 terminal / 3 spool / 4 rubble / 5 barrier)
__attribute__((export_name("dbg_warbarriers"))) int exp_dbg_warbarriers(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; int n=0;
    for (int t=0;t<S->war_tn;t++) if (S->war_ttype[t]==5 && S->war_thp[t]>0) n++; return n; }   // v1.18: live barrier segments
__attribute__((export_name("dbg_warhivexy"))) int exp_dbg_warhivexy(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    for (int s=0;s<WAR_MAXS;s++) if (S->war_shp[s]>0 && S->war_stype[s]==WAR_S_HIVE) return (int)(S->war_sx[s]-WAR_OX)*10000 + (int)(S->war_sy[s]-WAR_OY); return -1; }   // v1.18: the first live hive, dbg_warunitxy packing
__attribute__((export_name("dbg_warfeatxy"))) int exp_dbg_warfeatxy(int t){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (t<0 || t>=S->war_tn) return -1; return (int)(S->war_tx[t]-WAR_OX)*10000 + (int)(S->war_ty[t]-WAR_OY); }
__attribute__((export_name("dbg_warfeatpos"))) int exp_dbg_warfeatpos(int t){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (t<0 || t>=S->war_tn) return -1;
    int vx, vy, vw2, vh2; p1_view_rect(&vx, &vy, &vw2, &vh2);
    int sx = vx + vw2/2 + (int)(S->war_tx[t] - S->war_camx);
    int sy = vy + vh2/2 + (int)(S->war_ty[t] - S->war_camy - WAR_BAR_H/2);
    if (sx<0) sx=0; if (sy<0) sy=0; return sx + sy*10000; }
__attribute__((export_name("dbg_warfog"))) int exp_dbg_warfog(int x, int y){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (S->war_state==0) return -1; return war_fog_at(S, WAR_OX + (float)x, WAR_OY + (float)y); }
// WAR! co-op (v1.25 feat 13) test hooks: the second commander's link,
// view and the shared battle read from their side. All return
// sentinels when there is no local P2.
__attribute__((export_name("dbg_warduo"))) int exp_dbg_warduo(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_duo; }   // my sim's co-commander (-1 none — a fresh sim reads 0 until its first war_update self-heals it to -1)
__attribute__((export_name("dbg_warlink2"))) int exp_dbg_warlink2(void){ if (G.p2_slot<0) return -9; return BWS[G.p2_slot].war_guest; }   // P2's link: 0 = not a guest, else owner pi + 1
__attribute__((export_name("dbg_warduo2"))) int exp_dbg_warduo2(void){ if (G.p2_slot<0) return -9; return BWS[G.p2_slot].war_duo; }   // P2 as owner: their co-commander
__attribute__((export_name("dbg_war2state"))) int exp_dbg_war2state(void){ if (G.p2_slot<0) return -9; return (G.bud[G.p2_slot].in_bw && G.bud[G.p2_slot].bw_mode==8)? BWS[G.p2_slot].war_state : -1; }   // in_bw matters: bw_exit leaves bw_mode stale at 8 (dbg_bwstate2's convention)
__attribute__((export_name("dbg_war2cur"))) int exp_dbg_war2cur(void){ if (G.p2_slot<0) return -9; BwSim *S = &BWS[G.p2_slot];
    if (!S->war_curon) return -1; return (int)(S->war_curx-WAR_OX)*10000 + (int)(S->war_cury-WAR_OY); }
__attribute__((export_name("dbg_war2cam"))) int exp_dbg_war2cam(void){ if (G.p2_slot<0) return -9; BwSim *S = &BWS[G.p2_slot];
    return (int)(S->war_camx-WAR_OX)*10000 + (int)(S->war_camy-WAR_OY); }
__attribute__((export_name("dbg_war2sel"))) int exp_dbg_war2sel(void){ if (G.p2_slot<0) return -9; return war_sel_count(&BWS[G.p2_slot]); }
__attribute__((export_name("dbg_war2selstruct"))) int exp_dbg_war2selstruct(void){ if (G.p2_slot<0) return -9; return BWS[G.p2_slot].war_selstruct; }
__attribute__((export_name("dbg_war2barcap"))) int exp_dbg_war2barcap(void){ if (G.p2_slot<0) return -9; if (!BWS[G.p2_slot].war_barcap) return 0; return BWS[G.p2_slot].war_barhl_row? 2 : 1; }
__attribute__((export_name("dbg_war2barhl"))) int exp_dbg_war2barhl(void){ if (G.p2_slot<0) return -9; return BWS[G.p2_slot].war_barhl; }
__attribute__((export_name("dbg_war2bits"))) int exp_dbg_war2bits(void){ if (G.p2_slot<0) return -9; BwSim *S = &BWS[G.p2_slot];
    if (S->war_guest) S = &BWS[S->war_guest-1]; return S->war_bits; }   // the shared purse, read from the guest's side
__attribute__((export_name("dbg_war2structxy"))) int exp_dbg_war2structxy(int i){ if (G.p2_slot<0) return -9; BwSim *S = &BWS[G.p2_slot];
    if (S->war_guest) S = &BWS[S->war_guest-1];
    if (i<0 || i>=WAR_MAXS || S->war_shp[i]<=0) return -1;
    return (int)(S->war_sx[i]-WAR_OX)*10000 + (int)(S->war_sy[i]-WAR_OY); }   // dbg_warstructxy packing, read from P2's side of the link
__attribute__((export_name("dbg_war2find"))) int exp_dbg_war2find(int type){ if (G.p2_slot<0) return -9; BwSim *S = &BWS[G.p2_slot];
    if (S->war_guest) S = &BWS[S->war_guest-1];
    for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0 && S->war_utype[i]==type) return i; return -1; }
__attribute__((export_name("dbg_war2hurt"))) int exp_dbg_war2hurt(int what, int idx, int dmg){ if (G.p2_slot<0) return -9; int o = G.p2_slot; BwSim *S = &BWS[o];
    if (S->war_guest) o = S->war_guest-1;
    if (what==0){ war_hurt_unit(o, idx, dmg); return (idx>=0 && idx<WAR_MAXU)? (int)BWS[o].war_uhp[idx] : -1; }
    war_hurt_struct(o, idx, dmg); return (idx>=0 && idx<WAR_MAXS)? (int)BWS[o].war_shp[idx] : -1; }
__attribute__((export_name("dbg_war2pause"))) int exp_dbg_war2pause(void){ if (G.p2_slot<0) return -9; BwSim *S = &BWS[G.p2_slot];
    if (S->war_guest) S = &BWS[S->war_guest-1]; return S->war_pause; }
__attribute__((export_name("dbg_war2conq"))) int exp_dbg_war2conq(void){ if (G.p2_slot<0) return -9; return G.bud[G.p2_slot].war_prog; }
__attribute__((export_name("dbg_buddyxp2"))) int exp_dbg_buddyxp2(void){ if (G.p2_slot<0) return -9; return G.bud[G.p2_slot].xp; }
__attribute__((export_name("dbg_warmove"))) int exp_dbg_warmove(int i, int x, int y){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (i<0 || i>=WAR_MAXU || S->war_uhp[i]<=0) return -1;
    S->war_ux[i] = WAR_OX + (float)x; S->war_uy[i] = WAR_OY + (float)y; S->war_utx[i] = S->war_ux[i]; S->war_uty[i] = S->war_uy[i];
    return 1; }
__attribute__((export_name("dbg_warspawnbug"))) int exp_dbg_warspawnbug(int x, int y){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (G.bud[id].bw_mode!=8 || S->war_state!=1) return -1;
    int i = war_spawn_unit(S, WAR_U_CRAWL, WAR_OX + x, WAR_OY + y);
    if (i>=0) S->war_ustate[i]=1;
    return war_count_units(S, 1); }
__attribute__((export_name("dbg_warhurt"))) int exp_dbg_warhurt(int what, int idx, int dmg){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (what==0){ war_hurt_unit(id, idx, dmg); return (idx>=0 && idx<WAR_MAXU)? (int)S->war_uhp[idx] : -1; }
    war_hurt_struct(id, idx, dmg); return (idx>=0 && idx<WAR_MAXS)? (int)S->war_shp[idx] : -1; }
__attribute__((export_name("dbg_bwmode"))) int exp_dbg_bwmode(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].in_bw? G.bud[id].bw_mode : -1; }
__attribute__((export_name("dbg_platlvl"))) int exp_dbg_platlvl(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_lvl; }
__attribute__((export_name("dbg_platregion"))) int exp_dbg_platregion(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_region; }
__attribute__((export_name("dbg_platsub"))) int exp_dbg_platsub(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_sub; }
__attribute__((export_name("dbg_platvaultx"))) int exp_dbg_platvaultx(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_vault_tx<0? -1 : BWS[id].plat_vault_tx*24 + 24; }
__attribute__((export_name("dbg_platwarp"))) int exp_dbg_platwarp(int lvl){ int id = G.my_id>=0? G.my_id : 0; if (G.bud[id].bw_mode==3 && lvl>=1) plat_load_level(id, lvl); return BWS[id].plat_lvl; }
__attribute__((export_name("dbg_platbits"))) int exp_dbg_platbits(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_bits; }
__attribute__((export_name("dbg_plath"))) int exp_dbg_plath(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_hearts; }
__attribute__((export_name("dbg_platpow"))) int exp_dbg_platpow(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_power; }
__attribute__((export_name("dbg_platdone"))) int exp_dbg_platdone(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].plat_done; }
// MEDAL TIMES (v1.25 feat 3): per-level best + derived medal, the live
// level clock (read + set, so a test can land a flag touch at a chosen
// time through the real record path), the flag column's centre x, and
// the level-select state
__attribute__((export_name("dbg_plattbest"))) int exp_dbg_plattbest(int lvl){ int id = G.my_id>=0? G.my_id : 0; if (lvl<1 || lvl>16) return -1; return G.bud[id].plat_tbest[lvl-1]; }
__attribute__((export_name("dbg_platmedal"))) int exp_dbg_platmedal(int lvl){ int id = G.my_id>=0? G.my_id : 0; if (lvl<1 || lvl>16) return -1; return medal_tier(G.bud[id].plat_tbest[lvl-1], plat_medal[lvl-1]); }
__attribute__((export_name("dbg_plattime"))) int exp_dbg_plattime(void){ int id = G.my_id>=0? G.my_id : 0; return (int)(BWS[id].plat_lvl_t * 10.0f); }
__attribute__((export_name("dbg_platsettime"))) int exp_dbg_platsettime(int ds){ int id = G.my_id>=0? G.my_id : 0; BWS[id].plat_lvl_t = ds * 0.1f; return (int)(BWS[id].plat_lvl_t * 10.0f); }
__attribute__((export_name("dbg_platflagx"))) int exp_dbg_platflagx(void){ int id = G.my_id>=0? G.my_id : 0; return (BWS[id].plat_w - 3) * 24 + 12; }
__attribute__((export_name("dbg_platsel"))) int exp_dbg_platsel(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_sel; }
__attribute__((export_name("dbg_platpick"))) int exp_dbg_platpick(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_pick; }
// v1.33 test seams for runcontacttest (arrange/observe only, in the
// dbg_tankpick / dbg_platsettime house style): how many live critters
// of a kind a level holds, the first one's spot and state, a staging
// setter for the hotfix power, and the live fireball count. Nothing
// here deals damage, fires, or is ever saved.
__attribute__((export_name("dbg_platen"))) int exp_dbg_platen(int kind){ int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id]; int n=0; for (int i=0;i<S->plat_en_n;i++) if (S->plat_en[i].kind==kind) n++; return n; }
__attribute__((export_name("dbg_platenx"))) int exp_dbg_platenx(int kind){ int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id]; for (int i=0;i<S->plat_en_n;i++) if (S->plat_en[i].kind==kind) return (int)S->plat_en[i].x; return -1; }
__attribute__((export_name("dbg_plateny"))) int exp_dbg_plateny(int kind){ int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id]; for (int i=0;i<S->plat_en_n;i++) if (S->plat_en[i].kind==kind) return (int)S->plat_en[i].y; return -1; }
__attribute__((export_name("dbg_platens"))) int exp_dbg_platens(int kind){ int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id]; for (int i=0;i<S->plat_en_n;i++) if (S->plat_en[i].kind==kind) return S->plat_en[i].state; return -1; }
__attribute__((export_name("dbg_platsetpow"))) int exp_dbg_platsetpow(int v){ int id = G.my_id>=0? G.my_id : 0; if (v>=0 && v<=2) BWS[id].plat_power=v; return BWS[id].plat_power; }
__attribute__((export_name("dbg_platfire"))) int exp_dbg_platfire(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id]; int n=0; for (int i=0;i<8;i++) if (S->plat_fa[i]) n++; return n; }
__attribute__((export_name("dbg_tanklvl"))) int exp_dbg_tanklvl(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_lvl; }
__attribute__((export_name("dbg_tanky"))) int exp_dbg_tanky(void){ int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].y; }
__attribute__((export_name("dbg_tankpass"))) int exp_dbg_tankpass(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_pass; }
__attribute__((export_name("dbg_tankhull"))) int exp_dbg_tankhull(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_hull; }
__attribute__((export_name("dbg_tankdone"))) int exp_dbg_tankdone(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].tank_done; }
__attribute__((export_name("dbg_tankshells"))) int exp_dbg_tankshells(void){ int id = G.my_id>=0? G.my_id : 0; int n=0; for (int i=0;i<24;i++) if (BWS[id].tank_sh_a[i]) n++; return n; }
__attribute__((export_name("dbg_tankwep"))) int exp_dbg_tankwep(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_wep; }
__attribute__((export_name("dbg_tankgren"))) int exp_dbg_tankgren(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_gren; }
__attribute__((export_name("dbg_tankboom"))) int exp_dbg_tankboom(void){ int id = G.my_id>=0? G.my_id : 0; int n=0; for (int i=0;i<6;i++) if (BWS[id].tank_boom_t[i]>0) n++; for (int i=0;i<10;i++) if (BWS[id].tank_gr_a[i]) n+=10; return n; }
// v1.27 powerup seams (observation only): the tank's x, a live count
// of tank entities of a kind (3 wrench, 6 health, 7 launcher), the
// first such entity's position (-1 when none), and the launcher flag
__attribute__((export_name("dbg_tankx"))) int exp_dbg_tankx(void){ int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].x; }
__attribute__((export_name("dbg_tankpick"))) int exp_dbg_tankpick(int kind){ int id = G.my_id>=0? G.my_id : 0; int n=0; for (int i=0;i<BWS[id].tank_en_n;i++) if (BWS[id].tank_en[i].kind==kind) n++; return n; }
__attribute__((export_name("dbg_tankpickx"))) int exp_dbg_tankpickx(int kind){ int id = G.my_id>=0? G.my_id : 0; for (int i=0;i<BWS[id].tank_en_n;i++) if (BWS[id].tank_en[i].kind==kind) return (int)BWS[id].tank_en[i].x; return -1; }
__attribute__((export_name("dbg_tankpicky"))) int exp_dbg_tankpicky(int kind){ int id = G.my_id>=0? G.my_id : 0; for (int i=0;i<BWS[id].tank_en_n;i++) if (BWS[id].tank_en[i].kind==kind) return (int)BWS[id].tank_en[i].y; return -1; }
__attribute__((export_name("dbg_tankglauncher"))) int exp_dbg_tankglauncher(void){ int id = G.my_id>=0? G.my_id : 0; return tank_glauncher_st[id]; }
__attribute__((export_name("dbg_crtdrv"))) int exp_dbg_crtdrv(int k){ int id = G.my_id>=0? G.my_id : 0; if (k<0||k>5) return -1; return BWS[id].crt_species[k]*100 + BWS[id].crt_dcolidx[k]*10 + BWS[id].crt_nameidx[k]%10; }
__attribute__((export_name("dbg_tankobj"))) int exp_dbg_tankobj(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_obj; }
__attribute__((export_name("dbg_tankdeliv"))) int exp_dbg_tankdeliv(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_deliv; }
__attribute__((export_name("dbg_tankmid"))) int exp_dbg_tankmid(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_mid_hp; }
__attribute__((export_name("dbg_tankwarp"))) void exp_dbg_tankwarp(int y){ int id = G.my_id>=0? G.my_id : 0; if (G.bud[id].bw_mode==5){ G.bud[id].x = 12.5f*BW_TILE; G.bud[id].y = (float)y; } }
__attribute__((export_name("dbg_tankboss"))) int exp_dbg_tankboss(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_boss_hp; }
// MORDOR FUN RUN seams (v1.28): observers read the Buddy's saved run
// and the session's live state; the three setters are the house's
// usual staging seams (dbg_tankwarp / dbg_tankquota precedent) —
// mortotal shortens the road BEFORE entry, morwarp stands the
// runner at a distance, morhurt lands damage through the real
// mor_hurt path (grace, faint and all). Nothing here is ever saved
// beyond what the mode itself saves.
__attribute__((export_name("dbg_morpanel"))) int exp_dbg_morpanel(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].mor_panel; }
__attribute__((export_name("dbg_morpsel"))) int exp_dbg_morpsel(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].mor_psel; }
__attribute__((export_name("dbg_moron"))) int exp_dbg_moron(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].mor_on; }
__attribute__((export_name("dbg_mordist"))) int exp_dbg_mordist(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].mor_dist; }
__attribute__((export_name("dbg_mormoose"))) int exp_dbg_mormoose(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].mor_dist*10/MOR_MOOSE_PX; }   // tenths of a moose
__attribute__((export_name("dbg_mormile"))) int exp_dbg_mormile(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].mor_mile; }
__attribute__((export_name("dbg_morhp"))) int exp_dbg_morhp(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].mor_hp; }
__attribute__((export_name("dbg_mortime"))) int exp_dbg_mortime(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].mor_secs; }
__attribute__((export_name("dbg_mordone"))) int exp_dbg_mordone(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].mor_done; }
__attribute__((export_name("dbg_morbest"))) int exp_dbg_morbest(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].mor_best; }
// CRISPY DIRT (v1.35): the scorchtest rig reads the match through
// these, in the established dbg style — phases (0 aim, 1 fly,
// 2 settle, 3 shop, 4 banner, 5 end), the gauges, the purse, the
// field itself. The staging seams (setangle/setpower/givebits/
// hurt) exist so a suite can line up a kill or a shop visit
// without playing a whole match per assertion; shipped play never
// calls them.
__attribute__((export_name("dbg_scrphase"))) int exp_dbg_scrphase(void){ return scr_dbg_on()? scr_dbg_sim()->scr_phase : -1; }
__attribute__((export_name("dbg_scrturn"))) int exp_dbg_scrturn(void){ return scr_dbg_on()? scr_dbg_sim()->scr_turn : -1; }
__attribute__((export_name("dbg_scrangle"))) int exp_dbg_scrangle(void){ BwSim *S=scr_dbg_sim(); return scr_dbg_on()? (int)(S->scr_aim[S->scr_turn]+0.5f) : -1; }
__attribute__((export_name("dbg_scrpower"))) int exp_dbg_scrpower(void){ BwSim *S=scr_dbg_sim(); return scr_dbg_on()? (int)(S->scr_pow[S->scr_turn]+0.5f) : -1; }
__attribute__((export_name("dbg_scrwind"))) int exp_dbg_scrwind(void){ return scr_dbg_on()? scr_dbg_sim()->scr_wind : -999; }
__attribute__((export_name("dbg_scrbits"))) int exp_dbg_scrbits(int side){ return scr_dbg_on()? scr_dbg_sim()->scr_bits[side? 1:0] : -1; }
__attribute__((export_name("dbg_scrinv"))) int exp_dbg_scrinv(int side, int w){ return scr_dbg_on()? scr_dbg_sim()->scr_inv[side? 1:0][w] : -1; }
__attribute__((export_name("dbg_scrhp"))) int exp_dbg_scrhp(int idx){ BwSim *S=scr_dbg_sim(); return (scr_dbg_on() && idx>=0 && idx<S->scr_nu)? S->scr_u[idx].hp : -999; }
__attribute__((export_name("dbg_scralive"))) int exp_dbg_scralive(int idx){ BwSim *S=scr_dbg_sim(); return (scr_dbg_on() && idx>=0 && idx<S->scr_nu)? S->scr_u[idx].alive : -1; }
__attribute__((export_name("dbg_scrunits"))) int exp_dbg_scrunits(void){ return scr_dbg_on()? scr_dbg_sim()->scr_nu : -1; }
__attribute__((export_name("dbg_scrh"))) int exp_dbg_scrh(int x){ BwSim *S=scr_dbg_sim(); if (!scr_dbg_on() || x<0 || x>639) return -1; return (int)(S->scr_h[x]+0.5f); }
__attribute__((export_name("dbg_scrwins"))) int exp_dbg_scrwins(int side){ return scr_dbg_on()? scr_dbg_sim()->scr_wins[side? 1:0] : -1; }
__attribute__((export_name("dbg_scrround"))) int exp_dbg_scrround(void){ return scr_dbg_on()? scr_dbg_sim()->scr_round : -1; }
__attribute__((export_name("dbg_scrchamp"))) int exp_dbg_scrchamp(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].scr_champ; }
__attribute__((export_name("dbg_scrweap"))) int exp_dbg_scrweap(void){ BwSim *S=scr_dbg_sim(); return scr_dbg_on()? S->scr_weap[S->scr_turn] : -1; }
__attribute__((export_name("dbg_scrshots"))) int exp_dbg_scrshots(void){ return scr_dbg_on()? scr_dbg_sim()->scr_shots : -1; }
__attribute__((export_name("dbg_scrlastx"))) int exp_dbg_scrlastx(void){ return scr_dbg_on()? (int)(scr_dbg_sim()->scr_lastx+0.5f) : -1; }
__attribute__((export_name("dbg_scrlasty"))) int exp_dbg_scrlasty(void){ return scr_dbg_on()? (int)(scr_dbg_sim()->scr_lasty+0.5f) : -1; }
__attribute__((export_name("dbg_scrsetangle"))) void exp_dbg_scrsetangle(int a){ if (scr_dbg_on()){ BwSim *S=scr_dbg_sim(); S->scr_aim[S->scr_turn]=(float)a; } }
__attribute__((export_name("dbg_scrsetpower"))) void exp_dbg_scrsetpower(int p){ if (scr_dbg_on()){ BwSim *S=scr_dbg_sim(); S->scr_pow[S->scr_turn]=(float)p; } }
__attribute__((export_name("dbg_scrgivebits"))) void exp_dbg_scrgivebits(int side, int n){ if (scr_dbg_on()) scr_dbg_sim()->scr_bits[side? 1:0]+=n; }
__attribute__((export_name("dbg_scrhurt"))) void exp_dbg_scrhurt(int idx, int hp){ BwSim *S=scr_dbg_sim(); if (scr_dbg_on() && idx>=0 && idx<S->scr_nu) S->scr_u[idx].hp=hp; }
__attribute__((export_name("dbg_scrshield"))) int exp_dbg_scrshield(int idx){ BwSim *S=scr_dbg_sim(); return (scr_dbg_on() && idx>=0 && idx<S->scr_nu)? S->scr_u[idx].sh : -1; }
__attribute__((export_name("dbg_scrsetshield"))) void exp_dbg_scrsetshield(int idx, int n){ BwSim *S=scr_dbg_sim(); if (scr_dbg_on() && idx>=0 && idx<S->scr_nu) S->scr_u[idx].sh=n; }
__attribute__((export_name("dbg_scrdig"))) void exp_dbg_scrdig(int x){ if (scr_dbg_on()) scr_dbg_dig(x); }
__attribute__((export_name("dbg_scrregen"))) void exp_dbg_scrregen(void){ if (scr_dbg_on()) scr_dbg_regen(); }
__attribute__((export_name("dbg_moren"))) int exp_dbg_moren(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].mor_en_n; }
__attribute__((export_name("dbg_morkills"))) int exp_dbg_morkills(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].mor_kills; }
__attribute__((export_name("dbg_morx"))) int exp_dbg_morx(void){ int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].x; }
__attribute__((export_name("dbg_mory"))) int exp_dbg_mory(void){ int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].y; }
__attribute__((export_name("dbg_morenx"))) int exp_dbg_morenx(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; int bi=-1; float bd=1e18f; for (int i=0;i<BWS[id].mor_en_n;i++){ float dx=BWS[id].mor_en[i].x-b->x, dy=BWS[id].mor_en[i].y-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; bi=i; } } return bi<0? -1 : (int)BWS[id].mor_en[bi].x; }
__attribute__((export_name("dbg_moreny"))) int exp_dbg_moreny(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; int bi=-1; float bd=1e18f; for (int i=0;i<BWS[id].mor_en_n;i++){ float dx=BWS[id].mor_en[i].x-b->x, dy=BWS[id].mor_en[i].y-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; bi=i; } } return bi<0? -1 : (int)BWS[id].mor_en[bi].y; }
__attribute__((export_name("dbg_morenhp"))) int exp_dbg_morenhp(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; int bi=-1; float bd=1e18f; for (int i=0;i<BWS[id].mor_en_n;i++){ float dx=BWS[id].mor_en[i].x-b->x, dy=BWS[id].mor_en[i].y-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; bi=i; } } return bi<0? -1 : (int)BWS[id].mor_en[bi].hp; }
__attribute__((export_name("dbg_mortotal"))) void exp_dbg_mortotal(int moose){ mor_set_test_total(moose); }
__attribute__((export_name("dbg_morwarp"))) void exp_dbg_morwarp(int dist){ int id = G.my_id>=0? G.my_id : 0; if (G.bud[id].bw_mode==9 && G.bud[id].in_bw) mor_warp(id, dist); }
__attribute__((export_name("dbg_morhurt"))) void exp_dbg_morhurt(int dmg){ int id = G.my_id>=0? G.my_id : 0; if (G.bud[id].bw_mode==9 && G.bud[id].in_bw) mor_hurt(id, dmg); }
__attribute__((export_name("dbg_morpowx"))) int exp_dbg_morpowx(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; float bs=-1, bd=1e18f; for (int k=0;k<MOR_POW_N;k++){ if (mor_pow_taken(&BWS[id], k)) continue; float ps, pl; int kd; mor_pow_pos(id, k, &ps, &pl, &kd); float dx=(mor_cx(ps)+pl)-b->x, dy=((float)MOR_Y0-ps)-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; bs=ps; } } return bs<0? -1 : (int)bs; }   // v1.29: the nearest untaken powerup's road distance (s)
__attribute__((export_name("dbg_morpowlat"))) int exp_dbg_morpowlat(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; float bl=0, bd=1e18f; int found=0; for (int k=0;k<MOR_POW_N;k++){ if (mor_pow_taken(&BWS[id], k)) continue; float ps, pl; int kd; mor_pow_pos(id, k, &ps, &pl, &kd); float dx=(mor_cx(ps)+pl)-b->x, dy=((float)MOR_Y0-ps)-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; bl=pl; found=1; } } return found? (int)bl : -9999; }   // v1.29: its lateral offset
__attribute__((export_name("dbg_morpowkind"))) int exp_dbg_morpowkind(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; float bd=1e18f; int bk=-1; for (int k=0;k<MOR_POW_N;k++){ if (mor_pow_taken(&BWS[id], k)) continue; float ps, pl; int kd; mor_pow_pos(id, k, &ps, &pl, &kd); float dx=(mor_cx(ps)+pl)-b->x, dy=((float)MOR_Y0-ps)-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; bk=kd; } } return bk; }   // v1.29: its kind (0 health, 1 amp, 2 speed)
__attribute__((export_name("dbg_morpowtaken"))) int exp_dbg_morpowtaken(void){ int id = G.my_id>=0? G.my_id : 0; int n=0; for (int k=0;k<MOR_POW_N;k++) n += mor_pow_taken(&BWS[id], k); return n; }
__attribute__((export_name("dbg_morenidx"))) int exp_dbg_morenidx(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; int bi=-1; float bd=1e18f; for (int i=0;i<BWS[id].mor_en_n;i++){ float dx=BWS[id].mor_en[i].x-b->x, dy=BWS[id].mor_en[i].y-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; bi=i; } } return bi; }   // v1.29: the nearest foe's array index — HP reads pinned to it cannot be swapped mid-measurement by another foe becoming "nearest"
__attribute__((export_name("dbg_morenhpidx"))) int exp_dbg_morenhpidx(int i){ int id = G.my_id>=0? G.my_id : 0; if (i<0 || i>=BWS[id].mor_en_n) return -1; return (int)BWS[id].mor_en[i].hp; }
__attribute__((export_name("dbg_morpow0s"))) int exp_dbg_morpow0s(void){ int id = G.my_id>=0? G.my_id : 0; float ps, pl; int kd; mor_pow_pos(id, 0, &ps, &pl, &kd); return (int)ps; }   // v1.29: station 0's road distance, read straight from the derivation
__attribute__((export_name("dbg_morpow0kind"))) int exp_dbg_morpow0kind(void){ int id = G.my_id>=0? G.my_id : 0; float ps, pl; int kd; mor_pow_pos(id, 0, &ps, &pl, &kd); return kd; }   // v1.29: station 0's kind (always health by construction)
__attribute__((export_name("dbg_morpowwx"))) int exp_dbg_morpowwx(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; float bd=1e18f; int bx=-1; for (int k=0;k<MOR_POW_N;k++){ if (mor_pow_taken(&BWS[id], k)) continue; float ps, pl; int kd; mor_pow_pos(id, k, &ps, &pl, &kd); float wx=mor_cx(ps)+pl, wy=(float)MOR_Y0-ps, dx=wx-b->x, dy=wy-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; bx=(int)wx; } } return bx; }   // v1.29: the nearest untaken powerup's world x (steering)
__attribute__((export_name("dbg_morpowy"))) int exp_dbg_morpowy(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; float bd=1e18f; int by=-1; for (int k=0;k<MOR_POW_N;k++){ if (mor_pow_taken(&BWS[id], k)) continue; float ps, pl; int kd; mor_pow_pos(id, k, &ps, &pl, &kd); float wx=mor_cx(ps)+pl, wy=(float)MOR_Y0-ps, dx=wx-b->x, dy=wy-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; by=(int)wy; } } return by; }   // v1.29: its world y
__attribute__((export_name("dbg_moramp"))) int exp_dbg_moramp(void){ int id = G.my_id>=0? G.my_id : 0; return (int)(BWS[id].mor_amp_t*10); }   // tenths of a second left
__attribute__((export_name("dbg_morswings"))) int exp_dbg_morswings(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].mor_swings; }   // v1.29: swings this cabinet session — the input regression's witness (the buddy's atk_cd never decays inside Banzai, so it latches; this counter only moves when the swing block runs)
__attribute__((export_name("dbg_morspdb"))) int exp_dbg_morspdb(void){ int id = G.my_id>=0? G.my_id : 0; return (int)(BWS[id].mor_spd_t*10); }
__attribute__((export_name("dbg_mordrops"))) int exp_dbg_mordrops(void){ int id = G.my_id>=0? G.my_id : 0; int n=0; for (int i=0;i<MOR_DROP_N;i++) n += BWS[id].mor_drop_on[i]? 1 : 0; return n; }   // v1.33: live kill drops on the road
__attribute__((export_name("dbg_mordropspawned"))) int exp_dbg_mordropspawned(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].mor_drop_spawned; }   // v1.33: drops spawned this cabinet session (the rate witness — live drops alone undercount, a foe downed inside the 30u pickup radius drops loot the runner collects on the next frame)
__attribute__((export_name("dbg_mordropchance"))) int exp_dbg_mordropchance(void){ return (int)(mor_drop_chance*1000.0f + 0.5f); }   // v1.33: the drop chance, per mille
__attribute__((export_name("dbg_mordropwx"))) int exp_dbg_mordropwx(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; float bd=1e18f; int bx=-1; for (int i=0;i<MOR_DROP_N;i++){ if (!BWS[id].mor_drop_on[i]) continue; float wx=mor_cx(BWS[id].mor_drop_s[i])+BWS[id].mor_drop_lat[i], wy=(float)MOR_Y0-BWS[id].mor_drop_s[i], dx=wx-b->x, dy=wy-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; bx=(int)wx; } } return bx; }   // v1.33: the nearest live drop's world x (steering)
__attribute__((export_name("dbg_mordropwy"))) int exp_dbg_mordropwy(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; float bd=1e18f; int by=-1; for (int i=0;i<MOR_DROP_N;i++){ if (!BWS[id].mor_drop_on[i]) continue; float wx=mor_cx(BWS[id].mor_drop_s[i])+BWS[id].mor_drop_lat[i], wy=(float)MOR_Y0-BWS[id].mor_drop_s[i], dx=wx-b->x, dy=wy-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; by=(int)wy; } } return by; }   // v1.33: its world y
__attribute__((export_name("dbg_mordropkind"))) int exp_dbg_mordropkind(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; float bd=1e18f; int bk=-1; for (int i=0;i<MOR_DROP_N;i++){ if (!BWS[id].mor_drop_on[i]) continue; float wx=mor_cx(BWS[id].mor_drop_s[i])+BWS[id].mor_drop_lat[i], wy=(float)MOR_Y0-BWS[id].mor_drop_s[i], dx=wx-b->x, dy=wy-b->y, d=dx*dx+dy*dy; if (d<bd){ bd=d; bk=BWS[id].mor_drop_kind[i]; } } return bk; }   // v1.33: its kind (0 health, 1 amp, 2 speed)
__attribute__((export_name("dbg_tanklives"))) int exp_dbg_tanklives(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_lives; }
__attribute__((export_name("dbg_tankquota"))) void exp_dbg_tankquota(void){ int id = G.my_id>=0? G.my_id : 0; BWS[id].tank_deliv = BWS[id].tank_quota; }
__attribute__((export_name("dbg_craftmat"))) int exp_dbg_craftmat(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; return b->craft_wood + b->craft_stone*1000 + b->craft_fiber*1000000; }
__attribute__((export_name("dbg_craftcar"))) int exp_dbg_craftcar(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].craft_car; }
__attribute__((export_name("dbg_craftgear"))) int exp_dbg_craftgear(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].craft_gear; }
__attribute__((export_name("dbg_craftnode"))) int exp_dbg_craftnode(int tx, int ty){ int id = G.my_id>=0? G.my_id : 0; return craft_node_at(id, tx, ty); }
__attribute__((export_name("dbg_craftmenu"))) int exp_dbg_craftmenu(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].craft_menu; }
__attribute__((export_name("dbg_pausectl"))) int exp_dbg_pausectl(void){ return G.pause_ctl; }
__attribute__((export_name("dbg_pay"))) int exp_dbg_pay(void){ return pay_active? (int)pay_left+1 : 0; }
__attribute__((export_name("dbg_wallhp"))) int exp_dbg_wallhp(void){ float s=0; for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1) s += G.walls[i].maxhp; return (int)(s*10); }
__attribute__((export_name("dbg_wallstack"))) int exp_dbg_wallstack(void){ int m=0; for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){ int s=G.walls[i].stack>0? G.walls[i].stack:1; if (s>m) m=s; } return m; }
__attribute__((export_name("dbg_walllayers"))) int exp_dbg_walllayers(void){ int s=0; for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1) s += G.walls[i].stack>0? G.walls[i].stack:1; return s; }
// wall-pool instrumentation (Nov 15): place/reinforce/damage walls
// with fixed 100hp layers so the shared-pool math can be asserted
// exactly, without a carcass hunt. Test-only.
__attribute__((export_name("dbg_wallplace"))) int exp_dbg_wallplace(int x, int y, int horiz){
    for (int i=0;i<MAXWALLS;i++) if (!G.walls[i].alive){
        Wall *w=&G.walls[i];
        w->alive=1; w->horiz=horiz? 1:0; w->x=(float)x; w->y=(float)y;
        w->hp=w->maxhp=100.0f; w->stack=1; w->grp=-1; w->grp_old=-1; w->endpt=0;
        wall_build_n[i]=0; walls_recompute(); return i; }
    return -1; }
__attribute__((export_name("dbg_wallreinf"))) int exp_dbg_wallreinf(int i){
    if (i<0 || i>=MAXWALLS || G.walls[i].alive!=1) return -1;
    Wall *o=&G.walls[i];
    int oldL = o->grp; float ofrac = 1.0f;
    if (oldL>=0 && grp_cap[oldL]>0){ ofrac = grp_pool[oldL]/grp_cap[oldL];
        if (ofrac<0) ofrac=0; if (ofrac>1) ofrac=1; }
    o->maxhp += 100.0f; o->hp += 100.0f; if (o->hp>o->maxhp) o->hp=o->maxhp;
    o->stack = (o->stack>0? o->stack : 1) + 1;
    walls_recompute();
    { int L2 = o->grp;
      if (L2>=0 && grp_n[L2]>=2 && ofrac<1.0f){
          grp_pool[L2] += 100.0f*1.1f*(1.0f-ofrac);
          if (grp_pool[L2]>grp_cap[L2]) grp_pool[L2]=grp_cap[L2];
          float fr2 = grp_cap[L2]>0? grp_pool[L2]/grp_cap[L2] : 0;
          for (int m=0;m<MAXWALLS;m++) if (G.walls[m].alive==1 && G.walls[m].grp==L2)
              G.walls[m].hp = G.walls[m].maxhp * fr2;
      } }
    return o->stack; }
__attribute__((export_name("dbg_walldmg"))) int exp_dbg_walldmg(int i, int dmg){
    if (i<0 || i>=MAXWALLS || G.walls[i].alive!=1) return -1;
    damage_wall(i, (float)dmg);
    return G.walls[i].grp>=0? (int)(grp_pool[G.walls[i].grp]*10) : -2; }
__attribute__((export_name("dbg_wallhpof"))) int exp_dbg_wallhpof(int i){
    if (i<0 || i>=MAXWALLS || !G.walls[i].alive) return -1; return (int)(G.walls[i].hp*10); }
__attribute__((export_name("dbg_wallmaxof"))) int exp_dbg_wallmaxof(int i){
    if (i<0 || i>=MAXWALLS || !G.walls[i].alive) return -1; return (int)(G.walls[i].maxhp*10); }
__attribute__((export_name("dbg_wallstackof"))) int exp_dbg_wallstackof(int i){
    if (i<0 || i>=MAXWALLS || !G.walls[i].alive) return -1; return G.walls[i].stack>0? G.walls[i].stack : 1; }
__attribute__((export_name("dbg_poolof"))) int exp_dbg_poolof(int i){
    if (i<0 || i>=MAXWALLS || !G.walls[i].alive || G.walls[i].grp<0) return -1;
    return (int)(grp_pool[G.walls[i].grp]*10); }
__attribute__((export_name("dbg_capof"))) int exp_dbg_capof(int i){
    if (i<0 || i>=MAXWALLS || !G.walls[i].alive || G.walls[i].grp<0) return -1;
    return (int)(grp_cap[G.walls[i].grp]*10); }
__attribute__((export_name("dbg_grpnof"))) int exp_dbg_grpnof(int i){
    if (i<0 || i>=MAXWALLS || !G.walls[i].alive || G.walls[i].grp<0) return -1;
    return grp_n[G.walls[i].grp]; }
__attribute__((export_name("dbg_pausesel"))) int exp_dbg_pausesel(void){ return G.pause_sel; }
__attribute__((export_name("dbg_bwmenu"))) int exp_dbg_bwmenu(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].bw_menu; }
// THE SOURCE ROOM (v1.26): test-only seams — observe only, never saved
__attribute__((export_name("dbg_srcdoor"))) int exp_dbg_srcdoor(void){ int id = G.my_id>=0? G.my_id : 0; return src_gate_open(id); }
__attribute__((export_name("dbg_srcterm"))) int exp_dbg_srcterm(void){ int id = G.my_id>=0? G.my_id : 0; return srcterm_view_for(id); }
__attribute__((export_name("dbg_srcfile"))) int exp_dbg_srcfile(void){ int id = G.my_id>=0? G.my_id : 0; return srcterm_file_idx(id); }
__attribute__((export_name("dbg_srcmod"))) int exp_dbg_srcmod(void){ return src_edited_count(); }
// v1.27 wrap/touch seams — observe only: the editor's wrapped
// display-row total, the cursor's display row, the display-row
// scroll offset, the cursor packed as line*100000+col, the browser
// selection packed as bsel*10000+bscroll, and the touch-edit gate
__attribute__((export_name("dbg_srcdrows"))) int exp_dbg_srcdrows(void){ return srcterm_dbg_drows(); }
__attribute__((export_name("dbg_srcdrow"))) int exp_dbg_srcdrow(void){ return srcterm_dbg_drow(); }
__attribute__((export_name("dbg_srctop"))) int exp_dbg_srctop(void){ return srcterm_dbg_top(); }
__attribute__((export_name("dbg_srccur"))) int exp_dbg_srccur(void){ return srcterm_dbg_cursor(); }
__attribute__((export_name("dbg_srcbrowse"))) int exp_dbg_srcbrowse(void){ return srcterm_dbg_browse(); }
__attribute__((export_name("dbg_srccanedit"))) int exp_dbg_srccanedit(void){ int id = G.my_id>=0? G.my_id : 0; return srcterm_dbg_canedit(id); }
__attribute__((export_name("dbg_srctscale"))) int exp_dbg_srctscale(void){ return srcterm_dbg_scale(); }
__attribute__((export_name("dbg_givetrophy"))) int exp_dbg_givetrophy(int n){ dbg_pin_trophies(n); int id = G.my_id>=0? G.my_id : 0; return trophy_earned_count(&G.p[id]); }
// LIVE TUNING (v1.26 Item 3): the entry name rides in dbg_scratch;
// values cross as milli-units (x1000) so ints and floats share a seam
__attribute__((export_name("dbg_tune"))) int exp_dbg_tune(void){ int i = livedata_find(dbg_code_scratch); if (i<0) return -1; return (int)(livedata_get(i)*1000.0f + 0.5f); }
__attribute__((export_name("dbg_tuneset"))) int exp_dbg_tuneset(int milli){ int i = livedata_find(dbg_code_scratch); if (i<0) return -2; char err[48]; if (livedata_apply(i, milli/1000.0f, err, (int)sizeof(err))) return -1; livedata_persist_now(); return 0; }
__attribute__((export_name("dbg_tunecount"))) int exp_dbg_tunecount(void){ return livedata_overrides(); }
__attribute__((export_name("dbg_craftsel"))) int exp_dbg_craftsel(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].craft_sel; }
__attribute__((export_name("dbg_craftcat"))) int exp_dbg_craftcat(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].craft_cat; }
__attribute__((export_name("dbg_craftrowid"))) int exp_dbg_craftrowid(void){ int id = G.my_id>=0? G.my_id : 0; int rows[64]; int n = craft_menu_rows(&BWS[id], &G.bud[id], rows); int s = BWS[id].craft_sel; return (s>=0 && s<n)? rows[s] : -99; }
__attribute__((export_name("dbg_craftsearch"))) int exp_dbg_craftsearch(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].craft_search; }
__attribute__((export_name("dbg_craftrows"))) int exp_dbg_craftrows(void){ int id = G.my_id>=0? G.my_id : 0; int rows[64]; return craft_menu_rows(&BWS[id], &G.bud[id], rows); }
__attribute__((export_name("dbg_craftbuilds"))) int exp_dbg_craftbuilds(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].craft_bn; }
__attribute__((export_name("dbg_craftbuildk"))) int exp_dbg_craftbuildk(int i){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; return (i>=0 && i<b->craft_bn)? b->craft_bk[i] : -1; }
__attribute__((export_name("dbg_craftnight"))) int exp_dbg_craftnight(void){ int id = G.my_id>=0? G.my_id : 0; return (int)(craft_darkf(&G.bud[id])*100); }
__attribute__((export_name("dbg_crafttod"))) int exp_dbg_crafttod(void){ int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].craft_tod; }
__attribute__((export_name("dbg_craftsettod"))) void exp_dbg_craftsettod(int v){ int id = G.my_id>=0? G.my_id : 0; G.bud[id].craft_tod = v; }
__attribute__((export_name("dbg_craftbiome"))) int exp_dbg_craftbiome(int tx, int ty){ int id = G.my_id>=0? G.my_id : 0; return craft_biome_at(&G.bud[id], tx, ty); }
__attribute__((export_name("dbg_craftfish"))) int exp_dbg_craftfish(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].craft_fish; }
__attribute__((export_name("dbg_craftsharks"))) int exp_dbg_craftsharks(void){ int id = G.my_id>=0? G.my_id : 0; int n=0; for (int i=0;i<BWS[id].craft_shn;i++) if (BWS[id].craft_sh_hp[i]>0) n++; return n; }
__attribute__((export_name("dbg_craftbmr"))) int exp_dbg_craftbmr(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].craft_bmr_a; }
__attribute__((export_name("dbg_craftstews"))) int exp_dbg_craftstews(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].craft_stews; }
__attribute__((export_name("dbg_craftbugs"))) int exp_dbg_craftbugs(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].craft_bugn; }
__attribute__((export_name("dbg_craftdemolish"))) int exp_dbg_craftdemolish(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].craft_demolish>0? 1 : 0; }
__attribute__((export_name("dbg_craftgone"))) int exp_dbg_craftgone(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].craft_gn; }
__attribute__((export_name("dbg_craftsetm"))) void exp_dbg_craftsetm(int v){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; b->craft_wood = v % 1000; b->craft_stone = (v/1000) % 1000; b->craft_fiber = v / 1000000; }
__attribute__((export_name("dbg_craftsetcar"))) void exp_dbg_craftsetcar(int v){ int id = G.my_id>=0? G.my_id : 0; G.bud[id].craft_car = v; }
__attribute__((export_name("dbg_bwaim"))) int exp_dbg_bwaim(void){ int id = G.my_id>=0? G.my_id : 0; return (int)(G.bud[id].aim*1000); }
__attribute__((export_name("dbg_menusel"))) int exp_dbg_menusel(void){ return G.menu_sel; }
__attribute__((export_name("dbg_menucount"))) int exp_dbg_menucount(void){ return menu_count(); }
__attribute__((export_name("dbg_enttile"))) int exp_dbg_enttile(int tx, int ty){ int id = G.my_id>=0? G.my_id : 0; return ent_tile(id, tx, ty); }
__attribute__((export_name("dbg_entdiag"))) int exp_dbg_entdiag(int room, int which){
    // test-only: the locked-door roll for a room, computed from a
    // HYPOTHETICAL bw_seed (the suite plants one through the save's gz
    // key, then reads the roll back against the live seed) — which=0:
    // locked(0/1) under the live seed; 1: ent_rnd(room,400)%5 for the
    // live seed; 2: the live bw_seed itself.
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id];
    if (which==2) return b->bw_seed;
    if (which==1) return (int)(ent_rnd(b, room, 400) % 5);
    if (which==3){ float kx, ky; ent_key_pos(id, room, &kx, &ky); return (int)kx + (int)ky*100000; }
    if (which==4) return ent_door_ly(b, room);   // DEEPER DOORS (feat 11): the real door's row, so a suite can stand on a duet plate precisely
    return ent_door_locked(id, room);
}
__attribute__((export_name("dbg_entlant"))) int exp_dbg_entlant(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_lant; }
__attribute__((export_name("dbg_entchap"))) int exp_dbg_entchap(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ent_chap; }
__attribute__((export_name("dbg_ent2rec"))) int exp_dbg_ent2rec(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].ent2_best + G.bud[id].ent2_done*10000; }
__attribute__((export_name("dbg_entduet"))) int exp_dbg_entduet(int room){ int id = G.my_id>=0? G.my_id : 0; return ent_duet_door(id, room); }
__attribute__((export_name("dbg_entlu"))) int exp_dbg_entlu(void){ int id = G.my_id>=0? G.my_id : 0; return (int)BWS[id].ent_lux + (int)BWS[id].ent_luy*100000; }
__attribute__((export_name("dbg_enttypo"))) int exp_dbg_enttypo(void){ int id = G.my_id>=0? G.my_id : 0; return (int)BWS[id].ent_tyx + (int)BWS[id].ent_tyy*100000; }
__attribute__((export_name("dbg_bwenter"))) void exp_dbg_bwenter(void){
    int id = G.my_id>=0? G.my_id : 0; mall_enter(id); bw_enter_mode(id, 0); }
__attribute__((export_name("dbg_bwx"))) int exp_dbg_bwx(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].bw_x; }
__attribute__((export_name("dbg_bwy"))) int exp_dbg_bwy(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].bw_y; }
__attribute__((export_name("dbg_bwgivecrank"))) int exp_dbg_bwgivecrank(void){
    int id = G.my_id>=0? G.my_id : 0; G.bud[id].story_bits |= (1<<23); return 1; }
__attribute__((export_name("dbg_bwqtarget"))) int exp_dbg_bwqtarget(int which){
    int id = G.my_id>=0? G.my_id : 0; float tx=0, ty=0;
    int has = G.bud[id].in_bw? bw_quest_target(&G.bud[id], &tx, &ty) : 0;
    if (which==0) return (int)tx; if (which==1) return (int)ty; return has; }
__attribute__((export_name("dbg_bwseed"))) int exp_dbg_bwseed(void){
    int id = G.my_id>=0? G.my_id : 0; return G.bud[id].bw_seed; }
__attribute__((export_name("dbg_bwroam"))) int exp_dbg_bwroam(void){
    int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id];
    int n=0; for (int i=0;i<BW_ROAMN;i++) if (S->roam[i].cell || S->roam[i].x || S->roam[i].y) n++; return n; }
__attribute__((export_name("dbg_bwbattle"))) int exp_dbg_bwbattle(void){
    int id = G.my_id>=0? G.my_id : 0; return BWS[id].bat.active? BWS[id].bat.phase+1 : 0; }
__attribute__((export_name("dbg_bwhhp"))) int exp_dbg_bwhhp(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)BWS[id].bat.hhp; }
__attribute__((export_name("dbg_bwchoose"))) void exp_dbg_bwchoose(int n){
    int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id];
    if (S->bat.active && S->bat.phase==0 && n>=0 && n<S->bat.natt){ S->bat.sel=n; bw_battle_pick(id); } }
__attribute__((export_name("dbg_bwtele_hack"))) void exp_dbg_bwtele_hack(void){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; BwSim *S=&BWS[id];
    for (int i=0;i<BW_ROAMN;i++) if (S->roam[i].cell || S->roam[i].x || S->roam[i].y){
        b->x=S->roam[i].x; b->y=S->roam[i].y; return; } }
__attribute__((export_name("dbg_bwtele_tent"))) void exp_dbg_bwtele_tent(void){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; u32 seed=(u32)b->bw_seed;
    int cx0=bw_cellof(b->x), cy0=bw_cellof(b->y);
    for (int cx=cx0-8; cx<=cx0+8; cx++) for (int cy=cy0-8; cy<=cy0+8; cy++){
        float ox, oy; if (bw_cell_feat(cx, cy, seed, &ox, &oy, b->bw_mode==1)!=1) continue;
        b->x = cx*BW_CELL+ox; b->y = cy*BW_CELL+oy + 26; return; } }
__attribute__((export_name("dbg_bwexit"))) void exp_dbg_bwexit(void){
    int id = G.my_id>=0? G.my_id : 0; if (G.bud[id].in_bw) bw_exit(id, 0); }
__attribute__((export_name("dbg_codelen"))) int exp_dbg_codelen(void){
    int id = G.my_id>=0? G.my_id : 0; return whale_code_len[id]; }
__attribute__((export_name("dbg_budenergy"))) int exp_dbg_budenergy(void){
    int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].energy; }
__attribute__((export_name("dbg_setbudenergy"))) void exp_dbg_setbudenergy(int v){
    int id = G.my_id>=0? G.my_id : 0; G.bud[id].energy = v; }
__attribute__((export_name("dbg_budrevive"))) int exp_dbg_budrevive(void){
    int id = G.my_id>=0? G.my_id : 0; return bud_revive_price(&G.bud[id]); }
__attribute__((export_name("dbg_shield"))) int exp_dbg_shield(void){
    int id = G.my_id>=0? G.my_id : 0; return G.p[id].shield; }
__attribute__((export_name("dbg_setshield"))) void exp_dbg_setshield(int v){
    int id = G.my_id>=0? G.my_id : 0; Player *p=&G.p[id];
    if (p->shield_max<60) p->shield_max = 60;   // test scaffolding: guarantee headroom to regen into
    p->shield = v; p->shield_regen_t = 0; shield_frac[id]=0; }
__attribute__((export_name("dbg_budstage"))) int exp_dbg_budstage(void){
    int id = G.my_id>=0? G.my_id : 0; return bud_stage[id]; }
__attribute__((export_name("dbg_enter"))) void exp_dbg_enter(void){
    int id = G.my_id>=0? G.my_id : 0; mall_enter(id); }
__attribute__((export_name("dbg_adopt"))) void exp_dbg_adopt(int sp){
    int id = G.my_id>=0? G.my_id : 0; if (sp>=0 && sp<BUDN) buddy_adopt(id, sp); }
__attribute__((export_name("dbg_teach"))) void exp_dbg_teach(int bit){
    int id = G.my_id>=0? G.my_id : 0; if (G.bud[id].active) G.bud[id].skills |= bit; }
__attribute__((export_name("dbg_setkills"))) void exp_dbg_setkills(int k){
    int id = G.my_id>=0? G.my_id : 0; G.p[id].kills=k; }
__attribute__((export_name("dbg_bughp"))) int exp_dbg_bughp(int i){
    return (i>=0 && i<MAXBUGS && G.bugs[i].alive)? (int)G.bugs[i].hp : -1; }
__attribute__((export_name("dbg_bugw"))) int exp_dbg_bugw(int i){
    return (i>=0 && i<MAXBUGS && G.bugs[i].alive)? (int)(G.bugs[i].w*10) : -1; }
__attribute__((export_name("dbg_tp"))) void exp_dbg_tp(int x, int y){
    int id = G.my_id>=0? G.my_id : 0; G.p[id].x=(float)x; G.p[id].y=(float)y; }
__attribute__((export_name("dbg_xp"))) void exp_dbg_xp(int n){
    int id = G.my_id>=0? G.my_id : 0; if (G.bud[id].active) G.bud[id].xp=n; }
__attribute__((export_name("dbg_fainted"))) int exp_dbg_fainted(void){
    int id = G.my_id>=0? G.my_id : 0; return G.bud[id].fainted; }
__attribute__((export_name("dbg_faint"))) void exp_dbg_faint(void){
    int id = G.my_id>=0? G.my_id : 0;
    if (G.bud[id].active && !G.bud[id].fainted){ G.bud[id].hp = 0.01f; hurt_buddy(id, 1.0f); } }
__attribute__((export_name("dbg_mkwall"))) int exp_dbg_mkwall(int x, int y, int horiz){
    if (placement_closes_loop((float)x, (float)y, horiz)) return 0;
    for (int i=0;i<MAXWALLS;i++) if (!G.walls[i].alive){
        Wall *w=&G.walls[i];
        w->alive=1; w->horiz=horiz; w->x=(float)x; w->y=(float)y; w->hp=w->maxhp=57.0f;
        w->stack=1; w->grp=-1; w->grp_old=-1; w->endpt=0;
        walls_recompute_ex(0);
        return 1; }
    return -1; }
// run stats for the shell's share card: "name|heaviest hat kg|kills|seconds|zen"
static char share_buf[160];
__attribute__((export_name("share_info"))) const char *exp_share_info(void){
    int id = G.my_id>=0? G.my_id : 0; Player *p=&G.p[id];
    char *q=share_buf;
    q=sappend(q, p->name); *q++='|'; *q=0;
    q=sapp_int(q, over_have? (int)over_carried : (int)p->heaviest); *q++='|'; *q=0;   // the hat you died in
    q=sapp_int(q, G.total_kills); *q++='|'; *q=0;
    q=sapp_int(q, (int)G.time); *q++='|'; *q=0;
    q=sapp_int(q, G.zen_run); *q=0;
    return share_buf;
}
// HAT PORTRAIT BOOTH (v1.25 feat 9): the shell polls this beside
// share_info — "serial|x|y|w|h|name|kg" in stage coordinates. The
// serial only ever changes inside the booth draw, in the same
// frame whose framebuffer the shell is about to present, so the
// crop it takes is the portrait the player just snapped.
static char port_info_buf[128];
__attribute__((export_name("portrait_info"))) const char *exp_portrait_info(void){
    char *q=port_info_buf;
    q=sapp_int(q, G.port_req_serial); *q++='|'; *q=0;
    q=sapp_int(q, G.port_req_x); *q++='|'; *q=0;
    q=sapp_int(q, G.port_req_y); *q++='|'; *q=0;
    q=sapp_int(q, G.port_req_w); *q++='|'; *q=0;
    q=sapp_int(q, G.port_req_h); *q++='|'; *q=0;
    q=sappend(q, G.port_req_name); *q++='|'; *q=0;
    q=sapp_int(q, G.port_req_kg); *q=0;
    return port_info_buf;
}
__attribute__((export_name("dbg_overhat"))) void exp_dbg_overhat(int n){
    int id = G.my_id>=0? G.my_id : 0; Player *p=&G.p[id];
    if (n>MAXSTACK) n=MAXSTACK; if (n<0) n=0;
    p->stack_n=n; p->carried=n*3.0f;
    for (int i=0;i<n;i++) p->stack[i]=3.0f+(float)(i%5);
    over_refresh();
    G.state=GS_OVER; G.gameover_t=0;
}
__attribute__((export_name("dbg_budwall"))) int exp_dbg_budwall(void){
    int id = G.my_id>=0? G.my_id : 0; return G.bud[id].on_wall; }
__attribute__((export_name("dbg_roamlvl"))) int exp_dbg_roamlvl(void){
    int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id]; int m=-1;
    for (int i=0;i<BW_ROAMN;i++) if (S->roam[i].cell!=0 && S->roam[i].lvl>m) m=S->roam[i].lvl;
    return m; }
__attribute__((export_name("dbg_bwtp"))) void exp_dbg_bwtp(int x, int y){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id];
    b->x = x; b->y = y; b->bw_x = x; b->bw_y = y; }
__attribute__((export_name("dbg_termord"))) int exp_dbg_termord(void){
    int id = G.my_id>=0? G.my_id : 0; int o[3]; bw_term_order(&G.bud[id], o);
    return o[0] + o[1]*3 + o[2]*9; }
__attribute__((export_name("dbg_boss"))) int exp_dbg_boss(void){
    int id = G.my_id>=0? G.my_id : 0; return BWS[id].bat.active? BWS[id].bat.boss : -1; }
__attribute__((export_name("dbg_setq"))) void exp_dbg_setq(int q, int s){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id];
    // staging only: quests 0-3 shipped with this door; feat 14
    // extends it across the whole list so a suite can stage a
    // completed story the way bosstest stages its pieces. The
    // chapter's own suite stages only quests 0-9 with it — the
    // chapter itself is earned through the real doors there.
    if (q>=0 && q<4) bw_qset(b, q, s);
    else if (q>=4 && q<6) bw_qset_x(b, q, s);
    else if (q>=6 && q<10) bw_qset2(b, q, s);
    else if (q>=10 && q<14) bw_qset3(b, q, s); }
__attribute__((export_name("dbg_bwhackhere"))) void exp_dbg_bwhackhere(void){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; BwSim *S=&BWS[id];
    // bring a roamer to the buddy, re-homing it to a hacker cell the
    // roam_refresh is guaranteed to keep (undefeated, within 640) —
    // re-homing to the buddy's OWN cell backfired: the win marked that
    // cell defeated and refresh dropped the next roamer before contact
    u32 seed=(u32)b->bw_seed;
    int cx0=bw_cellof(b->x), cy0=bw_cellof(b->y), pin=0, have=0;
    for (int cx=cx0-3; cx<=cx0+3 && !have; cx++)
        for (int cy=cy0-3; cy<=cy0+3; cy++){
            float ox, oy; if (bw_cell_feat(cx, cy, seed, &ox, &oy, b->bw_mode==1)!=2) continue;
            int key=bw_cell_key(cx, cy);
            if (bw_defeated_has(S, key)) continue;
            float wx=cx*BW_CELL+ox, wy=cy*BW_CELL+oy, dx=wx-b->x, dy=wy-b->y;
            if (dx*dx+dy*dy > 640*640) continue;
            pin = key; have = 1; break;   // NOTE: cell keys can be negative — a flag, not a sentinel
        }
    if (!have) return;
    for (int i=0;i<BW_ROAMN;i++) if (S->roam[i].cell || S->roam[i].x || S->roam[i].y){
        S->roam[i].cell = pin;
        S->roam[i].x = b->x + 12; S->roam[i].y = b->y; return; } }
__attribute__((export_name("dbg_bwstory"))) void exp_dbg_bwstory(void){
    int id = G.my_id>=0? G.my_id : 0; bw_enter_mode(id, 1); }
__attribute__((export_name("dbg_gate"))) int exp_dbg_gate(void){
    int id = G.my_id>=0? G.my_id : 0; return bwgate[id]; }
__attribute__((export_name("dbg_log"))) int exp_dbg_log(void){
    int id = G.my_id>=0? G.my_id : 0; return BWS[id].log_open; }
__attribute__((export_name("dbg_quest"))) int exp_dbg_quest(void){
    int id = G.my_id>=0? G.my_id : 0; return G.bud[id].story_bits; }
__attribute__((export_name("dbg_items"))) int exp_dbg_items(void){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id];
    return b->items[0] + b->items[1]*10 + b->items[2]*100 + b->items[3]*1000; }
__attribute__((export_name("dbg_site"))) int exp_dbg_site(void){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; return bw_site_at_xy(b, b->x, b->y); }
__attribute__((export_name("dbg_giveitem"))) void exp_dbg_giveitem(int it){
    int id = G.my_id>=0? G.my_id : 0; if (it>=0 && it<4 && G.bud[id].items[it]<9) G.bud[id].items[it]++; }
__attribute__((export_name("dbg_budup"))) void exp_dbg_budup(int n){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id];
    if (b->active){ b->up_atk=n; b->up_hp=n; buddy_refresh(id); b->hp=b->maxhp; } }
__attribute__((export_name("dbg_bwtele_site"))) void exp_dbg_bwtele_site(int s){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id];
    if (s>=0 && s<4){ b->x = BW_SITE_XY[s][0]; b->y = BW_SITE_XY[s][1] + 60; b->bw_x=b->x; b->bw_y=b->y; } }
__attribute__((export_name("dbg_bwtele_npc"))) void exp_dbg_bwtele_npc(int n){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id];
    if (n>=0 && n<3){ b->x = BW_NPC_XY[n][0]; b->y = BW_NPC_XY[n][1] + 26; b->bw_x=b->x; b->bw_y=b->y; } }
__attribute__((export_name("dbg_adopt2"))) void exp_dbg_adopt2(int sp){
    if (G.p2_slot>=0 && sp>=0 && sp<BUDN) buddy_adopt(G.p2_slot, sp); }
__attribute__((export_name("dbg_tp2"))) void exp_dbg_tp2(int x, int y){
    if (G.p2_slot>=0){ G.p[G.p2_slot].x = x; G.p[G.p2_slot].y = y; } }
__attribute__((export_name("dbg_bwstate2"))) int exp_dbg_bwstate2(void){
    return (G.p2_slot>=0 && G.bud[G.p2_slot].in_bw)? 1 : 0; }
__attribute__((export_name("dbg_sq2v"))) int exp_dbg_sq2v(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].story_bits2; }
__attribute__((export_name("dbg_ch2q"))) int exp_dbg_ch2q(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id];
    return bw_qstate3(b,10) + bw_qstate3(b,11)*4 + bw_qstate3(b,12)*16 + bw_qstate3(b,13)*64; }   // THE REBUILDING (feat 14): chapter quest states, packed
__attribute__((export_name("dbg_ch2race"))) int exp_dbg_ch2race(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id];
    return S->ch2_race_on? (int)(S->ch2_race_t + 0.999f) : -1; }
__attribute__((export_name("dbg_ch2escd"))) int exp_dbg_ch2escd(void){ int id = G.my_id>=0? G.my_id : 0;
    float d = bw_duster_dist(&G.bud[id]); return d<0? -1 : (int)d; }
__attribute__((export_name("dbg_ch2ex"))) int exp_dbg_ch2ex(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ch2_esc_on? (int)BWS[id].ch2_esc_x : 99999; }
__attribute__((export_name("dbg_ch2solid"))) int exp_dbg_ch2solid(int x, int y){ int id = G.my_id>=0? G.my_id : 0; return bw_solid_b(&G.bud[id], (float)x, (float)y); }
__attribute__((export_name("dbg_ch2ey"))) int exp_dbg_ch2ey(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ch2_esc_on? (int)BWS[id].ch2_esc_y : 99999; }
__attribute__((export_name("dbg_ch2fest"))) int exp_dbg_ch2fest(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].ch2_fest_t>0? 1 : 0; }
__attribute__((export_name("dbg_story10"))) int exp_dbg_story10(void){ int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id];
    int n=0; for (int q=0;q<10;q++) if (bw_qstate_g(b,q)==2) n++; return n; }
__attribute__((export_name("dbg_ridge"))) int exp_dbg_ridge(int x, int y){ return bw_ridge_solid((float)x, (float)y); }
__attribute__((export_name("dbg_alertmax"))) int exp_dbg_alertmax(void){
    int id = G.my_id>=0? G.my_id : 0; BwSim *S=&BWS[id]; int m=0;
    for (int i=0;i<BW_ROAMN;i++){ int a=(int)(S->roam[i].alert*100); if (a>m) m=a; } return m; }
__attribute__((export_name("dbg_roamdist"))) int exp_dbg_roamdist(void){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; BwSim *S=&BWS[id]; int best=-1;
    for (int i=0;i<BW_ROAMN;i++){ BWHack *h=&S->roam[i]; if (!h->cell && !h->x && !h->y) continue;
        float dx=b->x-h->x, dy=b->y-h->y; int d=(int)fsqrt(dx*dx+dy*dy); if (best<0 || d<best) best=d; } return best; }
__attribute__((export_name("dbg_tentcount"))) int exp_dbg_tentcount(void){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; u32 seed=(u32)b->bw_seed; int n=0;
    for (int cx=-10; cx<=10; cx++) for (int cy=-10; cy<=10; cy++){
        float ox, oy; if (bw_cell_feat(cx, cy, seed, &ox, &oy, b->bw_mode==1)==1) n++; } return n; }
__attribute__((export_name("dbg_bitdist"))) int exp_dbg_bitdist(void){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; float x, y; bw_bitling_pos(&x, &y);
    float dx=x-b->x, dy=y-b->y; return (int)fsqrt(dx*dx+dy*dy); }
__attribute__((export_name("dbg_bittp"))) void exp_dbg_bittp(void){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; float x, y; bw_bitling_pos(&x, &y); b->x=x; b->y=y; }
__attribute__((export_name("dbg_hsigntp"))) void exp_dbg_hsigntp(int i){
    int id = G.my_id>=0? G.my_id : 0; Buddy *b=&G.bud[id]; b->x=BW_HSIGN_XY[i][0]; b->y=BW_HSIGN_XY[i][1]+16; }
__attribute__((export_name("dbg_bwx2"))) int exp_dbg_bwx2(void){
    return G.p2_slot>=0? (int)G.bud[G.p2_slot].bw_x : -99999; }
#endif // PLAT_WASM
