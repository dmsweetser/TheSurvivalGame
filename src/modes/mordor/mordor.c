// THE SURVIVAL GAME — modes/mordor/mordor.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).

// ===========================================================================
// MORDOR FUN RUN (Banzai World mode 9, v1.28 — Daniel): a top-down
// long-haul run in the Arcade's eighth cabinet. Your bonsai buddy
// runs the road from THE SHIRE to MORDOR, fighting goblins, wolves,
// spiders and orcs in real time as they show up along the way.
//
// THE MOOSE SCALE: the whole road is 1000 MOOSE long (one moose =
// MOR_MOOSE_PX world units at game scale; the living ruler and the
// measured MOOSE track stand by the start line). The HUD counts
// progress in moose, and the milestone markers every 50 moose are
// moose markers. At buddy speeds the full road is a 40-65 minute
// haul — it is MEANT to be chipped away across sessions, which is
// what the save is for.
//
// SAVE AND RESUME (the core requirement): the run lives on the
// Buddy (the mu save token: on, dist, milestone, hp, secs, lateral),
// not in this session state, so leaving the cabinet, quitting the
// game or closing the browser all keep it. Entering the cabinet
// with a run on the road raises the RESUME panel; resuming puts you
// back at the exact saved spot. Fainting is the soft version of the
// same rule: back to the last banked milestone, the current
// stretch's foes reset, the banked distance never lost.
//
// The road itself is a function, not a map: distance axis s runs
// 0 (the Shire, y = MOR_Y0) north to the total (Mordor), the
// centreline wanders by sines, and the foes are a hash function of
// (buddy seed, chunk, slot) — Tank Rescue's trick — so a stretch
// re-derives itself identically every visit, and the session's
// dead-bitmap is the only kill memory a stretch needs.
// ===========================================================================
static int mor_test_total_moose;   // test-only (dbg_mortotal): a shortened road for the suite; shipped play never sets it
static void mor_set_test_total(int moose){ mor_test_total_moose = moose<0? 0 : moose; }
static int mor_total_units(int pi){
    (void)pi;
    return mor_test_total_moose>0? mor_test_total_moose * MOR_MOOSE_PX : MOR_LEN;
}
static int mor_total_moose(int pi){ return mor_total_units(pi) / MOR_MOOSE_PX; }
static float mor_spacing(int pi){ return (float)mor_total_units(pi) / MOR_MILES; }   // world units per milestone
static float mor_cx(float s){   // the road's centreline, a lazy wander
    return 240.0f*fsin(s*0.000113f) + 150.0f*fsin(s*0.0000317f + 1.7f);
}
#define MOR_HALF 120            // the road's half width: firm ground
#define MOR_LATMAX (MOR_HALF + 150)   // the scenery's soft edge: past here the woods say no
static float mor_s_of(Buddy *b){ return (float)MOR_Y0 - b->y; }
static int mor_maxhp(Buddy *b){ int m = (int)b->maxhp; return m<1? 100 : m; }
static void mor_place(int pi, float s, float lat){
    Buddy *b=&G.bud[pi];
    if (lat > MOR_LATMAX) lat = MOR_LATMAX; if (lat < -MOR_LATMAX) lat = -MOR_LATMAX;
    b->x = mor_cx(s) + lat; b->y = (float)MOR_Y0 - s; b->aim = -PI/2;
}
static void mor_warp(int pi, int dist){   // the dbg_morwarp seam: stand at a distance on the centreline, foes re-derive around you
    BwSim *S=&BWS[pi];
    if (dist<0) dist=0; if (dist>mor_total_units(pi)) dist=mor_total_units(pi);
    S->mor_en_n = 0; S->mor_spawn_t = 0;
    mor_place(pi, (float)dist, 0);
}
// --- the dead-bitmap: one bit per (chunk, slot) spawn ----------------------
static int mor_dead_get(BwSim *S, int chunk, int slot){
    int idx = chunk*5 + slot; if (idx<0 || idx>=960*8) return 1;   // off the map reads as dead: never spawn there
    return (S->mor_dead[idx>>3] >> (idx&7)) & 1;
}
static void mor_dead_set(BwSim *S, int chunk, int slot){
    int idx = chunk*5 + slot; if (idx<0 || idx>=960*8) return;
    S->mor_dead[idx>>3] |= (unsigned char)(1 << (idx&7));
}
static void mor_dead_clear_from(BwSim *S, int chunk){   // a faint resets the current stretch: every spawn at/after this chunk lives again
    for (int idx = chunk*5; idx < 960*8; idx++)
        S->mor_dead[idx>>3] &= (unsigned char)~(1 << (idx&7));
}
// --- the road powerups: stations on the road, a hash of (seed, k) -----------
// v1.29 (Daniel): pickups ON the road, collected by running over
// them — health, weapon amplification, speed. Fifty stations ride
// the road at total/50 spacing (~20 moose apart on the full road),
// each placed by the same deterministic trick as the foes — a hash
// of (buddy seed, station) with a DIFFERENT salt — so a station
// re-derives identically every visit and the session's taken-
// bitmap is the only memory it needs. Station 0 is special: always
// HEALTH, early in the opening stretch, so a hurt runner meets
// help before the road gets mean. The timed buffs live in BwSim
// and are never saved (see story.c) — leaving the cabinet or
// fainting ends them.
#define MOR_POW_N 50
static float mor_pow_spacing(int pi){ return (float)mor_total_units(pi) / MOR_POW_N; }
static void mor_pow_pos(int pi, int k, float *spos, float *lat, int *kind){
    if (k==0){ *spos = mor_pow_spacing(pi)*0.22f; *lat = 0; *kind = 0; return; }
    Buddy *b=&G.bud[pi];
    u32 seed = (u32)b->bw_seed ^ 0x51AB3u;
    u32 h = hash2(k*29 + 11, k ^ 0x51, seed);
    float spacing = mor_pow_spacing(pi);
    // jitter within +-0.3 of the slot: stations stay ordered and
    // never crowd (the closest two can sit is 0.4 slots apart)
    *spos = (k + 0.5f)*spacing + ((float)((int)((h>>8)%100) - 50)/100.0f) * spacing * 0.6f;
    *lat = (float)((int)((h>>20)%161) - 80);
    int r = (int)((h>>16) % 100);
    *kind = r<40? 0 : r<70? 1 : 2;   // health 40 / amplification 30 / speed 30
}
static int mor_pow_taken(BwSim *S, int k){ return (S->mor_powtaken[k>>3] >> (k&7)) & 1; }
static void mor_pow_set(BwSim *S, int k){ S->mor_powtaken[k>>3] |= (unsigned char)(1 << (k&7)); }
static void mor_pow_clear_all(BwSim *S){ for (int i=0;i<8;i++) S->mor_powtaken[i] = 0; }
static void mor_pow_clear_from(int pi, BwSim *S, float s){   // a faint: stations at/after the banked milestone are back on the road
    for (int k=0;k<MOR_POW_N;k++){ float ps, pl; int kd; mor_pow_pos(pi, k, &ps, &pl, &kd); if (ps >= s) S->mor_powtaken[k>>3] &= (unsigned char)~(1 << (k&7)); }
}
// --- the foes: a hash function of (seed, chunk, slot) -----------------------
static const int MOR_EHP[4] = { 14, 10, 22, 34 };        // goblin, wolf, spider, orc
static const float MOR_ESPD[4] = { 92, 148, 78, 66 };
static const int MOR_EDMG[4] = { 6, 5, 8, 12 };
static int mor_spawn_roll(int pi, int chunk, int slot, float *spos, float *lat, int *kind){
    Buddy *b=&G.bud[pi];
    u32 seed = (u32)b->bw_seed ^ 0x402D09u;
    u32 h = hash2(chunk*13 + slot*7, chunk ^ (slot*31 + 7), seed);
    int total = mor_total_units(pi);
    float sc = (float)(chunk * MOR_CHUNK) / (float)total; if (sc>1) sc=1;
    // presence: ~18% of slots early, thickening to ~73% at the gates
    if ((int)((h >> (3 + slot*5)) % 100) >= 18 + (int)(sc*55.0f)) return 0;
    // the mix escalates with the road: goblins everywhere, wolves
    // once you're out of the Shire, spiders past the quarter,
    // orcs only in Mordor's shadow
    int r = (int)((h>>16) % 100);
    int wg = 60 - (int)(40*sc);
    int ww = sc<0.08f? 0 : 22;
    int ws = sc<0.25f? 0 : 20 + (int)(10*sc);
    int wo = sc<0.50f? 0 : 8 + (int)(30*sc);
    int k = 0;
    if (r >= wg){ r -= wg; if (r < ww) k = 1; else { r -= ww; if (r < ws) k = 2; else { r -= ws; if (r < wo) k = 3; } } }
    *kind = k;
    *spos = (float)(chunk * MOR_CHUNK) + 30.0f + (float)((h>>8) % 240);
    *lat = (float)((int)((h>>20) % 181) - 90);
    return 1;
}
static void mor_spawn_window(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    float s = mor_s_of(b);
    int total = mor_total_units(pi);
    int c0 = (int)((s - 320) / MOR_CHUNK); if (c0<0) c0=0;
    int c1 = (int)((s + 1080) / MOR_CHUNK);
    int cmax = total / MOR_CHUNK + 1;
    if (c1>cmax) c1=cmax;
    for (int c=c0; c<=c1; c++) for (int slot=0; slot<5; slot++){
        if (mor_dead_get(S, c, slot)) continue;
        int live = 0;
        for (int i=0;i<S->mor_en_n;i++) if (S->mor_en[i].chunk==c && S->mor_en[i].slot==slot){ live=1; break; }
        if (live) continue;
        float spos, lat; int kind;
        if (!mor_spawn_roll(pi, c, slot, &spos, &lat, &kind)) continue;
        float ex = mor_cx(spos) + lat, ey = (float)MOR_Y0 - spos;
        float dx = ex - b->x, dy = ey - b->y;
        if (dx*dx + dy*dy < 150*150) continue;   // nothing materialises on top of the runner (a warp/resume is never an ambush)
        if (S->mor_en_n >= MOR_MAXEN) return;
        MorEn *e = &S->mor_en[S->mor_en_n++];
        float sc = spos / (float)total; if (sc>1) sc=1;
        e->x = ex; e->y = ey; e->kind = kind; e->chunk = c; e->slot = slot;
        e->maxhp = e->hp = MOR_EHP[kind] * (1.0f + sc*0.6f);
        e->atk_t = 0; e->hit_t = 0;
    }
}
static void mor_start_run(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    b->mor_on = 1; b->mor_dist = 0; b->mor_mile = 0; b->mor_hp = mor_maxhp(b); b->mor_secs = 0; b->mor_lat = 0;
    S->mor_t = 0; S->mor_kills = 0; S->mor_en_n = 0;
    for (int i=0;i<960;i++) S->mor_dead[i] = 0;
    mor_pow_clear_all(S); S->mor_amp_t = 0; S->mor_spd_t = 0;
    S->mor_panel = 0;
    mor_place(pi, 0, 0);
    scpy(S->note, "A NEW RUN. THE SHIRE BEHIND YOU, MORDOR AHEAD. 1000 MOOSE.", 72); S->note_t = 3.6f;
    do_save();
}
static void mor_enter(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->mor_en_n = 0; S->mor_spawn_t = 0; S->mor_swing_cd = 0; S->mor_swing_t = 0;
    S->mor_grace = 0; S->mor_faintt = 0; S->mor_cleart = 0; S->mor_confirm_t = 0; S->mor_hurt_t = 0; S->mor_pick_lock = 0;
    S->mor_kills = 0; S->mor_swings = 0; S->mor_panel = 0;
    mor_pow_clear_all(S); S->mor_amp_t = 0; S->mor_spd_t = 0;   // v1.29: buffs lapse on leaving, so a fresh entry starts clean; the stations re-derive untaken
    for (int i=0;i<960;i++) S->mor_dead[i] = 0;   // a fresh entry re-derives the road's foes; the banked run (Buddy) is what persists
    if (b->mor_on){
        if (b->mor_hp<=0) b->mor_hp = mor_maxhp(b);
        S->mor_t = (float)b->mor_secs;
        mor_place(pi, (float)b->mor_dist, (float)b->mor_lat);   // resume at the exact saved spot (the milestone is the faint floor, not the resume rule)
        S->mor_panel = 1; S->mor_psel = 0;   // the RESUME prompt: the run waits for its runner's word
        S->mor_pick_lock = 0.35f;   // frame()'s host path runs bw_update in the SAME frame as the cabinet pick that entered the mode, with that select edge still live — without this lock the entering press picks RESUME itself
    } else {
        mor_start_run(pi);
    }
    mor_spawn_window(pi);
}
static void mor_hurt(int pi, int dmg){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->mor_grace>0 || S->mor_faintt>0 || S->mor_cleart>0 || S->mor_panel) return;
    b->mor_hp -= dmg; if (b->mor_hp<0) b->mor_hp = 0;
    S->mor_hurt_t = 0.3f; S->mor_grace = 0.4f;
    splat(b->x, b->y, COL(255,110,80), 7, 100); sfx(9);
    if (b->mor_hp<=0){
        S->mor_faintt = 1.6f;
        splat(b->x, b->y, COL(255,170,70), 24, 150); sfx(7); sfx(13);
        scpy(S->note, "DOWN! THE ROAD SPITS YOU BACK TO THE LAST MOOSE MARKER.", 72); S->note_t = 3.0f;
    }
}
static void mor_faint_home(int pi){   // the faint lands: back at the banked milestone, hale again, the stretch ahead reset
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    float sm = b->mor_mile * mor_spacing(pi);
    mor_dead_clear_from(S, (int)(sm / MOR_CHUNK));
    mor_pow_clear_from(pi, S, sm);   // v1.29: the stretch's stations come back too, and the buffs end with the fall
    S->mor_amp_t = 0; S->mor_spd_t = 0;
    S->mor_en_n = 0;
    b->mor_hp = mor_maxhp(b);
    b->mor_dist = (int)sm; b->mor_lat = 0;
    mor_place(pi, sm, 0);
    S->mor_grace = 1.5f;
    mor_spawn_window(pi);
    do_save();
}
static void mor_finish(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->mor_cleart = 0.001f;
    b->mor_on = 0; b->mor_dist = 0; b->mor_mile = 0; b->mor_secs = 0; b->mor_lat = 0;
    if (!G.zen_run){   // ZEN MODE sets no records (standing rule)
        int secs = (int)S->mor_t;
        b->mor_done++;
        if (!b->mor_best || secs < b->mor_best) b->mor_best = secs;
    }
    buddy_award_xp(pi, 60);
    splat(b->x, b->y - 20, COL(255,214,120), 30, 170); splat(b->x, b->y - 20, COL(255,120,80), 18, 130);
    sfx(11); sfx(12);
    do_save();
}
// --- the entry panel: RESUME / NEW RUN / LEAVE ------------------------------
// Geometry lives here and the render reads the same numbers, so a
// tap can never land anywhere the drawing didn't promise.
static void mor_panel_rect(int vx, int vy, int vw2, int vh2, int *x, int *y, int *w, int *h){
    *w = vw2<372? vw2-16 : 356; *h = 176;
    *x = vx + (vw2-*w)/2; *y = vy + (vh2-*h)/2; if (*y < vy+4) *y = vy+4;
}
static int mor_panel_row_y(int y, int r){ return y + 66 + r*26; }
static void mor_panel_pick(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->mor_psel==0){   // RESUME
        S->mor_panel = 0; sfx(11);
        char *q=S->note; q=sappend(q, "BACK ON THE ROAD. "); q=sapp_int(q, b->mor_dist/MOR_MOOSE_PX); q=sappend(q, " MOOSE IN, MARKER "); q=sapp_int(q, b->mor_mile); q=sappend(q, " BANKED."); *q=0;
        S->note_t = 3.0f;
    } else if (S->mor_psel==1){   // NEW RUN — with confirm (Daniel's spec): the first pick arms it, the second scraps the old run
        if (S->mor_confirm_t>0){ mor_start_run(pi); sfx(11); }
        else { S->mor_confirm_t = 3.5f; S->mor_pick_lock = 0.3f; sfx(6);   // the lock: a held/repeating select must not arm AND confirm in one press
            scpy(S->note, "SCRAP THE RUN ON THE ROAD? CHOOSE NEW RUN AGAIN TO BE SURE.", 72); S->note_t = 3.2f; }
    } else {   // LEAVE — the run stays on the road, saved, waiting
        S->mor_panel = 0;
        bw_exit(pi, 0);
    }
}
static void mor_panel_update(int pi, float dt, const BwIn *in){
    BwSim *S=&BWS[pi];
    if (S->mor_confirm_t>0) S->mor_confirm_t -= dt;
    if (S->mor_pick_lock>0) S->mor_pick_lock -= dt;
    if (in->exit_){ S->mor_panel = 0; bw_exit(pi, 0); return; }
    if (in->up){ S->mor_psel = (S->mor_psel+2)%3; S->mor_confirm_t = 0; sfx(6); }
    if (in->down){ S->mor_psel = (S->mor_psel+1)%3; S->mor_confirm_t = 0; sfx(6); }
    if (S->mor_pick_lock>0) return;   // picks wait out the lock; nav above still answers
    if (in->sel || in->interact){ mor_panel_pick(pi); return; }
    if (pi==G.my_id && IN.tapped){
        int vx,vy,vw2,vh2; pi_view_rect(pi, &vx,&vy,&vw2,&vh2);
        int x,y,w,h; mor_panel_rect(vx,vy,vw2,vh2,&x,&y,&w,&h);
        int tx=IN.tap_x, ty=IN.tap_y; IN.tapped = 0;
        if (tx<x || tx>=x+w || ty<y || ty>=y+h){ return; }   // a tap outside just waits — the panel is a question, not a trapdoor
        for (int r=0;r<3;r++){ int ry = mor_panel_row_y(y, r);
            if (ty>=ry-6 && ty<ry+17){ S->mor_psel = r; mor_panel_pick(pi); return; } }
    }
}
static void mor_update(int pi, float dt, const BwIn *in){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->mor_panel){ mor_panel_update(pi, dt, in); return; }
    if (in->exit_){ bw_exit(pi, 0); return; }
    int total = mor_total_units(pi);
    if (S->mor_grace>0) S->mor_grace -= dt;
    if (S->mor_swing_cd>0) S->mor_swing_cd -= dt;
    if (S->mor_swing_t>0) S->mor_swing_t -= dt;
    if (S->mor_hurt_t>0) S->mor_hurt_t -= dt;
    if (S->mor_amp_t>0) S->mor_amp_t -= dt;   // v1.29: the buffs tick in real time
    if (S->mor_spd_t>0) S->mor_spd_t -= dt;
    if (S->mor_faintt>0){
        S->mor_faintt -= dt;
        if (S->mor_faintt<=0) mor_faint_home(pi);
        return;   // the road holds its breath while you lie on it
    }
    if (S->mor_cleart>0){
        S->mor_cleart += dt;
        if (S->mor_cleart>2.8f){ bw_exit(pi, 0); }
        return;
    }
    S->mor_t += dt;
    // --- run: 8-way, the buddy's own legs (species speed, as the meadow) ---
    float mx=0, my=0; int k=in->keys;
    if (k&K_UP) my-=1; if (k&K_DOWN) my+=1; if (k&K_LEFT) mx-=1; if (k&K_RIGHT) mx+=1;
    float s = mor_s_of(b);
    if (mx!=0 || my!=0){
        float d=fsqrt(mx*mx+my*my); mx/=d; my/=d;
        b->aim = fatan2(my, mx);
        float sp = BUD_SPD[b->species%BUDN];
        if (S->mor_spd_t>0) sp *= 1.4f;   // v1.29: SPEED BOOST — swift feet while it lasts
        float lat = b->x - mor_cx(s);
        if (lat > MOR_HALF+30 || lat < -MOR_HALF-30) sp *= 0.55f;   // off the road is trudging country
        b->x += mx*sp*dt; b->y += my*sp*dt;
        b->walk_ph += dt*9; b->mv_t = 0.15f;
        s = mor_s_of(b);
    }
    // the scenery's soft bounds: the road wanders, the woods edge it,
    // and neither Mordor's gate nor the Shire's fence lets you past
    { float lat = b->x - mor_cx(s);
      if (lat > MOR_LATMAX) b->x = mor_cx(s) + MOR_LATMAX;
      if (lat < -MOR_LATMAX) b->x = mor_cx(s) - MOR_LATMAX;
      if (s < -30) b->y = (float)MOR_Y0 + 30;
      if (s > total + 30) b->y = (float)MOR_Y0 - total - 30;
      s = mor_s_of(b); }
    // the run, written onto the Buddy as it stands (every do_save
    // anywhere — a milestone, a faint, the exit — carries it whole)
    b->mor_dist = s<0? 0 : s>(float)total? total : (int)s;
    b->mor_lat = (int)(b->x - mor_cx(s));
    b->mor_secs = (int)S->mor_t;
    // --- the road powerups: run over one and it is yours ----------------------
    for (int pk=0; pk<MOR_POW_N; pk++){
        if (mor_pow_taken(S, pk)) continue;
        float ps, plat; int pkind; mor_pow_pos(pi, pk, &ps, &plat, &pkind);
        if (ps < s - 60 || ps > s + 60) continue;
        float px = mor_cx(ps) + plat, py = (float)MOR_Y0 - ps;
        float dx = px - b->x, dy = py - b->y;
        if (dx*dx + dy*dy > 30*30) continue;
        mor_pow_set(S, pk);
        if (pkind==0){ int mh = mor_maxhp(b); b->mor_hp += 50; if (b->mor_hp > mh) b->mor_hp = mh;
            scpy(S->note, "+50 HP. THE ROAD PROVIDES.", 72); }
        else if (pkind==1){ S->mor_amp_t = 30.0f; scpy(S->note, "WEAPON AMPLIFIED! DOUBLE DAMAGE FOR 30 SECONDS.", 72); }
        else { S->mor_spd_t = 20.0f; scpy(S->note, "SPEED BOOST! SWIFT FEET FOR 20 SECONDS.", 72); }
        S->note_t = 2.6f;
        splat(px, py, pkind==0? COL(255,120,120) : pkind==1? COL(255,170,60) : COL(110,220,255), 14, 120);
        sfx(11);
    }
    // --- the swing: the buddy's standard attack, real time ------------------
    if ((in->sel || in->sel_held) && S->mor_swing_cd<=0){
        S->mor_swing_cd = 0.34f; S->mor_swing_t = 0.18f; b->atk_cd = 0.34f; S->mor_swings++;
        sfx(4);
        float dmg = buddy_dmg(b);
        if (S->mor_amp_t>0) dmg *= 2;   // v1.29: WEAPON AMPLIFICATION — double damage while it lasts
        for (int i=0;i<S->mor_en_n;i++){
            MorEn *e=&S->mor_en[i];
            float dx=e->x-b->x, dy=e->y-b->y, d2=dx*dx+dy*dy;
            if (d2 > 54*54) continue;
            float dl = fsqrt(d2);
            if (d2 > 30*30){   // the outer band only counts inside the swing's arc
                if (dl>0.01f && (fcos(b->aim)*dx + fsin(b->aim)*dy)/dl < 0.25f) continue;
            }
            e->hp -= dmg; e->hit_t = 0.18f;
            if (dl>0.01f){ e->x += dx/dl*10; e->y += dy/dl*10; }
            splat(e->x, e->y, COL(255,230,160), 4, 80); sfx(5);
            if (e->hp<=0){
                mor_dead_set(S, e->chunk, e->slot);
                splat(e->x, e->y, e->kind==3? COL(200,90,60) : e->kind==2? COL(90,80,110) : e->kind==1? COL(170,170,180) : COL(120,220,110), 14, 130);
                sfx(7);
                S->mor_kills++;
                S->mor_en[i] = S->mor_en[--S->mor_en_n]; i--;
            }
        }
    }
    // --- the foes -------------------------------------------------------------
    S->mor_spawn_t -= dt;
    if (S->mor_spawn_t<=0){ S->mor_spawn_t = 0.15f; mor_spawn_window(pi); }
    for (int i=0;i<S->mor_en_n;i++){
        MorEn *e=&S->mor_en[i];
        float es = (float)MOR_Y0 - e->y;
        if (es < s - 1300 || es > s + 1600){ S->mor_en[i] = S->mor_en[--S->mor_en_n]; i--; continue; }   // far behind or far ahead: back into the hash they go
        if (e->hit_t>0) e->hit_t -= dt;
        if (e->atk_t>0) e->atk_t -= dt;
        float dx=b->x-e->x, dy=b->y-e->y, d2=dx*dx+dy*dy;
        if (d2 < 300*300){
            float dl = fsqrt(d2);
            if (dl>1){
                float spd = MOR_ESPD[e->kind];
                e->x += dx/dl*spd*dt; e->y += dy/dl*spd*dt;
                // the foes keep to the road's country too — no chasing through the deep woods
                float elat = e->x - mor_cx((float)MOR_Y0 - e->y);
                if (elat > MOR_LATMAX) e->x = mor_cx((float)MOR_Y0 - e->y) + MOR_LATMAX;
                if (elat < -MOR_LATMAX) e->x = mor_cx((float)MOR_Y0 - e->y) - MOR_LATMAX;
            }
            if (d2 < 24*24 && e->atk_t<=0){
                e->atk_t = 0.9f;
                mor_hurt(pi, MOR_EDMG[e->kind]);
                if (S->mor_faintt>0) return;   // the hit that downs you ends the frame's business
            }
        }
    }
    // --- milestones: banked is banked ------------------------------------------
    { float spacing = mor_spacing(pi);
      int mile_now = (int)(s / spacing); if (mile_now>MOR_MILES) mile_now = MOR_MILES;
      if (mile_now > b->mor_mile){
          b->mor_mile = mile_now;
          if (mile_now < MOR_MILES){
              char *q=S->note; q=sappend(q, "MOOSE MARKER "); q=sapp_int(q, mile_now); q=sappend(q, " - ");
              q=sapp_int(q, mile_now * mor_total_moose(pi) / MOR_MILES); q=sappend(q, " MOOSE BANKED."); *q=0;
              S->note_t = 2.8f;
              splat(b->x, b->y, COL(255,214,120), 16, 130); sfx(11);
          }
          do_save();
      } }
    // --- the gates of Mordor -----------------------------------------------------
    if (s >= (float)total) mor_finish(pi);
}
