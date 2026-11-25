// THE SURVIVAL GAME — banzai/input.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 1422-1846 in the Nov 2026 fine-grained refactor; code unchanged.

// --- Banzai World P1 input, unified across devices (Daniel, Nov 5) ----
// Daniel's rule: one touch scheme in every minigame — since v1.33
// that scheme is the virtual JOYSTICK alone (the drag scheme was
// removed whole at his word). Before this fill existed as one place,
// the touch FIRE button never produced a `sel` (no stick-throwing in
// Entrances, no swinging in Buddy Craft) and menu navigation ignored
// the stick entirely. Both P1 call sites (solo/co-op frame and the
// net-client frame) pack P1's BwIn through here, so every mode
// inherits the same deck: stick steers, FIRE selects/swings, USE
// interacts.
static int bw_tf_prev, bw_nav_dir; static float bw_nav_rep;
// The touch START chip (v1.26 Item 2, Daniel's ruling): on touch, the
// BW Start menu (RESUME/CONTROLS/LEAVE) could never be OPENED — its
// rows were already tappable, but only Start/P (keyboard) or a pad's
// Start could raise it, so CONTROLS was unreachable on a phone. The
// chip is the missing door: top-left of P1's full-screen view (the
// minimap owns the top-right), live only while no menu/battle/log is
// up. WAR! (mode 8) keeps its own pause — standing ruling, untouched.
static int bw_start_chip_live(void){
    BwSim *S = &BWS[G.my_id]; Buddy *b = &G.bud[G.my_id];
    if (!b->in_bw || S->bw_menu || S->bat.active || S->craft_menu || S->log_open) return 0;
    if (b->bw_mode==8) return 0;
    if (!mobile_ui() || pad_mode()) return 0;
    int vw2 = G.view_w>0? G.view_w : VW;
    if (vw2<VW) return 0;          // split view: full-screen furniture only
    return 1;
}
#define BW_CHIP_W 72
#define BW_CHIP_H 24
// v1.30 (audit C3): the chip is drawn OVER the top-left of the view, and
// several modes put their HUD text exactly there (Mordor's title read
// "N RUN" behind it). Mode HUDs shift their top-left text right by this
// much when the chip is live for the player whose view is being drawn.
static int bw_chip_dx(int pi){ return (pi==G.my_id && bw_start_chip_live())? BW_CHIP_W+6 : 0; }
static void bw_fill_p1(BwIn *bin, float dt){
    Buddy *b = &G.bud[G.my_id]; BwSim *S = &BWS[G.my_id];
    bin->keys = IN.keys | IN.touch_keys;
    int tf = (IN.touch_keys & K_FIRE)? 1:0;
    bin->sel = IN.select_pressed || (tf && !bw_tf_prev);
    // the K_UP fold belongs to the PLATFORMER alone (mode 3): there
    // Up is the jump button, and holding it must read as holding the
    // jump for the hold-to-jump-higher cut. Everywhere else Up is
    // steering, and steering must never pull the trigger — Tank
    // Rescue's cannon fired all the way up the field (Nov 10 fix),
    // and Mordor's swing answered Up instead of the FIRE button
    // (Nov 23, Daniel's playtest). Excluding modes one at a time
    // kept letting the next mode fall in; the fold now names the
    // one mode it exists for.
    bin->sel_held = ((IN.act_held>>ACT_FIRE)&1) || ((IN.raw>>G.bind[ACT_FIRE])&1) || ((bin->keys & K_UP) && b->bw_mode==3) || tf;
    bw_tf_prev = tf;
    bin->interact = IN.interact_pressed || IN.enter_pressed;
    bin->exit_ = IN.esc_pressed;
    bin->pause = (IN.pause_pressed && G.net_mode!=NET_CLIENT)? 1:0;   // Start/P opens the BW menu now (guests can't pause — standing rule)
    // a tap on the START chip is the same Start press, for touch
    if (IN.tapped && bw_start_chip_live()){
        int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2);
        if (IN.tap_x>=vx+8 && IN.tap_x<vx+8+BW_CHIP_W && IN.tap_y>=vy+8 && IN.tap_y<vy+8+BW_CHIP_H){
            if (G.net_mode!=NET_CLIENT) bin->pause = 1;
            IN.tapped = 0;
        }
    }
    bin->wall = IN.wall_pressed;
    bin->up = IN.menu_up; bin->down = IN.menu_down;
    // stick flicks walk BW menus (craft menu, story log, battles): a
    // fresh edge, then the same auto-repeat timing the arrow keys get
    int dir = (IN.touch_keys & K_UP)? -1 : (IN.touch_keys & K_DOWN)? 1 : 0;
    if (dir && dir!=bw_nav_dir){ if (dir<0) bin->up=1; else bin->down=1; bw_nav_rep = 0.38f; }
    else if (dir){ bw_nav_rep -= dt; if (bw_nav_rep<=0){ if (dir<0) bin->up=1; else bin->down=1; bw_nav_rep = 0.09f; } }
    bw_nav_dir = dir;
    // holding FIRE in the field-combat modes swings at the weapon's
    // own pace; the single edge above is for menus/throws
    if (tf && !S->bat.active && !S->craft_menu && !S->bw_menu
        && (b->bw_mode==2 || b->bw_mode==4 || b->bw_mode==9) && b->atk_cd<=0) bin->sel = 1;
    // v1.33: the DRAG legs that lived here are gone with the scheme —
    // drag steering toward the finger (and its platformer drag-up
    // jump), drag-hold auto-swing, and tap-as-interact. The joystick
    // steers through IN.touch_keys (bin->keys above), FIRE swings
    // (tf), and the USE button interacts (interact_pressed above) —
    // every verb the drag legs provided, on the one remaining deck.
}

// THE REBUILDING (feat 14): Duster's pathfinding. Greedy steering
// wedges in this meadow — the forest is per-seed, tree tiles form
// alcoves and slots at step scale, and every greedy variant (axis
// slides, diagonals, compass sidesteps) eventually 2-cycles inside
// one. So when the escort stalls, Duster paths on the TILE grid:
// a bounded breadth-first search (41x41 tiles around Duster, tile
// centres tested with the same point collision the buddy uses)
// toward whichever visited tile lies closest to the real target,
// and it follows the path's second tile centre as a waypoint
// until the greedy steering takes over again. Bounded loops only;
// the scratch is reused per call and each call finishes inside one
// player's update, so split-screen escorts never see each other.
#define CH2_BFS_W 41
static short ch2_bfs_par[CH2_BFS_W*CH2_BFS_W];
static short ch2_bfs_q[CH2_BFS_W*CH2_BFS_W];
static void bw_ch2_repath(Buddy *b, BwSim *S, float tx, float ty){
    int stx = bw_tileof(S->ch2_esc_x), sty = bw_tileof(S->ch2_esc_y);
    int ox = stx - CH2_BFS_W/2, oy = sty - CH2_BFS_W/2;
    int n = CH2_BFS_W*CH2_BFS_W;
    for (int i=0;i<n;i++) ch2_bfs_par[i] = -2;              // -2 unvisited, -3 blocked
    int qh = 0, qt = 0;
    int s0 = (sty-oy)*CH2_BFS_W + (stx-ox);
    ch2_bfs_par[s0] = -1; ch2_bfs_q[qt++] = (short)s0;      // the start tile counts as walkable: Duster is standing on it
    static const int D4[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    while (qh<qt){
        int cur = ch2_bfs_q[qh++];
        int cx = ox + cur%CH2_BFS_W, cy = oy + cur/CH2_BFS_W;
        for (int k=0;k<4;k++){
            int nx2 = cx + D4[k][0], ny2 = cy + D4[k][1];
            int lx = nx2-ox, ly = ny2-oy;
            if (lx<0 || ly<0 || lx>=CH2_BFS_W || ly>=CH2_BFS_W) continue;
            int ni = ly*CH2_BFS_W + lx;
            if (ch2_bfs_par[ni]!=-2) continue;
            float wx = nx2*BW_TILE + BW_TILE/2, wy = ny2*BW_TILE + BW_TILE/2;
            if (bw_solid_b(b, wx, wy)){ ch2_bfs_par[ni] = -3; continue; }
            ch2_bfs_par[ni] = (short)cur; ch2_bfs_q[qt++] = (short)ni;
        }
    }
    // the goal is not always reachable inside one window (and the
    // buddy may hug a tree, making their own tile's centre solid):
    // path to the visited tile closest to the target instead
    int best = -1; float bd = 1e18f;
    for (int i=0;i<n;i++){
        if (ch2_bfs_par[i]<-1) continue;
        int cx = ox + i%CH2_BFS_W, cy = oy + i/CH2_BFS_W;
        float wx = cx*BW_TILE + BW_TILE/2, wy = cy*BW_TILE + BW_TILE/2;
        float ddx = tx-wx, ddy = ty-wy, dd = ddx*ddx + ddy*ddy;
        if (dd<bd){ bd = dd; best = i; }
    }
    if (best<0 || best==s0){ S->ch2_wp_on = 0; return; }
    int hops = 0; for (int c = best; c!=s0 && c>=0; c = ch2_bfs_par[c]) hops++;
    int up = hops>2? hops-2 : 0, node = best;
    while (up-->0) node = ch2_bfs_par[node];
    S->ch2_wp_x = (ox + node%CH2_BFS_W)*BW_TILE + BW_TILE/2;
    S->ch2_wp_y = (oy + node/CH2_BFS_W)*BW_TILE + BW_TILE/2;
    S->ch2_wp_on = 1; S->ch2_stall = 0; S->ch2_base = 1e9f;
}

static void bw_update(int pi, float dt, const BwIn *in){
    g_dev_ctx = dev_of(pi);   // v1.25: notes composed in this sim name its owner's controls
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    // ADAPTIVE MUSIC (v1.25 feat 15): this world's danger, pushed to
    // the music piece — BwSim lives here, after music.c in the unity
    // build, so the flag flows downhill through mus_bw_note. A battle
    // running is danger; in Story Mode a hunter's alert is danger
    // before the battle starts. Free Play hackers never alert, so
    // there only a battle fills the Sheep in.
    { float dgr = S->bat.active? 1.0f : 0.0f;
      if (!dgr) for (int i=0;i<BW_ROAMN;i++) if (S->roam[i].alert>0){ dgr = 1.0f; break; }
      mus_bw_note(pi, dgr); }
    if (b->mv_t>0) b->mv_t-=dt;
    // the Start-menu (Daniel, Nov 6): Start/P used to dump you out of
    // the minigame; now it opens a menu — RESUME / CONTROLS / LEAVE —
    // and the CONTROLS panel shows THIS mode's buttons. While it is
    // open the world holds its breath. Escape still leaves instantly.
    if (S->bw_menu){
        if (S->bw_menu==2){   // the controls panel: anything backs out
            if (in->sel || in->interact || in->exit_ || in->pause) S->bw_menu=1;
            if (pi==G.my_id && IN.tapped){ S->bw_menu=1; IN.tapped=0; }
        } else {
            if (in->up){ S->bw_msel = (S->bw_msel+2)%3; sfx(6); }
            if (in->down){ S->bw_msel = (S->bw_msel+1)%3; sfx(6); }
            int act = (in->sel || in->interact)? S->bw_msel : -1;
            if (pi==G.my_id && IN.tapped){
                int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2);
                int pw=300, ph=140, px=vx+(vw2-pw)/2, py=vy+(vh2-ph)/2;
                int tx=IN.tap_x, ty=IN.tap_y; IN.tapped=0;
                act = -2;   // a tap outside the panel just closes it
                if (tx>=px && tx<px+pw && ty>=py && ty<py+ph)
                    for (int i=0;i<3;i++){ int ry=py+48+i*26;
                        if (ty>=ry-6 && ty<ry+17){ act=i; break; } }
            }
            if (act==0) S->bw_menu=0;
            else if (act==1) S->bw_menu=2;
            else if (act==2){ S->bw_menu=0; bw_exit(pi, 0); return; }
            else if (act==-2) S->bw_menu=0;
            else if (in->exit_ || in->pause) S->bw_menu=0;
        }
        return;
    }
    if (in->pause){ S->bw_menu=1; S->bw_msel=0; sfx(6); return; }
    if (!b->active){ bw_exit(pi, 0); return; }
    if (S->grace_t>0) S->grace_t-=dt;
    if (S->note_t>0) S->note_t-=dt;
    if (S->bat.active){ b->bw_x = b->x; b->bw_y = b->y; bw_battle_update(pi, dt, in); return; }
    if (S->sleep_t>0){
        S->sleep_t -= dt;
        if (!S->slept && S->sleep_t<=0.7f){
            S->slept=1; b->hp=b->maxhp; b->energy=BUD_ENERGY_MAX;
            if (b->items[0]<9) b->items[0]++;   // every camp keeps a spare Patch Notes
            scpy(S->note, "YOU SLEPT SOUNDLY. GOOD AS NEW.", 72); S->note_t=2.6f;
            do_save();
        }
        return;
    }
    if (!S->log_open && in->wall && !S->bat.active && b->bw_mode<2){ S->log_open = 1; S->log_sel=0; S->log_prevk = in->keys; sfx(6); return; }
    if (S->log_open){
        if (in->up){ S->log_sel = (S->log_sel+3)%4; sfx(6); }
        if (in->down){ S->log_sel = (S->log_sel+1)%4; sfx(6); }
        // the log's touch door (was v1.30 audit H8's DRAG block):
        // navigation rides in->up/down — stick flicks feed those on
        // touch exactly as the arrows do — and a tap uses the
        // selected item, the same tap every other panel answers.
        // (The old DRAG-only key-edge stepping is gone with the
        // scheme; it double-stepped beside in->up/down anyway.)
        if (pi==G.my_id && hint_dev()==DEV_TOUCH && IN.tapped) bw_use_item(pi, S->log_sel);
        S->log_prevk = in->keys;
        if (in->sel) bw_use_item(pi, S->log_sel);
        if (in->exit_ || in->wall) S->log_open = 0;
        return;
    }
    if (b->bw_mode==2){ ent_update(pi, dt, in); return; }
    if (b->bw_mode==3){ plat_update(pi, dt, in); return; }
    if (b->bw_mode==4){ craft_update(pi, dt, in); return; }
    if (b->bw_mode==5){ tank_update(pi, dt, in); return; }
    if (b->bw_mode==6){ cas_update(pi, dt, in); return; }
    if (b->bw_mode==7){ crt_update(pi, dt, in); return; }
    if (b->bw_mode==8){ war_update(pi, dt, in); return; }
    if (b->bw_mode==9){ mor_update(pi, dt, in); return; }
    if (b->bw_mode==10){ scr_update(pi, dt, in); return; }
    float mx=0, my=0; int k=in->keys;
    if (k&K_UP) my-=1; if (k&K_DOWN) my+=1; if (k&K_LEFT) mx-=1; if (k&K_RIGHT) mx+=1;
    if (mx!=0 || my!=0){
        float d=fsqrt(mx*mx+my*my); mx/=d; my/=d;
        b->aim = fatan2(my, mx);
        float sp = BUD_SPD[b->species%BUDN];
        float nx = b->x + mx*sp*dt, ny = b->y + my*sp*dt;
        if (!bw_solid_b(b, nx, b->y) && !bw_feat_solid(b, nx, b->y)) b->x = nx;
        if (!bw_solid_b(b, b->x, ny) && !bw_feat_solid(b, b->x, ny)) b->y = ny;
        b->walk_ph += dt*9;
        b->mv_t = 0.15f;
    }
    if (b->lunge_t>0) b->lunge_t-=dt;
    // NO passive energy regen in the story world (Daniel, Nov 6):
    // energy comes back at tents and from Energy Drinks, never by
    // just waiting around. (Entrances, Buddy Craft and the meadow
    // keep their own rules — this is the story/free roam only.)
    b->bw_x = b->x; b->bw_y = b->y;
    // roamers wander; one touching you is a battle
    // the two big residents are fought by walking up to them
    if (b->bw_mode==1 && !S->bat.active && S->sleep_t<=0 && !S->log_open){
        // (grace deliberately NOT checked here: it shields you from
        // being jumped by roamers, it must never refuse a boss you
        // deliberately walk up to — that refusal cost a test run)
        if (bw_puz_done(b) && !((b->story_bits>>16)&1)){
            float dx=b->x-BW_SITE_XY[1][0], dy=b->y-BW_SITE_XY[1][1];
            if (dx*dx+dy*dy < 27*27){ bw_boss_start(pi, 1); return; }
        }
        if (!((b->story_bits>>17)&1)){
            float dx=b->x-BW_SITE_XY[3][0], dy=b->y-BW_SITE_XY[3][1];
            if (dx*dx+dy*dy < 32*32){
                if (bw_qstate(b,1)==2 && bw_qstate(b,3)==0) bw_qset(b,3,1);
                bw_boss_start(pi, 2); return;
            }
        }
        if (!((b->story_bits>>22)&1)){
            float dx=b->x-BW_SITE_XY[4][0], dy=b->y-BW_SITE_XY[4][1];
            if (dx*dx+dy*dy < 30*30){ bw_boss_start(pi, 3); return; }
        }
        // THE REBUILDING (feat 14): the rematch waits at the farm's
        // centre aisle while quest 12 is active — gated on the quest,
        // never on story bit 16 (the original Raid is long down for
        // anyone who can reach this fight).
        if (bw_qstate3(b,12)==1){
            float dx=b->x-BW_SITE_XY[1][0], dy=b->y-BW_SITE_XY[1][1];
            if (dx*dx+dy*dy < 40*40){ bw_boss_start(pi, 4); return; }
        }
    }
    bw_roam_refresh(b);
    for (int i=0;i<BW_ROAMN;i++){
        BWHack *h=&S->roam[i];
        if (h->cell==0 && h->x==0 && h->y==0) continue;
        // Story Mode hunters (Nov 4, Daniel: "let bad guys actually
        // chase you if they see you where they are facing, or if you
        // get in proximity"): a hacker that spots the buddy — close
        // enough to hear (64), or inside its facing cone out to 180 —
        // gives chase at 88 u/s (every buddy outruns it, barely) and
        // remembers the hunt for 1.8 s after losing sight. Free Play
        // hackers just wander, as they always have.
        int hunting = 0;
        if (h->alert > 0) h->alert -= dt;   // the hunt is forgotten in every mode
        if (b->bw_mode==1){
            float dx=b->x-h->x, dy=b->y-h->y; float d2=dx*dx+dy*dy;
            int sees = 0;
            if (d2 < 64*64) sees = 1;
            else if (d2 < 180*180){
                float dl = fsqrt(d2);
                if (dl>0 && (fcos(h->dir)*dx + fsin(h->dir)*dy)/dl > 0.30f) sees = 1;
            }
            if (sees) h->alert = 1.8f;
            if (h->alert > 0){ h->dir = fatan2(dy, dx); hunting = 1; }
        }
        if (hunting){
            float hx = h->x + fcos(h->dir)*88*dt, hy = h->y + fsin(h->dir)*88*dt;
            if (!bw_solid_b(b, hx, hy)){ h->x=hx; h->y=hy; }
            else { float hx2 = h->x + fcos(h->dir)*88*dt; if (!bw_solid_b(b, hx2, h->y)) h->x=hx2;
                   else { float hy2 = h->y + fsin(h->dir)*88*dt; if (!bw_solid_b(b, h->x, hy2)) h->y=hy2; } }
        } else {
            h->wt -= dt;
            if (h->wt<=0){ h->wt = 1.6f + rng_f()*1.6f; h->dir = rng_f()*6.28318f; }
            float hx = h->x + fcos(h->dir)*26*dt, hy = h->y + fsin(h->dir)*26*dt;
            if (!bw_solid_b(b, hx, hy)){ h->x=hx; h->y=hy; }
        }
        float dx=b->x-h->x, dy=b->y-h->y;
        if (S->grace_t<=0 && dx*dx+dy*dy < 21*21){ bw_battle_start(b, h); return; }
    }
    // story: arriving somewhere new is worth a title card of a note
    if (b->bw_mode==1){
        int st = bw_site_at_xy(b, b->x, b->y);
        if (st!=S->site_cur){
            S->site_cur = st;
            if (st>=0){
                static const char *ARRIVE[5] = {
                    "PATCH VILLAGE. SOMEONE HERE KEEPS THE LIGHTS ON.",
                    "THE RUINED SERVER FARM. IT HUMS IN ITS SLEEP.",
                    "THE FROZEN LAKE OF DEPRECATED CODE.",
                    "ZED'S COMPOUND. THE FENCE IS A FIREWALL. LITERALLY.",
                    "THE OVERGROWN RUINS. SOMETHING SITS ON THE PAST." };
                scpy(S->note, ARRIVE[st], 72); S->note_t = 3.2f;
            }
        }
    }
    // THE REBUILDING (v1.25 feat 14): the chapter's three clocks —
    // the cider run, Duster's heels, and Patch Day. Story mode only,
    // and battles never reach this code (bw_update returns inside
    // the battle), so a hacker ambush pauses every clock for free.
    if (b->bw_mode==1){
        if (S->ch2_fest_t>0) S->ch2_fest_t -= dt;
        if (S->ch2_race_on && bw_qstate3(b,10)==1){
            if (bw_site_at_xy(b, b->x, b->y)==2){
                bw_qset3(b,10,2); b->xp += 40; S->ch2_race_on = 0; S->ch2_race_t = 0;
                scpy(S->note, "CIDER SAVED! STILL COLD. PATCHINA WILL BE GLAD. +40 XP.", 72); S->note_t=4.5f;
            } else {
                S->ch2_race_t -= dt;
                if (S->ch2_race_t<=0){ S->ch2_race_t = 0; S->ch2_race_on = 0;
                    scpy(S->note, "THE CIDER SANK. PATCHINA BREWS MORE — START AGAIN AT HER DOOR.", 72); S->note_t=4.2f; }
            }
        }
        if (bw_qstate3(b,11)==1 && S->ch2_esc_on){
            // Duster's heels: hurry at 320 when far behind, keep the
            // buddy's own pace when close, wait at heel distance. One
            // waypoint is taught: while Duster is still west of the
            // farm ridge and the buddy is through at the farm, the
            // farm pass mouth is its target — the pass the help signs
            // teach — until it is through, then the buddy again.
            float tx = b->x, ty = b->y; int to_pass = 0;
            if (S->ch2_esc_x < 1750 && bw_site_at_xy(b, b->x, b->y)==1){ tx = 1762; ty = -1900; to_pass = 1; }
            // pathing: a reached waypoint hands steering back to the
            // greedy; a stall (no real progress for half a second)
            // asks the tile grid for a new one
            if (S->ch2_wp_on){
                float wx = S->ch2_wp_x - S->ch2_esc_x, wy = S->ch2_wp_y - S->ch2_esc_y;
                if (wx*wx + wy*wy < 16*16){ S->ch2_wp_on = 0; S->ch2_stall = 0; S->ch2_base = 1e9f; }
            }
            float etx = S->ch2_wp_on? S->ch2_wp_x : tx, ety = S->ch2_wp_on? S->ch2_wp_y : ty;
            float dx = etx - S->ch2_esc_x, dy = ety - S->ch2_esc_y;
            float d = fsqrt(dx*dx + dy*dy);
            float db = bw_duster_dist(b);
            if (d > 44){
                if (d < S->ch2_base - 2){ S->ch2_base = d; S->ch2_stall = 0; }
                else S->ch2_stall += dt;
                S->ch2_path_t -= dt;
                if (S->ch2_stall > 0.45f && S->ch2_path_t <= 0){
                    bw_ch2_repath(b, S, tx, ty);
                    S->ch2_path_t = 0.6f;
                    etx = S->ch2_wp_on? S->ch2_wp_x : tx; ety = S->ch2_wp_on? S->ch2_wp_y : ty;
                    dx = etx - S->ch2_esc_x; dy = ety - S->ch2_esc_y; d = fsqrt(dx*dx + dy*dy);
                }
            } else { S->ch2_stall = 0; S->ch2_base = d; }
            if (d > 4){
                float sp = db > 260? 320.0f : BUD_SPD[b->species%BUDN] + 24.0f;
                float step = sp*dt;
                float want = (to_pass || S->ch2_wp_on)? d : d - 30; if (want < 0) want = 0;
                if (step > want) step = want;
                if (step > 0){
                    float ox = S->ch2_esc_x, oy = S->ch2_esc_y;
                    float nx = ox + dx/d*step, ny = oy + dy/d*step;
                    // the meadow grows where it likes: if worldgen
                    // put a tree on Duster's post, it squeezes out
                    // (a destination-only check from inside a tree
                    // tile can never pass — the buddy escapes that
                    // trap by being walked around, a bot cannot)
                    if (bw_solid_b(b, ox, oy)){ S->ch2_esc_x = nx; S->ch2_esc_y = ny; }
                    else {
                        // an aimed step counts only if it MEANINGFULLY
                        // closes the distance — when the target runs
                        // level, the free axis offers a micro-step that
                        // would otherwise mask a full wedge forever
                        float need = step*0.3f;
                        int moved = 0;
                        if (!bw_solid_b(b, nx, ny)){ float cdx = etx-nx, cdy = ety-ny;
                            if (fsqrt(cdx*cdx+cdy*cdy) < d - need){ S->ch2_esc_x = nx; S->ch2_esc_y = ny; moved = 1; } }
                        if (!moved && !bw_solid_b(b, nx, oy)){ float cdx = etx-nx, cdy = ety-oy;
                            if (fsqrt(cdx*cdx+cdy*cdy) < d - need){ S->ch2_esc_x = nx; moved = 1; } }
                        if (!moved && !bw_solid_b(b, ox, ny)){ float cdx = etx-ox, cdy = ety-ny;
                            if (fsqrt(cdx*cdx+cdy*cdy) < d - need){ S->ch2_esc_y = ny; moved = 1; } }
                        if (!moved){
                            // wedged in a thicket corner: the compass
                            // — any clear step that closes the distance,
                            // else a sidestep. The sidestep prefers the
                            // PERPENDICULAR: stepping back the way it
                            // came just re-aims at the same blocked
                            // edge next frame and 2-cycles forever (a
                            // level run at a tree tile's east edge did
                            // exactly that — jittering in place at heel
                            // distance while the buddy waited).
                            static const float C8[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{0.7071f,0.7071f},{0.7071f,-0.7071f},{-0.7071f,0.7071f},{-0.7071f,-0.7071f}};
                            static const int ORDH[8] = {2,3,5,7,1,4,6,0};   // running level: S,N,SW,NW,W,SE,NE,E
                            static const int ORDV[8] = {0,1,6,7,3,4,5,2};   // running vertical: E,W,NE,NW,N,SE,SW,S
                            const int *ord = (dx*dx >= dy*dy)? ORDH : ORDV;
                            float bd = d; int bi = -1, si = -1;
                            for (int i=0;i<8;i++){
                                float cx2 = ox + C8[i][0]*step, cy2 = oy + C8[i][1]*step;
                                if (bw_solid_b(b, cx2, cy2)) continue;
                                { float cdx = etx-cx2, cdy = ety-cy2; float cd = fsqrt(cdx*cdx+cdy*cdy);
                                  if (cd < bd){ bd = cd; bi = i; } }
                            }
                            if (bi<0) for (int k=0;k<8;k++){ int i = ord[k];
                                float cx2 = ox + C8[i][0]*step, cy2 = oy + C8[i][1]*step;
                                if (!bw_solid_b(b, cx2, cy2)){ si = i; break; } }
                            int pick = bi>=0? bi : si;
                            if (pick>=0){ S->ch2_esc_x = ox + C8[pick][0]*step; S->ch2_esc_y = oy + C8[pick][1]*step; }
                        }
                    }
                }
            }
            if (bw_site_at_xy(b, b->x, b->y)==1 && bw_duster_dist(b) < 170){
                bw_qset3(b,11,2); bw_qset3(b,12,1); b->xp += 50;   // the escort's finish arms the rematch
                scpy(S->note, "DUSTER SETS TO WORK. THEN — A RATTLING IN THE RACKS. +50 XP.", 72); S->note_t=4.5f;
            }
        }
    }
    if (in->exit_){ bw_exit(pi, 0); return; }   // pad: Back or Start; keys: Esc/P
    if (in->interact){
        int npc = bw_npc_near(b, 46);
        if (npc>=0){ bw_talk(pi, npc); return; }
        int term = bw_term_near(b, 44);
        if (term>=0){ bw_press_term(pi, term); return; }
        if (bw_sign_near(b, 44)){ bw_read_sign(pi); return; }
        int hs = bw_hsign_near(b, 44);
        if (hs>=0){ bw_read_hsign(pi, hs); return; }
        if (bw_bitling_near(b, 44)){ bw_meet_bitling(pi); return; }
        int tool = bw_tool_near(b, 44);
        if (tool>=0){ bw_pick_tool(pi, tool); return; }
        float dx=b->x, dy=b->y+6;
        if (dx*dx+dy*dy < 46*46){ bw_exit(pi, 0); return; }       // at the gate
        float tx, ty;
        if (bw_tent_near(b, 42, &tx, &ty)){ S->sleep_t=1.4f; S->slept=0; sfx(11); }
    }
}
static void bw_draw_hacker(float wx, float wy, int lvl, float lunge, int named);
static void bw_draw_big_hacker(float wx, float wy, int boss, int named);
static void bw_draw_tent(float wx, float wy);
