// THE SURVIVAL GAME — war/pad.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 2199-2497 in the Nov 2026 fine-grained refactor; code unchanged.

// --- WAR! on a gamepad (v1.16, Daniel) --------------------------------
// The pad never had a tap channel, so every WAR! verb — select,
// order, build, the command bar — was touch/mouse only; a pad could
// pan and nothing more. The fix is a cursor that IS a finger: the
// left stick flies it, A presses it through war_handle_tap itself
// (so the double/triple-tap muster ladder falls out of the existing
// streak code), B peels one layer back, LB/RB walk the command bar.
// Last device wins, as everywhere in this game: a real tap retires
// the cursor, and any pad input summons it again. Since v1.25 feat
// 13 this path serves BOTH local commanders: it is parameterized by
// the acting player's pad sources (war_pad_src), and P2's old
// centre-tap is gone — the second pad has a real cursor now.
static float war_axis(float a){   // deadzone 0.18, rescaled past it so a gentle push is a gentle fly
    float m = a<0? -a : a;
    if (m<=0.18f) return 0;
    float r = (m-0.18f)/0.82f; if (r>1.0f) r=1.0f;
    return a<0? -r : r;
}
static void war_pad_update(int pi, const float *ax, u32 pe, u32 held, float dt){
    BwSim *S = war_bs(pi);   // feat 13: the acting commander's battle (under the guest swap, wearing their view)
    int vx, vy, vw2, vh2; pi_view_rect(pi, &vx, &vy, &vw2, &vh2);
    float lx = war_axis(ax[0]), ly = war_axis(ax[1]);
    float rx = war_axis(ax[2]), ry = war_axis(ax[3]);
    int stick = (ax[0]>0.18f || ax[0]<-0.18f || ax[1]>0.18f || ax[1]<-0.18f
              || ax[2]>0.18f || ax[2]<-0.18f || ax[3]>0.18f || ax[3]<-0.18f);
    u32 btns = (1u<<ACT_FIRE)|(1u<<ACT_INTERACT)|(1u<<ACT_WPREV)|(1u<<ACT_WNEXT);
    // v1.25 fix (Daniel, Nov 19): the D-pad is a pad steering method
    // too — the game's own hints name it ("STICK/DPAD") — but it only
    // ever fed IN.keys (the camera pan), never the cursor: a D-pad
    // player could wake the cursor and then not aim it anywhere.
    // D-pad edges and holds now wake and fly it like the stick does.
    u32 dirm = (1u<<ACT_UP)|(1u<<ACT_DOWN)|(1u<<ACT_LEFT)|(1u<<ACT_RIGHT);
    int dirwake = (pe & dirm) || (held & dirm);
    if (!S->war_curon){
        if (!war_pad_live(pi) || !(stick || dirwake || (pe & btns))) return;
        S->war_curon = 1;
        // spawn on the first selected unit, else at the camera's centre
        int si=-1;
        for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0 && S->war_usel[i]){ si=i; break; }
        if (si>=0){ S->war_curx = S->war_ux[si]; S->war_cury = S->war_uy[si]; }
        else { S->war_curx = S->war_camx; S->war_cury = S->war_camy; }   // v1.22 fix: wake AT the camera centre — the point dbg_warcam and P2's centre-tap already treat as centre. The old +WAR_BAR_H/2 spawn sat 23u below it, a hair outside a small building's tap disc (Zapper 22u), so a wake-press A missed until the stick was nudged
        if (!stick && !dirwake) return;   // a button wake spends the press on waking; it does not also tap
        pe = 0;               // a stick/D-pad wake still flies this frame, but its buttons wait
    }
    // the bar highlight belongs to one structure's selection; any
    // change of selection — from any device — retires it
    if (S->war_barhl_s != S->war_selstruct){ S->war_barhl = -1; S->war_barhl_s = S->war_selstruct; }
    // v1.20: command-bar CAPTURE (Daniel's design — the cursor is
    // NOT flown down to the bar; the bar takes the pad over). A
    // building selected through the pad's own A-tap (the entry is at
    // the A press below) captures: the highlight lands on the first
    // option of the current row, the field cursor freezes and hides
    // (war_render), the camera holds still, and the stick walks the
    // menu — moving it no longer retires the highlight. Capture is
    // validated every frame: it lives only while its building does.
    if (S->war_barcap && (S->war_selstruct<0 || !war_struct_alive(S, S->war_selstruct))){
        S->war_barcap = 0; S->war_barhl = -1; S->war_barhl_row = 0;
    }
    if (S->war_barcap){
        int acts[5], costs[5]; const char *names[5];
        int ca[5], cc[5]; const char *cn[5];
        // stick edges: a fresh deflection steps once — holding the
        // stick does not spin the highlight
        int navx = (ax[0]>0.18f)? 1 : (ax[0]<-0.18f)? -1 : 0;
        int navy = (ax[1]>0.18f)? 1 : (ax[1]<-0.18f)? -1 : 0;
        int ex = (navx!=0 && navx!=S->war_capnx)? navx : 0;
        int ey = (navy!=0 && navy!=S->war_capny)? navy : 0;
        S->war_capnx = navx; S->war_capny = navy;
        if ((pe & (1u<<ACT_WNEXT)) && !ex) ex = 1;   // the bumpers walk the same row
        if ((pe & (1u<<ACT_WPREV)) && !ex) ex = -1;
        // v1.25 fix: D-pad edges walk the captured menu too (a D-pad
        // player could not navigate it at all). The axis guards keep
        // the shell's stick-to-digital fold from double-stepping: a
        // stick push past 0.45 raises an ACT edge the same frame as
        // its analog edge, so D-pad steps count only while the stick
        // itself is centred on that axis.
        if ((pe & (1u<<ACT_RIGHT)) && !ex && ax[0]<=0.18f) ex = 1;
        if ((pe & (1u<<ACT_LEFT)) && !ex && ax[0]>=-0.18f) ex = -1;
        if ((pe & (1u<<ACT_DOWN)) && !ey && ax[1]<=0.18f) ey = 1;
        if ((pe & (1u<<ACT_UP)) && !ey && ax[1]>=-0.18f) ey = -1;
        if (ex){
            int n = S->war_barhl_row==1? war_bar_cats(S, ca, cc, cn) : war_bar_list(S, acts, costs, names);
            if (n>0){ S->war_barhl = S->war_barhl<0? (ex>0? 0 : n-1) : (S->war_barhl + ex + n) % n; sfx(6); }
        }
        if (ey && S->war_bcat>=0){   // two rows are up: up/down hops between the options row and the category row
            if (ey>0 && S->war_barhl_row==0){
                int cn2 = war_bar_cats(S, ca, cc, cn);
                S->war_barhl_row = 1; S->war_barhl = 0;
                for (int i=0;i<cn2;i++) if (ca[i]==20+S->war_bcat){ S->war_barhl = i; break; }   // land on the open category
                sfx(6);
            } else if (ey<0 && S->war_barhl_row==1){
                S->war_barhl_row = 0; S->war_barhl = 0; sfx(6);
            }
        }
        // A works the menu: on the category row it switches the
        // category and hands back its options row; on the options
        // row it activates through war_bar_activate — a TRAIN option
        // queues and KEEPS capture (queue several), while arming a
        // placement hands the field cursor back for the placement
        if ((pe & (1u<<ACT_FIRE)) && S->war_grace<=0 && S->war_barhl>=0){
            if (S->war_barhl_row==1){
                int cn2 = war_bar_cats(S, ca, cc, cn);
                if (S->war_barhl<cn2){ S->war_bcat = ca[S->war_barhl]-20; S->war_barhl_row = 0; S->war_barhl = 0; sfx(6); }
            } else {
                int n = war_bar_list(S, acts, costs, names);
                if (S->war_barhl<n){
                    int act = acts[S->war_barhl];
                    war_bar_activate(pi, S->war_barhl);
                    if (S->war_place>=0){ S->war_barcap = 0; S->war_barhl = -1; S->war_barhl_row = 0; }
                    else if (act>=20 && act<=22) S->war_barhl = 0;   // a category opened from the top level: its options row starts at the first option
                }
            }
        }
        // B peels, captured-style: an open category on a
        // multi-category building closes back to the top-level row,
        // still captured; anything else exits capture AND clears the
        // selection — Daniel's "cancel to get out of the selection"
        if (pe & (1u<<ACT_INTERACT)){
            S->war_demoarm = -1;   // v1.21: B disarms a pending DEMOLISH
            int cn2 = war_bar_cats(S, ca, cc, cn);
            if (S->war_bcat>=0 && cn2>1){
                int old = S->war_bcat;
                S->war_bcat = -1; S->war_barhl_row = 0; S->war_barhl = 0;
                for (int i=0;i<cn2;i++) if (ca[i]==20+old){ S->war_barhl = i; break; }
                sfx(6);
            } else {
                S->war_barcap = 0; S->war_barhl = -1; S->war_barhl_row = 0;
                for (int i=0;i<WAR_MAXU;i++) S->war_usel[i]=0;
                S->war_selstruct = -1; S->war_bcat = -1; sfx(6);
            }
        }
        return;   // captured: no cursor flight, no pan, no edge-follow this frame
    }
    // the sticks fly the cursor (and retire the bar highlight — A
    // goes back to pressing the field). The D-pad flies it too, at
    // full deflection, on any axis the analog stick leaves centred
    // (v1.25 fix: the D-pad only ever panned the camera before, so
    // it could not aim the cursor at all). The RIGHT stick flies it
    // as well — its old job, panning the camera while the cursor
    // kept its screen spot, is gone under the centred model below:
    // both sticks move the cursor, the camera follows the cursor.
    float fx = lx, fy = ly;
    if (fx==0) fx = ((held>>ACT_RIGHT)&1)? 1.0f : ((held>>ACT_LEFT)&1)? -1.0f : 0;
    if (fy==0) fy = ((held>>ACT_DOWN)&1)? 1.0f : ((held>>ACT_UP)&1)? -1.0f : 0;
    fx += rx; fy += ry;
    { float fm = fsqrt(fx*fx + fy*fy); if (fm>1.0f){ fx/=fm; fy/=fm; } }
    if (fx || fy){
        S->war_curx += fx * 540.0f * dt;
        S->war_cury += fy * 540.0f * dt;
        S->war_barhl = -1;
    }
    // v1.25 (Daniel, Nov 19): THE CURSOR IS THE CAMERA. The old model
    // flew the cursor inside a clamped camera — and war_update's key
    // pan, which the pad's own stick feeds through IN.keys, shoved
    // the camera at 430 u/s while the cursor flew at 540, so the
    // difference piled up as a screen offset; the edge-follow then
    // pinned the cursor 28u inside the edge, the camera clamp at the
    // map's rim stranded it there, and once it was offset it stayed
    // offset in every direction. Now the pad cursor sits at the
    // centre of the field view, always: the cursor clamps to the
    // arena, the camera centres on it exactly, and at the rim the
    // view simply scrolls off the map — war_render already draws the
    // ground beyond the fence darker, so the off-map space reads as
    // compensated void, not as a broken clamp. war_clamp_cam still
    // governs every other camera driver (keys, drags, restores);
    // while the pad cursor is on, this re-centring runs after them
    // every frame, so no other pan can strand the cursor either.
    if (S->war_curx < WAR_OX) S->war_curx = WAR_OX;
    if (S->war_curx > WAR_OX+S->war_aw) S->war_curx = WAR_OX+S->war_aw;
    if (S->war_cury < WAR_OY) S->war_cury = WAR_OY;
    if (S->war_cury > WAR_OY+S->war_ah) S->war_cury = WAR_OY+S->war_ah;
    S->war_camx = S->war_curx; S->war_camy = S->war_cury;
    // LB / RB walk the selected structure's command bar
    if (S->war_selstruct>=0 && (pe & ((1u<<ACT_WPREV)|(1u<<ACT_WNEXT)))){
        int acts[5], costs[5]; const char *names[5];
        int n = war_bar_list(S, acts, costs, names);
        if (n>0){
            int dir = (pe & (1u<<ACT_WNEXT))? 1 : -1;
            S->war_barhl = S->war_barhl<0? (dir>0? 0 : n-1) : (S->war_barhl + dir + n) % n;
            sfx(6);
        }
    }
    // B peels one layer: a pending placement, then an open
    // category (v1.17), then the bar highlight, then the selection
    // itself. A single-category building's menu reopens itself, so
    // there B skips the category step and deselects as it always did.
    if (pe & (1u<<ACT_INTERACT)){
        S->war_demoarm = -1;   // v1.21: B disarms a pending DEMOLISH
        int ca[5], cc[5]; const char *cn[5];
        if (S->war_place>=0){ S->war_place = -1; S->war_bcat = -1; sfx(6); }
        else if (S->war_bcat>=0 && war_bar_cats(S, ca, cc, cn)>1){ S->war_bcat = -1; S->war_barhl = -1; sfx(6); }
        else if (S->war_barhl>=0){ S->war_barhl = -1; }
        else { for (int i=0;i<WAR_MAXU;i++) S->war_usel[i]=0; S->war_selstruct = -1; S->war_bcat = -1; sfx(6); }
    }
    // A: the finger comes down — on the lit bar button if one is
    // lit, else on the field at the cursor, through the very same
    // door a real tap uses (same grace gate, same streak ladder)
    if ((pe & (1u<<ACT_FIRE)) && S->war_grace<=0){
        if (S->war_barhl>=0){ int hl = S->war_barhl; S->war_barhl = -1; war_bar_activate(pi, hl); }
        else {
            int csx = vx + vw2/2 + (int)(S->war_curx - S->war_camx);
            int csy = vy + vh2/2 + (int)(S->war_cury - S->war_camy - WAR_BAR_H*0.5f);
            int ssb = S->war_selstruct;
            war_handle_tap(pi, csx, csy);
            // v1.20: a building that became selected through THIS
            // pad tap captures the command bar (the capture block
            // above) — if it has a menu at all. Selections of units,
            // deselects and rally taps never capture.
            if (S->war_selstruct>=0 && S->war_selstruct!=ssb){
                int ta[5], tc[5]; const char *tn[5];
                if (war_bar_list(S, ta, tc, tn)>0){
                    S->war_barcap = 1; S->war_barhl = 0; S->war_barhl_row = 0;
                    S->war_barhl_s = S->war_selstruct;
                    // v1.25 fix (Daniel, Nov 19): seed the captured
                    // stick-edge state from the stick's CURRENT sign.
                    // It used to reset to 0, so a player who pressed A
                    // while still pushing the stick toward the building
                    // — the natural way to select mid-flight — had the
                    // still-held stick read as a FRESH deflection on
                    // the next frame, and the highlight stepped once
                    // for free: capture opened on the wrong button.
                    S->war_capnx = (ax[0]>0.18f)? 1 : (ax[0]<-0.18f)? -1 : 0;
                    S->war_capny = (ax[1]>0.18f)? 1 : (ax[1]<-0.18f)? -1 : 0;
                }
            }
        }
    }
}
// the pause menu's input, for whoever opened it (feat 13: the
// pauser drives the shared pause — owner or guest — with their own
// devices; the other commander's inputs wait it out)
static void war_pause_input(int pi, float dt){
    if (pi==G.my_id){
        if (IN.tapped){ IN.tapped=0; war_pause_tap(pi, IN.tap_x, IN.tap_y); }
        if (IN.weapon_key>=1 && IN.weapon_key<=3){ int k=IN.weapon_key; IN.weapon_key=0; war_pause_pick(pi, k-1); }
    }
    { const float *ax; u32 pe, held; war_pad_src(pi, &ax, &pe, &held); war_pad_pause(pi, ax, pe, dt); }
}
// the battle's command-input section, extracted verbatim from
// war_update (feat 13): the owner runs it in place, exactly where
// it always ran; a guest co-commander runs it under the view swap.
static void war_command_input(int pi, float dt, const BwIn *in){
    BwSim *S = war_bs(pi);
    int dragging = (pi==G.my_id)? war_drag_pan(pi) : 0;
    if (!dragging){   // a field drag owns the camera; keys/stick pan only when no drag does
        int kx=0, ky=0;
        if (in->keys&K_LEFT) kx-=1; if (in->keys&K_RIGHT) kx+=1;
        if (in->keys&K_UP) ky-=1; if (in->keys&K_DOWN) ky+=1;
        if (kx || ky){ S->war_camx += (float)kx*430.0f*dt; S->war_camy += (float)ky*430.0f*dt; war_clamp_cam(S); }
    }
    if (pi==G.my_id && IN.tapped){
        S->war_curon=0;   // a real tap: the finger/mouse takes the field back (last device wins)
        if (S->war_barcap){ S->war_barcap=0; S->war_barhl=-1; S->war_barhl_row=0; }   // v1.20: ...and it ends a bar capture the same way
        if (S->war_grace<=0){ IN.tapped=0; war_handle_tap(pi, IN.tap_x, IN.tap_y); }
    }
    // feat 13: the old P2 centre-tap (in->interact taps at the
    // camera centre) is GONE — pad 2 has a real cursor now, and B
    // on that pad is the pad path's peel verb; a centre-tap on the
    // same edge would fight it. P2's WAR! verbs are P1's verbs.
    if (pi==G.my_id && IN.weapon_key>=1 && IN.weapon_key<=5){ war_bar_activate(pi, IN.weapon_key-1); IN.weapon_key=0; }
    { const float *ax; u32 pe, held; war_pad_src(pi, &ax, &pe, &held); war_pad_update(pi, ax, pe, held, dt); }
}
// a guest co-commander's frame (feat 13): the battle NEVER advances
// here — it validates the link, mirrors the state for rendering,
// and runs only the command layer, under the view swap.
static void war_guest_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    int owner = S->war_guest-1;
    BwSim *HS = &BWS[owner];
    int valid = owner==war_other_local(pi) && HS->war_duo==pi
        && G.bud[owner].active && G.bud[owner].in_bw && G.bud[owner].bw_mode==8
        && (HS->war_state==1 || HS->war_state==2 || HS->war_state==3);
    if (!valid){
        // the command is over (battle decided, commander saved &
        // quit, or the link broke from the other side): back to
        // your own map, with a fresh field's view defaults
        S->war_guest = 0;
        if (HS->war_duo==pi) HS->war_duo = -1;
        war_view_fresh(S, S);
        S->war_state = 0; S->war_map_sel = b->war_prog<8? b->war_prog : 7; S->war_grace = 0.3f;
        return;
    }
    S->war_state = HS->war_state;   // mirror, for the renderer and the state readers
    if (HS->war_state==2 || HS->war_state==3) return;   // the end screen runs on the owner's clock; the guest watches it
    if (S->war_ordt > 0) S->war_ordt -= dt;
    if (HS->war_pause){
        if (HS->war_pause_by==pi){
            if (in->exit_){ HS->war_pause=0; HS->war_grace=0.3f; sfx(6); return; }
            war_view_exchange(HS, S);
            war_pause_input(pi, dt);
            war_view_exchange(HS, S);
        }
        return;
    }
    if (in->exit_){ HS->war_pause=1; HS->war_pause_by=pi; sfx(6); return; }
    war_view_exchange(HS, S);
    war_command_input(pi, dt, in);
    war_view_exchange(HS, S);
}
