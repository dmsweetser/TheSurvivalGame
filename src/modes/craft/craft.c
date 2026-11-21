// THE SURVIVAL GAME — modes/craft/craft.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from craft.c lines 130-530 in the Nov 2026 fine-grained refactor; code unchanged.

static void craft_enter(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (b->craft_x==0 && b->craft_y==0){ b->x = 0; b->y = 56; }
    else { b->x = b->craft_x; b->y = b->craft_y; }
    if (b->craft_tod<=0) b->craft_tod = 40;   // first trip starts in the morning
    S->craft_menu=0; S->craft_sel=0; S->craft_cat=0; S->craft_scroll=0; S->craft_biome=-1; S->craft_hitn=0;
    S->craft_search=0; S->craft_filter[0]=0;
    S->craft_bugn=0; S->craft_grace=0; S->craft_bt=2.0f; S->craft_demolish=0;
    S->craft_swarm_t=70; S->craft_cache_t=40; S->craft_cache_on=0; S->craft_haste=0;
    S->craft_darkp=craft_darkf(b); S->craft_beam_t=0; S->craft_prevk=0;
    for (int i=0;i<4;i++) S->craft_bolt_a[i]=0;
    for (int i=0;i<48;i++) S->craft_tz[i]=0;
    S->craft_shn=0; S->craft_sh_t=3.0f; S->craft_bmr_a=0; S->craft_nap=0;
}
static const char *CRAFT_BIOME_NAME[6] = { "THE GREEN REACHES", "THE DEEPWOOD", "THE SUNBAKED STACKS", "THE FROZEN REACHES", "THE MURK", "THE WIDE WATER" };
// ---------------------------------------------------------------------------
// the recipe book: five categories, tiers with prerequisites.
// build >= 0 deploys that build kind; build -2 bakes a pie, -3 cooks
// a stew. bit is the gear flag set on craft (or the "built it once"
// flag for buildings); req is the gear flag you must already hold.
// ---------------------------------------------------------------------------
typedef struct { const char *name; int cat, w, s, f, c, bit, req, build; } CRec;
static const CRec CREC[31] = {
    { "WOODEN AXE",       0, 3,0,0,0, 1,      0,    -1 },   // 0
    { "STONE PICK",       0, 3,3,0,0, 2,      0,    -1 },   // 1
    { "STONE AXE",        0, 2,4,0,0, 16,     1,    -1 },   // 2
    { "CARAPACE PICK",    0, 0,2,0,4, 32,     2,    -1 },   // 3
    { "RAFT",             0, 5,0,3,0, 512,    2048, -1 },   // 4
    { "LANTERN",          0, 1,0,2,0, 8192,   0,    -1 },   // 5: a light of your own, all night
    { "FISHING ROD",      0, 3,0,2,0, 16384,  0,    -1 },   // 6
    { "SAIL RAFT",        0, 4,0,4,0, 262144, 512,  -1 },   // 7: the raft, faster — sharks permitting
    { "CARAPACE SWORD",   1, 2,0,0,4, 4,      0,    -1 },   // 8
    { "CARAPACE SPEAR",   1, 3,0,0,6, 64,     4,    -1 },   // 9
    { "HOTFIX ZAPPER",    1, 0,4,2,4, 128,    32,   -1 },   // 10
    { "BOOMERANG",        1, 3,0,2,0, 32768,  0,    -1 },   // 11: it comes back. They don't.
    { "STORM ZAPPER",     1, 0,2,0,5, 65536,  128,  -1 },   // 12: bolts pierce, and fly faster
    { "FIBER ARMOR",      1, 0,0,6,0, 8,      0,    -1 },   // 13
    { "REINFORCED ARMOR", 1, 0,0,4,4, 256,    8,    -1 },   // 14
    { "SHELL PLATE",      1, 0,0,2,8, 131072, 256,  -1 },   // 15
    { "WOODEN WALL",      2, 4,0,0,0, 1024,   0,    0 },    // 16
    { "STONE WALL",       2, 0,6,0,0, 4096,   2,    2 },    // 17
    { "CAMPFIRE",         2, 5,2,0,0, 2048,   0,    1 },    // 18
    { "FIREPLACE",        2, 4,6,0,0, 0,      2048, 3 },    // 19
    { "TORCH POST",       2, 2,0,1,0, 0,      0,    6 },    // 20
    { "DOCK",             2, 5,0,0,0, 0,      0,    7 },    // 21: the one build that WANTS the water
    { "BEDROLL",          2, 1,0,4,0, 0,      0,    8 },    // 22: a nap anywhere — the wild watches
    { "REINFORCED WALL",  2, 3,3,0,0, 0,      2,    10 },   // 23
    { "LANTERN POST",     2, 0,2,2,0, 0,      0,    11 },   // 24
    { "SPIKE TRAP",       3, 0,2,2,0, 0,      0,    5 },    // 25
    { "ZAPPER TOWER",     3, 0,6,0,3, 0,      4096, 4 },    // 26
    { "SCARECROW",        3, 3,0,3,0, 0,      0,    9 },    // 27: bugs bite IT first
    { "HEART PIE",        4, 0,0,2,1, 0,      0,    -2 },   // 28
    { "CARAPACE STEW",    4, 0,0,2,2, 0,      0,    -3 },   // 29
    { "COOKED FISH",      4, 0,0,0,0, 0,      2048, -5 },   // 30: eats a caught fish, needs a fire first
};
#define CREC_N 31
static const char *CRAFT_CAT_NAME[5] = { "TOOLS", "WEAPONS", "BUILDING", "DEFENSE", "FOOD" };
static const char *craft_reqname(int req){
    switch (req){
        case 1: return "WOODEN AXE"; case 2: return "STONE PICK";
        case 4: return "CARAPACE SWORD"; case 8: return "FIBER ARMOR";
        case 32: return "CARAPACE PICK"; case 1024: return "BUILD A WOODEN WALL";
        case 128: return "HOTFIX ZAPPER"; case 256: return "REINFORCED ARMOR";
        case 512: return "THE RAFT";
        case 2048: return "BUILD A CAMPFIRE"; case 4096: return "BUILD A STONE WALL";
        default: return "MORE CRAFTING";
    }
}
// menu rows for a category: 100 eat pie, 101 eat stew, 102 demolish,
// 103 eat a lake fish
static int craft_rows(Buddy *b, int cat, int *ids){
    int n=0;
    if (cat==4){ if (b->craft_fish>0) ids[n++]=103; if (b->craft_pies>0) ids[n++]=100; if (b->craft_stews>0) ids[n++]=101; }
    for (int i=0;i<CREC_N;i++) if (CREC[i].cat==cat) ids[n++]=i;
    if (cat==2) ids[n++]=102;
    return n;
}
// THE LIST: every category as a segment of ONE long list — a segment
// header is encoded -(cat+1), anything else is a craft_rows id. There
// are no tabs any more: a sloppy diagonal on the pad can only scroll,
// it can never flip you into another list (Daniel's daughter, Nov 6).
static int craft_all_rows(Buddy *b, int *rows){
    int n=0;
    for (int c=0;c<5;c++){
        int ids[16]; int m = craft_rows(b, c, ids);
        if (!m) continue;
        rows[n++] = -(c+1);
        for (int i=0;i<m;i++) rows[n++]=ids[i];
    }
    return n;
}
// the menu's rows, honouring the search filter (Nov 14): with a
// filter the list goes FLAT — matching items only, no segment
// headers — and an empty result is one -99 sentinel row so every
// consumer (nav, render, detail strip) has something safe to hold.
static const char *craft_row_name(int id);
static int craft_ci_contains(const char *hay, const char *needle){
    int nl = slen(needle); if (!nl) return 1;
    for (int i=0; hay[i]; i++){
        int j=0;
        while (needle[j] && hay[i+j]){
            char a = hay[i+j], b2 = needle[j];
            if (a>='a' && a<='z') a -= 32;
            if (b2>='a' && b2<='z') b2 -= 32;
            if (a != b2) break;
            j++;
        }
        if (j==nl) return 1;
    }
    return 0;
}
static int craft_menu_rows(BwSim *S, Buddy *b, int *rows){
    if (!S->craft_filter[0]) return craft_all_rows(b, rows);
    int n=0;
    for (int c=0;c<5;c++){
        int ids[16]; int m = craft_rows(b, c, ids);
        for (int i=0;i<m && n<64;i++)
            if (craft_ci_contains(craft_row_name(ids[i]), S->craft_filter)) rows[n++]=ids[i];
    }
    if (!n){ rows[0] = -99; return 1; }
    return n;
}
// move the selection one CRAFTABLE row in a direction, skipping the
// segment headers, wrapping around the ends of the list
static int craft_step(const int *rows, int n, int from, int dir){
    int i=from;
    for (int t=0;t<n;t++){ i+=dir; if (i>=n) i=0; if (i<0) i=n-1; if (rows[i]>=0) return i; }
    return from;
}
static int craft_row_cat(int id){
    if (id<0) return 0;
    if (id>=100) return id==102? 2 : 4;
    return CREC[id].cat;
}
static int craft_afford(Buddy *b, const CRec *R){
    return b->craft_wood>=R->w && b->craft_stone>=R->s && b->craft_fiber>=R->f && b->craft_car>=R->c;
}
static void craft_spend(Buddy *b, const CRec *R){
    b->craft_wood-=R->w; b->craft_stone-=R->s; b->craft_fiber-=R->f; b->craft_car-=R->c;
}
static void craft_note(BwSim *S, const char *t){ scpy(S->note, t, 72); S->note_t=2.0f; }
// v1.19: a completed craft pays the buddy +8 XP, surfaced on the
// note the craft just posted — call AFTER the note is set (whether
// it came through craft_note or a direct write), before the
// sfx/save tail of the success branch.
static void craft_xp_note(int pi){
    BwSim *S=&BWS[pi];
    buddy_award_xp(pi, 8);
    if (G.bud[pi].active){
        int l = slen(S->note);
        if (l > 0 && l < 58){ char *q = S->note + l; q = sappend(q, " +8 BUDDY XP"); *q = 0; }
    }
}
static void craft_do(int pi, int id){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (id==100){   // eat a heart pie
        b->craft_pies--; b->hp += b->maxhp*0.5f; if (b->hp>b->maxhp) b->hp=b->maxhp;
        craft_note(S, "HEART PIE. WARM AND BUTTERY. +50% HP"); sfx(11); do_save(); return;
    }
    if (id==101){   // eat a carapace stew
        b->craft_stews--; b->hp += b->maxhp*0.5f; if (b->hp>b->maxhp) b->hp=b->maxhp;
        S->craft_haste = 25.0f;
        craft_note(S, "CARAPACE STEW. WARM, AND SWIFT FOR A WHILE."); sfx(11); do_save(); return;
    }
    if (id==103){   // eat a lake fish, straight from the line
        b->craft_fish--; b->hp += b->maxhp*0.35f; if (b->hp>b->maxhp) b->hp=b->maxhp;
        craft_note(S, "DOWN THE HATCH. +35% HP. SLIGHTLY FISHY."); sfx(11); do_save(); return;
    }
    if (id==102){   // demolish mode: the next interacts take builds apart
        S->craft_demolish = 12.0f; S->craft_menu = 0;
        { char *q=S->note; q=sappend(q, "DEMOLISH ARMED - "); q=sappend(q, lbl_int()); q=sappend(q, " REMOVES A BUILD. HALF BACK."); *q=0; }
        S->note_t=2.4f; sfx(6); return;
    }
    const CRec *R = &CREC[id];
    if (R->req && !(b->craft_gear & R->req)){
        { char *q=S->note; q=sappend(q, "YOU NEED THE "); q=sappend(q, craft_reqname(R->req)); q=sappend(q, " FIRST."); *q=0; }
        S->note_t=1.8f; sfx(16); return;
    }
    if (!craft_afford(b, R)){ craft_note(S, "NOT ENOUGH MATERIALS. GO GATHER."); S->note_t=1.8f; sfx(16); return; }
    if (R->build==-5){   // cooked fish: eats a caught fish, right now, by the fire
        if (b->craft_fish<1){ craft_note(S, "NO FISH TO COOK. THE WATER PROVIDES."); S->note_t=1.8f; sfx(16); return; }
        b->craft_fish--; b->hp += b->maxhp*0.6f; if (b->hp>b->maxhp) b->hp=b->maxhp;
        craft_note(S, "COOKED FISH. CRISP EDGES, WARM MIDDLE. +60% HP"); craft_xp_note(pi); sfx(11); do_save(); return;
    }
    if (R->build==-2 || R->build==-3){   // food, into the pack
        if (R->build==-2 && b->craft_pies>=99){ craft_note(S, "YOUR PACK IS FULL OF PIES."); sfx(16); return; }
        if (R->build==-3 && b->craft_stews>=99){ craft_note(S, "YOUR PACK IS FULL OF STEWS."); sfx(16); return; }
        craft_spend(b, R);
        if (R->build==-2){ b->craft_pies++; craft_note(S, "A HEART PIE, BAKED AND BAGGED."); }
        else { b->craft_stews++; craft_note(S, "A CARAPACE STEW, SEALED AND STEAMING."); }
        craft_xp_note(pi); sfx(11); do_save(); return;
    }
    if (R->build<0){   // gear: one of each, forever
        if (b->craft_gear & R->bit){ craft_note(S, "YOU ALREADY CRAFTED THAT."); S->note_t=1.6f; sfx(16); return; }
        craft_spend(b, R);
        b->craft_gear |= R->bit;
        { char *q=S->note; q=sappend(q, "CRAFTED: "); q=sappend(q, R->name); q=sappend(q, "!"); *q=0; }
        S->note_t=2.0f; craft_xp_note(pi); sfx(11); do_save(); return;
    }
    // a build: fires plant at your feet, everything else on the tile ahead
    int kind = R->build;
    int tx, ty;
    if (kind==1 || kind==3){ tx = bw_tileof(b->x); ty = bw_tileof(b->y); }
    else { tx = bw_tileof(b->x + fcos(b->aim)*BW_TILE); ty = bw_tileof(b->y + fsin(b->aim)*BW_TILE); }
    if (craft_build_at(b, tx, ty)>=0){ craft_note(S, "SOMETHING ALREADY STANDS THERE."); S->note_t=1.8f; sfx(16); return; }
    if (kind==7){   // the dock is the exception: it belongs ON the water
        if (craft_biome_at(b, tx, ty)!=5){ craft_note(S, "A DOCK NEEDS WATER."); S->note_t=1.8f; sfx(16); return; }
    }
    else if (craft_biome_at(b, tx, ty)==5){ craft_note(S, "NOT IN THE WATER."); S->note_t=1.8f; sfx(16); return; }
    if (kind!=1 && kind!=3 && kind!=7){
        if (craft_node_at(pi, tx, ty)){ craft_note(S, "NO ROOM THERE. CLEAR IT FIRST."); S->note_t=1.8f; sfx(16); return; }
        // never entomb the builder: the tile must clear his own body
        float cxp = b->x < tx*BW_TILE? tx*BW_TILE : b->x > tx*BW_TILE+BW_TILE? tx*BW_TILE+BW_TILE : b->x;
        float cyp = b->y < ty*BW_TILE? ty*BW_TILE : b->y > ty*BW_TILE+BW_TILE? ty*BW_TILE+BW_TILE : b->y;
        float dx=b->x-cxp, dy=b->y-cyp;
        if (dx*dx+dy*dy < 23*23){ craft_note(S, "TOO CLOSE - STEP BACK A LITTLE."); S->note_t=1.8f; sfx(16); return; }
    }
    if (b->craft_bn>=48){ craft_note(S, "THAT IS ENOUGH BUILDING FOR ANYONE."); S->note_t=1.8f; sfx(16); return; }
    if (kind==4 && craft_build_kind_count(b, 4)>=3){ craft_note(S, "THREE TOWERS HUM ALREADY."); S->note_t=1.8f; sfx(16); return; }
    if (kind==5 && craft_build_kind_count(b, 5)>=8){ craft_note(S, "ENOUGH SPIKES FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    if (kind==6 && craft_build_kind_count(b, 6)>=8){ craft_note(S, "ENOUGH TORCHES FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    if ((kind==1 || kind==3) && craft_build_kind_count(b, 1)+craft_build_kind_count(b, 3)>=6){ craft_note(S, "ENOUGH FIRES FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    if (kind==7 && craft_build_kind_count(b, 7)>=10){ craft_note(S, "ENOUGH DOCKS FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    if (kind==8 && craft_build_kind_count(b, 8)>=4){ craft_note(S, "ENOUGH BEDROLLS FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    if (kind==9 && craft_build_kind_count(b, 9)>=3){ craft_note(S, "THREE SCARECROWS ARE WATCHING ALREADY."); S->note_t=1.8f; sfx(16); return; }
    if (kind==11 && craft_build_kind_count(b, 11)>=8){ craft_note(S, "ENOUGH LANTERNS FOR ONE WILD."); S->note_t=1.8f; sfx(16); return; }
    craft_spend(b, R);
    b->craft_btx[b->craft_bn]=tx; b->craft_bty[b->craft_bn]=ty;
    b->craft_bk[b->craft_bn]=kind; b->craft_bhp[b->craft_bn]=craft_buildhp_max(kind);
    b->craft_bn++;
    if (R->bit) b->craft_gear |= R->bit;
    if (kind==0) craft_note(S, "A WOODEN WALL! STURDY ENOUGH.");
    else if (kind==2) craft_note(S, "A STONE WALL. TRY CHEWING THAT.");
    else if (kind==1){ char *q=S->note; q=sappend(q, "A CAMPFIRE! STAND CLOSE AND REST WITH "); q=sappend(q, lbl_int()); q=sappend(q, "."); *q=0; S->note_t=2.2f; }
    else if (kind==3) craft_note(S, "A FIREPLACE. THE HEART OF A HOME.");
    else if (kind==4) craft_note(S, "A ZAPPER TOWER! IT GUARDS THIS GROUND.");
    else if (kind==5) craft_note(S, "SPIKES SET. MIND YOUR OWN FEET.");
    else if (kind==7) craft_note(S, "A DOCK! THE WATER IS A FLOOR NOW. MOSTLY.");
    else if (kind==8){ char *q=S->note; q=sappend(q, "A BEDROLL. REST WITH "); q=sappend(q, lbl_int()); q=sappend(q, " - A NAP, NOT A SLEEP."); *q=0; S->note_t=2.2f; }
    else if (kind==9) craft_note(S, "A SCARECROW. BUGS BITE IT FIRST. USUALLY.");
    else if (kind==10) craft_note(S, "A REINFORCED WALL. COME AND CHEW. WE DARE YOU.");
    else if (kind==11) craft_note(S, "A LANTERN POST. THE DARK BACKS UP.");
    else craft_note(S, "A TORCH POST. A LITTLE SUN, ON A STICK.");
    craft_xp_note(pi); sfx(19); do_save();
}
static void craft_hit_node(int pi, int tx, int ty){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int nd = craft_node_at(pi, tx, ty);
    if (!nd) return;
    int key = craft_key(tx, ty);
    int tool = 0;   // 0 hands, else the gear bit of the best tool for this node
    if (nd==1) tool = (b->craft_gear&16)? 16 : (b->craft_gear&1)? 1 : 0;
    else if (nd==2) tool = (b->craft_gear&32)? 32 : (b->craft_gear&2)? 2 : 0;
    else if (nd==3 && ((b->craft_gear&16) || (b->craft_gear&32))) tool = 1;
    int base = nd==1? 3 : nd==2? 4 : nd==3? 2 : 1;
    if (tool) base = 1;
    int idx=-1;
    for (int i=0;i<S->craft_hitn;i++) if (S->craft_hk[i]==key) idx=i;
    int left = idx>=0? S->craft_hv[idx] : base;
    left--;
    sfx(4);
    splat(tx*BW_TILE+24, ty*BW_TILE+24, nd==1? COL(90,160,80) : nd==2? COL(150,150,155) : nd==3? COL(110,180,90) : COL(200,220,120), 4, 70);
    if (left>0){
        if (idx<0 && S->craft_hitn<24){ S->craft_hk[S->craft_hitn]=key; S->craft_hv[S->craft_hitn]=left; S->craft_hitn++; }
        else if (idx>=0) S->craft_hv[idx]=left;
        return;
    }
    if (idx>=0){ S->craft_hitn--; S->craft_hk[idx]=S->craft_hk[S->craft_hitn]; S->craft_hv[idx]=S->craft_hv[S->craft_hitn]; }
    if (b->craft_gn<96) b->craft_gone[b->craft_gn++]=key;
    int yw=0, ys=0, yf=0;
    if (nd==1){ yw = (b->craft_gear&16)? 5 : (b->craft_gear&1)? 4 : 3; }
    if (nd==2){ ys = (b->craft_gear&32)? 5 : (b->craft_gear&2)? 4 : 3; }
    if (nd==3) yf = 2;
    if (nd==4) yf = 1;
    b->craft_wood += yw; if (b->craft_wood>999) b->craft_wood=999;
    b->craft_stone += ys; if (b->craft_stone>999) b->craft_stone=999;
    b->craft_fiber += yf; if (b->craft_fiber>999) b->craft_fiber=999;
    { char *q=S->note;
      if (yw){ q=sappend(q, "+"); q=sapp_int(q, yw); q=sappend(q, " WOOD"); }
      else if (ys){ q=sappend(q, "+"); q=sapp_int(q, ys); q=sappend(q, " STONE"); }
      else { q=sappend(q, "+"); q=sapp_int(q, yf); q=sappend(q, " FIBER"); }
      *q=0; S->note_t=1.4f; }
    sfx(11);
    do_save();   // a felled tree is progress; keep it
}
static void craft_loot_cache(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->craft_cache_on=0; S->craft_cache_t=55;
    u32 h = hash2((int)(b->x), (int)(G.anim_t*10), craft_seed(b));
    int m1 = (int)(h%4), a1 = 2 + (int)((h>>4)%3);
    int m2 = (int)((h>>8)%4), a2 = 1 + (int)((h>>12)%3);
    int gain[4] = {0,0,0,0}; gain[m1]+=a1; gain[m2]+=a2;
    b->craft_wood += gain[0]; b->craft_stone += gain[1]; b->craft_fiber += gain[2]; b->craft_car += gain[3];
    if (b->craft_wood>999) b->craft_wood=999; if (b->craft_stone>999) b->craft_stone=999;
    if (b->craft_fiber>999) b->craft_fiber=999; if (b->craft_car>999) b->craft_car=999;
    static const char *MN[4] = { "WOOD", "STONE", "FIBER", "CARAPACE" };
    { char *q=S->note; int first=1;
      for (int i=0;i<4;i++) if (gain[i]){ if (!first) q=sappend(q, " +"); else q=sappend(q, "+");
          q=sapp_int(q, gain[i]); q=sappend(q, " "); q=sappend(q, MN[i]); first=0; }
      q=sappend(q, " - A SUPPLY CACHE!"); *q=0; S->note_t=2.2f; }
    sfx(11); do_save();
}
static void craft_demolish_at(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int txs[2] = { bw_tileof(b->x + fcos(b->aim)*BW_TILE), bw_tileof(b->x) };
    int tys[2] = { bw_tileof(b->y + fsin(b->aim)*BW_TILE), bw_tileof(b->y) };
    static const int KIND2REC[12] = { 16, 18, 17, 19, 26, 25, 20, 21, 22, 27, 23, 24 };
    for (int t=0;t<2;t++){
        int bi = craft_build_at(b, txs[t], tys[t]);
        if (bi<0) continue;
        const CRec *R = &CREC[KIND2REC[b->craft_bk[bi]%12]];
        int rw=R->w/2, rs=R->s/2, rf=R->f/2, rc=R->c/2;
        b->craft_wood+=rw; b->craft_stone+=rs; b->craft_fiber+=rf; b->craft_car+=rc;
        splat(txs[t]*BW_TILE+24, tys[t]*BW_TILE+24, COL(170,140,90), 8, 90);
        craft_build_remove(b, bi);
        { char *q=S->note; q=sappend(q, "DEMOLISHED.");
          if (rw){ q=sappend(q, " +"); q=sapp_int(q, rw); q=sappend(q, "W"); }
          if (rs){ q=sappend(q, " +"); q=sapp_int(q, rs); q=sappend(q, "S"); }
          if (rf){ q=sappend(q, " +"); q=sapp_int(q, rf); q=sappend(q, "F"); }
          if (rc){ q=sappend(q, " +"); q=sapp_int(q, rc); q=sappend(q, "C"); }
          q=sappend(q, " BACK."); *q=0; S->note_t=2.0f; }
        S->craft_demolish=0; sfx(4); do_save(); return;
    }
    craft_note(S, "NOTHING TO DEMOLISH THERE."); S->note_t=1.4f; sfx(16);
}
static void craft_bug_die(int pi, int i){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    int kind = S->craft_bk[i];
    splat(S->craft_bx[i], S->craft_by[i], kind==2? COL(250,205,80) : COL(150,80,70), 6, 90);
    if (kind==2){
        b->craft_car += 5; if (b->craft_car>999) b->craft_car=999;
        b->craft_fiber += 3; if (b->craft_fiber>999) b->craft_fiber=999;
        craft_note(S, "A GOLDEN BUG! +5 CARAPACE +3 FIBER");
    } else {
        int gain = kind==1? 2 + (int)(hash2(i*7+1, (int)(b->x), craft_seed(b))%2)
                           : 1 + (int)(hash2(i*7+1, (int)(b->x), craft_seed(b))%2);
        b->craft_car += gain; if (b->craft_car>999) b->craft_car=999;
        { char *q=S->note; q=sappend(q, "+"); q=sapp_int(q, gain); q=sappend(q, " CARAPACE"); *q=0; S->note_t=1.4f; }
    }
    S->craft_bhp[i]=0;
    sfx(11);
}
static void craft_spawn_bug(int pi, float ang, float dist, int force_kind){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->craft_bugn>=14) return;
    float nx = b->x + fcos(ang)*dist, ny = b->y + fsin(ang)*dist;
    if (craft_solid_at(pi, nx, ny, 0)) return;
    int i=S->craft_bugn++;
    int kind = force_kind;
    if (kind<0){
        u32 h = hash2((int)(nx*3), (int)(ny*3) + (int)(G.anim_t*7), craft_seed(b));
        int r = (int)(h%100);
        kind = r<4? 2 : (craft_isnight(b) && r<44)? 1 : 0;
    }
    S->craft_bx[i]=nx; S->craft_by[i]=ny; S->craft_bk[i]=kind;
    S->craft_bhp[i] = kind==1? 4 : kind==2? 1 : 2;
    S->craft_bph[i] = hash_f(i*13, (int)(nx), craft_seed(b))*6.28f;
    S->craft_btr[i]=0;
}
static int craft_bedroll_near(Buddy *b){
    for (int i=0;i<b->craft_bn;i++){
        if (b->craft_bk[i]!=8) continue;
        float dx=b->x-(b->craft_btx[i]*BW_TILE+24), dy=b->y-(b->craft_bty[i]*BW_TILE+24);
        if (dx*dx+dy*dy < 70*70) return 1;
    }
    return 0;
}
static void craft_fish_at(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    float r = hash_f((int)(b->x/24), (int)(b->y/24) + (int)(G.anim_t*7), craft_seed(b));
    sfx(12);
    splat(b->x + fcos(b->aim)*34, b->y + fsin(b->aim)*34, COL(170,220,255), 4, 60);
    if (r < 0.45f){ b->craft_fish++; craft_note(S, "CAUGHT A LAKE FISH!"); }
    else if (r < 0.70f){ b->craft_fiber++; craft_note(S, "SOGGY FIBER. THE LAKE PROVIDES. SORT OF."); }
    else if (r < 0.85f){ b->craft_car++; craft_note(S, "A SNAPPING TURTLE-BUG! +1 CARAPACE."); }
    else craft_note(S, "IT GOT AWAY. THE ONE THAT GOT AWAY WAS HUGE.");
    S->note_t = 2.0f;
    do_save();
}
static void craft_shark_die(int pi, int i){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    splat(S->craft_sh_x[i], S->craft_sh_y[i], COL(190,200,210), 10, 110); sfx(17);
    b->craft_car += 2; if ((i&1)==0) b->craft_car += 1;
    b->craft_fiber += 1;
    craft_note(S, "SHARK DOWN! THE TEETH STOP HERE. +CARAPACE");
    S->craft_sh_hp[i]=0;
    do_save();
}
static void craft_shark_spawn(int pi, float ang, float dist){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->craft_shn>=6) return;
    for (int t=0;t<6;t++){
        float a = ang + t*1.05f;
        float x = b->x + fcos(a)*dist, y = b->y + fsin(a)*dist;
        if (craft_biome_at(b, bw_tileof(x), bw_tileof(y))!=5) continue;
        int i=S->craft_shn++;
        S->craft_sh_x[i]=x; S->craft_sh_y[i]=y; S->craft_sh_hp[i]=3;
        S->craft_sh_ph[i]=hash_f((int)x, (int)y, craft_seed(b))*6.28f; S->craft_sh_tr[i]=0; S->craft_sh_hd[i]=a;
        return;
    }
}
