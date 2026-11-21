// THE SURVIVAL GAME — input/touch.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 624-705 in the Nov 2026 fine-grained refactor; code unchanged.

// ---- touch controls: the shell forwards every finger to the core.
// Scheme 0 (drag): the first finger steers + fires; a quick, still touch
// is a tap (menus, store, entering the store). Scheme 1 (d-pad): during
// play, on-screen controls — a d-pad bottom-left, FIRE / USE (interact)
// / WPN (cycle weapon) bottom-right — while taps still drive the menus.
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
static void disc_fill(int cx, int cy, int r, u32 c){
    for (int dy=-r; dy<=r; dy++){ int dx=(int)fsqrt((float)(r*r-dy*dy)); rect_fill(cx-dx, cy+dy, dx*2, 1, c); }
}
static int chip_at(Player *me, int tx, int ty);   // weapon chips (defined with the HUD)
static void touch_update(float dt){
    // weapon chips: a tap on the strip equips that weapon directly —
    // the only way to switch on a touchscreen in drag mode
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
    IN.touch_down=0;
    for (int i=0;i<6;i++){
        if (!IN.tp[i].active) continue;
        IN.tp[i].age += dt;
        if (G.touch_ctl==0){
            if (!IN.touch_down && IN.tp[i].zone!=9 && IN.tp[i].zone!=7){ IN.touch_x=IN.tp[i].x; IN.touch_y=IN.tp[i].y; IN.touch_down=1; }
        } else if (G.state==GS_PLAY && IN.tp[i].zone==5){
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
