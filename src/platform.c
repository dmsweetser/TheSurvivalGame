// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: platform.c — PLATFORM hosts — WebAssembly, Windows, POSIX shared, Linux, macOS interface, and the embedded web server + game networking.
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
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
    IN.touch_seen=1; IN.touch_last=IN.dev_t+0.001f;
    if (phase==0){   // finger down: claim a slot, note d-pad zone/buttons
        for (int i=0;i<6;i++) if (!IN.tp[i].active){
            IN.tp[i].active=1; IN.tp[i].id=id;
            IN.tp[i].x=IN.tp[i].x0=x; IN.tp[i].y=IN.tp[i].y0=y;
            IN.tp[i].age=0; IN.tp[i].moved=0; IN.tp[i].zone=0;
            if (G.state==GS_PLAY){
                int z = tp_zone_at(x, y);
                if (z==9){ IN.tp[i].zone=9; IN.tp_wall_edge=1; }        // WALL: both schemes
                else if (z==7){ IN.tp[i].zone=7; IN.tp_wpn_edge=1; }   // WPN: both schemes
                else if (G.touch_ctl==1){
                    if (z==5 || z==6){                  // a button
                        IN.tp[i].zone = z;
                        if (z==6) IN.tp_act_edge=1;
                    } else {                            // anywhere else: the joystick
                        IN.tp[i].zone = 8; IN.tp[i].ax = x; IN.tp[i].ay = y;
                    }
                }
            }
            return;
        }
        return;
    }
    for (int i=0;i<6;i++) if (IN.tp[i].active && IN.tp[i].id==id){
        if (phase==1){
            IN.tp[i].x=x; IN.tp[i].y=y;
            if (x-IN.tp[i].x0>14 || IN.tp[i].x0-x>14 || y-IN.tp[i].y0>14 || IN.tp[i].y0-y>14) IN.tp[i].moved=1;
        } else {   // finger up: a quick, still tap (or an unmoved stick touch) is a tap
            if ((IN.tp[i].zone==0 || IN.tp[i].zone==8) && !IN.tp[i].moved && IN.tp[i].age<0.4f){
                IN.tap_x=x; IN.tap_y=y; IN.tapped=1;
            }
            IN.tp[i].active=0;
        }
        return;
    }
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
    if (v==0 || v==1) G.touch_ctl=v; return G.touch_ctl; }
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
__attribute__((export_name("dbg_crtmedal"))) int exp_dbg_crtmedal(int track){ int id = G.my_id>=0? G.my_id : 0; if (track<0 || track>7) return -1; return medal_tier(G.bud[id].crt_tbest[track], CRT_MEDAL[track]); }
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
__attribute__((export_name("dbg_warhbits"))) int exp_dbg_warhbits(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; for (int s=0;s<28;s++) if (S->war_shp[s]>0 && S->war_stype[s]==WAR_S_HIVE) return S->war_hbits[s]; return -1; }   // v1.21: the first living hive's purse (-1 = no hive)
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
__attribute__((export_name("dbg_warregion"))) int exp_dbg_warregion(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_region; }   // v1.17: the region the current battle is for
__attribute__((export_name("dbg_warsnap"))) int exp_dbg_warsnap(void){ int id = G.my_id>=0? G.my_id : 0; return war_snap_on[id]; }   // v1.17: a battle snapshot is parked for this player
__attribute__((export_name("dbg_warbcat"))) int exp_dbg_warbcat(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_bcat; }   // v1.17: open bar category (-1 top level, 0 TRAIN, 1 BUILD, 2 DEFENSE)
__attribute__((export_name("dbg_warnodes"))) int exp_dbg_warnodes(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; int sum=0;
    for (int n=0;n<S->war_nn;n++) sum += S->war_namt[n]; return S->war_nn*100000 + sum; }   // v1.17: node count *100000 + total bits left on the field
__attribute__((export_name("dbg_warcarry"))) int exp_dbg_warcarry(void){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id]; int m=-1;
    for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0 && S->war_utype[i]==WAR_U_HARV && S->war_ucarry[i]>m) m = S->war_ucarry[i]; return m; }   // v1.17: the fullest harvester's load
__attribute__((export_name("dbg_warqueue"))) int exp_dbg_warqueue(int s){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (s<0 || s>=WAR_MAXS || S->war_shp[s]<=0) return -1; return S->war_qlen[s]; }   // v1.17: a building's training-queue depth
__attribute__((export_name("dbg_warrally"))) int exp_dbg_warrally(int s){ int id = G.my_id>=0? G.my_id : 0; BwSim *S = &BWS[id];
    if (s<0 || s>=WAR_MAXS || S->war_shp[s]<=0 || S->war_rallyx[s]==0) return -1;
    return (int)(S->war_rallyx[s]-WAR_OX)*10000 + (int)(S->war_rallyy[s]-WAR_OY); }   // v1.17: a building's rally point, dbg_warunitxy packing, -1 = unset
__attribute__((export_name("dbg_warterrain"))) int exp_dbg_warterrain(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].war_tn; }
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
__attribute__((export_name("dbg_platmedal"))) int exp_dbg_platmedal(int lvl){ int id = G.my_id>=0? G.my_id : 0; if (lvl<1 || lvl>16) return -1; return medal_tier(G.bud[id].plat_tbest[lvl-1], PLAT_MEDAL[lvl-1]); }
__attribute__((export_name("dbg_plattime"))) int exp_dbg_plattime(void){ int id = G.my_id>=0? G.my_id : 0; return (int)(BWS[id].plat_lvl_t * 10.0f); }
__attribute__((export_name("dbg_platsettime"))) int exp_dbg_platsettime(int ds){ int id = G.my_id>=0? G.my_id : 0; BWS[id].plat_lvl_t = ds * 0.1f; return (int)(BWS[id].plat_lvl_t * 10.0f); }
__attribute__((export_name("dbg_platflagx"))) int exp_dbg_platflagx(void){ int id = G.my_id>=0? G.my_id : 0; return (BWS[id].plat_w - 3) * 24 + 12; }
__attribute__((export_name("dbg_platsel"))) int exp_dbg_platsel(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_sel; }
__attribute__((export_name("dbg_platpick"))) int exp_dbg_platpick(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].plat_pick; }
__attribute__((export_name("dbg_tanklvl"))) int exp_dbg_tanklvl(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_lvl; }
__attribute__((export_name("dbg_tanky"))) int exp_dbg_tanky(void){ int id = G.my_id>=0? G.my_id : 0; return (int)G.bud[id].y; }
__attribute__((export_name("dbg_tankpass"))) int exp_dbg_tankpass(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_pass; }
__attribute__((export_name("dbg_tankhull"))) int exp_dbg_tankhull(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_hull; }
__attribute__((export_name("dbg_tankdone"))) int exp_dbg_tankdone(void){ int id = G.my_id>=0? G.my_id : 0; return G.bud[id].tank_done; }
__attribute__((export_name("dbg_tankshells"))) int exp_dbg_tankshells(void){ int id = G.my_id>=0? G.my_id : 0; int n=0; for (int i=0;i<24;i++) if (BWS[id].tank_sh_a[i]) n++; return n; }
__attribute__((export_name("dbg_tankwep"))) int exp_dbg_tankwep(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_wep; }
__attribute__((export_name("dbg_tankgren"))) int exp_dbg_tankgren(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_gren; }
__attribute__((export_name("dbg_tankboom"))) int exp_dbg_tankboom(void){ int id = G.my_id>=0? G.my_id : 0; int n=0; for (int i=0;i<6;i++) if (BWS[id].tank_boom_t[i]>0) n++; for (int i=0;i<10;i++) if (BWS[id].tank_gr_a[i]) n+=10; return n; }
__attribute__((export_name("dbg_crtdrv"))) int exp_dbg_crtdrv(int k){ int id = G.my_id>=0? G.my_id : 0; if (k<0||k>5) return -1; return BWS[id].crt_species[k]*100 + BWS[id].crt_dcolidx[k]*10 + BWS[id].crt_nameidx[k]%10; }
__attribute__((export_name("dbg_tankobj"))) int exp_dbg_tankobj(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_obj; }
__attribute__((export_name("dbg_tankdeliv"))) int exp_dbg_tankdeliv(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_deliv; }
__attribute__((export_name("dbg_tankmid"))) int exp_dbg_tankmid(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_mid_hp; }
__attribute__((export_name("dbg_tankwarp"))) void exp_dbg_tankwarp(int y){ int id = G.my_id>=0? G.my_id : 0; if (G.bud[id].bw_mode==5){ G.bud[id].x = 12.5f*BW_TILE; G.bud[id].y = (float)y; } }
__attribute__((export_name("dbg_tankboss"))) int exp_dbg_tankboss(void){ int id = G.my_id>=0? G.my_id : 0; return BWS[id].tank_boss_hp; }
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

#if defined(PLAT_WIN32) || defined(PLAT_LINUX) || defined(PLAT_MAC)
// civil date -> days since 1970-01-01 (Hinnant's algorithm, integer
// only) — shared by the native plat_daynum() implementations below
// (v1.25 feat 6: the weekly challenge's calendar).
static int days_from_civil(int y, int m, int d){
    y -= m <= 2;
    int era = (y >= 0 ? y : y-399) / 400;
    int yoe = y - era * 400;                                   // [0, 399]
    int doy = (153*(m + (m > 2 ? -3 : 9)) + 2)/5 + d-1;        // [0, 365]
    int doe = yoe * 365 + yoe/4 - yoe/100 + doy;               // [0, 146096]
    return era * 146097 + doe - 719468;
}
#endif

// ===========================================================================
// PLATFORM: Windows — Win32 + GDI + winmm MIDI + Winsock UDP. Single .exe.
// ===========================================================================
#ifdef PLAT_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#undef IN
#undef OUT
#ifdef _MSC_VER
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#endif

static HWND g_hwnd=0;
static u64 g_raw=0;
static LARGE_INTEGER qpf; static int qpf_ok=0;
double plat_now(void){
    if (!qpf_ok){ QueryPerformanceFrequency(&qpf); qpf_ok=1; }
    LARGE_INTEGER t; QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)qpf.QuadPart;
}
int plat_daynum(void){
    SYSTEMTIME st; GetLocalTime(&st);
    return days_from_civil((int)st.wYear, (int)st.wMonth, (int)st.wDay);
}
float plat_minutes(void){
    SYSTEMTIME st; GetLocalTime(&st);
    return (float)st.wHour*60.0f + (float)st.wMinute + (float)st.wSecond/60.0f + (float)st.wMilliseconds/60000.0f;
}
// --- MIDI ---
static HMIDIOUT g_midi=0;
typedef struct { double off; int ch, note; } MidiOff;
static MidiOff g_offs[160]; static int g_offs_n=0;
static void midi_prog(int ch, int prog){
    if (g_midi) midiOutShortMsg(g_midi, 0xC0 | ch | (prog<<8));
}
void plat_note(int ch, int note, int vel, int instr){
    if (!g_midi) return;
    { static int kz_state = -1;   // WHALES ONLY: KAZOO MODE re-voices the music channels
      int kz = (G.whale_on & 16384)? 1 : 0;
      if (kz != kz_state){ kz_state = kz;
          midi_prog(0, kz? 71 : 80); midi_prog(1, kz? 70 : 38); midi_prog(2, kz? 71 : 81); } }
    int mc = ch, mn = note;
    if (ch==3 && instr==3){ mc=9; mn = vel>90? 49 : 42; }        // noise -> drums
    if (note<0 || note>127) return;
    midiOutShortMsg(g_midi, 0x90 | mc | (mn<<8) | (vel<<16));
    if (g_offs_n<160){ g_offs[g_offs_n].off = plat_now() + (ch==1? 0.22 : 0.16);
        g_offs[g_offs_n].ch=mc; g_offs[g_offs_n].note=mn; g_offs_n++; }
}
static void plat_audio_pump(void){
    double now = plat_now();
    for (int i=0;i<g_offs_n;i++) if (g_offs[i].off<=now){
        midiOutShortMsg(g_midi, 0x80 | g_offs[i].ch | (g_offs[i].note<<8));
        g_offs[i]=g_offs[--g_offs_n]; i--;
    }
}
// --- save file ---
// Saves live in ONE stable home (%APPDATA%) — they used to be loose
// files in whatever folder the exe ran from, so a fresh download
// (TheSurvivalGame(14).exe in Downloads) found a stale set from an
// older run, or none, and players "lost" progress that was sitting
// in another folder (Daniel's vanishing buddy, Nov 5). The legacy
// beside-the-exe file is adopted ONLY when the stable home has no
// copy yet (the one-time upgrade migration), and is then lifted into
// the stable home. It must never win just for being newer: a legacy
// file can only be written by an OLD build, so a Downloads folder
// full of old exes would otherwise keep rewinding the player's run
// to whichever stale set was touched last (Daniel, Nov 5 again —
// the buddy "lost" on Continue was the stable run being replaced
// by a fresher-mtime legacy set from an older exe in the folder).
static void win_save_paths(int slot, char *stable, char *legacy){
    save_slot_name(legacy, slot);
    const char *app = getenv("APPDATA");
    if (app && app[0]) snprintf(stable, 1024, "%s\\%s", app, legacy);
    else { int i=0; while (legacy[i]){ stable[i]=legacy[i]; i++; } stable[i]=0; }
}
void plat_save_slot(int slot, const char *d, int len){
    char sp[1024], lp[64]; win_save_paths(slot, sp, lp);
    HANDLE h = CreateFileA(sp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (h==INVALID_HANDLE_VALUE) h = CreateFileA(lp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);   // never lose a save
    if (h==INVALID_HANDLE_VALUE) return;
    DWORD wr; WriteFile(h, d, (DWORD)len, &wr, 0); CloseHandle(h);
}
int plat_load_slot(int slot, char *buf, int cap){
    char sp[1024], lp[64]; win_save_paths(slot, sp, lp);
    WIN32_FILE_ATTRIBUTE_DATA sa, la;
    int hs = GetFileAttributesExA(sp, GetFileExInfoStandard, &sa)!=0;
    int hl = GetFileAttributesExA(lp, GetFileExInfoStandard, &la)!=0;
    const char *use = 0; int import = 0;
    if (hs) use=sp;                          // the stable home is canonical once it exists
    else if (hl){ use=lp; import=1; }        // one-time migration only
    if (!use) return -1;
    HANDLE h = CreateFileA(use, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h==INVALID_HANDLE_VALUE) return -1;
    DWORD rd; BOOL ok = ReadFile(h, buf, (DWORD)cap, &rd, 0); CloseHandle(h);
    if (!ok) return -1;
    if (import){   // lift the legacy copy into the stable home
        HANDLE w = CreateFileA(sp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        if (w!=INVALID_HANDLE_VALUE){ DWORD wr; WriteFile(w, buf, rd, &wr, 0); CloseHandle(w); }
    }
    return (int)rd;
}
void plat_save(const char *d, int len){ plat_save_slot(0, d, len); }
int plat_load(char *buf, int cap){ return plat_load_slot(0, buf, cap); }
void plat_open_url(const char *url){   // Whales Only: only buy.stripe.com links ever reach here
    if (!url || strncmp(url, "https://buy.stripe.com/", 23)!=0) return;
    ShellExecuteA(0, "open", url, 0, 0, SW_SHOWNORMAL);
}
int plat_whale_poll(char *buf, int cap){   // a whale-code.txt: cwd, else beside the saves in %APPDATA%
    char sp[1024]; const char *app = getenv("APPDATA");
    HANDLE h = CreateFileA("whale-code.txt", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h==INVALID_HANDLE_VALUE && app && app[0]){
        snprintf(sp, sizeof(sp), "%s\\whale-code.txt", app);
        h = CreateFileA(sp, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    } else sp[0]=0;
    if (h==INVALID_HANDLE_VALUE) return 0;
    DWORD rd; BOOL ok = ReadFile(h, buf, (DWORD)(cap-1), &rd, 0); CloseHandle(h);
    DeleteFileA("whale-code.txt");
    if (sp[0]) DeleteFileA(sp);
    if (!ok || rd==0) return 0;
    buf[rd]=0; return (int)rd;
}
// (networking lives in the shared embedded-web-server section at the
// end of this file: the app serves the web build and browser guests
// relay straight into the host — there is no separate UDP transport.)

// --- window / input: report physical keys; the core applies bindings ---
static int vk_pk(int vk){
    if (vk>='A' && vk<='Z') return vk-'A';
    if (vk>='0' && vk<='9') return 26+(vk-'0');
    if (vk>=VK_NUMPAD0 && vk<=VK_NUMPAD9) return 26+(vk-VK_NUMPAD0);
    switch (vk){
        case VK_UP: return PK_UP; case VK_DOWN: return PK_DOWN;
        case VK_LEFT: return PK_LEFT; case VK_RIGHT: return PK_RIGHT;
        case VK_SPACE: return PK_SPACE; case VK_RETURN: return PK_ENTER;
        case VK_ESCAPE: return PK_ESC;
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: return PK_SHIFT;
        case VK_BACK: return PK_BACKSPACE;
    }
    return -1;
}
static void key_bit(int vk, int down){
    int pk = vk_pk(vk);
    if (pk>=0){ if (down) g_raw |= (1ULL<<pk); else g_raw &= ~(1ULL<<pk); }
}
static LRESULT CALLBACK wndproc(HWND hw, UINT msg, WPARAM wp, LPARAM lp){
    (void)lp;
    switch (msg){
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_KEYDOWN: { int pk = vk_pk((int)wp);
        if (pk>=0 && !((g_raw>>pk)&1)) IN.edges_in |= (1ULL<<pk);   // one edge per physical press
        key_bit((int)wp, 1); return 0; }
    case WM_KEYUP: key_bit((int)wp, 0); return 0;
    case WM_CHAR:
        // typed characters: the game core decides whether a text field
        // is selected right now; everywhere else they are ignored.
        if (wp>=32 && wp<127 && IN.typed_n<16) IN.typed[IN.typed_n++]=(char)wp;
        return 0;
    }
    return DefWindowProcA(hw, msg, wp, lp);
}
// --- gamepad: XInput, loaded dynamically (no import lib needed) ---
typedef struct { u32 packet; u16 buttons; u8 lt, rt; short lx, ly, rx, ry; } XIState;
typedef u32 (__stdcall *XInputGetState_t)(u32, XIState*);
static XInputGetState_t xi_get=0; static int xi_tried=0;
static void pad_set(int a, int down){
    if (down){ if (!((IN.act_held>>a)&1)) IN.act_edges |= (1u<<a); IN.act_held |= (1u<<a); }
    else IN.act_held &= ~(1u<<a);
}
static void pad2_set(int a, int down){
    if (down){ if (!((IN.act2_held>>a)&1)) IN.act2_edges |= (1u<<a); IN.act2_held |= (1u<<a); }
    else IN.act2_held &= ~(1u<<a);
}
static void win32_pad_apply(XIState *s, int ch){
    u16 b = s->buttons;
    void (*set)(int,int) = ch? pad2_set : pad_set;
    if (ch==0){   // pad 1's raw sticks for WAR!'s cursor (v1.16); XInput's Y is up-positive, the game's axis order wants down-positive
        IN.pad_ax[0] = s->lx/32767.0f; IN.pad_ax[1] = -s->ly/32767.0f;
        IN.pad_ax[2] = s->rx/32767.0f; IN.pad_ax[3] = -s->ry/32767.0f;
    }
    if (ch==1){   // pad 2's raw sticks (v1.25 feat 13), same flip
        IN.pad2_ax[0] = s->lx/32767.0f; IN.pad2_ax[1] = -s->ly/32767.0f;
        IN.pad2_ax[2] = s->rx/32767.0f; IN.pad2_ax[3] = -s->ry/32767.0f;
    }
    set(ACT_UP,    (b&0x0001) || s->ly>12000);
    set(ACT_DOWN,  (b&0x0002) || s->ly<-12000);
    set(ACT_LEFT,  (b&0x0004) || s->lx<-12000);
    set(ACT_RIGHT, (b&0x0008) || s->lx>12000);
    set(ACT_FIRE,  (b&0x1000) || s->rt>30);
    set(ACT_INTERACT, (b&0x2000)!=0);
    set(ACT_WALL,  (b&0x4000)!=0);
    set(ACT_WPREV, (b&0x0100)!=0);
    set(ACT_WNEXT, (b&0x0200)!=0);
    set(ACT_PAUSE, (b&0x0010)!=0);
    set(ACT_ESC,   (b&0x0020)!=0);
    set(ACT_BEAN,  (b&0x8000)!=0);   // Y: the Backfire Bean
    set(ACT_SPRINT,(b&0x0040) || s->lt>30);
}
static void win32_pad_poll(void){
    if (!xi_tried){ xi_tried=1;
        HMODULE h = LoadLibraryA("xinput1_4.dll");
        if (!h) h = LoadLibraryA("xinput1_3.dll");
        if (h) xi_get = (XInputGetState_t)GetProcAddress(h, "XInputGetState");
    }
    if (!xi_get) return;
    // connected pads in user order: first drives P1, second drives P2
    int ch=0, count=0;
    for (int u=0; u<4 && ch<2; u++){
        XIState s; memset(&s, 0, sizeof(s));
        if (xi_get(u, &s)!=0) continue;
        count++;
        win32_pad_apply(&s, ch++);
    }
    if (!count) IN.pad_ax[0]=IN.pad_ax[1]=IN.pad_ax[2]=IN.pad_ax[3]=0;   // no pad: no phantom sticks
    if (count<2) IN.pad2_ax[0]=IN.pad2_ax[1]=IN.pad2_ax[2]=IN.pad2_ax[3]=0;   // no second pad: no phantom sticks either
    IN.pad_count = count;
}
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show){
    (void)hPrev; (void)cmd;
    web_server_start();   // serve the game page + relay browser guests
    WNDCLASSA wc; ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc=wndproc; wc.hInstance=hInst; wc.lpszClassName="TSGWindow";
    wc.hCursor=LoadCursorA(0, MAKEINTRESOURCEA(32515)); /* IDC_CROSSHAIR */
    wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassA(&wc);
    RECT r={0,0,1280,720}; AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
    g_hwnd = CreateWindowA("TSGWindow", "THE SURVIVAL GAME", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right-r.left, r.bottom-r.top, 0, 0, hInst, 0);
    if (!g_hwnd) return 1;
    ShowWindow(g_hwnd, show);
    if (midiOutOpen(&g_midi, MIDI_MAPPER, 0, 0, CALLBACK_NULL)==MMSYSERR_NOERROR){
        midi_prog(0, 80);   // square lead
        midi_prog(1, 38);   // synth bass
        midi_prog(2, 81);   // saw arp
        midi_prog(3, 80);   // sfx lead
    }
    BITMAPINFO bmi; ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth=PW; bmi.bmiHeader.biHeight=-PH;
    bmi.bmiHeader.biPlanes=1; bmi.bmiHeader.biBitCount=32; bmi.bmiHeader.biCompression=BI_RGB;
    int running=1;
    while (running){
        MSG m;
        while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)){
            if (m.message==WM_QUIT) running=0;
            TranslateMessage(&m); DispatchMessageA(&m);
        }
        if (!running) break;
        IN.raw = g_raw;
        win32_pad_poll();
        frame();
        plat_audio_pump();
        HDC dc = GetDC(g_hwnd);
        RECT rc; GetClientRect(g_hwnd, &rc);
        SetStretchBltMode(dc, HALFTONE);                 // smooth upscale
        SetBrushOrgEx(dc, 0, 0, 0);
        StretchDIBits(dc, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
                      0, 0, PW, PH, fb, &bmi, DIB_RGB_COLORS, SRCCOPY);
        ReleaseDC(g_hwnd, dc);
        Sleep(1);
    }
    if (g_midi) midiOutClose(g_midi);
    plat_net_stop();
    return 0;
}

#endif // PLAT_WIN32

// ===========================================================================
// POSIX shared bits for the Linux and macOS builds: a small software
// synth (the WebAudio voices, re-implemented in C) and BSD-socket UDP.
// ===========================================================================
#if defined(PLAT_LINUX) || defined(PLAT_MAC)
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

double plat_now(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}
int plat_daynum(void){
    time_t t = time(0); struct tm lt;
    localtime_r(&t, &lt);
    return days_from_civil(lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday);
}
float plat_minutes(void){
    time_t t = time(0); struct tm lt;
    localtime_r(&t, &lt);
    return (float)lt.tm_hour*60.0f + (float)lt.tm_min + (float)lt.tm_sec/60.0f;
}

// --- software synth: square / triangle / noise voices with decay ---
typedef struct { double t0; float freq, dur, gain; int instr, ch; unsigned seed; } Voice;
static Voice g_voices[48];
void plat_note(int ch, int note, int vel, int instr){
    if (vel<=0 || note<0 || note>127) return;
    double now = plat_now();
    for (int i=0;i<48;i++){
        Voice *v=&g_voices[i];
        if (v->t0<=0 || now > v->t0 + v->dur){
            v->t0 = now;
            v->freq = (float)(440.0 * pow(2.0, (note-69)/12.0));
            v->dur = (instr==3)? 0.13f : (ch==1? 0.24f : 0.16f);
            float vv = vel/110.0f; if (vv>1) vv=1;
            v->gain = vv * (ch==0? 0.30f : 0.36f) * (instr==3? 0.7f : 1.0f);
            v->instr = instr; v->ch = ch;
            v->seed = (unsigned)(now*44100.0) + (unsigned)i*777u;
            return;
        }
    }
}
// render n mono float samples at 44100 Hz starting at absolute time `now`
void synth_render(float *out, int n, double now){
    for (int i=0;i<n;i++) out[i]=0;
    for (int vi=0; vi<48; vi++){
        Voice *v=&g_voices[vi]; if (v->t0<=0) continue;
        for (int i=0;i<n;i++){
            double t = now + (double)i/44100.0 - v->t0;
            if (t<0) continue;
            if (t>=v->dur) break;
            float s;
            if (v->instr==3){
                unsigned sd = v->seed + (unsigned)(t*44100.0);
                sd = sd*1664525u + 1013904223u;
                s = ((float)((sd>>9)&0xFFFF)/32768.0f - 1.0f) * (1.0f - (float)(t/v->dur));
            } else {
                double ph = t * v->freq; ph -= (double)(long long)ph;
                if (v->instr==4){   // WHALES ONLY: KAZOO — buzzy square, 29 Hz buzz, a breath of noise
                    float sq = ph<0.5? 1.0f : -1.0f;
                    float am = 0.72f + 0.28f*(float)sin(6.2831853*29.0*t);
                    unsigned sd2 = v->seed + (unsigned)(t*44100.0); sd2 = sd2*1664525u + 1013904223u;
                    s = sq*am + ((float)((sd2>>9)&0xFFFF)/32768.0f - 1.0f)*0.16f;
                }
                else s = (v->instr==1)? (float)(2.0*fabs(2.0*ph-1.0)-1.0) : (ph<0.5? 1.0f : -1.0f);
                s *= (float)exp(-6.0 * t / v->dur);
            }
            out[i] += s * v->gain;
        }
    }
    for (int i=0;i<n;i++){ if (out[i]>1) out[i]=1; else if (out[i]<-1) out[i]=-1; }
}
// (networking lives in the shared embedded-web-server section at the
// end of this file: the app serves the web build and browser guests
// relay straight into the host — there is no separate UDP transport.)

#endif // PLAT_LINUX || PLAT_MAC

// ===========================================================================
// PLATFORM: Linux — X11 window, ALSA synth output, stdio save file.
// ===========================================================================
#ifdef PLAT_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/joystick.h>
#include <X11/XKBlib.h>

// Saves live in ONE stable home (~/.local/share) — see the Windows
// backend for why (loose beside-the-exe files lost players' runs).
// The legacy beside-the-exe file is adopted only when the stable
// home has no copy yet, and is then lifted into the stable home;
// it never wins just for being newer (an old build's save set must
// not rewind the canonical one).
static void lin_save_paths(int slot, char *stable, char *legacy){
    save_slot_name(legacy, slot);
    const char *home = getenv("HOME");
    if (home && home[0]) snprintf(stable, 1024, "%s/.local/share/%s", home, legacy);
    else { int i=0; while (legacy[i]){ stable[i]=legacy[i]; i++; } stable[i]=0; }
}
static void lin_save_mkdir(void){
    const char *home = getenv("HOME"); if (!home || !home[0]) return;
    char p[1024];
    snprintf(p, sizeof(p), "%s/.local", home); mkdir(p, 0755);
    snprintf(p, sizeof(p), "%s/.local/share", home); mkdir(p, 0755);
}
void plat_save_slot(int slot, const char *d, int len){
    char sp[1024], lp[64]; lin_save_paths(slot, sp, lp);
    lin_save_mkdir();
    FILE *f = fopen(sp, "wb");
    if (!f) f = fopen(lp, "wb");   // never lose a save
    if (!f) return;
    fwrite(d, 1, (size_t)len, f); fclose(f);
}
int plat_load_slot(int slot, char *buf, int cap){
    char sp[1024], lp[64]; lin_save_paths(slot, sp, lp);
    struct stat ss, ls;
    int hs = stat(sp, &ss)==0, hl = stat(lp, &ls)==0;
    const char *use = 0; int import = 0;
    if (hs) use=sp;                          // the stable home is canonical once it exists
    else if (hl){ use=lp; import=1; }        // one-time migration only
    if (!use) return -1;
    FILE *f = fopen(use, "rb");
    if (!f) return -1;
    int n = (int)fread(buf, 1, (size_t)cap, f); fclose(f);
    if (import){ lin_save_mkdir();
        FILE *w = fopen(sp, "wb");
        if (w){ fwrite(buf, 1, (size_t)n, w); fclose(w); } }
    return n;
}
void plat_save(const char *d, int len){ plat_save_slot(0, d, len); }
int plat_load(char *buf, int cap){ return plat_load_slot(0, buf, cap); }
void plat_open_url(const char *url){   // Whales Only: only buy.stripe.com links ever reach here
    if (!url || strncmp(url, "https://buy.stripe.com/", 23)!=0) return;
    if (fork()==0){ execlp("xdg-open", "xdg-open", url, (char*)0); _exit(1); }
}
int plat_whale_poll(char *buf, int cap){   // a whale-code.txt: cwd, else beside the saves
    char sp[1024]; const char *home = getenv("HOME");
    FILE *f = fopen("whale-code.txt", "rb");
    if (!f && home && home[0]){ snprintf(sp, sizeof(sp), "%s/.local/share/whale-code.txt", home); f = fopen(sp, "rb"); }
    else sp[0]=0;
    if (!f) return 0;
    int n = (int)fread(buf, 1, (size_t)(cap-1), f); fclose(f);
    remove("whale-code.txt");
    if (sp[0]) remove(sp);
    if (n<=0) return 0;
    buf[n]=0; return n;
}

// --- ALSA output thread: pulls the software synth in blocks ---
static void *alsa_thread(void *arg){
    (void)arg;
    snd_pcm_t *pcm=0;
    if (snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)!=0) return 0;
    if (snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                           1, 44100, 1, 120000)!=0){ snd_pcm_close(pcm); return 0; }
    static float fbuf[1024]; static short sbuf[1024];
    double rt = plat_now();
    for (;;){
        synth_render(fbuf, 1024, rt);
        rt += 1024/44100.0;
        for (int i=0;i<1024;i++) sbuf[i] = (short)(fbuf[i]*32767.0f);
        int w = (int)snd_pcm_writei(pcm, sbuf, 1024);
        if (w<0) snd_pcm_prepare(pcm);
    }
    return 0;
}

// --- window / input ---
#define WIN_W 1280
#define WIN_H 720
static Display *g_dpy=0;
static u64 g_raw=0;
static u32 g_scaled[WIN_W*WIN_H];
static void blit_scale(void){
    // bilinear 640x360 -> window size (exactly 2x at the default size)
    for (int y=0;y<WIN_H;y++){
        float sy = (y+0.5f)*(float)PH/WIN_H - 0.5f;
        int y0 = (int)floorf(sy); float fy = sy - y0;
        if (y0<0){ y0=0; fy=0; } if (y0>PH-2){ y0=PH-2; fy=1; }
        for (int x=0;x<WIN_W;x++){
            float sx = (x+0.5f)*(float)PW/WIN_W - 0.5f;
            int x0 = (int)floorf(sx); float fx = sx - x0;
            if (x0<0){ x0=0; fx=0; } if (x0>PW-2){ x0=PW-2; fx=1; }
            u32 c00=fb[y0*PW+x0], c10=fb[y0*PW+x0+1], c01=fb[(y0+1)*PW+x0], c11=fb[(y0+1)*PW+x0+1];
            int r = (int)((COL_R(c00)*(1-fx)*(1-fy) + COL_R(c10)*fx*(1-fy) + COL_R(c01)*(1-fx)*fy + COL_R(c11)*fx*fy) + 0.5f);
            int g = (int)((COL_G(c00)*(1-fx)*(1-fy) + COL_G(c10)*fx*(1-fy) + COL_G(c01)*(1-fx)*fy + COL_G(c11)*fx*fy) + 0.5f);
            int b = (int)((COL_B(c00)*(1-fx)*(1-fy) + COL_B(c10)*fx*(1-fy) + COL_B(c01)*(1-fx)*fy + COL_B(c11)*fx*fy) + 0.5f);
            g_scaled[y*WIN_W+x] = COL(r, g, b);
        }
    }
}
static int ks_pk(KeySym ks){
    if (ks>=XK_a && ks<=XK_z) return (int)(ks-XK_a);
    if (ks>=XK_A && ks<=XK_Z) return (int)(ks-XK_A);
    if (ks>=XK_0 && ks<=XK_9) return 26+(int)(ks-XK_0);
    if (ks>=XK_KP_0 && ks<=XK_KP_9) return 26+(int)(ks-XK_KP_0);
    switch (ks){
        case XK_Up: case XK_KP_Up: return PK_UP;
        case XK_Down: case XK_KP_Down: return PK_DOWN;
        case XK_Left: case XK_KP_Left: return PK_LEFT;
        case XK_Right: case XK_KP_Right: return PK_RIGHT;
        case XK_space: return PK_SPACE;
        case XK_Return: case XK_KP_Enter: return PK_ENTER;
        case XK_Escape: return PK_ESC;
        case XK_Shift_L: case XK_Shift_R: return PK_SHIFT;
        case XK_BackSpace: return PK_BACKSPACE;
    }
    return -1;
}
static void key_bit(KeySym ks, int down){
    int pk = ks_pk(ks);
    if (pk>=0){ if (down) g_raw |= (1ULL<<pk); else g_raw &= ~(1ULL<<pk); }
}
// --- gamepad: /dev/input/js0 (standard joystick API) ---
static void pad_set(int a, int down){
    if (down){ if (!((IN.act_held>>a)&1)) IN.act_edges |= (1u<<a); IN.act_held |= (1u<<a); }
    else IN.act_held &= ~(1u<<a);
}
static int js_fd=-1, js_btn[16], js_ax[8], js_retry=0;
static int js2_fd=-1, js2_btn[16], js2_ax[8], js2_retry=0;
static void pad2_set(int a, int down){
    if (down){ if (!((IN.act2_held>>a)&1)) IN.act2_edges |= (1u<<a); IN.act2_held |= (1u<<a); }
    else IN.act2_held &= ~(1u<<a);
}
static int linux_js_drain(int *fdp, const char *path, int *retry, int *btn, int *ax){
    if (*fdp<0){
        if (--(*retry)>0) return 0;
        *retry = 300;
        *fdp = open(path, O_RDONLY|O_NONBLOCK);
        if (*fdp<0) return 0;
    }
    struct js_event ev;
    ssize_t r;
    int alive=1;
    while ((r=read(*fdp, &ev, sizeof(ev)))==(ssize_t)sizeof(ev)){
        int type = ev.type & ~JS_EVENT_INIT;
        if (type==JS_EVENT_BUTTON && ev.number<16) btn[ev.number]=ev.value;
        else if (type==JS_EVENT_AXIS && ev.number<8) ax[ev.number]=ev.value;
    }
    if (r==0) alive=0;
    if (!alive){ close(*fdp); *fdp=-1; memset(btn,0,16*sizeof(int)); memset(ax,0,8*sizeof(int)); return 0; }
    return 1;
}
static void linux_pad_apply(int *btn, int *ax, int ch){
    void (*set)(int,int) = ch? pad2_set : pad_set;
    if (ch==0){   // pad 1's raw sticks for WAR!'s cursor (v1.16); js axes 0/1 are the left stick (down-positive, as the digital map below reads them), 3/4 the right stick on the common mapping
        IN.pad_ax[0] = ax[0]/32767.0f; IN.pad_ax[1] = ax[1]/32767.0f;
        IN.pad_ax[2] = ax[3]/32767.0f; IN.pad_ax[3] = ax[4]/32767.0f;
    }
    if (ch==1){   // pad 2's raw sticks (v1.25 feat 13), same mapping
        IN.pad2_ax[0] = ax[0]/32767.0f; IN.pad2_ax[1] = ax[1]/32767.0f;
        IN.pad2_ax[2] = ax[3]/32767.0f; IN.pad2_ax[3] = ax[4]/32767.0f;
    }
    set(ACT_UP,    ax[1]<-12000 || ax[7]<-12000);
    set(ACT_DOWN,  ax[1]>12000  || ax[7]>12000);
    set(ACT_LEFT,  ax[0]<-12000 || ax[6]<-12000);
    set(ACT_RIGHT, ax[0]>12000  || ax[6]>12000);
    set(ACT_FIRE,  btn[0] || ax[5]>0);
    set(ACT_INTERACT, btn[1]!=0);
    set(ACT_WALL,  btn[2]!=0);
    set(ACT_WPREV, btn[4]!=0);
    set(ACT_WNEXT, btn[5]!=0);
    set(ACT_ESC,   btn[6]!=0);
    set(ACT_PAUSE, btn[7]!=0);
    set(ACT_BEAN,  btn[3]!=0);   // Y: the Backfire Bean
    set(ACT_SPRINT, btn[9] || ax[2]>0);
}
static void linux_pad_poll(void){
    int n=0, n2=0;
    if (linux_js_drain(&js_fd, "/dev/input/js0", &js_retry, js_btn, js_ax)){ linux_pad_apply(js_btn, js_ax, 0); n++; }
    if (linux_js_drain(&js2_fd, "/dev/input/js1", &js2_retry, js2_btn, js2_ax)){ linux_pad_apply(js2_btn, js2_ax, 1); n++; n2=1; }
    if (!n) IN.pad_ax[0]=IN.pad_ax[1]=IN.pad_ax[2]=IN.pad_ax[3]=0;   // no pad: no phantom sticks
    if (!n2) IN.pad2_ax[0]=IN.pad2_ax[1]=IN.pad2_ax[2]=IN.pad2_ax[3]=0;   // no second pad: no phantom sticks either
    IN.pad_count = n;
}
int main(int argc, char **argv){
    (void)argc; (void)argv;
    web_server_start();   // serve the game page + relay browser guests
    g_dpy = XOpenDisplay(0);
    if (!g_dpy){ fprintf(stderr, "TheSurvivalGame: cannot open X display\n"); return 1; }
    // (keyboard auto-repeat arrives as release+press pairs with identical
    // timestamps; the event loop swallows those pairs, so a held key reads
    // as held and produces exactly one edge — no XKB mode needed.)
    int screen = DefaultScreen(g_dpy);
    Window win = XCreateSimpleWindow(g_dpy, RootWindow(g_dpy, screen), 0, 0, WIN_W, WIN_H, 0, 0, 0);
    XStoreName(g_dpy, win, "THE SURVIVAL GAME");
    { XSizeHints h; memset(&h, 0, sizeof(h));
      h.flags = PMinSize|PMaxSize; h.min_width=WIN_W; h.max_width=WIN_W; h.min_height=WIN_H; h.max_height=WIN_H;
      XSetWMNormalHints(g_dpy, win, &h); }
    Atom wm_delete = XInternAtom(g_dpy, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(g_dpy, win, &wm_delete, 1);
    XSelectInput(g_dpy, win, KeyPressMask|KeyReleaseMask|ExposureMask|StructureNotifyMask);
    XMapWindow(g_dpy, win);
    GC gc = XCreateGC(g_dpy, win, 0, 0);
    Visual *vis = DefaultVisual(g_dpy, screen);
    XImage *img = XCreateImage(g_dpy, vis, (unsigned)DefaultDepth(g_dpy, screen), ZPixmap, 0,
                               (char*)g_scaled, WIN_W, WIN_H, 32, WIN_W*4);
    pthread_t th; pthread_create(&th, 0, alsa_thread, 0); pthread_detach(th);
    int running=1;
    while (running){
        while (XPending(g_dpy)){
            XEvent ev; XNextEvent(g_dpy, &ev);
            if (ev.type==KeyRelease && XPending(g_dpy)){
                // auto-repeat pair (release+press, same keycode, same
                // timestamp): swallow both — the key never really came up
                XEvent nx; XPeekEvent(g_dpy, &nx);
                if (nx.type==KeyPress && nx.xkey.keycode==ev.xkey.keycode && nx.xkey.time==ev.xkey.time){
                    XNextEvent(g_dpy, &nx);
                    continue;
                }
            }
            if (ev.type==ClientMessage && (Atom)ev.xclient.data.l[0]==wm_delete) running=0;
            else if (ev.type==KeyPress || ev.type==KeyRelease){
                int down = (ev.type==KeyPress);
                char chbuf[8]; KeySym ks=0;
                int chn = XLookupString(&ev.xkey, chbuf, sizeof(chbuf), &ks, 0);
                { int pk = ks_pk(ks);
                  if (down && pk>=0 && !((g_raw>>pk)&1)) IN.edges_in |= (1ULL<<pk); }
                key_bit(ks, down);
                if (down && chn>0){
                    unsigned char c = (unsigned char)chbuf[0];
                    if (c>=32 && c<127 && IN.typed_n<16) IN.typed[IN.typed_n++]=(char)c;
                }
            }
        }
        if (!running) break;
        IN.raw = g_raw;
        linux_pad_poll();
        frame();
        blit_scale();
        XPutImage(g_dpy, win, gc, img, 0, 0, 0, 0, WIN_W, WIN_H);
        XFlush(g_dpy);
        usleep(1000);
    }
    plat_net_stop();
    XCloseDisplay(g_dpy);
    return 0;
}
#endif // PLAT_LINUX

// ===========================================================================
// PLATFORM: macOS — interface for the pure-C Cocoa host in mac_main.c.
// That file drives NSApplication through the Objective-C runtime (this
// build environment has no AppKit headers), renders the framebuffer via
// CGBitmapContext, and pulls audio from synth_render via AudioQueue.
// ===========================================================================
#ifdef PLAT_MAC
void plat_save_slot(int slot, const char *d, int len){
    char nm[64]; save_slot_name(nm, slot);
    char path[1024]; const char *home = getenv("HOME"); if (!home) return;
    snprintf(path, sizeof(path), "%s/Library/Application Support/%s", home, nm);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fwrite(d, 1, (size_t)len, f); fclose(f);
}
int plat_load_slot(int slot, char *buf, int cap){
    char nm[64]; save_slot_name(nm, slot);
    char path[1024]; const char *home = getenv("HOME"); if (!home) return -1;
    snprintf(path, sizeof(path), "%s/Library/Application Support/%s", home, nm);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    int n = (int)fread(buf, 1, (size_t)cap, f); fclose(f);
    return n;
}
void plat_save(const char *d, int len){ plat_save_slot(0, d, len); }
int plat_load(char *buf, int cap){ return plat_load_slot(0, buf, cap); }
void plat_open_url(const char *url){   // Whales Only: only buy.stripe.com links ever reach here
    if (!url || strncmp(url, "https://buy.stripe.com/", 23)!=0) return;
    if (fork()==0){ execl("/usr/bin/open", "open", url, (char*)0); _exit(1); }
}
int plat_whale_poll(char *buf, int cap){   // whale-code.txt: cwd, else beside the saves
    char path[1024]; const char *home = getenv("HOME");
    FILE *f = fopen("whale-code.txt", "rb");
    if (!f && home){ snprintf(path, sizeof(path), "%s/Library/Application Support/whale-code.txt", home);
                     f = fopen(path, "rb"); }
    if (!f) return 0;
    int n = (int)fread(buf, 1, (size_t)(cap-1), f); fclose(f);
    remove("whale-code.txt");
    if (home){ snprintf(path, sizeof(path), "%s/Library/Application Support/whale-code.txt", home); remove(path); }
    if (n<=0) return 0;
    buf[n]=0; return n;
}
u32 *tsg_fb(void){ return fb; }
void tsg_frame(void){ frame(); }
double tsg_now(void){ return plat_now(); }
void tsg_web_start(void){ web_server_start(); }
void tsg_synth_render(float *out, int n, double now){ synth_render(out, n, now); }
void tsg_set_raw(int lo, int hi){ IN.raw = (u64)(u32)lo | ((u64)(u32)hi<<32); }
void tsg_press(int pk){ if (pk>=0 && pk<64) IN.edges_in |= (1ULL<<pk); }
void tsg_act(int a, int down){
    if (a<0 || a>=ACT_COUNT) return;
    if (down){ if (!((IN.act_held>>a)&1)) IN.act_edges |= (1u<<a); IN.act_held |= (1u<<a); }
    else IN.act_held &= ~(1u<<a);
}
void tsg_act2(int a, int down){
    if (a<0 || a>=ACT_COUNT) return;
    if (down){ if (!((IN.act2_held>>a)&1)) IN.act2_edges |= (1u<<a); IN.act2_held |= (1u<<a); }
    else IN.act2_held &= ~(1u<<a);
}
void tsg_pads(int n){ IN.pad_count = n; }
void tsg_char(int c){ if (IN.typed_n<16) IN.typed[IN.typed_n++]=(char)c; }
#endif // PLAT_MAC

// ===========================================================================
// PLATFORM (all native builds): the embedded web server + game networking.
//
// This is what makes co-op zero-setup: the native app serves the web
// build of the game over HTTP on port 8379 and relays browser guests
// straight into the host's simulation over WebSocket. The host player
// just runs the app; friends open http://<this machine>:8379 in any
// browser and they are in — no installs, no mode to pick, no separate
// server to run. plat_net_* below is backed by two thread-safe queues
// between the game thread and the server thread.
// ===========================================================================
#if defined(PLAT_WIN32) || defined(PLAT_LINUX) || defined(PLAT_MAC)

// the web build, generated into webbuild.h by build.py (may be absent
// when compiling survival.c standalone — then a placeholder is served)
#if defined(__has_include)
#  if __has_include("webbuild.h")
#    include "webbuild.h"
#    define TSG_HAVE_WEB 1
#  endif
#endif
#ifndef TSG_HAVE_WEB
static const unsigned char TSG_WEB_HTML[] =
    "<html><body><h1>THE SURVIVAL GAME</h1>"
    "<p>The web build is not embedded in this binary.</p></body></html>";
static const unsigned TSG_WEB_HTML_LEN = sizeof(TSG_WEB_HTML)-1;
#endif

#define WEB_PORT 8379

// --- socket / thread / mutex shims ---
#ifdef PLAT_WIN32
typedef SOCKET sock_t;
#define SOCK_BAD INVALID_SOCKET
#define sock_close(s) closesocket(s)
#define SEND_FLAGS 0
static CRITICAL_SECTION g_qmtx; static int g_qmtx_ok=0;
static void qlock(void){ if (!g_qmtx_ok){ InitializeCriticalSection(&g_qmtx); g_qmtx_ok=1; } EnterCriticalSection(&g_qmtx); }
static void qunlock(void){ LeaveCriticalSection(&g_qmtx); }
#else
typedef int sock_t;
#define SOCK_BAD (-1)
#define sock_close(s) close(s)
#define SEND_FLAGS MSG_NOSIGNAL
static pthread_mutex_t g_qmtx = PTHREAD_MUTEX_INITIALIZER;
static void qlock(void){ pthread_mutex_lock(&g_qmtx); }
static void qunlock(void){ pthread_mutex_unlock(&g_qmtx); }
#endif
static void server_loop(void);
#ifdef PLAT_WIN32
static DWORD WINAPI server_thread_main(LPVOID p){ (void)p; server_loop(); return 0; }
static void start_server_thread(void){ HANDLE h=CreateThread(0,0,server_thread_main,0,0,0); if (h) CloseHandle(h); }
#else
static void *server_thread_main(void *p){ (void)p; server_loop(); return 0; }
static void start_server_thread(void){ pthread_t t; if (pthread_create(&t,0,server_thread_main,0)==0) pthread_detach(t); }
#endif

// --- SHA-1 + base64 (for the WebSocket handshake) ---
typedef struct { unsigned h[5]; unsigned long long len; unsigned char buf[64]; int n; } Sha1;
static unsigned rol32(unsigned v, int b){ return (v<<b) | (v>>(32-b)); }
static void sha1_block(Sha1 *s, const unsigned char *p){
    unsigned w[80];
    for (int i=0;i<16;i++) w[i]=((unsigned)p[i*4]<<24)|((unsigned)p[i*4+1]<<16)|((unsigned)p[i*4+2]<<8)|(unsigned)p[i*4+3];
    for (int i=16;i<80;i++) w[i]=rol32(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
    unsigned a=s->h[0], b=s->h[1], c=s->h[2], d=s->h[3], e=s->h[4];
    for (int i=0;i<80;i++){
        unsigned f,k;
        if (i<20){ f=(b&c)|((~b)&d); k=0x5A827999u; }
        else if (i<40){ f=b^c^d; k=0x6ED9EBA1u; }
        else if (i<60){ f=(b&c)|(b&d)|(c&d); k=0x8F1BBCDCu; }
        else { f=b^c^d; k=0xCA62C1D6u; }
        unsigned t=rol32(a,5)+f+e+k+w[i]; e=d; d=c; c=rol32(b,30); b=a; a=t;
    }
    s->h[0]+=a; s->h[1]+=b; s->h[2]+=c; s->h[3]+=d; s->h[4]+=e;
}
static void sha1_init(Sha1 *s){
    s->h[0]=0x67452301u; s->h[1]=0xEFCDAB89u; s->h[2]=0x98BADCFEu; s->h[3]=0x10325476u; s->h[4]=0xC3D2E1F0u;
    s->len=0; s->n=0;
}
static void sha1_update(Sha1 *s, const unsigned char *p, int n){
    s->len += (unsigned)n;
    while (n>0){
        int take = 64 - s->n; if (take>n) take=n;
        memcpy(s->buf + s->n, p, (size_t)take); s->n += take; p += take; n -= take;
        if (s->n==64){ sha1_block(s, s->buf); s->n=0; }
    }
}
static void sha1_final(Sha1 *s, unsigned char out[20]){
    unsigned long long bits = s->len * 8;
    s->buf[s->n++] = 0x80;
    if (s->n > 56){ while (s->n<64) s->buf[s->n++]=0; sha1_block(s, s->buf); s->n=0; }
    while (s->n<56) s->buf[s->n++]=0;
    for (int i=0;i<8;i++) s->buf[56+i] = (unsigned char)(bits >> (56-8*i));
    sha1_block(s, s->buf);
    for (int i=0;i<5;i++){
        out[i*4]=(unsigned char)(s->h[i]>>24); out[i*4+1]=(unsigned char)(s->h[i]>>16);
        out[i*4+2]=(unsigned char)(s->h[i]>>8); out[i*4+3]=(unsigned char)s->h[i];
    }
}
static void b64_encode(const unsigned char *in, int n, char *out){
    static const char T[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int o=0;
    for (int i=0;i<n;i+=3){
        int rem = n-i; unsigned v = (unsigned)in[i]<<16;
        if (rem>1) v |= (unsigned)in[i+1]<<8;
        if (rem>2) v |= in[i+2];
        out[o++] = T[(v>>18)&63]; out[o++] = T[(v>>12)&63];
        out[o++] = rem>1? T[(v>>6)&63] : '=';
        out[o++] = rem>2? T[v&63] : '=';
    }
    out[o]=0;
}

// --- packet queues between the game thread and the server thread ---
#define WQ_DEPTH 96
#define WQ_PKT 8192
typedef struct { int src; int len; int raw; unsigned char data[WQ_PKT]; } WPkt;
static WPkt g_inq[WQ_DEPTH];  static int g_inq_h=0, g_inq_t=0;
static WPkt g_outq[WQ_DEPTH]; static int g_outq_h=0, g_outq_t=0;
static int q_push(WPkt *q, int *h, int *t, int src, const unsigned char *d, int len, int raw){
    qlock();
    int nt = (*t+1) % WQ_DEPTH;
    if (nt == *h){ qunlock(); return 0; }              // full: drop
    WPkt *p = &q[*t];
    if (len > WQ_PKT) len = WQ_PKT;
    p->src = src; p->len = len; p->raw = raw;
    if (len > 0) memcpy(p->data, d, (size_t)len);
    *t = nt;
    qunlock();
    return 1;
}
static int q_pop(WPkt *q, int *h, int *t, WPkt *out){
    qlock();
    if (*h == *t){ qunlock(); return 0; }
    *out = q[*h];
    *h = (*h+1) % WQ_DEPTH;
    qunlock();
    return 1;
}

// --- connections ---
#define MAXCONN 8
typedef struct {
    sock_t fd; int used; int upgraded; int token;
    unsigned char rbuf[8192]; int rlen;
    unsigned char msg[WQ_PKT]; int mlen;
} Conn;
static Conn g_conns[MAXCONN];
static sock_t g_listen = SOCK_BAD;
static char g_join_addr[64] = "";
static unsigned char g_page[sizeof(TSG_WEB_HTML) + 4096];   // sized from the embedded page itself (+ room for the hosted-flag injection) — a fixed 300000 cap silently overflowed when the web build grew past it
static int g_page_len = 0;

static int send_all(sock_t fd, const unsigned char *d, int len){
    int sent = 0;
    while (sent < len){
        int n = (int)send(fd, (const char*)d + sent, len - sent, SEND_FLAGS);
        if (n <= 0) return 0;
        sent += n;
    }
    return 1;
}
static int ws_frame_build(unsigned char *out, const unsigned char *d, int len){
    int h = 0;
    out[h++] = 0x82;                                   // FIN + binary
    if (len < 126){ out[h++] = (unsigned char)len; }
    else if (len < 65536){ out[h++] = 126; out[h++] = (unsigned char)(len>>8); out[h++] = (unsigned char)len; }
    else { out[h++] = 127; for (int i=7;i>=0;i--) out[h++] = (unsigned char)((unsigned long long)len >> (i*8)); }
    if (len > 0) memcpy(out + h, d, (size_t)len);
    return h + len;
}
static void conn_close(int i){
    Conn *c = &g_conns[i];
    if (!c->used) return;
    if (c->upgraded){
        unsigned char l = 'L';
        q_push(g_inq, &g_inq_h, &g_inq_t, c->token, &l, 1, 0);   // tell the game
    }
    sock_close(c->fd);
    c->used = 0; c->upgraded = 0; c->rlen = 0; c->mlen = 0;
}
static void conn_http(int i, char *req){
    Conn *c = &g_conns[i];
    if (strstr(req, "pgrade: websocket") || strstr(req, "pgrade:WebSocket")){
        // WebSocket upgrade: answer the handshake, become a game peer
        const char *kp = strstr(req, "Sec-WebSocket-Key:");
        char key[128]; key[0]=0;
        if (kp){
            kp += strlen("Sec-WebSocket-Key:");
            while (*kp==' ') kp++;
            int n=0; while (kp[n] && kp[n]!='\r' && kp[n]!='\n' && n<120){ key[n]=kp[n]; n++; }
            key[n]=0;
        }
        char combo[256];
        snprintf(combo, sizeof(combo), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", key);
        Sha1 s; sha1_init(&s); sha1_update(&s, (const unsigned char*)combo, (int)strlen(combo));
        unsigned char digest[20]; sha1_final(&s, digest);
        char accept[64]; b64_encode(digest, 20, accept);
        char resp[512];
        int rn = snprintf(resp, sizeof(resp),
            "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
            "Connection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", accept);
        if (send_all(c->fd, (const unsigned char*)resp, rn)){
            c->upgraded = 1; c->token = i+1; c->rlen = 0; c->mlen = 0;
#ifdef PLAT_WIN32
            { unsigned long nd = 1; setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&nd, sizeof(nd)); }
#else
            { int nd = 1; setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY, &nd, sizeof(nd)); }
#endif
            return;
        }
        conn_close(i);
        return;
    }
    // plain HTTP: serve the game page (with the hosted flag injected)
    if (g_page_len > 0){
        char hdr[256];
        int hn = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\nConnection: close\r\n\r\n", g_page_len);
        send_all(c->fd, (const unsigned char*)hdr, hn);
        send_all(c->fd, g_page, g_page_len);
    } else {
        const char *nf = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send_all(c->fd, (const unsigned char*)nf, (int)strlen(nf));
    }
    sock_close(c->fd);
    c->used = 0; c->rlen = 0;
}
static void conn_read(int i){
    Conn *c = &g_conns[i];
    int space = (int)sizeof(c->rbuf) - c->rlen;
    if (space <= 0){ conn_close(i); return; }
    int n = (int)recv(c->fd, (char*)c->rbuf + c->rlen, space, 0);
    if (n <= 0){ conn_close(i); return; }
    c->rlen += n;
    if (!c->upgraded){
        // waiting for a full HTTP request header
        if (c->rlen >= 4){
            char tmp[4200];
            int m = c->rlen < 4100? c->rlen : 4100;
            memcpy(tmp, c->rbuf, (size_t)m); tmp[m]=0;
            if (strstr(tmp, "\r\n\r\n")) conn_http(i, tmp);
            else if (c->rlen > 4096) conn_close(i);
        }
        return;
    }
    // WebSocket frames
    for (;;){
        if (c->rlen < 2) return;
        unsigned b0 = c->rbuf[0], b1 = c->rbuf[1];
        int fin = (b0 & 0x80)!=0, opcode = b0 & 0x0F;
        int masked = (b1 & 0x80)!=0;
        unsigned long long len = b1 & 0x7F;
        int hdr = 2;
        if (len == 126){
            if (c->rlen < 4) return;
            len = ((unsigned)c->rbuf[2]<<8) | c->rbuf[3]; hdr = 4;
        } else if (len == 127){
            if (c->rlen < 10) return;
            len = 0; for (int k=0;k<8;k++) len = (len<<8) | c->rbuf[2+k];
            hdr = 10;
        }
        if (len > WQ_PKT){ conn_close(i); return; }
        if (masked) hdr += 4;
        if (c->rlen < hdr + (int)len) return;          // incomplete: wait
        unsigned char *pl = c->rbuf + hdr;
        if (masked){
            const unsigned char *mk = c->rbuf + hdr - 4;
            for (unsigned long long k=0;k<len;k++) pl[k] ^= mk[k&3];
        }
        if (opcode == 8){ conn_close(i); return; }     // close
        else if (opcode == 9){                          // ping -> pong
            unsigned char fr[16];
            int h = 0; fr[h++] = 0x8A;
            if (len < 126){ fr[h++] = (unsigned char)len; if (len > 0) memcpy(fr+h, pl, (size_t)len); h += (int)len; }
            q_push(g_outq, &g_outq_h, &g_outq_t, c->token, fr, h, 1);
        } else if (opcode == 0 || opcode == 1 || opcode == 2){
            if (c->mlen + (int)len <= WQ_PKT){
                if (len > 0) memcpy(c->msg + c->mlen, pl, (size_t)len);
                c->mlen += (int)len;
            }
            if (fin){
                if (c->mlen > 0) q_push(g_inq, &g_inq_h, &g_inq_t, c->token, c->msg, c->mlen, 0);
                c->mlen = 0;
            }
        }
        int consumed = hdr + (int)len;
        memmove(c->rbuf, c->rbuf + consumed, (size_t)(c->rlen - consumed));
        c->rlen -= consumed;
    }
}
static void drain_outq(void){
    WPkt p;
    static unsigned char frame[WQ_PKT + 16];
    while (q_pop(g_outq, &g_outq_h, &g_outq_t, &p)){
        if (p.raw){                                    // pre-framed (pong)
            if (p.src >= 1 && p.src <= MAXCONN){
                Conn *c = &g_conns[p.src-1];
                if (c->used && c->upgraded) send_all(c->fd, p.data, p.len);
            }
            continue;
        }
        int fl = ws_frame_build(frame, p.data, p.len);
        if (p.src < 0){                                // broadcast
            for (int i=0;i<MAXCONN;i++)
                if (g_conns[i].used && g_conns[i].upgraded)
                    send_all(g_conns[i].fd, frame, fl);
        } else if (p.src >= 1 && p.src <= MAXCONN){
            Conn *c = &g_conns[p.src-1];
            if (c->used && c->upgraded) send_all(c->fd, frame, fl);
        }
    }
}
static void server_loop(void){
    while (g_listen != SOCK_BAD){
        fd_set rf; FD_ZERO(&rf); FD_SET(g_listen, &rf);
        sock_t mx = g_listen;
        for (int i=0;i<MAXCONN;i++) if (g_conns[i].used){
            FD_SET(g_conns[i].fd, &rf);
            if (g_conns[i].fd > mx) mx = g_conns[i].fd;
        }
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 5000;
        int r = select((int)(mx+1), &rf, 0, 0, &tv);
        if (r > 0){
            if (FD_ISSET(g_listen, &rf)){
                sock_t fd = accept(g_listen, 0, 0);
                if (fd != SOCK_BAD){
                    int slot = -1;
                    for (int i=0;i<MAXCONN;i++) if (!g_conns[i].used){ slot = i; break; }
                    if (slot < 0){ sock_close(fd); }
                    else {
                        Conn *c = &g_conns[slot];
                        memset(c, 0, sizeof(*c));
                        c->fd = fd; c->used = 1;
                        struct timeval to; to.tv_sec = 3; to.tv_usec = 0;
                        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&to, sizeof(to));
                    }
                }
            }
            for (int i=0;i<MAXCONN;i++)
                if (g_conns[i].used && FD_ISSET(g_conns[i].fd, &rf)) conn_read(i);
        }
        drain_outq();
    }
}
void web_server_start(void){
    static int started = 0;
    if (started) return;
    started = 1;
#ifdef PLAT_WIN32
    { WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa); }
#endif
    // build the page to serve: the web build + the hosted flag, so a
    // browser that loads it joins this machine's game automatically
    {
        const char *inject = "<script>window.TSG_HOSTED=1;</script>";
        const char *body = 0;
        for (unsigned i=0; i + 7 < TSG_WEB_HTML_LEN; i++)
            if (memcmp(TSG_WEB_HTML + i, "</body>", 7) == 0){ body = (const char*)TSG_WEB_HTML + i; break; }
        if (body && TSG_WEB_HTML_LEN + strlen(inject) <= sizeof(g_page)){
            unsigned pre = (unsigned)(body - (const char*)TSG_WEB_HTML);
            memcpy(g_page, TSG_WEB_HTML, pre);
            memcpy(g_page + pre, inject, strlen(inject));
            memcpy(g_page + pre + strlen(inject), body, TSG_WEB_HTML_LEN - pre);
            g_page_len = (int)(TSG_WEB_HTML_LEN + strlen(inject));
        } else {
            memcpy(g_page, TSG_WEB_HTML, TSG_WEB_HTML_LEN);
            g_page_len = (int)TSG_WEB_HTML_LEN;
        }
    }
    // find this machine's LAN address (UDP connect trick: no traffic)
    {
        sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
        int got = 0;
        if (s != SOCK_BAD){
            struct sockaddr_in dst; memset(&dst, 0, sizeof(dst));
            dst.sin_family = AF_INET; dst.sin_port = htons(53);
            dst.sin_addr.s_addr = inet_addr("8.8.8.8");
            if (connect(s, (struct sockaddr*)&dst, sizeof(dst)) == 0){
                struct sockaddr_in me; socklen_t ml = (socklen_t)sizeof(me);
                if (getsockname(s, (struct sockaddr*)&me, &ml) == 0 && me.sin_addr.s_addr){
                    snprintf(g_join_addr, sizeof(g_join_addr), "%s:%d", inet_ntoa(me.sin_addr), WEB_PORT);
                    got = 1;
                }
            }
            sock_close(s);
        }
        if (!got) snprintf(g_join_addr, sizeof(g_join_addr), "127.0.0.1:%d", WEB_PORT);
    }
    g_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen == SOCK_BAD){ g_join_addr[0]=0; return; }
    { int one = 1; setsockopt(g_listen, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(one)); }
    struct sockaddr_in a; memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET; a.sin_addr.s_addr = INADDR_ANY; a.sin_port = htons(WEB_PORT);
    if (bind(g_listen, (struct sockaddr*)&a, sizeof(a)) != 0 || listen(g_listen, 8) != 0){
        sock_close(g_listen); g_listen = SOCK_BAD; g_join_addr[0]=0; return;
    }
    start_server_thread();
}
const char *plat_join_addr(void){ return g_join_addr; }

// --- plat_net_* on top of the queues ---
static int g_net_mode = 0;
int plat_net_start(int mode, const char *addr){
    (void)addr;
    if (mode == 1){ g_net_mode = 1; return 0; }        // hosting (server already runs)
    if (mode == 0){ g_net_mode = 0; return 0; }
    return -1;                                         // native builds don't join as clients
}
void plat_net_stop(void){ g_net_mode = 0; }
int plat_net_send(const u8 *d, int len, int dest){
    if (g_net_mode != 1) return -1;
    return q_push(g_outq, &g_outq_h, &g_outq_t, dest, (const unsigned char*)d, len, 0)? 0 : -1;
}
int plat_net_recv(u8 *buf, int cap, int *src){
    WPkt p;
    if (!q_pop(g_inq, &g_inq_h, &g_inq_t, &p)) return 0;
    int n = p.len < cap? p.len : cap;
    memcpy(buf, p.data, (size_t)n);
    *src = p.src;
    return n;
}
#endif // native embedded web server
