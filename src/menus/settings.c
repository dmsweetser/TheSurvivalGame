// THE SURVIVAL GAME — menus/settings.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 2250-2388 in the Nov 2026 fine-grained refactor; code unchanged.

#define SET_ROWS 20   // 4 comfort + 14 actions + RESET DEFAULTS + BACK (v1.31, audit P9/H11: a tappable way out)
#define SET_BIND0 4   // first key-binding row
static int set_row_y(int i){
    // v1.31: rows sit 2px tighter so the BACK row fits above the
    // footer line without growing the panel
    if (i<SET_BIND0) return 50 + i*14;
    return 124 + (i-SET_BIND0)*12;
}
static void comfort_changed(int row){
    // feedback at the NEW level: a click for the SFX row itself, and
    // one as mute lifts, so the player hears the game come back
    if (row==1 || (row==2 && !G.mute_all)) sfx(1);
    do_save();
}
static void comfort_adjust(int row, int dir){   // LEFT/RIGHT: volumes clamp, toggles flip
    if (row==0){ G.vol_music+=dir; if (G.vol_music<0) G.vol_music=0; if (G.vol_music>10) G.vol_music=10; }
    else if (row==1){ G.vol_sfx+=dir; if (G.vol_sfx<0) G.vol_sfx=0; if (G.vol_sfx>10) G.vol_sfx=10; }
    else if (row==2){ G.mute_all ^= 1; }
    else { G.shake_on ^= 1; if (!G.shake_on) G.shake=0; }   // settle the screen the moment it flips off
    comfort_changed(row);
}
static void comfort_bump(int row){              // SELECT / re-tap: volumes cycle, toggles flip
    if (row==0){ G.vol_music = G.vol_music>=10? 0 : G.vol_music+1; }
    else if (row==1){ G.vol_sfx = G.vol_sfx>=10? 0 : G.vol_sfx+1; }
    else if (row==2){ G.mute_all ^= 1; }
    else { G.shake_on ^= 1; if (!G.shake_on) G.shake=0; }
    comfort_changed(row);
}
static void settings_reset(void){
    bind_defaults(G.bind);
    G.vol_music=10; G.vol_sfx=10; G.mute_all=0; G.shake_on=1;
    do_save();
}
static void settings_input(void){
    if (G.set_capture){
        // v1.30 (audit T1): capture read physical keys only — a pad-only
        // player who entered it (A on a binding row) had NO way out
        // short of a keyboard, and a touch player could wander in by
        // double-tapping a row. Pad B / Back and a tap now CANCEL the
        // capture without binding anything.
        if (((IN.pad_edges>>ACT_INTERACT)&1) || ((IN.pad_edges>>ACT_ESC)&1) || IN.tapped){ G.set_capture=0; return; }
        // the next physical key pressed becomes the binding (ESC cancels)
        if (g_raw_edges){
            if (raw_edge(PK_ESC)) G.set_capture=0;
            else for (int pk=0;pk<PK_COUNT;pk++) if ((g_raw_edges>>pk)&1){
                G.bind[G.set_sel-SET_BIND0]=pk; G.set_capture=0; do_save(); break;
            }
        }
        return;
    }
    if (IN.menu_up) G.set_sel=(G.set_sel+SET_ROWS-1)%SET_ROWS;
    if (IN.menu_down) G.set_sel=(G.set_sel+1)%SET_ROWS;
    if (G.set_sel<SET_BIND0){
        // adjust on the LOSSLESS edges (the same raw_edge stream the
        // capture below reads, plus the pad-edge snapshot): the
        // held-mask key_edges would merge fast repeat taps into one
        // step — the shell latches key releases ~120 ms, so a quick
        // second tap never drops the held bit in between.
        int ladj = raw_edge(PK_LEFT) || raw_edge(G.bind[ACT_LEFT]) || ((IN.pad_edges>>ACT_LEFT)&1);
        int radj = raw_edge(PK_RIGHT) || raw_edge(G.bind[ACT_RIGHT]) || ((IN.pad_edges>>ACT_RIGHT)&1);
        if (ladj) comfort_adjust(G.set_sel, -1);
        else if (radj) comfort_adjust(G.set_sel, +1);
    }
    if (IN.tapped){
        for (int i=0;i<SET_ROWS;i++){ int ry=set_row_y(i);
            if (IN.tap_y>=ry-3 && IN.tap_y<=ry+10){
                if (i==G.set_sel){
                    if (i<SET_BIND0) comfort_bump(i);
                    else if (i==SET_ROWS-1){ G.state=GS_MENU; }   // v1.31 (audit P9/H11): the BACK row — tap it (twice, like every row) to leave
                    else if (i==SET_ROWS-2) settings_reset();
                    else G.set_capture=1;
                } else G.set_sel=i;
                return;
            }
        }
    }
    if (IN.esc_pressed || IN.interact_pressed){ G.state=GS_MENU; return; }
    if (IN.select_pressed){
        if (G.set_sel<SET_BIND0) comfort_bump(G.set_sel);
        else if (G.set_sel==SET_ROWS-1){ G.state=GS_MENU; }   // v1.31: the BACK row, on select too
        else if (G.set_sel==SET_ROWS-2) settings_reset();
        else G.set_capture=1;
    }
}
static void render_settings(void){
    rect_fill(0,0,VW,VH,COL(24,22,38));
    ui_panel(104, 4, 432, 330);   // v1.24 panel chrome, grown to hold the comfort rows
    draw_text_center(VW/2, 10, "SETTINGS", 3, COL(255,214,64));
    { char hb[96]; char *q=hb;   // v1.30 (audit F14): the keyboard half names the real FIRE bind, not a hard-coded SPACE
      if (hint_dev()==DEV_PAD) q=sappend(q, "STICK/DPAD CHOOSE - LEFT/RIGHT ADJUSTS - A SELECTS");
      else if (hint_dev()==DEV_TOUCH) q=sappend(q, "TAP A ROW - TAP AGAIN TO CHANGE IT");
      else { q=sappend(q, "ARROWS CHOOSE - LEFT/RIGHT ADJUSTS - "); q=sappend(q, key_name(G.bind[ACT_FIRE])); q=sappend(q, " REBINDS"); }
      *q=0; draw_text_center(VW/2, 38, hb, 1, COL(160,230,255)); }
    { static const char *CN[4] = { "MUSIC VOLUME", "SOUND FX VOLUME", "MUTE ALL", "SCREEN SHAKE" };
      for (int i=0;i<SET_BIND0;i++){
        int ry=set_row_y(i);
        int sel = (i==G.set_sel);
        if (sel) rect_fill(120, ry-3, 400, 14, COL(255,205,40));
        u32 tc = sel? COL(30,26,44) : COL(235,238,245);
        draw_text(150, ry, CN[i], 1, tc);
        if (i<2){
            int v = i==0? G.vol_music : G.vol_sfx;
            for (int b=0;b<10;b++){
                u32 bc;
                if (b<v) bc = sel? COL(30,26,44) : (G.mute_all? COL(120,116,130) : COL(255,205,40));
                else bc = sel? COL(150,138,100) : COL(70,66,90);
                rect_fill(356+b*13, ry+1, 10, 7, bc);
            }
        } else {
            const char *val = i==2? (G.mute_all? "ON" : "OFF") : (G.shake_on? "ON" : "OFF");
            draw_text(490-text_w(val,1), ry, val, 1, tc);
        }
      } }
    draw_text_center(VW/2, 112, "KEY BINDINGS", 1, COL(255,214,64));
    for (int i=SET_BIND0;i<SET_ROWS;i++){
        int ry=set_row_y(i);
        int sel = (i==G.set_sel);
        if (sel) rect_fill(120, ry-2, 400, 13, COL(255,205,40));
        u32 tc = sel? COL(30,26,44) : COL(235,238,245);
        if (i<SET_ROWS-2){
            draw_text(150, ry, act_name(i-SET_BIND0), 1, tc);
            const char *kn = (sel && G.set_capture)? "PRESS A KEY..." : key_name(G.bind[i-SET_BIND0]);
            draw_text(490-text_w(kn,1), ry, kn, 1, tc);
        } else if (i==SET_ROWS-2) {
            draw_text(150, ry, "RESET DEFAULTS", 1, tc);
            draw_text(490-text_w("WASD + SPACE E F P SHIFT",1), ry, "WASD + SPACE E F P SHIFT", 1, tc);
        } else {
            draw_text(150, ry, "BACK", 1, tc);   // v1.31 (audit P9/H11): the exit row
            draw_text(490-text_w("TO THE MENU",1), ry, "TO THE MENU", 1, tc);
        }
    }
    if (G.set_capture)
        draw_text_center(VW/2, 344, hint_dev()==DEV_PAD? "PRESS ANY KEY FOR THE HIGHLIGHTED ACTION - B CANCELS" : (hint_dev()==DEV_TOUCH? "PRESS ANY KEY FOR THE HIGHLIGHTED ACTION - TAP CANCELS" : "PRESS ANY KEY FOR THE HIGHLIGHTED ACTION - ESC CANCELS"), 1, COL(255,240,150));
    else if (pad_mode()){
        // v1.31 (audit P4/F20a): the summary names the WHOLE pad —
        // Y (bean), LB/RB (weapons), LT (sprint) and Back were missing
        draw_text_center(VW/2, 330, "GAMEPAD: STICK/DPAD MOVE - A FIRE - B INTERACT - X WALL - Y BEAN", 1, COL(140,150,170));
        draw_text_center(VW/2, 342, "LB / RB WEAPONS - LT SPRINT - BACK EXITS - START PAUSE", 1, COL(140,150,170));
    }
    else { char b[80]; char *q=b; q=sappend(q, key_name(G.bind[ACT_INTERACT]));
      q=sappend(q, " - BACK TO MENU (ARROW KEYS ALWAYS WORK FOR MOVING)");
      draw_text_center(VW/2, 344, b, 1, COL(140,150,170)); }
}

static void pause_save_quit(void){
    do_save();                       // GS_PAUSE counts as in-run: writes the slot
    if (G.net_mode==NET_HOST) plat_net_stop();
    G.net_mode=NET_OFF;
    for (int i=0;i<MAXP;i++){ G.p[i].connected=0; G.p[i].is_local=0; }
    G.my_id=0; G.menu_sel=0;
    G.state=GS_MENU;
    sfx(6);
}
// ---------------------------------------------------------------------------
// THE CONTROLS PANEL (Daniel, Nov 6): the bottom-of-screen control lines
// are gone — they overlapped everything. Controls now live in menus:
// the pause menu's CONTROLS row in the main game, and the Start-menu
// inside Banzai World. One panel, drawn per place-you-are-standing
// (ctx), and the buttons named follow the device in your hands.
// ctx: 0 meadow+mall, 1 story/free, 2 entrances, 3 run, 4 craft, 5 bowl
