// CRISPY DIRT (bw_mode 10, v1.35) — the Scorched Earth cabinet.
// Daniel's commission (Nov 25, 2026): a clone of the DOS artillery
// classic, reskinned for the game — your buddy works a catapult on
// one dirt hill, a battery of bugs digs in on the other, and the two
// sides trade lobbed carcasses over destructible terrain. Angle,
// power, wind; a best-of-3 match; a session-only bits purse and a
// between-rounds shop; EASY / NORMAL / HARD paying Buddy XP 30 / 50
// / 80 at a solo match win; HARD carries the Trophy Case plaque; a
// hot-seat 2P puts a second buddy on the far catapult for glory
// only (no XP, no trophy — the Mordor rule).
//
// THE FIELD is a 640-wide heightfield, one surface height per
// column, generated per round from a hashed seed (layered sines in
// unsigned-hash phases, a guaranteed valley in the middle). Every
// explosion carves it for real; every unit re-settles afterwards —
// ground gone means a fall, a long fall means damage, and a unit
// buried past rescue pops back to the surface so a turn can never
// soft-lock. Terrain generation and every hash here run on unsigned
// math with bounded loops; the projectile integrator is fixed-
// substep, and the AI's solver simulates with the same constants
// the live flight uses, so what it aims is what it flies.
//
// SESSION-ONLY: the match, the bits and the bought arsenal live in
// BwSim scratch and die with the cabinet visit (leaving mid-match
// forfeits). The only things that outlive a match are the XP paid
// once at the end of a won solo match and the Buddy's scr_champ
// flag (the "sc" save token), set on a HARD solo win.
#define SCR_W 640
#define SCR_GRAV 260.0f
enum { SP_AIM=0, SP_FLY=1, SP_SETTLE=2, SP_SHOP=3, SP_BANNER=4, SP_END=5 };
enum { SW_CAR=0, SW_TRA=1, SW_BIG=2, SW_DIRT=3, SW_HON=4, SW_CLU=5, SW_BEAN=6, SW_FW=7 };
static const int SCR_PRICE[8] = { 0, 5, 40, 30, 70, 90, 150, 80 };
static const int SCR_RAD[8]   = { 26, 4, 42, 30, 10, 16, 0, 0 };   // the Bean's radius is rolled per shot; the FIREWALL is not a shell at all
static const int SCR_DMG[8]   = { 70, 0, 125, 0, 0, 45, 0, 0 };    // Cluster = per submunition; Honey damages by pool ticks, Dirt not at all
#define SCR_FW_STR 100   // the FIREWALL's strength (v1.36): blast points the dome eats before the unit's hp feels a thing
static const char *SCR_WNAME[8]  = { "CARCASS", "TRACER CARCASS", "BIG CARCASS", "DIRT CLOD", "HONEY POT", "CLUSTER CARCASS", "BACKFIRE BEAN", "FIREWALL" };
static const char *SCR_WSHORT[7] = { "CARCASS", "TRACER", "BIG", "DIRT", "HONEY", "CLUSTER", "BEAN" };
static const char *SCR_WDESC[8]  = { "THE CLASSIC LOB. FREE, FOREVER.",
                                     "A SCOUT SHOT - NO DAMAGE, ALL TRUTH.",
                                     "A WHOPPER. BIG CRATER, BIG HURT.",
                                     "ADDS DIRT. BUILD A WALL, FILL A GRAVE.",
                                     "LEAVES A BURNING POOL. HURTS PER TURN.",
                                     "SPLITS INTO THREE AT THE TOP.",
                                     "THE FUNKY ONE. RANDOM YIELD. MIND THE DUD.",
                                     "A DOME THAT EATS BLASTS. BLOCKS BLASTS, NOT FALLS." };
static const int SCR_XP[3] = { 30, 50, 80 };
static const char *SCR_DNAME[3] = { "EASY", "NORMAL", "HARD" };

// ---- shared geometry: the control bar, the weapon chips, the shop
// rows and the view transform live here so the update side (input)
// and the render side can never disagree about a single pixel.
static void scr_layout(int vx, int vy, int vw2, int vh2, int *bar_y, int *ax, int *aw, int *px, int *pw, int *fx, int *fy){
    *bar_y = vy + vh2 - 46;
    *ax = vx + 56; *aw = vw2*30/100; if (*aw<80) *aw=80;
    *px = *ax + *aw + 58; *pw = vw2*24/100; if (*pw<70) *pw=70;
    *fx = vx + vw2 - 58; *fy = *bar_y + 1;
}
static void scr_chip_rect(int vx, int vy, int vw2, int vh2, int i, int *x, int *y, int *w, int *h){
    int cw = (vw2-16)/7;
    *x = vx + 8 + i*cw; *w = cw-3; *y = vy + vh2 - 46 - 24; *h = 20;
}
static void scr_shop_rect(int vx, int vy, int vw2, int vh2, int *x, int *y, int *w, int *h){
    *w = vw2<420? vw2-16 : 404; *h = 276;   // h grew 254 -> 276 in v1.36: the eighth row is the FIREWALL
    *x = vx + (vw2-*w)/2; *y = vy + (vh2-*h)/2; if (*y<vy+4) *y = vy+4;
}
static void scr_shop_row_rect(int vx, int vy, int vw2, int vh2, int r, int *x, int *y, int *w, int *h){
    int px0, py0, pw2, ph2; scr_shop_rect(vx, vy, vw2, vh2, &px0, &py0, &pw2, &ph2);
    *x = px0+12; *w = pw2-24; *y = py0 + 62 + r*22; *h = 19;
}
// Field (640x360) -> view transform: uniform scale, centred. At the
// native 640x360 view this is exactly 1:1.
static void scr_view_xf(int vx, int vy, int vw2, int vh2, float *ox, float *oy, float *sc){
    float s = vw2/640.0f; float s2 = vh2/360.0f; if (s2<s) s = s2;
    *sc = s; *ox = vx + (vw2 - 640*s)*0.5f; *oy = vy + (vh2 - 360*s)*0.5f;
}

static int scr_side_ai(BwSim *S, int side){ return !S->scr_2p && side==1; }
static int scr_unit_of(BwSim *S, int side){ return side==0? 0 : 1 + S->scr_ui[1]; }
static int scr_side_alive(BwSim *S, int side){
    int n=0; for (int i=0;i<S->scr_nu;i++) if (S->scr_u[i].alive && S->scr_u[i].side==side) n++;
    return n;
}
static float scr_muzzle_v(int power){ return 60.0f + power*3.4f; }
static void scr_roll_wind(BwSim *S){
    S->scr_wind = (int)((hash2(S->scr_shots*7+3, S->scr_round*31 + S->scr_turn*17, (u32)S->scr_seed)>>8)%61) - 30;
}
static int scr_weap_ok(BwSim *S, int side, int w){
    if (w<=0) return 0;
    return S->scr_inv[side][w]>0? w : 0;
}

static void scr_fx_burst(BwSim *S, float x, float y, int n, u32 col, float spd){
    for (int k=0;k<n;k++){
        int i = S->scr_fxn%40; S->scr_fxn++;
        u32 h = hash2((int)(x*3)+k*7, (int)(y*3)+k*13, (u32)(S->scr_shots*31+k+5));
        float a = (h%628)/100.0f, s2 = spd*(0.3f + ((h>>8)%70)/100.0f);
        S->scr_fxx[i]=x; S->scr_fxy[i]=y;
        S->scr_fxv[i]=fcos(a)*s2; S->scr_fyv[i]=fsin(a)*s2 - 40.0f;
        S->scr_fxt[i]=0; S->scr_fxl[i]=0.5f + ((h>>16)%40)/100.0f; S->scr_fxc[i]=col;
    }
}
static void scr_fx_tick(BwSim *S, float dt){
    for (int i=0;i<40;i++) if (S->scr_fxt[i]<S->scr_fxl[i]){
        S->scr_fxt[i]+=dt;
        float dr = 1.0f - 1.8f*dt; if (dr<0) dr=0;
        S->scr_fxv[i]*=dr; S->scr_fyv[i]+=300.0f*dt;
        S->scr_fxx[i]+=S->scr_fxv[i]*dt; S->scr_fxy[i]+=S->scr_fyv[i]*dt;
    }
}

// ---- terrain ------------------------------------------------------
static void scr_gen_field(int pi, int round){
    BwSim *S = &BWS[pi];
    u32 sd = (u32)(S->scr_seed + round*7919);
    float p1 = hash_f(11, 7, sd)*6.28318f, p2 = hash_f(23, 13, sd)*6.28318f, p3 = hash_f(37, 29, sd)*6.28318f;
    float a1 = 26.0f + hash_f(41, 3, sd)*16.0f, a2 = 12.0f + hash_f(53, 9, sd)*11.0f, a3 = 4.0f + hash_f(67, 21, sd)*5.0f;
    for (int x=0;x<SCR_W;x++){
        float y = 214.0f + a1*fsin(x*0.0105f+p1) + a2*fsin(x*0.0237f+p2) + a3*fsin(x*0.0513f+p3);
        float d = (x-320)/120.0f; if (d<0) d=-d;
        if (d<1.0f) y += 15.0f*(1.0f + fcos(d*3.14159f));   // the guaranteed valley between the hills
        if (y<148) y=148; if (y>276) y=276;
        S->scr_h[x]=y;
    }
    // Rosters. Side 0 is always the one catapult; side 1 is the bug
    // battery (1/2/3 by difficulty) or the rival catapult in 2P.
    S->scr_nu = 0;
    S->scr_u[0].x = 84; S->scr_u[0].side = 0; S->scr_u[0].kind = 0;
    S->scr_nu = 1;
    if (S->scr_2p){
        S->scr_u[1].x = 556; S->scr_u[1].side = 1; S->scr_u[1].kind = 0; S->scr_nu = 2;
    } else {
        static const int BX[3][3] = { {556,-1,-1}, {556,486,-1}, {556,492,436} };
        int n = S->scr_diff+1;
        for (int i=0;i<n;i++){
            S->scr_u[1+i].x = BX[S->scr_diff][i];
            S->scr_u[1+i].side = 1;
            S->scr_u[1+i].kind = 1 + (i%2);   // crawlers and spitters alternate
        }
        S->scr_nu = 1+n;
    }
    for (int i=0;i<S->scr_nu;i++){
        S->scr_u[i].hp = 100; S->scr_u[i].alive = 1;
        S->scr_fall_from[i]=0; S->scr_fall_v[i]=0;
    }
    // A flattened pad under every unit (blend radius 20), so nobody
    // starts the round balanced on a spike.
    for (int i=0;i<S->scr_nu;i++){
        int cx = (int)S->scr_u[i].x;
        float base = S->scr_h[cx];
        for (int dx=-20;dx<=20;dx++){
            int c = cx+dx; if (c<0||c>=SCR_W) continue;
            float w = (20 - (dx<0? -dx : dx))/20.0f;
            S->scr_h[c] = S->scr_h[c]*(1.0f-w) + base*w;
        }
        S->scr_u[i].y = S->scr_h[cx];
    }
    for (int k=0;k<4;k++) S->scr_pool_left[k]=0;
    for (int i=0;i<3;i++) S->scr_pact[i]=0;
}

static void scr_start_round(int pi){
    BwSim *S = &BWS[pi];
    scr_gen_field(pi, S->scr_round);
    S->scr_turn = (S->scr_round-1)%2;   // the opening shot alternates by round
    S->scr_ui[0]=0; S->scr_ui[1]=0;
    scr_roll_wind(S);
    S->scr_phase = SP_BANNER; S->scr_phase_t = 1.6f; S->scr_settle_stage = 0;   // settle_stage doubles as the banner kind: 0 = round start
    S->scr_exit_arm = 0;
}

static void scr_shop_begin(int pi);   // below (the shop section)
static void scr_enter(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    int req = S->scr_req;   // consumed from the cabinet panel by bw_enter_mode
    S->scr_2p = (req==3);
    S->scr_diff = S->scr_2p? 1 : (req<0? 0 : (req>2? 2 : req));
    S->scr_matches++;
    S->scr_seed = (int)hash2(b->bw_seed, S->scr_matches*131+7, 0x5C09u);
    S->scr_round = 1; S->scr_wins[0]=0; S->scr_wins[1]=0;
    S->scr_bits[0]=100; S->scr_bits[1]=100;   // the starting purse (v1.36, Daniel: 100 a side — a Firewall and change, or a Honey Pot and a Dirt Clod; never the Bean outright)
    for (int s=0;s<2;s++){ for (int w=0;w<7;w++) S->scr_inv[s][w]=0; S->scr_weap[s]=0; S->scr_aim[s]=45.0f; S->scr_pow[s]=62.0f; }
    for (int i=0;i<4;i++) S->scr_u[i].sh = 0;   // v1.36: shields live and die with the match — a new match starts bare (scr_gen_field never touches sh, so a bought dome survives every round change)
    S->scr_ui[0]=0; S->scr_ui[1]=0;
    S->scr_shots=0; S->scr_lastmiss=0; S->scr_xp_paid=0; S->scr_xp_amt=0; S->scr_fxn=0; S->scr_exit_arm=0;
    for (int i=0;i<40;i++) S->scr_fxl[i]=0;
    for (int i=0;i<6;i++){ S->scr_tp_was[i]=0; S->scr_tp_act[i]=0; }
    // v1.36 (Daniel): the shop opens BEFORE round 1. Round 1's own
    // hills are generated now so the panel stands over the field the
    // match will open on (scr_start_round re-rolls the identical
    // deterministic field when the shop closes), the battery shops
    // silently inside scr_shop_begin, and scr_preshop tells
    // scr_shop_done to start round 1 instead of advancing it.
    scr_gen_field(pi, 1);
    S->scr_preshop = 1;
    scr_shop_begin(pi);
    S->scr_phase_t = 1.0f;   // the launch gate (v1.36 fix): NOT a timer — a latch. scr_update keeps the pre-round shop's input closed until the press that launched the cabinet is fully released, however long that takes. A timed hold raced real fingers and test harnesses alike: an up/down/select edge landing inside the window was eaten whole, and the shop desynced from the hand working it.
}

// ---- firing and flight ---------------------------------------------
static void scr_fire(int pi){
    BwSim *S = &BWS[pi];
    int side = S->scr_turn;
    ScrUnit *u = &S->scr_u[scr_unit_of(S, side)];
    int w = S->scr_weap[side];
    if (w<0 || w>6) w = 0;
    if (w>0){
        if (S->scr_inv[side][w]<=0){ w = 0; S->scr_weap[side]=0; }
        else S->scr_inv[side][w]--;
    }
    S->scr_pweap = w; S->scr_pside = side;
    float th = S->scr_aim[side]*3.14159f/180.0f;
    float dirx = side==0? 1.0f : -1.0f;
    float v = scr_muzzle_v((int)S->scr_pow[side]);
    for (int i=0;i<3;i++) S->scr_pact[i]=0;
    S->scr_px[0] = u->x + dirx*fcos(th)*18.0f;
    S->scr_py[0] = (u->y-12.0f) - fsin(th)*18.0f;
    S->scr_pvx[0] = dirx*v*fcos(th);
    S->scr_pvy[0] = -v*fsin(th);
    S->scr_pact[0]=1; S->scr_psplit[0] = (w==SW_CLU)? 1 : 0;
    if (w==SW_BEAN){
        // The Funky Bomb homage: the yield is rolled at the muzzle,
        // in the open (the HUD names it when it lands). 15% duds.
        u32 h = hash2(S->scr_shots*31 + side*7, S->scr_round*17, (u32)S->scr_seed);
        if ((h%100)<15){ S->scr_bean_r=8; S->scr_bean_d=12; }
        else { S->scr_bean_r = 36 + (int)((h>>8)%45); S->scr_bean_d = 70 + (int)((h>>16)%106); }
    }
    S->scr_shots++;
    S->scr_phase = SP_FLY;
    sfx(4);
}

static void scr_carve(BwSim *S, float x, float y, int r){
    int cx = (int)x;
    for (int c=cx-r;c<=cx+r;c++){
        if (c<0||c>=SCR_W) continue;
        float dx = (float)(c-cx);
        float dy = fsqrt((float)(r*r) - dx*dx);
        float bot = y + dy;
        if (bot>352) bot=352;
        if (S->scr_h[c] < bot) S->scr_h[c] = bot;
    }
}

static void scr_explode(int pi, float x, float y, int w){
    BwSim *S = &BWS[pi];
    int side = S->scr_pside;
    S->scr_lastx = x; S->scr_lasty = y;
    if (side==1 && !S->scr_2p)   // the battery remembers its miss, signed along its firing direction
        S->scr_lastmiss = (int)((x - S->scr_u[0].x) * -1.0f);
    if (w==SW_TRA){
        scr_fx_burst(S, x, y, 6, COL(235,235,225), 55.0f);
        sfx(5);
        return;
    }
    if (w==SW_DIRT){
        int r = SCR_RAD[w];
        int cx = x<0? 0 : (x>639? 639 : (int)x);
        float base = S->scr_h[cx];
        for (int c=cx-r;c<=cx+r;c++){
            if (c<0||c>=SCR_W) continue;
            float dx = (float)(c-cx);
            float dy = fsqrt((float)(r*r) - dx*dx);
            float top = base - dy*0.92f;
            if (top<64) top=64;
            if (S->scr_h[c] > top) S->scr_h[c] = top;
        }
        scr_fx_burst(S, x, y, 14, COL(146,102,58), 85.0f);
        sfx(15);
        return;
    }
    if (w==SW_HON){
        scr_carve(S, x, y, 10);
        int slot = 0;
        for (int k=0;k<4;k++) if (S->scr_pool_left[k]<=0){ slot=k; break; }
        S->scr_pool_x0[slot] = x<30? 0 : (int)x-30;
        S->scr_pool_x1[slot] = x>609? 639 : (int)x+30;
        S->scr_pool_left[slot] = 3;
        S->scr_pool_side[slot] = side;
        scr_fx_burst(S, x, y, 12, COL(255,176,66), 70.0f);
        sfx(7);
        return;
    }
    int r = (w==SW_BEAN)? S->scr_bean_r : SCR_RAD[w];
    int dmg = (w==SW_BEAN)? S->scr_bean_d : SCR_DMG[w];
    scr_carve(S, x, y, r);
    scr_fx_burst(S, x, y, 16, COL(240,150,60), 130.0f);
    scr_fx_burst(S, x, y, 10, COL(146,102,58), 90.0f);
    sfx(7); sfx(13);
    if (dmg>0){
        for (int i=0;i<S->scr_nu;i++){
            ScrUnit *u = &S->scr_u[i];
            if (!u->alive) continue;
            float dx = x - u->x, dy = y - (u->y-9.0f);
            float d = fsqrt(dx*dx + dy*dy);
            float f = 0;
            if (d < r) f = 1.0f - 0.6f*(d/r);
            else if (d < r*1.9f) f = 0.4f*(1.0f - (d-r)/(r*0.9f));
            int dd = (int)(dmg*f);
            if (dd>0){
                if (u->side != side) S->scr_bits[side] += 10;   // the hit bounty, per enemy unit hurt — a shielded hit still pays (the dome ate it, not the aim)
                if (u->sh>0){ int ab = u->sh<dd? u->sh : dd; u->sh -= ab; dd -= ab; }   // v1.36: the FIREWALL eats blast damage before hp; only the overflow lands
                u->hp -= dd;
            }
        }
    }
}

static void scr_fly_step(int pi, float dt){
    BwSim *S = &BWS[pi];
    int any = 0;
    for (int i=0;i<3;i++) if (S->scr_pact[i]) any = 1;
    if (!any){ S->scr_phase = SP_SETTLE; S->scr_settle_stage = 0; S->scr_phase_t = 0.15f; return; }
    for (int i=0;i<3;i++){
        if (!S->scr_pact[i]) continue;
        for (int st=0; st<2 && S->scr_pact[i]; st++){
            float h2 = dt*0.5f;
            S->scr_pvy[i] += SCR_GRAV*h2;
            S->scr_pvx[i] += S->scr_wind*1.15f*h2;
            S->scr_px[i] += S->scr_pvx[i]*h2;
            S->scr_py[i] += S->scr_pvy[i]*h2;
            if (S->scr_psplit[i] && S->scr_pvy[i]>=0){
                // Cluster Carcass: the shell breaks open at the top
                // of its arc into three bomblets.
                S->scr_psplit[i]=0;
                int made=0;
                for (int j=0;j<3 && made<2;j++) if (!S->scr_pact[j]){
                    S->scr_pact[j]=1; S->scr_psplit[j]=0;
                    S->scr_px[j]=S->scr_px[i]; S->scr_py[j]=S->scr_py[i];
                    S->scr_pvx[j]=S->scr_pvx[i] + (made==0? -30.0f : 30.0f);
                    S->scr_pvy[j]=S->scr_pvy[i]*0.9f;
                    made++;
                }
                scr_fx_burst(S, S->scr_px[i], S->scr_py[i], 5, COL(220,200,160), 40.0f);
            }
            float px = S->scr_px[i], py = S->scr_py[i];
            if (px<-24 || px>664 || py<-80){
                if (S->scr_pside==1 && !S->scr_2p){
                    float cxp = px<0? 0 : (px>639? 639 : px);
                    S->scr_lastmiss = (int)((cxp - S->scr_u[0].x) * -1.0f);
                }
                S->scr_pact[i]=0;
                break;
            }
            int hit = 0;
            for (int ui=0;ui<S->scr_nu;ui++){
                ScrUnit *u = &S->scr_u[ui];
                if (!u->alive) continue;
                float dx = px - u->x, dy = py - (u->y-9.0f);
                if (dx*dx + dy*dy < 196.0f){ hit = 1; break; }
            }
            if (hit || (px>=0 && px<640 && py >= S->scr_h[(int)px])){
                S->scr_pact[i]=0;
                scr_explode(pi, px, py, S->scr_pweap);
            }
        }
    }
}

// ---- settling: falls, deaths, the turn passing ----------------------
static void scr_honey_tick(int pi){
    BwSim *S = &BWS[pi];
    for (int k=0;k<4;k++){
        if (S->scr_pool_left[k]<=0) continue;
        S->scr_pool_left[k]--;
        for (int i=0;i<S->scr_nu;i++){
            ScrUnit *u = &S->scr_u[i];
            if (!u->alive) continue;
            int cx = (int)u->x;
            if (cx < S->scr_pool_x0[k] || cx > S->scr_pool_x1[k]) continue;
            if (u->y < S->scr_h[cx]-26.0f) continue;   // buried deep under the pool: the honey never reaches it
            int hd = 18;   // v1.36: honey burn is weapon damage, so the FIREWALL eats it too — falls are the dome's only blind spot
            if (u->sh>0){ int ab = u->sh<hd? u->sh : hd; u->sh -= ab; hd -= ab; }
            u->hp -= hd;
            scr_fx_burst(S, u->x, u->y-14, 4, COL(255,176,66), 45.0f);
            if (u->hp<=0){
                u->alive = 0;
                if (u->side != S->scr_pool_side[k]) S->scr_bits[S->scr_pool_side[k]] += 50;
                scr_fx_burst(S, u->x, u->y-10, 10, COL(150,200,80), 80.0f);
            }
        }
    }
}

static void scr_round_end(int pi);   // below
static void scr_next_turn(int pi){
    BwSim *S = &BWS[pi];
    // The outgoing battery's next bug steps up (round-robin), so a
    // 3-bug HARD battery cycles its guns instead of leaning on one.
    if (S->scr_turn==1 && S->scr_nu>2){
        int n = S->scr_nu-1;
        for (int k=0;k<n;k++){
            S->scr_ui[1] = (S->scr_ui[1]+1)%n;
            if (S->scr_u[1+S->scr_ui[1]].alive) break;
        }
    }
    S->scr_turn = 1 - S->scr_turn;
    // If the incoming side's gunner died while waiting (honey, a
    // collapse), advance to a living one.
    if (S->scr_turn==1 && S->scr_nu>1 && !S->scr_u[scr_unit_of(S,1)].alive){
        int n = S->scr_nu-1;
        for (int k=0;k<n;k++){
            S->scr_ui[1] = (S->scr_ui[1]+1)%n;
            if (S->scr_u[1+S->scr_ui[1]].alive) break;
        }
    }
    scr_honey_tick(pi);   // pools burn at the new turn's start
    if (scr_side_alive(S,0)==0 || scr_side_alive(S,1)==0){ scr_round_end(pi); return; }
    scr_roll_wind(S);
    S->scr_weap[S->scr_turn] = scr_weap_ok(S, S->scr_turn, S->scr_weap[S->scr_turn]);
    S->scr_phase = SP_AIM;
    if (scr_side_ai(S, S->scr_turn))
        S->scr_ai_t = 0.85f + hash_f(S->scr_shots, 3, (u32)S->scr_seed)*0.5f;
}

static void scr_settle_step(int pi, float dt){
    BwSim *S = &BWS[pi];
    if (S->scr_phase_t>0){ S->scr_phase_t -= dt; return; }
    if (S->scr_settle_stage==0){
        int falling = 0;
        for (int i=0;i<S->scr_nu;i++){
            ScrUnit *u = &S->scr_u[i];
            if (!u->alive) continue;
            int cx = (int)u->x; if (cx<0) cx=0; if (cx>639) cx=639;
            float ground = S->scr_h[cx];
            if (ground >= 351.0f){ u->hp = 0; continue; }   // the floor itself gave way
            if (u->y < ground - 0.5f){
                falling = 1;
                if (S->scr_fall_v[i]==0 && S->scr_fall_from[i]==0) S->scr_fall_from[i] = u->y;
                S->scr_fall_v[i] += 620.0f*dt;
                u->y += S->scr_fall_v[i]*dt;
                if (u->y >= ground){
                    float fell = ground - S->scr_fall_from[i];
                    u->y = ground;
                    if (fell > 14.0f){
                        int dd = (int)((fell - 14.0f)*1.2f);
                        if (dd>0) u->hp -= dd;
                    }
                    S->scr_fall_v[i]=0; S->scr_fall_from[i]=0;
                    scr_fx_burst(S, u->x, ground, 5, COL(170,130,80), 45.0f);
                }
            } else if (ground < u->y - 110.0f){
                u->y = ground;   // buried past rescue by a Dirt Clod: pop back to the surface — a turn must never soft-lock
            }
        }
        if (!falling){ S->scr_settle_stage = 1; S->scr_phase_t = 0.12f; }
        return;
    }
    if (S->scr_settle_stage==1){
        for (int i=0;i<S->scr_nu;i++){
            ScrUnit *u = &S->scr_u[i];
            if (u->alive && u->hp<=0){
                u->alive = 0;
                if (u->side != S->scr_pside) S->scr_bits[S->scr_pside] += 50;   // the kill bounty
                scr_fx_burst(S, u->x, u->y-10, 12, COL(150,200,80), 95.0f);
                sfx(7);
            }
        }
        S->scr_settle_stage = 2; S->scr_phase_t = 0.25f;
        return;
    }
    if (scr_side_alive(S,0)==0 || scr_side_alive(S,1)==0){ scr_round_end(pi); return; }
    scr_next_turn(pi);
}

static void scr_round_end(int pi){
    BwSim *S = &BWS[pi];
    S->scr_preshop = 0;   // v1.36: any shop past here is a between-rounds shop, never the pre-round one
    int winner = scr_side_alive(S,0)>0? 0 : (scr_side_alive(S,1)>0? 1 : -1);
    S->scr_lastwin = winner;
    if (winner>=0){ S->scr_wins[winner]++; S->scr_bits[winner] += 100; }   // the round purse
    S->scr_phase = SP_BANNER; S->scr_phase_t = 2.4f; S->scr_settle_stage = 1;   // banner kind 1: round over
    sfx(winner==0? 11 : 16);
}

static void scr_match_end(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    S->scr_phase = SP_END;
    if (!S->scr_2p && S->scr_wins[0]>=2 && !S->scr_xp_paid){
        // Solo win: the XP pays once, here, at the match's end —
        // win or lose the rounds, never per round (the spec's rule).
        int xp = SCR_XP[S->scr_diff];
        buddy_award_xp(pi, xp);
        S->scr_xp_paid = 1; S->scr_xp_amt = xp;
        if (S->scr_diff==2 && !G.zen_run) b->scr_champ = 1;   // the HARD plaque (Zen keeps its no-records rule)
        do_save();
        sfx(11); sfx(12);
    } else if (S->scr_wins[0]>=2){
        sfx(11); sfx(12);
    }
}

// ---- the between-rounds shop -----------------------------------------
static void scr_ai_shop(BwSim *S){
    // The battery shops first, instantly. v1.36: the FIREWALL
    // joined its list, over its current gunner — HARD shields
    // before anything else (whenever it can afford a dome that is
    // not already full), NORMAL restocks a Big Carcass first and
    // shields with what is left, EASY never shields. Then the
    // v1.35 order: Big Carcasses, a Cluster if rich, Tracers for
    // pocket change.
    int side = 1;
    ScrUnit *gu = &S->scr_u[scr_unit_of(S, 1)];
    if (S->scr_diff==2 && gu->sh<SCR_FW_STR && S->scr_bits[side]>=SCR_PRICE[SW_FW]){ S->scr_bits[side]-=SCR_PRICE[SW_FW]; gu->sh=SCR_FW_STR; }
    while (S->scr_bits[side]>=40 && S->scr_inv[side][SW_BIG]<2){ S->scr_bits[side]-=40; S->scr_inv[side][SW_BIG]++; }
    if (S->scr_diff==1 && gu->sh<SCR_FW_STR && S->scr_bits[side]>=SCR_PRICE[SW_FW]){ S->scr_bits[side]-=SCR_PRICE[SW_FW]; gu->sh=SCR_FW_STR; }
    if (S->scr_bits[side]>=90 && S->scr_inv[side][SW_CLU]==0){ S->scr_bits[side]-=90; S->scr_inv[side][SW_CLU]++; }
    while (S->scr_bits[side]>=5 && S->scr_inv[side][SW_TRA]<3){ S->scr_bits[side]-=5; S->scr_inv[side][SW_TRA]++; }
}
static void scr_shop_begin(int pi){
    BwSim *S = &BWS[pi];
    if (!S->scr_2p) scr_ai_shop(S);
    S->scr_shop_side = 0; S->scr_shop_sel = 0;
    S->scr_phase = SP_SHOP;
    sfx(19);
}
static void scr_shop_done(int pi){
    BwSim *S = &BWS[pi];
    if (S->scr_2p && S->scr_shop_side==0){ S->scr_shop_side = 1; S->scr_shop_sel = 0; sfx(6); return; }
    if (S->scr_preshop){ S->scr_preshop = 0; scr_start_round(pi); return; }   // v1.36: the pre-round shop hands over to round 1 as it stands — no round advance
    S->scr_round++;
    scr_start_round(pi);
}
static void scr_shop_input(int pi, const BwIn *in){
    BwSim *S = &BWS[pi];
    int side = S->scr_shop_side;
    if (in->up){ S->scr_shop_sel = (S->scr_shop_sel+7)%8; sfx(6); }
    if (in->down){ S->scr_shop_sel = (S->scr_shop_sel+1)%8; sfx(6); }
    int act = -1;
    if (in->sel || in->interact) act = S->scr_shop_sel;
    if (act<0 && pi==G.my_id && IN.tapped){
        int vx,vy,vw2,vh2; pi_view_rect(pi, &vx,&vy,&vw2,&vh2);
        int tx = IN.tap_x, ty = IN.tap_y; IN.tapped = 0;
        for (int r=0;r<8;r++){
            int x,y,w,h; scr_shop_row_rect(vx,vy,vw2,vh2, r, &x,&y,&w,&h);
            if (tx>=x && tx<x+w && ty>=y && ty<y+h){ act = r; S->scr_shop_sel = r; break; }
        }
    }
    if (act<0) return;
    if (act==7){ sfx(11); scr_shop_done(pi); return; }
    if (act==6){
        // FIREWALL (v1.36): not a stock item — a recharge for the
        // shopping side's gunner. Buying at any strength restores
        // the full 100 for the same 80; there are no partial top-ups.
        ScrUnit *gu = &S->scr_u[scr_unit_of(S, side)];
        if (S->scr_bits[side]>=SCR_PRICE[SW_FW]){ S->scr_bits[side]-=SCR_PRICE[SW_FW]; gu->sh=SCR_FW_STR; sfx(11); }
        else sfx(16);
        return;
    }
    int w = act+1, price = SCR_PRICE[w];
    if (S->scr_bits[side]>=price){ S->scr_bits[side]-=price; S->scr_inv[side][w]++; sfx(11); }
    else sfx(16);
}

// ---- the battery's aim -----------------------------------------------
static void scr_ai_fire(int pi){
    BwSim *S = &BWS[pi];
    ScrUnit *me = &S->scr_u[scr_unit_of(S,1)];
    ScrUnit *tg = &S->scr_u[0];
    // A brute-force solver over the same integrator the live flight
    // uses (fixed 1/30 substeps, this turn's wind): the shot it
    // picks is the shot it would fly, so difficulty lives purely in
    // the error added afterwards.
    static const int POWS[4] = { 55, 70, 85, 100 };
    float best = 1e18f, bang = 45, bpow = 70;
    for (int p2=0;p2<4;p2++) for (int a=15;a<=165;a+=2){
        float th = a*3.14159f/180.0f, v = scr_muzzle_v(POWS[p2]);
        float x = me->x - fcos(th)*18.0f, y = (me->y-12.0f) - fsin(th)*18.0f;
        float vx = -v*fcos(th), vy = -v*fsin(th);
        float m = 1e18f;
        for (int st=0;st<240;st++){
            float h2 = 1.0f/30.0f;
            vy += SCR_GRAV*h2; vx += S->scr_wind*1.15f*h2;
            x += vx*h2; y += vy*h2;
            float dx = x - tg->x, dy = y - (tg->y-9.0f);
            float d = dx*dx + dy*dy;
            if (d<m) m = d;
            if (y>=360 || x<0 || x>640) break;
            if (x>=0 && x<640 && y >= S->scr_h[(int)x]) break;   // a hillside eats the shell here — arcs that bury themselves score only what they reached
        }
        if (m<best){ best = m; bang = (float)a; bpow = (float)POWS[p2]; }
    }
    float ea = S->scr_diff==0? 9.0f : (S->scr_diff==1? 4.5f : 1.8f);
    float ep = S->scr_diff==0? 10.0f : (S->scr_diff==1? 5.0f : 2.0f);
    u32 h = hash2(S->scr_shots*13+3, S->scr_round*7, (u32)S->scr_seed);
    float e1 = ((int)(h%1000)/1000.0f*2.0f - 1.0f)*ea;
    float e2 = ((int)((h>>10)%1000)/1000.0f*2.0f - 1.0f)*ep;
    // The last-miss correction rides on POWER (range's honest
    // lever): overshot last time, ease off; fell short, lean in.
    float corr = -S->scr_lastmiss*0.03f; if (corr>6) corr=6; if (corr<-6) corr=-6;
    S->scr_aim[1] = bang + e1; if (S->scr_aim[1]<2) S->scr_aim[1]=2; if (S->scr_aim[1]>178) S->scr_aim[1]=178;
    S->scr_pow[1] = bpow + e2 + corr; if (S->scr_pow[1]<5) S->scr_pow[1]=5; if (S->scr_pow[1]>100) S->scr_pow[1]=100;
    int w = 0;
    if (S->scr_inv[1][SW_BEAN]>0 && (h%100)<25) w = SW_BEAN;
    else if (S->scr_inv[1][SW_BIG]>0 && (h%100)<55) w = SW_BIG;
    else if (S->scr_inv[1][SW_CLU]>0 && (h%100)<30) w = SW_CLU;
    S->scr_weap[1] = w;
    scr_fire(pi);
}

// ---- aiming input -----------------------------------------------------
static void scr_cycle_weap(BwSim *S, int side, int dir){
    for (int n=0;n<7;n++){
        S->scr_weap[side] = (S->scr_weap[side] + dir + 7)%7;
        if (S->scr_weap[side]==0 || S->scr_inv[side][S->scr_weap[side]]>0){ sfx(6); return; }
    }
}
static void scr_select_weap(BwSim *S, int side, int w){
    if (w<0 || w>6) return;
    if (w==0 || S->scr_inv[side][w]>0){ S->scr_weap[side]=w; sfx(6); }
    else sfx(16);
}

// Touch and mouse ride the same point channel (the shell forwards
// the mouse as a point), and the mode carved itself out of the
// generic deck zones — so every press here is a raw point, claimed
// by where it STARTED: the FIRE disc fires on the press edge, the
// gauge steppers repeat while held, the gauge middles drag, the
// weapon chips select. A press that began mid-flight is synced as
// already-down (in scr_update) and can never fire when AIM returns:
// the v1.34 lesson, applied to a whole mode.
static int scr_tp_input(int pi, float dt){
    BwSim *S = &BWS[pi];
    int vx,vy,vw2,vh2; pi_view_rect(pi, &vx,&vy,&vw2,&vh2);
    int side = S->scr_turn;
    int bar_y, ax, aw, px, pw, fx, fy;
    scr_layout(vx,vy,vw2,vh2, &bar_y,&ax,&aw,&px,&pw,&fx,&fy);
    int drove = 0;
    for (int i=0;i<6;i++){
        int act = IN.tp[i].active? 1 : 0;
        if (act && !S->scr_tp_was[i] && S->scr_phase==SP_AIM){
            int x0 = IN.tp[i].x0, y0 = IN.tp[i].y0;
            S->scr_tp_act[i] = 0;
            if (x0>=fx && x0<fx+44 && y0>=fy && y0<fy+44){
                S->scr_tp_act[i]=1; scr_fire(pi); drove=1;
            } else {
                int claimed = 0;
                for (int c=0;c<7;c++){
                    int cx,cy,cw,ch; scr_chip_rect(vx,vy,vw2,vh2, c, &cx,&cy,&cw,&ch);
                    if (x0>=cx && x0<cx+cw && y0>=cy && y0<cy+ch){ scr_select_weap(S, side, c); S->scr_tp_act[i]=8; claimed=1; break; }
                }
                if (!claimed && y0>=bar_y+13 && y0<bar_y+37){
                    if (x0>=ax && x0<ax+aw){
                        if (x0<ax+22) S->scr_tp_act[i]=2;
                        else if (x0>=ax+aw-22) S->scr_tp_act[i]=3;
                        else { S->scr_tp_act[i]=6; float f=(IN.tp[i].x-(ax+22))/(float)(aw-44); if (f<0) f=0; if (f>1) f=1; S->scr_aim[side]=f*180.0f; }
                        claimed=1;
                    } else if (x0>=px && x0<px+pw){
                        if (x0<px+22) S->scr_tp_act[i]=4;
                        else if (x0>=px+pw-22) S->scr_tp_act[i]=5;
                        else { S->scr_tp_act[i]=7; float f=(IN.tp[i].x-(px+22))/(float)(pw-44); if (f<0) f=0; if (f>1) f=1; S->scr_pow[side]=f*100.0f; }
                        claimed=1;
                    }
                }
                if (claimed) drove=1;
            }
        }
        if (act && S->scr_tp_act[i] && S->scr_phase==SP_AIM){
            drove = 1;
            int a = S->scr_tp_act[i];
            if (a==2) S->scr_aim[side] -= 62.0f*dt;
            else if (a==3) S->scr_aim[side] += 62.0f*dt;
            else if (a==4) S->scr_pow[side] -= 55.0f*dt;
            else if (a==5) S->scr_pow[side] += 55.0f*dt;
            else if (a==6){ float f=(IN.tp[i].x-(ax+22))/(float)(aw-44); if (f<0) f=0; if (f>1) f=1; S->scr_aim[side]=f*180.0f; }
            else if (a==7){ float f=(IN.tp[i].x-(px+22))/(float)(pw-44); if (f<0) f=0; if (f>1) f=1; S->scr_pow[side]=f*100.0f; }
            if (S->scr_aim[side]<0) S->scr_aim[side]=0; if (S->scr_aim[side]>180) S->scr_aim[side]=180;
            if (S->scr_pow[side]<0) S->scr_pow[side]=0; if (S->scr_pow[side]>100) S->scr_pow[side]=100;
        }
        if (!act) S->scr_tp_act[i]=0;
        S->scr_tp_was[i]=act;
    }
    return drove;
}

static void scr_aim_input(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi];
    int side = S->scr_turn;
    int tp_drove = (pi==G.my_id)? scr_tp_input(pi, dt) : 0;
    if (tp_drove) return;   // a hand on the glass (or the mouse) owns this frame's aim
    float ar = 62.0f*dt, pr = 55.0f*dt;
    // LEFT walks the barrel counter-clockwise ON SCREEN for both
    // sides — the gauge measures elevation toward the enemy, so for
    // the east catapult the value runs the other way.
    if (in->keys & K_LEFT)  S->scr_aim[side] += (side==0? ar : -ar);
    if (in->keys & K_RIGHT) S->scr_aim[side] += (side==0? -ar : ar);
    if (in->keys & K_UP)   S->scr_pow[side] += pr;
    if (in->keys & K_DOWN) S->scr_pow[side] -= pr;
    if (S->scr_aim[side]<0) S->scr_aim[side]=0; if (S->scr_aim[side]>180) S->scr_aim[side]=180;
    if (S->scr_pow[side]<0) S->scr_pow[side]=0; if (S->scr_pow[side]>100) S->scr_pow[side]=100;
    int cyc = IN.weapon_cycle;
    if (pi==G.my_id && (raw_edge(PK_TAB) || raw_edge('W'-'A'))) cyc = 1;
    if (cyc) scr_cycle_weap(S, side, cyc);
    if (pi==G.my_id && IN.weapon_key>=1 && IN.weapon_key<=7) scr_select_weap(S, side, IN.weapon_key-1);
    if (in->sel) scr_fire(pi);
}

// ---- the mode's update -------------------------------------------------
static void scr_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi];
    if (S->scr_phase!=SP_AIM)
        for (int i=0;i<6;i++){ S->scr_tp_was[i]=IN.tp[i].active? 1:0; S->scr_tp_act[i]=0; }
    if (S->scr_exit_arm>0) S->scr_exit_arm -= dt;
    scr_fx_tick(S, dt);
    if (in->exit_){
        if (S->scr_phase==SP_END){ bw_exit(pi, 0); return; }
        if (S->scr_exit_arm>0){ bw_exit(pi, 0); return; }   // forfeit confirmed
        S->scr_exit_arm = 2.5f;
        scpy(S->note, "LEAVING FORFEITS THE MATCH - ESC AGAIN TO GO.", 72);
        S->note_t = 2.5f;
        sfx(6);
        return;
    }
    switch (S->scr_phase){
      case SP_BANNER:
        S->scr_phase_t -= dt;
        if (S->scr_phase_t<=0){
            if (S->scr_settle_stage==1){   // the round ended: shop, or the match
                if (S->scr_wins[0]>=2 || S->scr_wins[1]>=2) scr_match_end(pi);
                else scr_shop_begin(pi);
            } else {
                S->scr_phase = SP_AIM;
                if (scr_side_ai(S, S->scr_turn))
                    S->scr_ai_t = 0.85f + hash_f(S->scr_shots, 3, (u32)S->scr_seed)*0.5f;
            }
        }
        return;
      case SP_FLY: scr_fly_step(pi, dt); return;
      case SP_SETTLE: scr_settle_step(pi, dt); return;
      case SP_SHOP:
        // v1.36: the panel press that launched the cabinet is still
        // lifting when the pre-round shop opens — gate the shop's
        // input until that press is fully released (keys up, no
        // select held, no tap pending), or its tail buys a Tracer
        // nobody chose (the v1.34 lesson, at the door). The gate is
        // a release latch, never a timer: a timed hold eats whole
        // input edges that land inside it, and the first fix's
        // 0.3 s window did exactly that to the suite's opening
        // ArrowDown — the shop sat one row off for the whole run.
        if (S->scr_preshop && S->scr_phase_t>0){
            int held = in->sel || in->interact || in->up || in->down || in->sel_held;
            if (pi==G.my_id && IN.tapped) held = 1;
            if (pi==G.my_id) IN.tapped = 0;
            if (!held) S->scr_phase_t = 0;   // the launching hand is off: the shop is live
            if (S->scr_phase_t>0) return;
        }
        scr_shop_input(pi, in); return;
      case SP_END:
        if (in->sel || in->interact){ bw_exit(pi, 0); return; }
        if (pi==G.my_id && IN.tapped){ IN.tapped=0; bw_exit(pi, 0); return; }
        return;
      default: break;
    }
    // SP_AIM
    if (scr_side_ai(S, S->scr_turn)){
        S->scr_ai_t -= dt;
        if (S->scr_ai_t<=0) scr_ai_fire(pi);
        return;
    }
    scr_aim_input(pi, dt, in);
}

// ---- test hooks (dbg_scorch_* in platform/wasm.c read through these) ----
static BwSim *scr_dbg_sim(void){
    int id = G.my_id>=0? G.my_id : 0;
    return &BWS[id];
}
static int scr_dbg_on(void){
    int id = G.my_id>=0? G.my_id : 0;
    return (G.bud[id].in_bw && G.bud[id].bw_mode==10)? 1 : 0;
}
// Test staging (the wasm dbg_scrdig export): carve a pit under a
// field column, so the next settle drops whoever stands there —
// the fall-damage path, exercised on demand.
static void scr_dbg_dig(int x){
    BwSim *S = scr_dbg_sim();
    if (x<0) x=0; if (x>639) x=639;
    scr_carve(S, (float)x, S->scr_h[x], 30);
}
// Test staging (the wasm dbg_scrregen export): restore the round's
// generated field — units back on their spawn pads at full HP,
// pools cleared; turn, wind, purses, stocks and shields untouched.
// The ballistic proofs each start from the field the round actually
// generated, not from whatever the previous proof's live fire left
// of the launch hill: the self-shot proofs dig the player's own
// column toward the world floor by design, and from the bottom of
// that pit no honest arc reaches the far hill at all.
static void scr_dbg_regen(void){
    int id = G.my_id>=0? G.my_id : 0;
    scr_gen_field(id, BWS[id].scr_round);
}
