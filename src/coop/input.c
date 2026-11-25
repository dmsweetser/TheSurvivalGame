// THE SURVIVAL GAME — coop/input.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 2588-2707 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Frame — input resolution, state machine, camera. Platform entry point.
// ---------------------------------------------------------------------------
static void resolve_local_input(void){
    Player *me=&G.p[G.my_id];
    if (!me) return;
    me->cur_keys = IN.keys | IN.touch_keys;   // keyboard/pad bits + d-pad bits
    // facing follows the movement keys; standing still keeps the last facing.
    // (On a client this also aims our sprite between host snapshots.)
    float mx=0, my=0;
    if (IN.keys & K_UP) my-=1;
    if (IN.keys & K_DOWN) my+=1;
    if (IN.keys & K_LEFT) mx-=1;
    if (IN.keys & K_RIGHT) mx+=1;
    if (mx!=0 || my!=0) me->aim = fatan2(my, mx);
    client_aim = me->aim;
    // v1.33: the drag scheme's steering consumer lived here — hold/
    // drag to run toward the finger, firing as you go. It is GONE
    // with the scheme: on touch, the joystick's direction bits ride
    // IN.touch_keys into cur_keys above and the FIRE button is the
    // trigger, exactly like a gamepad. (Desktop mouse keeps its
    // taps; it never had a keyboard's job taken from it.)
    if (IN.weapon_key>=1 && IN.weapon_key<=5){
        int w=IN.weapon_key-1;
        if (me->owned[w]){ me->weapon=w; client_weapon=w; sfx(6); }
        else sfx(16);
    }
    if (IN.bean_key){ IN.bean_key=0; bean_use(G.my_id); }
    if (IN.weapon_cycle!=0){          // gamepad bumpers: next/prev owned weapon
        int w=me->weapon, tries=0;
        do { w=(w+IN.weapon_cycle+NWEAP)%NWEAP; tries++; } while (!me->owned[w] && tries<NWEAP);
        if (me->owned[w] && w!=me->weapon){ me->weapon=w; client_weapon=w; sfx(6); }
    }
    if (IN.wall_pressed){
        if (G.net_mode==NET_CLIENT) client_want_wall=1;
        else me->want_wall=1;
    }
}
// ---------------------------------------------------------------------------
// Local co-op: player 2 on the second controller, same machine, same run.
// P1 keeps the keyboard + first pad (resolve_local_input); P2 is fed
// from the pad-2 channel. The pair shares one camera while they stay
// close and splits the screen when they separate — and whenever either
// of them is in the store, so the shopper gets the Bug Bounty (shield
// and all) while the other keeps playing.
// ---------------------------------------------------------------------------
static float vcamx[2], vcamy[2], vzoom[2];
static int vsplit=0; static float vfit_t=0;
// the split is a place, not a switch: vsplit_f eases 0 (one shared
// view) toward 1 (two EVEN half views) — the seam sweeps in as the
// players separate and back out as they return, but it always comes
// to rest exactly in the middle: halves are never lopsided at rest
// (the kids were right — an uneven split feels broken). vorient
// picks the seam's direction from the direction of apartness
// (1 = side by side, 0 = stacked), vp1first remembers which side
// P1's view grew from.
static float vsplit_f=0; static int vorient=1; static int vp1first=1;
static int p2_rep_dir=0; static float p2_rep_t=0;
static int act2_edge(int a){ return (int)((IN.act2_edges>>a)&1); }
static int act2_down(int a){ return (int)((IN.act2_held>>a)&1); }
static void resolve_p2_input(void){
    Player *p=&G.p[G.p2_slot];
    u32 h=IN.act2_held; int k=0;
    if ((h>>ACT_UP)&1) k|=K_UP;
    if ((h>>ACT_DOWN)&1) k|=K_DOWN;
    if ((h>>ACT_LEFT)&1) k|=K_LEFT;
    if ((h>>ACT_RIGHT)&1) k|=K_RIGHT;
    if ((h>>ACT_FIRE)&1) k|=K_FIRE;
    if ((h>>ACT_SPRINT)&1) k|=K_SPRINT;
    p->cur_keys = k;
    float mx=0, my=0;
    if (k & K_UP) my-=1;
    if (k & K_DOWN) my+=1;
    if (k & K_LEFT) mx-=1;
    if (k & K_RIGHT) mx+=1;
    if (mx!=0 || my!=0) p->aim = fatan2(my, mx);
    int cyc = act2_edge(ACT_WNEXT)? 1 : act2_edge(ACT_WPREV)? -1 : 0;
    if (cyc){
        int w=p->weapon, tries=0;
        do { w=(w+cyc+NWEAP)%NWEAP; tries++; } while (!p->owned[w] && tries<NWEAP);
        if (p->owned[w] && w!=p->weapon){ p->weapon=w; sfx(6); }
    }
    if (act2_edge(ACT_WALL)) p->want_wall=1;
}
// P2's store cursor: edges plus the same hold-to-repeat feel as P1's menus
static void p2_store_nav(int *up, int *down, float dt){
    *up=0; *down=0;
    if (act2_edge(ACT_UP)){ *up=1; p2_rep_dir=-1; p2_rep_t=0.38f; }
    else if (act2_edge(ACT_DOWN)){ *down=1; p2_rep_dir=1; p2_rep_t=0.38f; }
    else if (p2_rep_dir!=0 && (act2_down(ACT_UP) || act2_down(ACT_DOWN))){
        p2_rep_t -= dt;
        if (p2_rep_t<=0){
            if (p2_rep_dir<0 && act2_down(ACT_UP)) *up=1;
            if (p2_rep_dir>0 && act2_down(ACT_DOWN)) *down=1;
            p2_rep_t=0.09f;
        }
    } else p2_rep_dir=0;
}
static float zoom_target_of(Player *p){
    if (p->in_mall) return 1.0f;
    float sn=(float)p->stack_n;
    float z = 1.0f - (sn*0.0016f>0.38f? 0.38f : sn*0.0016f);
    // as THREAT climbs the bugs swell without limit — the camera gives
    // ground in steps at set threat levels so the giants stay in frame
    float th = G.threat;
    z -= th>=8.0f? 0.16f : th>=5.0f? 0.12f : th>=3.0f? 0.08f : th>=2.0f? 0.04f : 0.0f;
    return z<0.45f? 0.45f : z;
}
// a simple strip of what you've got: one icon slot per weapon, in
// store order, gold frame on the one in your hands, Xn for stacks.
// P2's HUD is text-only otherwise — this is how they see their kit.
