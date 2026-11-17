// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: tank.c — TANK RESCUE (Banzai World mode 5) + the mission chain.
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
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
    b->x = 12.5f*BW_TILE; b->y = (TANK_LEN-3)*BW_TILE + 24;
    S->tank_respawn_y = b->y;
}
static void tank_enter(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->tank_score=0; S->tank_lives=3; S->tank_t=0;
    S->tank_wep=0; S->tank_gren=8;
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
    else if (kind==1) S->tank_score += 150;
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
        if (e->kind<0 || e->kind==3) continue;
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
            if (e->kind<0 || e->kind==3) continue;
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
        if (e->kind<0 || e->kind==3) continue;
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
        } else if (S->tank_gren>0){
            S->tank_fire_cd = 0.5f; S->tank_gren--;
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
static void tank_render(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    float camx = b->x, half = vw2/2.0f;
    if (camx < half) camx = half;
    if (camx > TANK_W*BW_TILE - half) camx = TANK_W*BW_TILE - half;
    float camy = b->y, halfy = vh2/2.0f;
    if (camy < halfy) camy = halfy;
    if (camy > TANK_LEN*BW_TILE - halfy) camy = TANK_LEN*BW_TILE - halfy;
    G.camx = camx; G.camy = camy;
    rect_fill(vx, vy, vw2, vh2, COL(26,28,22));
    int warm = (S->tank_lvl*3)%18;
    int tx0=bw_tileof(camx - half - BW_TILE), tx1=bw_tileof(camx + half + BW_TILE);
    int ty0=bw_tileof(camy - halfy - BW_TILE), ty1=bw_tileof(camy + halfy + BW_TILE);
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int x=SX(tx*BW_TILE), y=SY(ty*BW_TILE);
        int t = tank_tile(S, tx, ty);
        u32 h = hash2(tx, ty, (u32)S->tank_seed);
        if (ty<2 && tx>=0 && tx<TANK_W){   // the finish strip
            rect_fill(x, y, BW_TILE+1, BW_TILE+1, ((tx+ty)&1)? COL(232,232,238) : COL(30,30,36));
            continue;
        }
        if (ty<TANK_GATE_TY && tx>=0 && tx<TANK_W){   // the arena plaza
            rect_fill(x, y, BW_TILE+1, BW_TILE+1, ((tx+ty)&1)? COL(122,124,130) : COL(112,114,120));
            if (t==0) continue;
        } else {   // the field: two-tone scrub, warmed a step per level
            u32 g = ((tx+ty)&1)? COL(104+warm, 146, 66) : COL(95+warm, 137, 59);
            rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(g, 0.95f + (float)((h>>20)%10)*0.01f));
        }
        if (t==1){   // water
            rect_fill(x, y, BW_TILE+1, BW_TILE+1, ((tx+ty)&1)? COL(52,110,178) : COL(46,100,168));
            if (h % 7 == 0){ int wxp = x + (int)((h>>6) % 30) + 6, wyp = y + (int)((h>>11) % 32) + 8;
                line_thick(wxp, wyp, wxp+7, wyp, 1, COL(150,200,235)); }
        } else if (t==2){   // concrete
            rect_fill(x+2, y+2, BW_TILE-4, BW_TILE-4, COL(148,150,156));
            rect_fill(x+2, y+2, BW_TILE-4, 4, COL(172,174,180));
            rect_line(x+2, y+2, BW_TILE-4, BW_TILE-4, COL(96,98,104));
            disc_fill(x+7, y+7, 1, COL(104,106,112)); disc_fill(x+BW_TILE-8, y+BW_TILE-8, 1, COL(104,106,112));
        } else if (t==3){   // trees
            rect_fill(x+22, y+26, 4, 12, COL(110,76,44));
            disc_fill(x+16, y+18, 11, COL(52,116,52)); disc_fill(x+31, y+15, 10, COL(62,130,58));
            disc_fill(x+24, y+26, 11, COL(44,104,48)); disc_fill(x+19, y+13, 3, COL(120,180,110));
        } else if (t==4){   // sandbags (cracked once they've taken a hit)
            rect_fill(x+3, y+22, 20, 10, COL(198,168,110)); rect_fill(x+25, y+22, 20, 10, COL(188,158,102));
            rect_fill(x+13, y+12, 20, 10, COL(204,176,118)); rect_fill(x+8, y+32, 15, 9, COL(190,160,104));
            rect_fill(x+26, y+32, 15, 9, COL(196,166,108));
            if (S->tank_hit[ty*TANK_W+tx]>0){ line_thick(x+10, y+14, x+22, y+26, 1, COL(90,70,40)); line_thick(x+30, y+24, x+38, y+36, 1, COL(90,70,40)); }
        } else if (t==5){   // the gate: a striped steel band, shut tight
            rect_fill(x, y+14, BW_TILE+1, 20, COL(58,58,64));
            for (int s2=0;s2<6;s2++) rect_fill(x+s2*8, y+17, 4, 14, (s2&1)? COL(240,200,60) : COL(24,24,28));
            rect_fill(x, y+14, BW_TILE+1, 3, COL(90,90,98)); rect_fill(x, y+31, BW_TILE+1, 3, COL(90,90,98));
        }
    }
    // helipads
    for (int k2=0;k2<3;k2++){
        int pt = tank_pad_ty(k2);
        int x=SX(11*BW_TILE), y=SY(pt*BW_TILE), w=3*BW_TILE, hgt=2*BW_TILE;
        if (x+w<vx || x>vx+vw2 || y+hgt<vy || y>vy+vh2) continue;
        rect_fill(x, y, w, hgt, COL(52,54,58));
        rect_line(x+2, y+2, w-4, hgt-4, COL(240,200,60));
        draw_text_center(x+w/2, y+hgt/2-12, "H", 3, COL(240,200,60));
    }
    // entities
    for (int j=0;j<S->tank_en_n;j++){
        TankEn *e=&S->tank_en[j];
        if (e->kind<0) continue;
        int x=SX(e->x), y=SY(e->y);
        if (x<vx-70 || x>vx+vw2+70 || y<vy-70 || y>vy+vh2+70) continue;
        if (e->kind==3){   // the wrench, bobbing on its box
            int bob = (int)(fsin(S->tank_t*3 + j)*2.0f);
            shadow_ellipse(x, y+10, 10, 3);
            rect_fill(x-10, y-8+bob, 20, 16, COL(238,240,244)); rect_line(x-10, y-8+bob, 20, 16, COL(150,60,60));
            rect_fill(x-2, y-6+bob, 4, 12, COL(200,50,50)); rect_fill(x-6, y-2+bob, 12, 4, COL(200,50,50));
        } else if (e->kind==2){   // a prison hut
            shadow_ellipse(x, y+16, 24, 6);
            rect_fill(x-22, y-8, 44, 26, COL(146,104,62)); rect_line(x-22, y-8, 44, 26, COL(96,66,36));
            tri_fill(x, y-24, x-26, y-8, x+26, y-8, COL(120,62,44));
            rect_fill(x-5, y+2, 10, 16, COL(74,50,30));
            rect_fill(x+10, y-4, 9, 9, COL(40,36,44));
            rect_fill(x+12, y-4, 1, 9, COL(190,196,204)); rect_fill(x+15, y-4, 1, 9, COL(190,196,204)); rect_fill(x+18, y-4, 1, 9, COL(190,196,204));
            if (e->hp<40){ int w = 40*e->hp/40; rect_fill(x-20, y-30, 40, 4, COL(40,20,20)); rect_fill(x-20, y-30, w, 4, COL(240,120,70)); }
        } else if (e->kind==0){   // a turret: base, dome, a barrel that watches you
            shadow_ellipse(x, y+10, 16, 5);
            disc_fill(x, y, 15, COL(104,108,98)); circle_out(x, y, 15, COL(70,72,64));
            line_thick(x, y, x + (int)(fcos(e->aim)*23), y + (int)(fsin(e->aim)*23), 4, COL(52,54,48));
            disc_fill(x, y, 8, COL(126,130,118)); disc_fill(x, y, 3, COL(220,60,50));
            { int mx = 30 + S->tank_lvl*4; if (e->hp<mx){ int w = 28*e->hp/mx; if (w<0) w=0; rect_fill(x-14, y-22, 28, 3, COL(40,20,20)); rect_fill(x-14, y-22, w, 3, COL(240,120,70)); } }
        } else if (e->kind==1){   // a patrol tank
            shadow_ellipse(x, y+11, 18, 5);
            int horiz = (e->dir==1 || e->dir==-1);
            if (horiz){ rect_fill(x-17, y-12, 34, 7, COL(40,40,44)); rect_fill(x-17, y+5, 34, 7, COL(40,40,44));
                        rect_fill(x-14, y-8, 28, 16, COL(128,84,60)); }
            else { rect_fill(x-12, y-17, 7, 34, COL(40,40,44)); rect_fill(x+5, y-17, 7, 34, COL(40,40,44));
                   rect_fill(x-8, y-14, 16, 28, COL(128,84,60)); }
            line_thick(x, y, x + (int)(fcos(e->aim)*22), y + (int)(fsin(e->aim)*22), 4, COL(70,50,36));
            disc_fill(x, y, 6, COL(146,100,70));
            { int mx = 40 + S->tank_lvl*5; if (e->hp<mx){ int w = 32*e->hp/mx; if (w<0) w=0; rect_fill(x-16, y-22, 32, 3, COL(40,20,20)); rect_fill(x-16, y-22, w, 3, COL(240,120,70)); } }
        } else if (e->kind==5){   // the blockade gun: a sandbagged bunker squatting on the road
            shadow_ellipse(x, y+20, 34, 8);
            rect_fill(x-32, y-16, 64, 34, COL(120,96,58)); rect_line(x-32, y-16, 64, 34, COL(84,66,38));
            rect_fill(x-32, y-16, 64, 7, COL(198,168,110)); rect_fill(x-32, y+11, 64, 7, COL(198,168,110));
            line_thick(x-8, y+6, x-8 + (int)(fcos(e->aim)*26), y+6 + (int)(fsin(e->aim)*26), 4, COL(52,54,48));
            line_thick(x+8, y+6, x+8 + (int)(fcos(e->aim)*26), y+6 + (int)(fsin(e->aim)*26), 4, COL(52,54,48));
            disc_fill(x, y-4, 9, COL(126,130,118)); disc_fill(x, y-4, 3, COL(220,60,50));
            draw_text_center(x, y-30, "BLOCKADE", 1, COL(255,220,150));
            if (S->tank_mid_max>0 && e->hp<S->tank_mid_max){ int w = 56*e->hp/S->tank_mid_max; if (w<0) w=0; rect_fill(x-28, y+22, 56, 4, COL(40,20,20)); rect_fill(x-28, y+22, w, 4, COL(240,120,70)); }
        } else if (e->kind==4){   // the fortress core
            shadow_ellipse(x, y+40, 48, 12);
            rect_fill(x-46, y-42, 92, 84, COL(96,100,108)); rect_line(x-46, y-42, 92, 84, COL(60,62,68));
            rect_fill(x-46, y-42, 92, 8, COL(120,124,132)); rect_fill(x-46, y+6, 92, 4, COL(74,76,84));
            disc_fill(x-34, y-30, 6, COL(70,72,78)); disc_fill(x+34, y-30, 6, COL(70,72,78));
            disc_fill(x-34, y+30, 6, COL(70,72,78)); disc_fill(x+34, y+30, 6, COL(70,72,78));
            line_thick(x, y+10, x + (int)(fcos(e->aim)*34), y + 10 + (int)(fsin(e->aim)*34), 6, COL(52,54,60));
            { float pulse = 0.75f + 0.25f*fsin(S->tank_t*4);
              disc_fill(x, y, 19, COL(140,60,60)); disc_fill(x, y, (int)(12*pulse)+4, COL(255,90,70));
              disc_fill(x-4, y-4, 3, COL(255,200,170)); }
            draw_text_center(x, y-58, "FORTRESS CORE", 1, COL(255,170,150));
        }
    }
    // captives, standing free and waving
    for (int i=0;i<24;i++) if (S->tank_cap_a[i]){
        int x=SX(S->tank_cap_x[i]), y=SY(S->tank_cap_y[i]);
        if (x<vx-30 || x>vx+vw2+30 || y<vy-30 || y>vy+vh2+30) continue;
        shadow_ellipse(x, y+8, 7, 2);
        rect_fill(x-3, y-4, 7, 10, COL(240,140,40));   // the orange suit
        disc_fill(x, y-8, 4, COL(238,196,150));
        int wv = (int)(fsin(G.anim_t*7 + i*1.3f)*4.0f);
        line_thick(x+3, y-3, x+8+wv, y-10, 2, COL(238,196,150));
        line_thick(x-3, y-3, x-6, y+2, 2, COL(238,196,150));
    }
    // shells
    for (int i=0;i<24;i++) if (S->tank_sh_a[i]){
        int x=SX(S->tank_sh_x[i]), y=SY(S->tank_sh_y[i]);
        disc_fill(x, y, 3, COL(255,240,170)); disc_fill(x, y, 1, COL(255,255,255));
    }
    for (int i=0;i<32;i++) if (S->tank_es_a[i]){
        int x=SX(S->tank_es_x[i]), y=SY(S->tank_es_y[i]);
        disc_fill(x, y, 3, COL(255,90,60)); disc_fill(x, y, 1, COL(255,180,140));
    }
    // grenades in flight, and their blasts
    for (int i=0;i<10;i++) if (S->tank_gr_a[i]){
        int x=SX(S->tank_gr_x[i]), y=SY(S->tank_gr_y[i]);
        disc_fill(x, y+2, 4, COL(20,30,16));
        disc_fill(x, y, 4, COL(52,84,44)); disc_fill(x-1, y-1, 1, COL(200,230,180));
    }
    for (int i=0;i<6;i++) if (S->tank_boom_t[i]>0){
        int x=SX(S->tank_boom_x[i]), y=SY(S->tank_boom_y[i]);
        int rr = (int)((0.3f - S->tank_boom_t[i]) * 260.0f) + 8;
        circle_out(x, y, rr, COL(255,190,90)); circle_out(x, y, rr*2/3 + 2, COL(255,130,60));
        disc_fill(x, y, rr/3 + 2, COL(255,220,150));
    }
    // your tank (gone while it burns)
    if (S->tank_deadt<=0 && !(S->tank_grace>0.6f && ((int)(S->tank_t*14)&1))){
        int x=SX(b->x), y=SY(b->y);
        float fx=S->tank_fx, fy=S->tank_fy, px=-fy, py=fx;
        shadow_ellipse(x, y+11, 19, 6);
        for (int s2=-1;s2<=1;s2+=2){   // treads, marks scrolling with the walk phase
            float cx2 = x + px*13*s2, cy2 = y + py*13*s2;
            line_thick((int)(cx2 - fx*15), (int)(cy2 - fy*15), (int)(cx2 + fx*15), (int)(cy2 + fy*15), 7, COL(38,38,42));
            for (int m=0;m<3;m++){
                int off = -12 + ((m*10 + (int)(b->walk_ph*8)) % 30);
                int mxp = (int)(cx2 + fx*off), myp = (int)(cy2 + fy*off);
                line_thick((int)(mxp - px*3), (int)(myp - py*3), (int)(mxp + px*3), (int)(myp + py*3), 2, COL(74,74,80));
            }
        }
        disc_fill(x, y, 16, COL(96,110,52)); circle_out(x, y, 16, COL(64,74,36));
        disc_fill(x, y, 12, COL(110,126,60));
        // the passengers ride behind the turret, heads up
        for (int p2=0;p2<S->tank_pass;p2++){
            int hx2 = (int)(x - fx*11 + px*(p2*7 - 10)), hy2 = (int)(y - fy*11 + py*(p2*7 - 10));
            disc_fill(hx2, hy2, 3, COL(238,196,150));
        }
        line_thick((int)(x + fx*6), (int)(y + fy*6), (int)(x + fx*27), (int)(y + fy*27), 4, COL(52,60,34));
        disc_fill(x, y, 8, COL(80,94,44));
        disc_fill(x, y, 5, bud_coat(b, COL(199,152,94)));   // the driver's head, out of the hatch
    }
    // HUD
    rect_fill(vx, vy, vw2, 20, COL(16,20,28));
    { char buf[96]; char *q=buf;
      q=sappend(q, "LEVEL "); q=sapp_int(q, S->tank_lvl);
      q=sappend(q, "   SCORE "); q=sapp_int(q, S->tank_score);
      q=sappend(q, "   LIVES "); q=sapp_int(q, S->tank_lives);
      q=sappend(q, "   ABOARD "); q=sapp_int(q, S->tank_pass); q=sappend(q, "/4");
      *q=0; draw_text(vx+8, vy+6, buf, 1, COL(240,244,250)); }
    { char buf[96]; char *q=buf;
      if (S->tank_wep==0){ q=sappend(q, "WEAPON: CANNON  ("); q=sappend(q, lbl_wall()); q=sappend(q, ": GRENADES x"); }
      else { q=sappend(q, "WEAPON: GRENADE x"); }
      q=sapp_int(q, S->tank_gren);
      if (S->tank_wep) { q=sappend(q, "  ("); q=sappend(q, lbl_wall()); q=sappend(q, ": CANNON)"); }
      else q=sappend(q, ")");
      *q=0; draw_text(vx+8, vy+24, buf, 1, S->tank_wep? COL(255,190,90) : COL(170,190,200)); }
    { int bw3 = 120, bx = vx+vw2-bw3-10, by = vy+6;
      draw_text(bx-32, by, "HULL", 1, COL(240,244,250));
      rect_fill(bx-1, by-1, bw3+2, 9, COL(8,8,12));
      int hw = S->tank_hull*bw3/100; if (hw<0) hw=0;
      rect_fill(bx, by, hw, 7, S->tank_hull>50? COL(90,200,90) : S->tank_hull>25? COL(240,190,70) : COL(240,80,70)); }
    if (S->tank_boss_hp>0 && b->y < 16*BW_TILE){   // the core's health, once you're in its shadow
        int bw3 = 260, bx = vx+(vw2-bw3)/2, by = vy+26;
        draw_text_center(vx+vw2/2, by-11, "FORTRESS CORE", 1, COL(255,170,150));
        rect_fill(bx-1, by-1, bw3+2, 8, COL(8,8,12));
        int hw = S->tank_boss_max>0? S->tank_boss_hp*bw3/S->tank_boss_max : 0; if (hw<0) hw=0;
        rect_fill(bx, by, hw, 6, COL(240,80,70));
    }
    if (S->tank_mid_hp>0 && b->y < 58*BW_TILE){   // the blockade's health, once you're in its shadow
        int bw3 = 260, bx = vx+(vw2-bw3)/2, by = vy+26;
        draw_text_center(vx+vw2/2, by-11, "BLOCKADE GUN", 1, COL(255,220,150));
        rect_fill(bx-1, by-1, bw3+2, 8, COL(8,8,12));
        int hw = S->tank_mid_max>0? S->tank_mid_hp*bw3/S->tank_mid_max : 0; if (hw<0) hw=0;
        rect_fill(bx, by, hw, 6, COL(240,170,70));
    }
    // the mission line: one objective, always on screen, always current
    { const char *ob = S->tank_obj==0? "RESCUE THE CAPTIVES" : S->tank_obj==1? "DELIVER TO A HELIPAD" :
                       S->tank_obj==2? "BREAK THE BLOCKADE!" : S->tank_obj==3? "DESTROY THE FORTRESS CORE!" : "CROSS THE FINISH LINE!";
      char buf[96]; char *q=buf; q=sappend(q, "OBJECTIVE: "); q=sappend(q, ob);
      if (S->tank_obj<=1){ q=sappend(q, "  (DELIVERED "); q=sapp_int(q, S->tank_deliv); q=sappend(q, "/"); q=sapp_int(q, S->tank_quota); q=sappend(q, ")"); }
      *q=0;
      rect_fill(vx, vy+38, vw2, 16, COL(16,20,28));
      draw_text_center(vx+vw2/2, vy+42, buf, 1, COL(255,220,90)); }
    // the yellow arrow: a bouncing chevron over the target when it's on
    // screen, a bold pointer pinned to the screen edge when it isn't
    if (S->tank_deadt<=0){
        float txp = (float)SX(S->tank_obj_x), typ = (float)SY(S->tank_obj_y);
        float pxp = (float)SX(b->x), pyp = (float)SY(b->y);
        int onscr = txp>vx+16 && txp<vx+vw2-16 && typ>vy+104 && typ<vy+vh2-16;
        if (onscr){ int bob = (int)(fsin(S->tank_t*5)*3.0f); int ay = (int)typ - 42 + bob;
            tri_fill((int)txp, ay+11, (int)txp-7, ay, (int)txp+7, ay, COL(255,220,90)); }
        else { float ang = fatan2(typ-pyp, txp-pxp);
            float ax = txp, ay = typ;
            if (ax<vx+30) ax=(float)vx+30; if (ax>vx+vw2-30) ax=(float)vx+vw2-30;
            if (ay<vy+66) ay=(float)vy+66; if (ay>vy+vh2-30) ay=(float)vy+vh2-30;
            float ca=fcos(ang), sa=fsin(ang), pxx=-sa, pyy=ca;
            tri_fill((int)(ax+ca*14), (int)(ay+sa*14), (int)(ax-ca*10+pxx*8), (int)(ay-sa*10+pyy*8), (int)(ax-ca*10-pxx*8), (int)(ay-sa*10-pyy*8), COL(24,20,8));
            tri_fill((int)(ax+ca*11), (int)(ay+sa*11), (int)(ax-ca*8+pxx*6), (int)(ay-sa*8+pyy*6), (int)(ax-ca*8-pxx*6), (int)(ay-sa*8-pyy*6), COL(255,220,90)); }
    }
    if (S->tank_cleart>0) draw_text_center(vx+vw2/2, vy+vh2/3, "LEVEL CLEAR!", 2, COL(255,244,180));
    if (S->note_t>0){
        rect_fill(vx, vy+vh2-22, vw2, 22, COL(14,14,20));
        draw_text_center(vx+vw2/2, vy+vh2-15, S->note, 1, COL(255,240,200));
    }
}

