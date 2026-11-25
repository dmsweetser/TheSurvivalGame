// THE SURVIVAL GAME — menus/menu.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 1818-1917 in the Nov 2026 fine-grained refactor; code unchanged.

static const char *menu_items(int *count){
    static char buf[8][24];
    int n=0;
    if (G.has_save){ scpy(buf[n++], "CONTINUE", 24); }
    scpy(buf[n++], "SINGLE PLAYER", 24);
    scpy(buf[n++], "HOST LAN GAME", 24);
    scpy(buf[n++], "JOIN LAN GAME", 24);
    scpy(buf[n++], "HOW TO PLAY", 24);
    *count=n;
    // return base pointer via a hack: caller uses menu_item(i)
    (void)buf;
    return 0;
}
// Every build has the same dead-simple menu — START and go. There is
// no LAN mode to pick: native builds are always hosts (friends join
// from a browser), and the web build is a guest when a native host
// served the page, single-player otherwise.
#define MENU_SIMPLE 1
#define MENU_FIELDS 1
#ifdef PLAT_WASM
#define START_MODE NET_OFF
#else
#define START_MODE NET_HOST
#endif
// the 2P CO-OP row exists only while two controllers are detected —
// Daniel's rule: the option appears automatically with the hardware.
static int coop_row(void){ return IN.pad_count>=2; }
// with gamepads in use the SETTINGS / CONTROLS row (keyboard bindings,
// touch scheme) leaves the menu — pad players never see it.
static int menu_base_count(void){ return (G.has_save? 7:6) - (menu_pad_mode()? 1:0); }
static const char *menu_item(int i){
    static char buf[24];
    static char dbuf[24];
    int idx=i;
    if (coop_row() && i==menu_base_count()) return G.coop_pref? "2P CO-OP: ON" : "2P CO-OP: OFF";
    if (G.has_save){ if (idx==0) return "CONTINUE"; idx--; }
    if (menu_pad_mode() && idx>=2) idx++;   // step over the hidden controls row
    if (MENU_SIMPLE){
        switch (idx){ case 0: return "START"; case 1: return "HOW TO PLAY";
                      case 2: return "SETTINGS";   // v1.33: this row was the touch-scheme toggle on mobile (CONTROLS: DRAG/STICK) — the scheme is gone, so mobile gets SETTINGS like everyone else (no row count changes anywhere)
                      case 3: { char *q=dbuf; q=sappend(q, "DIFFICULTY: "); q=sappend(q, DIFF_NAME[G.difficulty&3]); *q=0; return dbuf; }
                      case 4: return G.zen? "ZEN MODE: ON" : "ZEN MODE: OFF";
                      default: return G.eddie? "EDDIE MODE: ON" : "EDDIE MODE: OFF"; }
    }
    switch (idx){
        case 0: return "SINGLE PLAYER";
        case 1: return "HOST LAN GAME";
        case 2: return "JOIN LAN GAME";
        default: return "HOW TO PLAY";
    }
    (void)buf;
}
static int menu_count(void){
    if (MENU_SIMPLE) return menu_base_count() + (coop_row()? 1:0);
    return G.has_save? 5:4;
}
static void slots_activate(int i){
    if (i==3){ G.state=GS_MENU; return; }                       // BACK row
    int s=i+1;
    if (G.slots_mode==0){                                       // load
        if (!G.slot_has[s]){ store_note("THAT SLOT IS EMPTY"); return; }
        continue_run(s);
    } else {                                                    // new game
        if (G.slot_has[s] && G.slots_confirm!=s){ G.slots_confirm=s; return; }
        G.slots_confirm=0; G.slot=s; start_run(START_MODE);
    }
}
static void menu_activate(int sel){
    int idx=sel;
    if (coop_row() && sel==menu_base_count()){ G.coop_pref ^= 1; do_save(); sfx(6); return; }
    if (G.has_save){ if (idx==0){ open_slots(0); return; } idx--; }
    if (menu_pad_mode() && idx>=2) idx++;   // step over the hidden controls row
    if (MENU_SIMPLE){
        if (idx==0) open_slots(1);
        else if (idx==1){ G.prev_state=G.state; G.state=GS_HELP; }
        else if (idx==3){ G.difficulty = (G.difficulty+1)&3; do_save(); sfx(6); }   // difficulty for the next run
        else if (idx==4){ G.zen ^= 1; do_save(); sfx(6); }              // zen for the next run
        else if (idx==5){ G.eddie ^= 1; do_save(); sfx(6); }            // eddie: puffball bugs (cosmetic)
        else { G.set_sel=0; G.set_capture=0; G.state=GS_SETTINGS; }   // v1.33: row 2 is SETTINGS on mobile too now (it was the scheme toggle)
        return;
    }
    switch (idx){
        case 0: start_run(NET_OFF); break;
        case 1: start_run(NET_HOST); break;
        case 2:
            if (!G.join_ip[0]){ store_note("TYPE THE HOST IP FIRST"); G.menu_sel = menu_count()+1; }
            else start_run(NET_CLIENT);
            break;
        default: G.prev_state=G.state; G.state=GS_HELP; break;
    }
}
static void field_append(char *buf, int cap){
    int n=slen(buf);
    for (int i=0;i<IN.typed_n;i++){
        char c=IN.typed[i];
        if (n<cap-1){ buf[n++]=c; buf[n]=0; }
    }
    if (IN.backspace && n>0) buf[n-1]=0;
}
