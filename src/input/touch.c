// THE SURVIVAL GAME — input/touch.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 624-705 in the Nov 2026 fine-grained refactor; code unchanged.

// ---- touch controls: the shell forwards every finger to the core.
// ONE scheme since v1.33 (Daniel: the drag scheme is gone entirely):
// the virtual joystick — a floating stick bottom-left, FIRE / USE
// (interact) / WPN (cycle weapon) / WALL buttons bottom-right —
// while taps still drive the menus, panels and chips. (The old
// scheme 0, drag-to-steer-and-fire, was removed whole: its steering
// leg, its input folds and its Settings-era title toggle. The
// touch_ctl field survives only as a save-token fossil, pinned to
// 1 at load; nothing branches on it any more.)
#define TP_DPAD_CX 66
#define TP_DPAD_CY 288
#define TP_FIRE_CX (VW-58)
#define TP_FIRE_CY 278
#define TP_ACT_CX  (VW-126)
#define TP_ACT_CY  318
#define TP_WPN_CX  (VW-130)
#define TP_WPN_CY  232
#define TP_WALL_CX (VW-58)
#define TP_WALL_CY 198
// zones: 5 = FIRE, 6 = USE, 7 = WPN, 8 = joystick, 9 = WALL
static int tp_zone_at(int x, int y){
    // the FIRE disc is a claimable button everywhere (v1.33: with
    // the drag scheme gone there is no steering finger for it to
    // fight — the v1.31 Tank-only carve-out is moot)
    int dx=x-TP_FIRE_CX, dy=y-TP_FIRE_CY; if (dx*dx+dy*dy<38*38) return 5;
    dx=x-TP_ACT_CX; dy=y-TP_ACT_CY; if (dx*dx+dy*dy<26*26) return 6;
    dx=x-TP_WPN_CX; dy=y-TP_WPN_CY; if (dx*dx+dy*dy<22*22) return 7;
    dx=x-TP_WALL_CX; dy=y-TP_WALL_CY; if (dx*dx+dy*dy<26*26) return 9;
    dx=x-TP_DPAD_CX; dy=y-TP_DPAD_CY;
    if (dx*dx+dy*dy<60*60){
        if (dx>=-12 && dx<=12 && dy>=-12 && dy<=12) return 0;
        if (dx*dx>dy*dy) return dx>0? 4:3;
        return dy>0? 2:1;
    }
    return 0;
}
// the one touch entry point, in the core since v1.31 (audit P5):
// the web shell's touch_point export forwards here, and the native
// backends feed their mouse through it too — a click is a tap and a
// drag is a drag on every build, exactly as the web shell's mouse
// path has always behaved. (Before v1.31 this body lived in the
// wasm platform piece, where no native build could reach it.)
static void core_touch_point(int id, int x, int y, int phase){
    IN.touch_seen=1; IN.touch_last=IN.dev_t+0.001f;
    if (phase==0){   // finger down: claim a slot, note d-pad zone/buttons
        for (int i=0;i<6;i++) if (!IN.tp[i].active){
            IN.tp[i].active=1; IN.tp[i].id=id;
            IN.tp[i].x=IN.tp[i].x0=x; IN.tp[i].y=IN.tp[i].y0=y;
            IN.tp[i].age=0; IN.tp[i].moved=0; IN.tp[i].zone=0;
            if (G.state==GS_PLAY){
                // WAR! (bw_mode 8) draws no main-game touch buttons,
                // so its screens must not claim their zones: the
                // SKIRMISH button sits under the FIRE/USE zones and
                // its taps were eaten whole (v1.27, Daniel). Zone 0
                // here means the touch stays a plain tap/drag, which
                // is all WAR! ever reads.
                int z = (G.my_id>=0 && G.bud[G.my_id].in_bw && (G.bud[G.my_id].bw_mode==8 || G.bud[G.my_id].bw_mode==10))? 0 : tp_zone_at(x, y);   // v1.35: CRISPY DIRT (mode 10) reads raw points + taps for its own gauges/FIRE, exactly like WAR! — the deck zones must not eat its presses
                if (z==9){ IN.tp[i].zone=9; IN.tp_wall_edge=1; }        // WALL button
                else if (z==7){ IN.tp[i].zone=7; IN.tp_wpn_edge=1; }   // WPN button
                else if (z==5 || z==6){                  // a button (FIRE / USE)
                    IN.tp[i].zone = z;
                    if (z==6) IN.tp_act_edge=1;
                } else {                            // anywhere else: the joystick
                    IN.tp[i].zone = 8; IN.tp[i].ax = x; IN.tp[i].ay = y;
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
// the main game's touch pause chip (v1.31, audit P6/H2): Banzai World
// has had its START chip since v1.26; the meadow/mall never did, so a
// touch player's only pause was the web shell's own ESC button. Same
// chip, drawn by render_hud under the zone pill (the status card owns
// the top-left here; the spot is clear of the weapon chips' strip,
// the TIME line and the home arrow's orbit), live exactly where the
// pause menu itself can open: effectively solo — offline, or hosting
// with no guests yet (a client never pauses; a host with guests
// connected can't either, the GS_PAUSE case bounces straight back).
#define MG_CHIP_W 72
#define MG_CHIP_H 24
// v1.32 (Daniel: the v1.31 spot "is awful"): the chip first lived at
// (244,92) — open playfield under the zone pill, right on top of the
// player's own hat stack and the home arrow. It now sits in the top-
// right corner under the leaderboard, Banzai-chip style (a corner,
// not the middle of the fight): the chip is only live solo or for a
// host alone, when the leaderboard is always its one-row form
// (x VW-158..VW-8, y 8..59), so y 67 tucks under it with an 8px gap
// and the chip's right edge is flush with the panel's and the margin.
// Clear of everything else that draws: the status card (x<=232), the
// zone pill (x<=468, y<=51), the TIME line (y 92, x from 14 — even a
// seven-digit squash count ends before x 250), the weapon-chip strip
// (left edge), and the touch buttons (topmost is WALL, y>=174).
#define MG_CHIP_X (VW-8-MG_CHIP_W)
#define MG_CHIP_Y 67
static int mg_pause_chip_live(void){
    if (!mobile_ui() || pad_mode()) return 0;
    if (G.state!=GS_PLAY || G.my_id<0) return 0;
    { int vw2 = G.view_w>0? G.view_w : VW; if (vw2<VW) return 0; }   // split view: full-screen furniture only (the BW chip's rule)
    if (G.bud[G.my_id].in_bw) return 0;              // Banzai World has its own chip
    if (G.net_mode==NET_CLIENT) return 0;
    if (G.net_mode==NET_HOST){
        for (int i=0;i<MAXP;i++) if (G.p[i].connected && !G.p[i].is_local) return 0;
    }
    return 1;
}
static void disc_fill(int cx, int cy, int r, u32 c){
    for (int dy=-r; dy<=r; dy++){ int dx=(int)fsqrt((float)(r*r-dy*dy)); rect_fill(cx-dx, cy+dy, dx*2, 1, c); }
}
static int chip_at(Player *me, int tx, int ty);   // weapon chips (defined with the HUD)
static void touch_update(float dt){
    // weapon chips: a tap on the strip equips that weapon directly
    if (G.state==GS_PLAY && IN.tapped){
        Player *me=&G.p[G.my_id];
        int cw = chip_at(me, IN.tap_x, IN.tap_y);
        if (cw>=0){ IN.weapon_key = cw+1; IN.tapped=0; }
        // the BEAN readout in the status card is a button too
        else if (me->beans>0 && IN.tap_x>=138 && IN.tap_x<232 && IN.tap_y>=64 && IN.tap_y<80){ IN.bean_key = 1; IN.tapped=0; }
    }
    IN.touch_keys=0; IN.tp_held=0; IN.joy_on=0;
    if (IN.tp_act_edge){ IN.interact_pressed=1; IN.tp_act_edge=0; }
    if (IN.tp_wpn_edge){ IN.weapon_cycle=1; IN.tp_wpn_edge=0; }
    if (IN.tp_wall_edge){ IN.wall_pressed=1; IN.tp_wall_edge=0; }
    IN.touch_down=0;   // v1.33: no steering finger exists any more — the field stays for the legacy set_touch export and the Source Room's drag-scroll reads raw points itself now
    for (int i=0;i<6;i++){
        if (!IN.tp[i].active) continue;
        IN.tp[i].age += dt;
        if (G.state==GS_PLAY && IN.tp[i].zone==5){
            IN.tp_held |= 32; IN.touch_keys |= K_FIRE;
        } else if (G.state==GS_PLAY && IN.tp[i].zone==8){
            // floating joystick (Minecraft-style): the base sits where the
            // thumb landed and slides along when the thumb drags past the
            // rim, so the knob is always at full reach under the finger.
            float dx = (float)(IN.tp[i].x - IN.tp[i].ax), dy = (float)(IN.tp[i].y - IN.tp[i].ay);
            float len = fsqrt(dx*dx + dy*dy);
            if (len > 48.0f){
                float s = (len - 48.0f) / len;
                IN.tp[i].ax += (int)(dx * s); IN.tp[i].ay += (int)(dy * s);
                dx = (float)(IN.tp[i].x - IN.tp[i].ax); dy = (float)(IN.tp[i].y - IN.tp[i].ay);
            }
            IN.joy_on=1; IN.joy_ax=IN.tp[i].ax; IN.joy_ay=IN.tp[i].ay;
            IN.joy_kx=IN.tp[i].ax+(int)dx; IN.joy_ky=IN.tp[i].ay+(int)dy;
            if (dx>9) IN.touch_keys|=K_RIGHT; else if (dx<-9) IN.touch_keys|=K_LEFT;
            if (dy>9) IN.touch_keys|=K_DOWN; else if (dy<-9) IN.touch_keys|=K_UP;
        }
    }
}
// raw-key bookkeeping for derive_input (defined later); the settings
// screen's capture mode reads g_raw_edges directly
static u64 prev_raw=0, g_raw_edges=0; static int prev_sem=0;
static float menu_rep_t=0; static int menu_rep_dir=0;
static int raw_dn(int pk){ return pk>=0 && pk<64? (int)((IN.raw>>pk)&1) : 0; }
static int raw_edge(int pk){ return pk>=0 && pk<64? (int)((g_raw_edges>>pk)&1) : 0; }
static void move_key_str(char *b);   // forward: defined with the help screen

// palettes
