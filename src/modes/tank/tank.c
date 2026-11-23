// THE SURVIVAL GAME — modes/tank/tank.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from tank.c lines 1-499 in the Nov 2026 fine-grained refactor; code unchanged.

// ===========================================================================
// TANK RESCUE (Banzai World mode 5, Nov 10 — Daniel asked for a Jackal
// (NES) clone for the Arcade): the buddy drives a tank up a long field,
// blasting turrets, patrol tanks and prison huts, ferrying the freed
// captives to helipads, and breaking the fortress core at the top —
// which drops the gate so he can roll over the finish line. Tiles are
// a pure hash function of (buddy seed, level); entities are generated
// once per level into the session. The record (deepest level cleared)
// lives on the Buddy as td.
// ===========================================================================
// v1.27 POWERUPS (Daniel: the tank game is too hard): kind 6 is a
// HEALTH medkit (+50 hull), kind 7 is the GRENADE LAUNCHER upgrade
// (endless grenades, faster lob, for the rest of the run). The
// launcher flag and the patrol-kill counter live here rather than
// in BwSim: the launcher must survive tank_load_level (it is reset
// only by tank_enter, a fresh cabinet entry), and the counter only
// feeds the deterministic drop roll in tank_kill_en.
static int tank_glauncher_st[MAXP];
static int tank_killct[MAXP];
static int tank_pad_ty(int k){ return TANK_LEN - 30 - k*28; }   // helipad rows, marching north
// 0 ground, 1 water (blocks the tank, not shells), 2 concrete (blocks,
// indestructible), 3 trees (blocks, destructible), 4 sandbags (blocks,
// destructible, tougher), 5 the boss gate (blocks the tank while the
// core lives; shells fly clean over it)
static int tank_tile(BwSim *S, int tx, int ty){
    if (tx<0 || tx>=TANK_W || ty<0 || ty>=TANK_LEN) return 2;
    if (ty==TANK_GATE_TY) return S->tank_boss_hp>0? 5 : 0;
    if (ty==TANK_MID_TY) return (S->tank_mid_hp>0 || S->tank_deliv < S->tank_quota)? 5 : 0;   // the blockade barrier: shells pass, the tank doesn't — and it stays shut until the rescue quota is earned, so the bosses come in sequence
    if (ty<TANK_GATE_TY) return 0;                          // finish strip + the boss arena plaza
    if (ty>=TANK_LEN-5) return 0;                           // the spawn apron
    for (int k=0;k<3;k++){ int pt = tank_pad_ty(k);         // helipads stay clear
        if (ty>=pt && ty<pt+2 && tx>=10 && tx<15) return 0; }
    if (tx==12 || tx==13) return 0;                         // the spine: one lane is always drivable
    u32 seed = (u32)S->tank_seed;
    u32 h = hash2(tx, ty, seed);
    int r = (int)(h % 1000);
    int ct = 30 + S->tank_lvl; if (ct>46) ct=46;            // cover thickens a touch per level
    int t = 0;
    if ((int)(hash2(tx>>1, ty>>1, seed ^ 0xA17u) % 100) < 6) t = 1;        // water, in pond-ish blobs
    else if (r < ct) t = 2;
    else if (r < ct+62) t = 3;
    else if (r < ct+92) t = 4;
    if ((t==3 || t==4) && S->tank_dead[ty*TANK_W+tx]) t = 0;               // shot away earlier
    return t;
}
static int tank_tank_blocked(BwSim *S, float x, float y){
    static const float off[8][2] = { {-16,-16},{0,-17},{16,-16},{-17,0},{17,0},{-16,16},{0,17},{16,16} };
    for (int i=0;i<8;i++){
        int t = tank_tile(S, bw_tileof(x+off[i][0]), bw_tileof(y+off[i][1]));
        if (t==1 || t==2 || t==3 || t==4 || t==5) return 1;
    }
    return 0;
}
static TankEn *tank_en_add(BwSim *S, int kind, float x, float y, int hp){
    if (S->tank_en_n>=TANK_MAXEN) return 0;
    TankEn *e = &S->tank_en[S->tank_en_n++];
    e->x=x; e->y=y; e->aim=PI/2; e->lo=0; e->hi=0; e->kind=kind; e->hp=hp; e->dir=1;
    e->ft = 0.8f + (float)(S->tank_en_n%7)*0.31f;   // staggered, so the field never volleys as one
    return e;
}
static void tank_load_level(int pi, int lvl){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->tank_lvl = lvl;
    S->tank_seed = (int)((u32)b->bw_seed ^ (0x7A9Cu + (u32)lvl*104729u));
    S->tank_hull = 100; S->tank_pass = 0;
    S->tank_deliv = 0; S->tank_quota = 3; S->tank_obj = 0; S->tank_obj_x = 12.5f*BW_TILE; S->tank_obj_y = 60*BW_TILE;
    S->tank_fire_cd = 0; S->tank_grace = 0; S->tank_deadt = 0; S->tank_cleart = 0;
    tank_killct[pi] = 0;   // the patrol-drop roll restarts with the level's seed (the launcher flag above does NOT reset here — it lasts the run)
    S->tank_fx = 0; S->tank_fy = -1;
    for (int i=0;i<TANK_W*TANK_LEN;i++){ S->tank_dead[i]=0; S->tank_hit[i]=0; }
    for (int i=0;i<24;i++){ S->tank_cap_a[i]=0; S->tank_sh_a[i]=0; }
    for (int i=0;i<32;i++) S->tank_es_a[i]=0;
    S->tank_en_n = 0;
    // the fortress core and its two guards
    int bosshp = 430 + 90*lvl;
    S->tank_boss_hp = bosshp; S->tank_boss_max = bosshp;
    tank_en_add(S, 4, 12.5f*BW_TILE, 3.5f*BW_TILE, bosshp);
    if (S->tank_en_n>0) S->tank_en[0].ft = 1.6f;
    tank_en_add(S, 0, 6*BW_TILE+24, 5*BW_TILE+24, 30 + lvl*4);
    tank_en_add(S, 0, 19*BW_TILE+24, 5*BW_TILE+24, 30 + lvl*4);
    // the blockade gun, squatting on the road behind the mid-field
    // barrier: the second boss of the run, and it dies second-to-last
    int midhp = 110 + 30*lvl;
    S->tank_mid_hp = midhp; S->tank_mid_max = midhp;
    tank_en_add(S, 5, 12.5f*BW_TILE, 44*BW_TILE+24, midhp);
    // three prison huts are GUARANTEED south of the blockade, so the
    // delivery quota can always be earned before the road north opens
    int gh=0;
    for (int k=0; k<400 && gh<3; k++){
        u32 h = hash2(k*5+2, lvl*11+3, (u32)S->tank_seed ^ 0xB10Cu);
        int tx = 2 + (int)(h % 22);
        int ty = 58 + (int)((h>>8) % 38);
        if (tank_tile(S, tx, ty)!=0) continue;
        float ex = tx*BW_TILE+24, ey = ty*BW_TILE+24;
        int clear = 1;
        for (int i=0;i<S->tank_en_n;i++){ float dx=S->tank_en[i].x-ex, dy=S->tank_en[i].y-ey; if (dx*dx+dy*dy < 115*115){ clear=0; break; } }
        if (!clear) continue;
        // the gate aprons stay clear: nothing camps the road through
        // either barrier, so a fresh boss kill always has an open road
        { float gx=ex-600, gy=ey-2232; if (gx*gx+gy*gy < 130*130) continue; }
        { float gx=ex-600, gy=ey-408;  if (gx*gx+gy*gy < 130*130) continue; }
        if (tank_en_add(S, 2, ex, ey, 40)) gh++;
    }
    // scatter the garrison: hash-picked spots, on open ground, spread out
    int want_t = 11 + lvl*2; if (want_t>28) want_t=28;
    int want_p = 6 + lvl;   if (want_p>16) want_p=16;
    int want_h = 5 + lvl/2; if (want_h>8) want_h=8;
    int want_r = 2;
    int gt=0, gp=0, gr=0;
    for (int k=0; k<700; k++){
        if (gt>=want_t && gp>=want_p && gh>=want_h && gr>=want_r) break;
        if (S->tank_en_n >= TANK_MAXEN-1) break;
        u32 h = hash2(k*3+1, lvl*7+5, (u32)S->tank_seed ^ 0x6A11u);
        int tx = 2 + (int)(h % 22);
        int ty = 12 + (int)((h>>8) % (TANK_LEN-22));
        if (tank_tile(S, tx, ty)!=0) continue;
        int nearpad = 0;
        for (int pk=0; pk<3; pk++){ int pt = tank_pad_ty(pk);
            if (tx>=9 && tx<16 && ty>=pt-1 && ty<pt+3) nearpad = 1; }
        if (nearpad) continue;
        float ex = tx*BW_TILE+24, ey = ty*BW_TILE+24;
        int clear = 1;
        for (int i=0;i<S->tank_en_n;i++){ float dx=S->tank_en[i].x-ex, dy=S->tank_en[i].y-ey; if (dx*dx+dy*dy < 115*115){ clear=0; break; } }
        if (!clear) continue;
        // the gate aprons stay clear: nothing camps the road through
        // either barrier, so a fresh boss kill always has an open road
        { float gx=ex-600, gy=ey-2232; if (gx*gx+gy*gy < 130*130) continue; }
        { float gx=ex-600, gy=ey-408;  if (gx*gx+gy*gy < 130*130) continue; }
        int roll = (int)((h>>16) % 100);
        int kind = -1;
        if (roll<38 && gt<want_t) kind=0;
        else if (roll<62 && gp<want_p) kind=1;
        else if (roll<84 && gh<want_h) kind=2;
        else if (gr<want_r) kind=3;
        else if (gt<want_t) kind=0;
        else if (gp<want_p) kind=1;
        else if (gh<want_h) kind=2;
        if (kind<0) continue;
        int hp = kind==0? 30 + lvl*4 : kind==1? 40 + lvl*5 : kind==2? 40 : 1;
        TankEn *e = tank_en_add(S, kind, ex, ey, hp);
        if (!e) break;
        if (kind==0) gt++; else if (kind==1) gp++; else if (kind==2) gh++; else gr++;
        if (kind==1){   // a patrol: an axis, a span, a way to start
            float span = (2.0f + (float)((h>>20)%3))*BW_TILE;
            e->dir = (h&64)? 1 : -1;
            if (!(h&128)) e->dir *= 2;   // vertical patrol
            if (e->dir==1 || e->dir==-1){ e->lo = ex-span; e->hi = ex+span; e->aim = e->dir>0? 0 : PI; }
            else { e->lo = ey-span; e->hi = ey+span; e->aim = e->dir>0? PI/2 : -PI/2; }
        }
    }
    // v1.27 POWERUPS, guaranteed placements (the scatter's wrenches
    // can land anywhere, so a struggling driver needs these on the
    // route): one HEALTH medkit a short drive north of the spawn,
    // and one GRENADE LAUNCHER in the mid-field south of the
    // blockade. Both want open ground with open neighbours (so the
    // drive-over is never walled off), the usual 115px entity
    // clearance, spine-near columns first, and a spine fallback —
    // the spine lane is open by construction.
    { static const int porder[6] = { 12, 13, 11, 14, 10, 15 };
      int done6 = 0, done7 = 0;
      for (int a=0; a<6 && !done6; a++) for (int ty=TANK_LEN-14; ty<=TANK_LEN-8 && !done6; ty++){
          int tx = porder[a];
          if (tank_tile(S, tx, ty)!=0 || tank_tile(S, tx-1, ty)!=0 || tank_tile(S, tx+1, ty)!=0) continue;
          float ex = tx*BW_TILE+24, ey = ty*BW_TILE+24;
          int clear = 1;
          for (int i=0;i<S->tank_en_n;i++){ float dx=S->tank_en[i].x-ex, dy=S->tank_en[i].y-ey; if (dx*dx+dy*dy < 115*115){ clear=0; break; } }
          if (clear && tank_en_add(S, 6, ex, ey, 1)) done6 = 1;
      }
      if (!done6) for (int ty=TANK_LEN-14; ty<=TANK_LEN-8 && !done6; ty++){
          float ex = 12*BW_TILE+24, ey = ty*BW_TILE+24;
          int clear = 1;
          for (int i=0;i<S->tank_en_n;i++){ float dx=S->tank_en[i].x-ex, dy=S->tank_en[i].y-ey; if (dx*dx+dy*dy < 115*115){ clear=0; break; } }
          if (clear && tank_en_add(S, 6, ex, ey, 1)) done6 = 1;
      }
      if (!done6) tank_en_add(S, 6, 12*BW_TILE+24, (TANK_LEN-10)*BW_TILE+24, 1);
      for (int a=0; a<6 && !done7; a++) for (int ty=TANK_MID_TY+6; ty<=TANK_LEN-24 && !done7; ty++){
          int tx = porder[a];
          if (tank_tile(S, tx, ty)!=0 || tank_tile(S, tx-1, ty)!=0 || tank_tile(S, tx+1, ty)!=0) continue;
          float ex = tx*BW_TILE+24, ey = ty*BW_TILE+24;
          int clear = 1;
          for (int i=0;i<S->tank_en_n;i++){ float dx=S->tank_en[i].x-ex, dy=S->tank_en[i].y-ey; if (dx*dx+dy*dy < 115*115){ clear=0; break; } }
          if (clear && tank_en_add(S, 7, ex, ey, 1)) done7 = 1;
      }
      if (!done7) tank_en_add(S, 7, 12*BW_TILE+24, (TANK_MID_TY+6)*BW_TILE+24, 1);
    }
    b->x = 12.5f*BW_TILE; b->y = (TANK_LEN-3)*BW_TILE + 24;
    S->tank_respawn_y = b->y;
}
static void tank_enter(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->tank_score=0; S->tank_lives=3; S->tank_t=0;
    S->tank_wep=0; S->tank_gren=8;
    tank_glauncher_st[pi]=0;   // a fresh cabinet entry starts without the launcher; level loads and respawns keep it
    for (int i=0;i<10;i++) S->tank_gr_a[i]=0;
    for (int i=0;i<6;i++) S->tank_boom_t[i]=0;
    tank_load_level(pi, b->tank_done+1);
    scpy(S->note, "FOLLOW THE YELLOW ARROW: RESCUE, DELIVER, BREAK THROUGH!", 72); S->note_t=3.6f;
}
static void tank_kill_en(int pi, TankEn *e){
    BwSim *S=&BWS[pi];
    int kind = e->kind;
    if (kind==2){   // a hut breaks open: one captive, standing and waving
        for (int i=0;i<24;i++) if (!S->tank_cap_a[i]){
            S->tank_cap_a[i]=1; S->tank_cap_x[i]=e->x; S->tank_cap_y[i]=e->y; break; }
        scpy(S->note, "A CAPTIVE! DRIVE OVER TO PICK THEM UP.", 72); S->note_t=2.4f;
        S->tank_score += 200;
    } else if (kind==0) S->tank_score += 100;
    else if (kind==1){ S->tank_score += 150;
        // v1.27: a dead patrol drops a HEALTH medkit at the wreck,
        // 35% of the time — rolled on the per-level kill counter
        // against the level seed, so a run's drops are set the
        // moment the level is (no rand(), same as the field).
        tank_killct[pi]++;
        u32 dh = hash2((u32)tank_killct[pi]*7u + 1u, (u32)S->tank_lvl*13u + 5u, (u32)S->tank_seed ^ 0xD20Du);
        if ((int)(dh % 100) < 35) tank_en_add(S, 6, e->x, e->y, 1);
    }
    else if (kind==5){
        S->tank_score += 500; S->tank_mid_hp = 0;
        for (int i=0;i<32;i++) S->tank_es_a[i]=0;   // same courtesy as the core: its shells die with it
        scpy(S->note, "BLOCKADE BROKEN! THE ROAD NORTH IS OPEN.", 72); S->note_t=3.2f;
        sfx(12);
    }
    else if (kind==4){
        S->tank_score += 1000; S->tank_boss_hp = 0;
        for (int i=0;i<32;i++) S->tank_es_a[i]=0;   // a dead boss's shells fizzle — no posthumous kills on your victory lap
        scpy(S->note, "BOSS DOWN! THE GATE IS OPEN - DRIVE NORTH!", 72); S->note_t=3.2f;
        sfx(12);
    }
    if (kind!=3){ splat(e->x, e->y, kind==4? COL(255,160,60) : COL(200,120,70), kind==4? 26 : 12, 130); sfx(7); }
    e->kind = -1;
}
static void tank_hurt(int pi, int dmg){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->tank_grace>0 || S->tank_deadt>0 || S->tank_cleart>0) return;
    S->tank_hull -= dmg; sfx(9);
    splat(b->x, b->y, COL(255,120,60), 6, 90);
    if (S->tank_hull<=0){
        S->tank_hull = 0; S->tank_lives--; S->tank_deadt = 1.5f;
        splat(b->x, b->y, COL(255,170,70), 24, 150); sfx(7); sfx(13);
        if (S->tank_lives<=0){ scpy(S->note, "THE TANK IS SCRAP. THE BUDDY WALKS HOME.", 72); S->note_t=3.0f; }
        else { scpy(S->note, "TANK DOWN! BACK TO THE LAST PAD.", 72); S->note_t=2.4f; }
    } else if (S->tank_grace<0.35f) S->tank_grace = 0.35f;   // a breath of mercy between hits
}
static void tank_fire_eshell(BwSim *S, float x, float y, float ang, float spd){
    for (int i=0;i<32;i++) if (!S->tank_es_a[i]){
        S->tank_es_a[i]=1; S->tank_es_t[i]=1.7f;
        S->tank_es_x[i]=x; S->tank_es_y[i]=y;
        S->tank_es_vx[i]=fcos(ang)*spd; S->tank_es_vy[i]=fsin(ang)*spd;
        return;
    }
}
static void tank_boom(int pi, float x, float y){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    for (int i=0;i<6;i++) if (S->tank_boom_t[i]<=0){ S->tank_boom_x[i]=x; S->tank_boom_y[i]=y; S->tank_boom_t[i]=0.3f; break; }
    splat(x, y, COL(255,170,70), 18, 150); splat(x, y, COL(255,120,60), 10, 110); sfx(7);
    for (int j=0;j<S->tank_en_n;j++){
        TankEn *e=&S->tank_en[j];
        if (e->kind<0 || e->kind==3 || e->kind==6 || e->kind==7) continue;   // pickups are blast-proof: a stray burst never eats your medkit
        float dx=e->x-x, dy=e->y-y;
        if (dx*dx+dy*dy < 62*62){
            e->hp -= 15;
            if (e->kind==4) S->tank_boss_hp = e->hp>0? e->hp : 0;
            if (e->kind==5) S->tank_mid_hp = e->hp>0? e->hp : 0;
            if (e->hp<=0) tank_kill_en(pi, e);
        }
    }
    for (int ty2=0; ty2<TANK_LEN; ty2++) for (int tx2=0; tx2<TANK_W; tx2++){
        int t = tank_tile(S, tx2, ty2);
        if (t!=3 && t!=4) continue;
        float dx=tx2*BW_TILE+24-x, dy=ty2*BW_TILE+24-y;
        if (dx*dx+dy*dy < 62*62){
            int idx=ty2*TANK_W+tx2;
            if (t==3){ S->tank_dead[idx]=1; S->tank_score+=10; }
            else { S->tank_hit[idx]+=2; if (S->tank_hit[idx]>=2){ S->tank_dead[idx]=1; S->tank_score+=25; } }
        }
    }
    { float dx=b->x-x, dy=b->y-y; if (dx*dx+dy*dy < 42*42) tank_hurt(pi, 6); }   // mind your own splash
}
static void tank_update_shells(int pi, float dt){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    // grenades: lobbed, short fuse, area blast at the end of it
    for (int i=0;i<10;i++) if (S->tank_gr_a[i]){
        S->tank_gr_t[i]-=dt;
        S->tank_gr_x[i]+=S->tank_gr_vx[i]*dt; S->tank_gr_y[i]+=S->tank_gr_vy[i]*dt;
        int t=tank_tile(S, bw_tileof(S->tank_gr_x[i]), bw_tileof(S->tank_gr_y[i]));
        if (S->tank_gr_t[i]<=0 || t==2 || t==3 || t==4){
            S->tank_gr_a[i]=0; tank_boom(pi, S->tank_gr_x[i], S->tank_gr_y[i]);
        }
    }
    for (int i=0;i<6;i++) if (S->tank_boom_t[i]>0) S->tank_boom_t[i]-=dt;
    // your shells: over water and the gate, stopped by hard cover
    for (int i=0;i<24;i++) if (S->tank_sh_a[i]){
        S->tank_sh_t[i]-=dt; if (S->tank_sh_t[i]<=0){ S->tank_sh_a[i]=0; continue; }
        S->tank_sh_x[i]+=S->tank_sh_vx[i]*dt; S->tank_sh_y[i]+=S->tank_sh_vy[i]*dt;
        float sx=S->tank_sh_x[i], sy=S->tank_sh_y[i];
        int tx=bw_tileof(sx), ty=bw_tileof(sy);
        int t=tank_tile(S, tx, ty);
        if (t==2){ S->tank_sh_a[i]=0; splat(sx, sy, COL(190,190,200), 3, 60); continue; }
        if (t==3 || t==4){
            S->tank_sh_a[i]=0;
            int idx=ty*TANK_W+tx;
            if (t==3){ S->tank_dead[idx]=1; S->tank_score+=10; splat(sx, sy, COL(90,160,70), 6, 80); sfx(5); }
            else { S->tank_hit[idx]++; splat(sx, sy, COL(210,180,120), 5, 80); sfx(5);
                   if (S->tank_hit[idx]>=2){ S->tank_dead[idx]=1; S->tank_score+=25; } }
            continue;
        }
        for (int j=0;j<S->tank_en_n;j++){
            TankEn *e=&S->tank_en[j];
            if (e->kind<0 || e->kind==3 || e->kind==6 || e->kind==7) continue;   // shells fly over pickups, same as the wrench
            float rr = e->kind==4? 48.0f : e->kind==5? 30.0f : e->kind==2? 24.0f : 18.0f;
            float dx=sx-e->x, dy=sy-e->y;
            if (dx*dx+dy*dy < rr*rr){
                S->tank_sh_a[i]=0;
                e->hp -= 6;
                if (e->kind==4) S->tank_boss_hp = e->hp>0? e->hp : 0;
                if (e->kind==5) S->tank_mid_hp = e->hp>0? e->hp : 0;
                splat(sx, sy, COL(255,210,120), 4, 80); sfx(5);
                if (e->hp<=0) tank_kill_en(pi, e);
                break;
            }
        }
    }
    // their shells: hard cover works for you, too
    for (int i=0;i<32;i++) if (S->tank_es_a[i]){
        S->tank_es_t[i]-=dt; if (S->tank_es_t[i]<=0){ S->tank_es_a[i]=0; continue; }
        S->tank_es_x[i]+=S->tank_es_vx[i]*dt; S->tank_es_y[i]+=S->tank_es_vy[i]*dt;
        float sx=S->tank_es_x[i], sy=S->tank_es_y[i];
        int t=tank_tile(S, bw_tileof(sx), bw_tileof(sy));
        if (t==2 || t==3 || t==4){ S->tank_es_a[i]=0; continue; }
        float dx=sx-b->x, dy=sy-b->y;
        if (dx*dx+dy*dy < 20*20){ S->tank_es_a[i]=0; tank_hurt(pi, 8); }
    }
}
static void tank_update_foes(int pi, float dt){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    for (int j=0;j<S->tank_en_n;j++){
        TankEn *e=&S->tank_en[j];
        if (e->kind<0) continue;
        float dx=b->x-e->x, dy=b->y-e->y;
        float d2=dx*dx+dy*dy;
        if (e->kind==0){   // a turret: the barrel tracks you, the shots are aimed
            if (d2 < 430*430) e->aim = fatan2(dy, dx);
            e->ft -= dt;
            if (e->ft<=0 && d2 < 340*340){
                float cd = 1.5f - (float)S->tank_lvl*0.05f; if (cd<1.05f) cd=1.05f;
                e->ft = cd;
                tank_fire_eshell(S, e->x + fcos(e->aim)*20, e->y + fsin(e->aim)*20, e->aim, 335);
                sfx(2);
            }
        } else if (e->kind==5){   // the blockade gun: twin barrels, patient and heavy
            e->aim = fatan2(dy, dx);
            e->ft -= dt;
            if (e->ft<=0 && d2 < 540*540){
                float cd = 1.7f - (float)S->tank_lvl*0.03f; if (cd<1.3f) cd=1.3f;
                e->ft = cd;
                for (int s2=-1;s2<=1;s2+=2) tank_fire_eshell(S, e->x + (float)s2*10, e->y + 26, e->aim + (float)s2*0.09f, 320);
                sfx(2);
            }
        } else if (e->kind==1){   // a patrol tank, trundling its beat
            float sp = 55.0f;
            if (e->dir==1 || e->dir==-1){
                float nx2 = e->x + e->dir*sp*dt;
                if (nx2 > e->hi){ nx2 = e->hi; e->dir = -1; }
                else if (nx2 < e->lo){ nx2 = e->lo; e->dir = 1; }
                else if (tank_tile(S, bw_tileof(nx2 + e->dir*22), bw_tileof(e->y))!=0) e->dir = -e->dir;
                else e->x = nx2;
                e->aim = e->dir>0? 0 : PI;
            } else {
                float ny2 = e->y + (e->dir>0? sp : -sp)*dt;
                if (ny2 > e->hi){ ny2 = e->hi; e->dir = -2; }
                else if (ny2 < e->lo){ ny2 = e->lo; e->dir = 2; }
                else if (tank_tile(S, bw_tileof(e->x), bw_tileof(ny2 + (e->dir>0? 22 : -22)))!=0) e->dir = -e->dir;
                else e->y = ny2;
                e->aim = e->dir>0? PI/2 : -PI/2;
            }
            if (d2 < 30*30) tank_hurt(pi, 10);   // don't trade paint with a tank
            e->ft -= dt;
            if (e->ft<=0 && d2 < 320*320){
                e->ft = 1.8f;
                float ang = fatan2(dy, dx);
                tank_fire_eshell(S, e->x + fcos(ang)*22, e->y + fsin(ang)*22, ang, 330);
                sfx(2);
            }
        } else if (e->kind==4){   // the fortress core: slow triple shots from the south face
            e->aim = fatan2(dy, dx);
            e->ft -= dt;
            if (e->ft<=0 && d2 < 560*560){
                float cd = 2.45f - (float)S->tank_lvl*0.04f; if (cd<1.8f) cd=1.8f;
                e->ft = cd;
                for (int s2=-1;s2<=1;s2++) tank_fire_eshell(S, e->x, e->y + 42, e->aim + (float)s2*0.22f, 285);
                sfx(2);
            }
        }
    }
}
// THE MISSION CHAIN (Nov 11, Daniel: progression was unclear — now it
// reads like Jackal: one objective at a time, in order, with the yellow
// arrow always pointing at it). Phases: 0 rescue, 1 deliver, 2 break
// the blockade, 3 destroy the fortress core, 4 cross the finish line.
// The blockade barrier stays shut until the delivery quota is earned
// AND the gun is dead; the core waits behind it. Bosses in sequence.
static void tank_objective(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->tank_boss_hp<=0){ S->tank_obj=4; S->tank_obj_x=12.5f*BW_TILE; S->tank_obj_y=1.0f*BW_TILE; return; }
    if (S->tank_deliv >= S->tank_quota){
        if (S->tank_mid_hp>0){ S->tank_obj=2; S->tank_obj_x=12.5f*BW_TILE; S->tank_obj_y=44*BW_TILE+24;
            for (int j=0;j<S->tank_en_n;j++) if (S->tank_en[j].kind==5){ S->tank_obj_x=S->tank_en[j].x; S->tank_obj_y=S->tank_en[j].y; break; }
            return; }
        S->tank_obj=3; S->tank_obj_x=12.5f*BW_TILE; S->tank_obj_y=3.5f*BW_TILE; return; }
    if (S->tank_pass>0){ S->tank_obj=1; float bd=1e18f;
        for (int k2=0;k2<3;k2++){ float px=12.5f*BW_TILE, py=tank_pad_ty(k2)*BW_TILE+48;
            float dx=b->x-px, dy=b->y-py, d2=dx*dx+dy*dy; if (d2<bd){ bd=d2; S->tank_obj_x=px; S->tank_obj_y=py; } }
        return; }
    S->tank_obj=0; float bd=1e18f; int found=0;
    for (int j=0;j<S->tank_en_n;j++){ TankEn *e=&S->tank_en[j]; if (e->kind!=2) continue;
        float dx=b->x-e->x, dy=b->y-e->y, d2=dx*dx+dy*dy; if (d2<bd){ bd=d2; S->tank_obj_x=e->x; S->tank_obj_y=e->y; found=1; } }
    for (int i=0;i<24;i++) if (S->tank_cap_a[i]){
        float dx=b->x-S->tank_cap_x[i], dy=b->y-S->tank_cap_y[i], d2=dx*dx+dy*dy; if (d2<bd){ bd=d2; S->tank_obj_x=S->tank_cap_x[i]; S->tank_obj_y=S->tank_cap_y[i]; found=1; } }
    if (!found){ S->tank_obj=1; S->tank_obj_x=12.5f*BW_TILE; S->tank_obj_y=tank_pad_ty(0)*BW_TILE+48; }
}
static void tank_update(int pi, float dt, const BwIn *in){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (in->exit_){ bw_exit(pi, 0); return; }
    S->tank_t += dt;
    tank_objective(pi);
    if (S->tank_grace>0) S->tank_grace -= dt;
    if (S->tank_fire_cd>0) S->tank_fire_cd -= dt;
    if (S->tank_deadt>0){
        S->tank_deadt -= dt;
        tank_update_shells(pi, dt);   // the world keeps simmering while you burn
        if (S->tank_deadt<=0){
            if (S->tank_lives<=0){ bw_exit(pi, 0); return; }
            b->x = 12.5f*BW_TILE; b->y = S->tank_respawn_y;
            S->tank_hull = 100; S->tank_grace = 1.5f; S->tank_fx=0; S->tank_fy=-1;
            for (int i=0;i<32;i++) S->tank_es_a[i]=0;   // no welcome-back volley
        }
        return;
    }
    if (S->tank_cleart>0){
        S->tank_cleart += dt;
        b->y -= 60*dt; if (b->y<24) b->y=24;   // rolling over the line
        if (S->tank_cleart>1.6f){
            int lvl = S->tank_lvl;
            if (S->tank_pass>0){ b->xp += 6*S->tank_pass; S->tank_score += 150*S->tank_pass; }   // whoever's still aboard gets flown out
            b->xp += 10 + 4*lvl;
            buddy_award_xp(pi, 40);   // v1.19: a completed mission pays the buddy (on top of the mission's own small XP above)
            if (lvl > b->tank_done){ b->tank_done = lvl; }
            do_save();
            tank_load_level(pi, lvl+1);
            { char *q=S->note; q=sappend(q, "LEVEL "); q=sapp_int(q, lvl+1); q=sappend(q, " - THEY'RE READY FOR YOU"); if (b->active) q=sappend(q, " +40 BUDDY XP"); *q=0; }
            S->note_t=2.8f;
        }
        return;
    }
    // --- drive: 8-way, facing is wherever you last drove ---
    float mx=0, my=0; int k=in->keys;
    if (k&K_UP) my-=1; if (k&K_DOWN) my+=1; if (k&K_LEFT) mx-=1; if (k&K_RIGHT) mx+=1;
    if (mx!=0 || my!=0){
        float d=fsqrt(mx*mx+my*my); mx/=d; my/=d;
        S->tank_fx = mx; S->tank_fy = my;
        b->aim = fatan2(my, mx);
        float nx = b->x + mx*150.0f*dt;
        if (!tank_tank_blocked(S, nx, b->y)) b->x = nx;
        float ny = b->y + my*150.0f*dt;
        if (!tank_tank_blocked(S, b->x, ny)) b->y = ny;
        b->walk_ph += dt*9; b->mv_t = 0.15f;
    }
    // buildings and turrets are furniture: shove the tank out of their
    // circles (a push-out slides you along the wall, never a dead stop)
    for (int j=0;j<S->tank_en_n;j++){
        TankEn *e=&S->tank_en[j];
        if (e->kind<0 || e->kind==3 || e->kind==6 || e->kind==7) continue;   // pickups don't shove the tank — you roll straight over them
        float rr = e->kind==4? 50.0f : e->kind==5? 34.0f : e->kind==2? 26.0f : 20.0f;
        float dx=b->x-e->x, dy=b->y-e->y;
        float d2=dx*dx+dy*dy, want=(rr+16)*(rr+16);
        if (d2<want && d2>0.01f){ float dl=fsqrt(d2); b->x = e->x + dx/dl*(rr+16); b->y = e->y + dy/dl*(rr+16); }
    }
    if (b->x < 26) b->x = 26; if (b->x > TANK_W*BW_TILE-26) b->x = TANK_W*BW_TILE-26;
    if (b->y < 20) b->y = 20; if (b->y > TANK_LEN*BW_TILE-26) b->y = TANK_LEN*BW_TILE-26;
    // --- weapons: the cannon, or the grenade pouch (WALL button swaps) ---
    if (in->wall){ S->tank_wep ^= 1; sfx(6);
        scpy(S->note, S->tank_wep? "GRENADES OUT! LOBBED - AREA BLAST." : "CANNON UP!", 72); S->note_t=1.4f; }
    if ((in->sel_held || in->sel) && S->tank_fire_cd<=0){
        if (S->tank_wep==0){
            S->tank_fire_cd = 0.21f;
            for (int i=0;i<24;i++) if (!S->tank_sh_a[i]){
                S->tank_sh_a[i]=1; S->tank_sh_t[i]=0.75f;   // Jackal range: you must roll INTO their range to reach them
                S->tank_sh_x[i]=b->x + S->tank_fx*26; S->tank_sh_y[i]=b->y + S->tank_fy*26;
                S->tank_sh_vx[i]=S->tank_fx*520; S->tank_sh_vy[i]=S->tank_fy*520;
                sfx(4);
                break;
            }
        } else if (S->tank_gren>0 || tank_glauncher_st[pi]){
            // v1.27: with the GRENADE LAUNCHER aboard the pouch is
            // bottomless (no decrement, no empty block) and the lob
            // cycles faster; without it the pouch rules are untouched
            S->tank_fire_cd = tank_glauncher_st[pi]? 0.38f : 0.5f;
            if (!tank_glauncher_st[pi]) S->tank_gren--;
            for (int i=0;i<10;i++) if (!S->tank_gr_a[i]){
                S->tank_gr_a[i]=1; S->tank_gr_t[i]=0.62f;
                S->tank_gr_x[i]=b->x + S->tank_fx*26; S->tank_gr_y[i]=b->y + S->tank_fy*26;
                S->tank_gr_vx[i]=S->tank_fx*400; S->tank_gr_vy[i]=S->tank_fy*400;
                sfx(4);
                break;
            }
        } else if (S->note_t<=0){ scpy(S->note, "OUT OF GRENADES - DELIVER CAPTIVES, GRAB WRENCHES!", 72); S->note_t=1.8f; sfx(6); }
    }
    tank_update_foes(pi, dt);
    tank_update_shells(pi, dt);
    // --- repair pickups: drive over the wrench ---
    for (int j=0;j<S->tank_en_n;j++){
        TankEn *e=&S->tank_en[j];
        if (e->kind!=3) continue;
        float dx=b->x-e->x, dy=b->y-e->y;
        if (dx*dx+dy*dy < 26*26){
            e->kind=-1; S->tank_hull += 40; if (S->tank_hull>100) S->tank_hull=100;
            S->tank_gren += 2; if (S->tank_gren>12) S->tank_gren=12;
            scpy(S->note, "PATCHED UP! +40 HULL, +2 GRENADES", 72); S->note_t=2.2f; sfx(11);
        }
    }
    // --- v1.27 powerups: the medkit and the grenade launcher ---
    for (int j=0;j<S->tank_en_n;j++){
        TankEn *e=&S->tank_en[j];
        if (e->kind!=6 && e->kind!=7) continue;
        float dx=b->x-e->x, dy=b->y-e->y;
        if (dx*dx+dy*dy < 26*26){
            if (e->kind==6){
                e->kind=-1; S->tank_hull += 50; if (S->tank_hull>100) S->tank_hull=100;
                scpy(S->note, "MEDKIT! +50 HULL", 72); S->note_t=2.2f; sfx(11);
            } else {
                e->kind=-1;
                if (tank_glauncher_st[pi]){   // already packing it: the spare rack just tops the pouch up
                    S->tank_gren += 4; if (S->tank_gren>12) S->tank_gren=12;
                    scpy(S->note, "SPARE LAUNCHER RACK! +4 GRENADES", 72);
                } else {
                    tank_glauncher_st[pi]=1;   // for the REST OF THE RUN: level loads and respawns keep it, a fresh cabinet entry resets it
                    scpy(S->note, "GRENADE LAUNCHER! ENDLESS GRENADES!", 72);
                }
                S->note_t=2.4f; sfx(11);
            }
        }
    }
    // --- captives: four seats, no standing room ---
    for (int i=0;i<24;i++) if (S->tank_cap_a[i]){
        float dx=b->x-S->tank_cap_x[i], dy=b->y-S->tank_cap_y[i];
        if (dx*dx+dy*dy < 28*28){
            if (S->tank_pass>=4){ if (S->note_t<=0){ scpy(S->note, "TANK FULL! FIND A HELIPAD.", 72); S->note_t=1.6f; } }
            else { S->tank_cap_a[i]=0; S->tank_pass++; scpy(S->note, "CAPTIVE ABOARD!", 72); S->note_t=1.8f; sfx(19); }
        }
    }
    // --- helipads: drop-offs, and the respawn point marches north ---
    for (int k2=0;k2<3;k2++){
        int pt = tank_pad_ty(k2);
        float py0 = pt*BW_TILE;
        if (b->y < py0 + 96 && S->tank_respawn_y > py0 + 72) S->tank_respawn_y = py0 + 72;
        int ttx = bw_tileof(b->x), tty = bw_tileof(b->y);
        if (S->tank_pass>0 && ttx>=11 && ttx<=13 && tty>=pt && tty<pt+2){
            int n = S->tank_pass; S->tank_pass = 0;
            S->tank_gren += 2*n; if (S->tank_gren>12) S->tank_gren=12;   // the pad crew restocks your pouch
            int was = S->tank_deliv; S->tank_deliv += n;
            b->xp += 6*n; S->tank_score += 150*n;
            if (was < S->tank_quota && S->tank_deliv >= S->tank_quota){
                scpy(S->note, S->tank_mid_hp>0? "QUOTA MET! BREAK THE BLOCKADE UP NORTH." : "QUOTA MET! THE FORTRESS CORE IS NEXT.", 72);
            } else { char *q=S->note; q=sappend(q, "DELIVERED! +"); q=sapp_int(q, 6*n); q=sappend(q, " XP"); *q=0; }
            S->note_t=2.4f; sfx(11);
        }
    }
    // --- the finish line, once the core is dead ---
    if (S->tank_boss_hp<=0 && b->y < 2*BW_TILE){
        S->tank_cleart = 0.001f;
        scpy(S->note, "LEVEL CLEAR!", 72); S->note_t=2.6f; sfx(11);
    }
}
