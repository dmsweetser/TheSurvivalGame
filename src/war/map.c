// THE SURVIVAL GAME — war/map.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 2002-2198 in the Nov 2026 fine-grained refactor; code unchanged.

static int war_drag_pan(int pi){
    BwSim *S = war_bs(pi);   // feat 13: under the guest swap this is the battle, wearing the guest's view
    int found=-1;
    for (int i=0;i<6;i++){
        if (!IN.tp[i].active) continue;
        if (S->war_drag_id>=0 && IN.tp[i].id != S->war_drag_id) continue;   // one drag pointer at a time
        if (IN.tp[i].zone!=0 && IN.tp[i].zone!=8) continue;                 // field touches only — buttons never pan
        found=i; break;
    }
    if (found<0){
        if (S->war_wdrag==1){   // v1.33: the wall drag just ended — a run of 2+ stands for approval; a one-segment drag builds at once, as a tap always did
            float xs[2], ys[2];
            int n = war_wall_run(S, xs, ys, 2);
            if (n>=2){ S->war_wdrag=2; sfx(6); }
            else { S->war_wdrag=0; IN.tapped=0; war_try_place(pi, S->war_wdx1, S->war_wdy1); }   // the tap is eaten: this release WAS the placement
        }
        S->war_drag_id=-1; return 0;
    }
    int vx, vy, vw2, vh2; pi_view_rect(pi, &vx, &vy, &vw2, &vh2);
    if (IN.tp[found].y0 >= vy+vh2-WAR_BAR_H){ S->war_drag_id=-1; return 0; }   // drags off the bar don't pan
    if (S->war_place==WAR_S_WALL){
        // v1.33 (Section G): with FIREWALL armed, a field drag draws
        // the wall RUN instead of panning the camera — and it owns
        // the field (return 1) so the key/stick pan stays out of it
        if (S->war_curon || S->war_barcap){ S->war_curon=0; S->war_barcap=0; S->war_barhl=-1; S->war_barhl_row=0; }
        if (S->war_wdrag!=1){
            S->war_wdrag=1; S->war_drag_id = IN.tp[found].id;
            S->war_wdx0 = S->war_camx + (float)(IN.tp[found].x0 - vx - vw2/2);
            S->war_wdy0 = S->war_camy + WAR_BAR_H*0.5f + (float)(IN.tp[found].y0 - vy - vh2/2);
        }
        S->war_wdx1 = S->war_camx + (float)(IN.tp[found].x - vx - vw2/2);
        S->war_wdy1 = S->war_camy + WAR_BAR_H*0.5f + (float)(IN.tp[found].y - vy - vh2/2);
        return 1;
    }
    // v1.25: a drag is the finger/mouse taking the field back (last
    // device wins, the same rule a tap follows) — it retires the pad
    // cursor and any bar capture, so the pad's centred camera (see
    // war_pad_update) never fights a drag for the view.
    if (S->war_curon || S->war_barcap){ S->war_curon=0; S->war_barcap=0; S->war_barhl=-1; S->war_barhl_row=0; }
    if (S->war_drag_id != IN.tp[found].id){
        S->war_drag_id = IN.tp[found].id;
        S->war_drag_cx = S->war_camx; S->war_drag_cy = S->war_camy;
    }
    S->war_camx = S->war_drag_cx - (float)(IN.tp[found].x - IN.tp[found].x0);
    S->war_camy = S->war_drag_cy - (float)(IN.tp[found].y - IN.tp[found].y0);
    war_clamp_cam(S);
    return 1;
}
static void war_node_pos(int vx, int vy, int vw2, int vh2, int r, int *nx, int *ny){
    float cx = (float)(vx + vw2/2), cy = (float)(vy + vh2/2 + 14);
    float a = -PI/2 + (float)r * (PI/4);
    *nx = (int)(cx + fcos(a) * (float)vw2 * 0.36f);
    *ny = (int)(cy + fsin(a) * (float)vh2 * 0.295f);
}
// v1.18: the ONE door into a battle from the map. A region with a
// battle parked in it (SAVE & QUIT) RESUMES that battle — before
// this helper existed, both confirm paths called war_start_battle
// unconditionally, so the natural player move (walk to the map, tap
// the region you were fighting) silently started a fresh fight and
// the autosave then overwrote the parked one in the save file.
static void war_map_launch(int pi, int r){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (war_snap_on[pi] && war_snap_reg[pi]==r){
        war_snap_restore(pi);   // sets war_state=1 and posts the RESTORED note itself
        war_snap_clear(pi);     // consumed — the live BwSim is authoritative now
        war_link_pull(pi);      // feat 13: a resumed battle pulls a waiting commander in
        return;
    }
    if (r <= b->war_prog){ war_start_battle(pi, r); }
    else { sfx(16); war_note(S, "THAT REGION IS STILL WILD. CONQUER THE ONES BEFORE IT!"); }
}
// v1.19: the SKIRMISH button on the world map's bottom strip —
// one rect shared by the tap handler and the renderer.
static void war_skirmish_btn(int vx, int vy, int vw2, int vh2, int *bx, int *by){
    (void)vy;
    *bx = vx+vw2-140; *by = vy+vh2-68;
}
// v1.25 fix: open the skirmish setup panel — one body for every
// door in ('S', the button tap, and the map selection's 9th stop).
static void war_open_setup(BwSim *S, u32 keys){
    if (S->war_size<1 || S->war_size>5) S->war_size = 2;   // v1.33: unset (or pre-ladder) picks land on the standard field
    if (S->war_diff<1 || S->war_diff>3) S->war_diff = 2;
    if (S->war_clanpick<1 || S->war_clanpick>9) S->war_clanpick = 1;
    S->war_state = 4; S->war_setup_sel = 0; S->war_setup_prevk = (int)keys; sfx(6);
}
// v1.29: the RESUME / NEW SKIRMISH panel (war_state 0, raised by
// war_enter when a skirmish is parked) — the Mordor entry panel's
// pattern on WAR!'s own idiom: one rect set shared by the tap
// handler and the renderer (below, in war_render_map), keyboard
// edges to walk, a two-step confirm on NEW SKIRMISH, and an entry
// pick lock so the cabinet-entering press cannot pick RESUME
// itself. Esc dismisses the panel to the bare map — the saved
// skirmish stays parked.
static void war_sk_panel_rects(int vx, int vy, int vw2, int vh2, int *px, int *py, int *pw, int *ph){
    *pw = vw2<420? vw2-24 : 396; *ph = 168;
    *px = vx+(vw2-*pw)/2; *py = vy+(vh2-*ph)/2 - 10;
}
static void war_sk_panel_pick(int pi, int n){
    BwSim *S = &BWS[pi];
    if (n==0){   // RESUME SKIRMISH
        S->war_sk_panel = 0;
        war_snap_sk_restore(pi);
        war_snap_sk_clear(pi);   // consumed — the live BwSim is authoritative now; later saves re-serialize from it
        war_link_pull(pi);   // feat 13: a resumed battle pulls a waiting commander in
    } else {   // NEW SKIRMISH — the first pick arms, the second discards the saved battle and frees the map
        if (S->war_sk_confirm>0){
            war_snap_sk_clear(pi);
            S->war_sk_panel = 0; S->war_sk_confirm = 0; S->war_grace = 0.3f;
            war_note(S, "THE SAVED SKIRMISH IS DISCARDED.");
            sfx(6);
        } else { S->war_sk_confirm = 2.5f; S->war_sk_lock = 0.3f; sfx(6); }   // the lock: a held select must not arm AND confirm in one press
    }
}
static void war_sk_panel_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi];
    if (S->war_sk_confirm>0) S->war_sk_confirm -= dt;
    if (S->war_sk_lock>0) S->war_sk_lock -= dt;
    if (in->exit_){ S->war_sk_panel = 0; S->war_sk_confirm = 0; S->war_grace = 0.3f; sfx(6); return; }
    if (in->up){ S->war_sk_psel = (S->war_sk_psel+1)%2; S->war_sk_confirm = 0; sfx(6); }
    if (in->down){ S->war_sk_psel = (S->war_sk_psel+1)%2; S->war_sk_confirm = 0; sfx(6); }
    if (S->war_sk_lock>0) return;   // picks wait out the lock; nav above still answers
    if (in->sel || in->interact){ war_sk_panel_pick(pi, S->war_sk_psel); return; }
    if (pi==G.my_id){
        if (IN.tapped){
            IN.tapped = 0;
            int vx, vy, vw2, vh2; p1_view_rect(&vx, &vy, &vw2, &vh2);
            int px, py, pw, ph; war_sk_panel_rects(vx, vy, vw2, vh2, &px, &py, &pw, &ph);
            for (int n=0;n<2;n++){
                int ry = py+58+n*38;
                if (IN.tap_x>=px+16 && IN.tap_x<px+pw-16 && IN.tap_y>=ry && IN.tap_y<ry+32){
                    S->war_sk_psel = n;
                    war_sk_panel_pick(pi, n);
                    return;
                }
            }
        }
        if (IN.pad_edges & (1u<<ACT_FIRE)){ war_sk_panel_pick(pi, S->war_sk_psel); return; }
    }
}
static void war_map_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi];
    if (S->war_sk_panel){ war_sk_panel_update(pi, dt, in); return; }   // v1.29: the entry panel is modal over the map
    if (in->exit_){ bw_exit(pi, 0); return; }
    if (S->war_grace > 0) return;
    if (pi==G.my_id){   // v1.19: S opens the skirmish setup panel
        for (int t=0;t<IN.typed_n;t++) if (IN.typed[t]=='s' || IN.typed[t]=='S'){
            war_open_setup(S, in->keys);
            return;
        }
    }
    // v1.25 fix: the selection walks NINE stops — the eight regions
    // plus the SKIRMISH button as the last stop. Before this, the
    // pad had no door into skirmish at all: the panel opened only
    // on a typed 'S' or a tap, the pad cursor never runs on the
    // map, and A/B here are the region launch — so a pad player
    // could invade every region and never once skirmish. Now the
    // stick/dpad/arrows land on the button like any other stop
    // and A confirms it, the same verb as everywhere else.
    if ((in->keys&K_LEFT) || in->up) { S->war_map_sel = (S->war_map_sel+8)%9; sfx(6); }
    if ((in->keys&K_RIGHT) || in->down) { S->war_map_sel = (S->war_map_sel+1)%9; sfx(6); }
    if (in->sel || in->interact){
        if (S->war_map_sel==8){ war_open_setup(S, in->keys); return; }
        war_map_launch(pi, S->war_map_sel);
        return;
    }
    if (pi==G.my_id && IN.tapped){
        IN.tapped = 0;
        int vx, vy, vw2, vh2; p1_view_rect(&vx, &vy, &vw2, &vh2);
        { int bx, by; war_skirmish_btn(vx, vy, vw2, vh2, &bx, &by);
          if (IN.tap_x>=bx && IN.tap_x<bx+128 && IN.tap_y>=by && IN.tap_y<by+28){
              war_open_setup(S, in->keys);
              return;
          } }
        for (int r=0;r<8;r++){
            int nx, ny; war_node_pos(vx, vy, vw2, vh2, r, &nx, &ny);
            int dx=IN.tap_x-nx, dy=IN.tap_y-ny;
            if (dx*dx+dy*dy < 34*34){
                S->war_map_sel = r;
                war_map_launch(pi, r);
                return;
            }
        }
    }
}
// v1.19: the SKIRMISH SETUP panel (war_state 4) — the map shows
// through, dimmed, and the rows do everything: MAP SIZE, ENEMY,
// LAUNCH, BACK. Same idiom as the pause panel: one rect set
// shared by the tap handler and the renderer, keyboard edges for
// the arrows, the pad's bumpers walk a value.
// v1.33: five rows — CLANS (1-9) joined, MAP SIZE is labelled by
// its DIMENSIONS off the five-size ladder, and a footer line
// quotes the win's XP for the picks in hand.
static void war_render_map(int pi, int vx, int vy, int vw2, int vh2);   // defined below; the setup panel draws it as its backdrop
static void war_setup_rects(int vx, int vy, int vw2, int vh2, int *px, int *py, int *pw, int *ph){
    *pw = vw2<420? vw2-24 : 396; *ph = 282;   // v1.33: five rows now (CLANS joined), plus the XP line
    *px = vx+(vw2-*pw)/2; *py = vy+(vh2-*ph)/2 - 10;
}
static void war_setup_pick(int pi, int n){
    BwSim *S = &BWS[pi];
    if (n==0){ S->war_size = S->war_size%5 + 1; sfx(6); }   // v1.33: the ladder runs 1..5
    else if (n==1){ S->war_diff = S->war_diff%3 + 1; sfx(6); }
    else if (n==2){ S->war_clanpick = S->war_clanpick%9 + 1; sfx(6); }   // v1.33: CLANS 1..9
    else if (n==3){ war_start_skirmish(pi); }
    else { S->war_state = 0; S->war_grace = 0.3f; sfx(6); }
}
// one value step on a setup row (dir -1/+1), shared by the key
// edges, the pad bumpers and the tap thirds — v1.33: row 2 (CLANS)
// steps here too
static void war_setup_step(BwSim *S, int row, int dir){
    if (row==0) S->war_size = ((S->war_size-1 + dir + 5)%5) + 1;
    else if (row==1) S->war_diff = ((S->war_diff-1 + dir + 3)%3) + 1;
    else if (row==2) S->war_clanpick = ((S->war_clanpick-1 + dir + 9)%9) + 1;
}
static void war_setup_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi];
    (void)dt;
    if (in->exit_){ S->war_state = 0; S->war_grace = 0.3f; sfx(6); return; }
    if (S->war_grace > 0) return;
    if (in->up) { S->war_setup_sel = (S->war_setup_sel+4)%5; sfx(6); }
    if (in->down) { S->war_setup_sel = (S->war_setup_sel+1)%5; sfx(6); }
    u32 kd = in->keys & ~(u32)S->war_setup_prevk;   // arrow EDGES: BwIn's keys are levels, so a held arrow must not spin the value
    if (kd & K_LEFT){ if (S->war_setup_sel<3){ war_setup_step(S, S->war_setup_sel, -1); sfx(6); } }
    if (kd & K_RIGHT){ if (S->war_setup_sel<3){ war_setup_step(S, S->war_setup_sel, 1); sfx(6); } }
    S->war_setup_prevk = (int)in->keys;
    // v1.25 fix: the pad's B backs out of this panel, exactly as
    // its own hint line promises — but the same ACT_INTERACT edge
    // also arrives as in->interact, and the row pick below spent
    // it first, so B CONFIRMED the row (on LAUNCH, the opposite
    // of backing out). Read the pad edge here, before the pick;
    // the keyboard's interact key still picks, Esc still backs
    // out — only the pad's B changes, to what the hint says.
    if (pi==G.my_id && (IN.pad_edges & (1u<<ACT_INTERACT))){ S->war_state = 0; S->war_grace = 0.3f; sfx(6); return; }
    if (in->sel || in->interact){ war_setup_pick(pi, S->war_setup_sel); return; }
    if (pi==G.my_id){
        if (IN.tapped){
            IN.tapped = 0;
            int vx, vy, vw2, vh2; p1_view_rect(&vx, &vy, &vw2, &vh2);
            int px, py, pw, ph; war_setup_rects(vx, vy, vw2, vh2, &px, &py, &pw, &ph);
            for (int n=0;n<5;n++){
                int ry = py+58+n*38;
                if (IN.tap_x>=px+16 && IN.tap_x<px+pw-16 && IN.tap_y>=ry && IN.tap_y<ry+32){
                    S->war_setup_sel = n;
                    if (n<3){   // a value row: the left third steps back, the rest steps forward
                        war_setup_step(S, n, IN.tap_x < px+pw/3? -1 : 1);
                        sfx(6);
                    } else war_setup_pick(pi, n);
                    return;
                }
            }
        }
        u32 pe = IN.pad_edges;
        if (pe & ((1u<<ACT_WPREV)|(1u<<ACT_WNEXT))){
            int d = (pe & (1u<<ACT_WNEXT))? 1 : -1;
            if (S->war_setup_sel<3){ war_setup_step(S, S->war_setup_sel, d); sfx(6); }
        }
        if (pe & (1u<<ACT_FIRE)){ war_setup_pick(pi, S->war_setup_sel); return; }
        // (ACT_INTERACT is handled above, before the row pick —
        // here it would be dead code: in->interact fires first.)
    }
}
static void war_render_setup(int pi, int vx, int vy, int vw2, int vh2){
    BwSim *S = &BWS[pi];
    war_render_map(pi, vx, vy, vw2, vh2);
    blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 150);
    int px, py, pw, ph; war_setup_rects(vx, vy, vw2, vh2, &px, &py, &pw, &ph);
    rect_fill(px, py, pw, ph, COL(16,18,28)); rect_line(px, py, pw, ph, COL(255,170,90));
    draw_text_center(px+pw/2, py+14, "SKIRMISH SETUP", 2, COL(255,210,150));
    static const char *DF[4] = { "", "EASY", "NORMAL", "HARD" };
    int sz = (S->war_size>=1 && S->war_size<=5)? S->war_size : 2;
    int cp = (S->war_clanpick>=1 && S->war_clanpick<=9)? S->war_clanpick : 1;
    for (int n=0;n<5;n++){
        int ry = py+58+n*38;
        int sel = (S->war_setup_sel==n);
        rect_fill(px+16, ry, pw-32, 32, sel? COL(56,64,88) : COL(38,44,60));
        rect_line(px+16, ry, pw-32, 32, sel? COL(255,220,120) : COL(198,164,84));
        char buf[64]; char *q=buf;
        if (n==0){ q=sappend(q, "SIZE  < "); q=sapp_int(q, WAR_SIZE_AW[sz]); q=sappend(q, " X "); q=sapp_int(q, WAR_SIZE_AH[sz]); q=sappend(q, " >"); }   // v1.33: labelled by dimensions (Daniel). Pass #4: the row is SIZE, not MAP SIZE — at scale 2 the full string (25 chars) overflowed the row box and spilled past the panel; the dimensions are the label that matters
        else if (n==1){ q=sappend(q, "ENEMY  < "); q=sappend(q, DF[S->war_diff]); q=sappend(q, " >"); }
        else if (n==2){ q=sappend(q, "CLANS  < "); q=sapp_int(q, cp); q=sappend(q, " >"); }
        else if (n==3){ q=sappend(q, "LAUNCH"); }
        else { q=sappend(q, "BACK"); }
        *q=0;
        // v1.33 cosmetic fix (pass #4): the MAP SIZE row's string
        // ("MAP SIZE  < 4800 X 3600 >") is wider than the row box at
        // scale 2 — its ends spilled past the panel edges. Fit-draw
        // every row: strings that fit render exactly as before, the
        // wide one shrinks a step instead of overflowing.
        draw_text_center_fit(px+pw/2, ry+10, pw-56, buf, 2, COL(240,244,250));
    }
    { char buf[64]; char *q=buf; q=sappend(q, "WIN PAYS "); q=sapp_int(q, war_sk_xp(S, cp)); q=sappend(q, " BUDDY XP"); *q=0;   // v1.33: the award for the picks in hand, quoted before you commit
      draw_text_center(px+pw/2, py+58+5*38+2, buf, 1, COL(255,205,40)); }
    draw_text_center(px+pw/2, py+ph-16, hint_dev()==DEV_PAD? "STICK + A - B BACKS OUT" : (hint_dev()==DEV_TOUCH? "TAP A ROW" : "ARROWS + SPACE - ESC BACKS OUT"), 1, COL(150,160,185));
}
