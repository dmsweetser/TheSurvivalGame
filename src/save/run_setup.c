// THE SURVIVAL GAME — save/run_setup.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 330-377 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Run setup + continue-from-save
// ---------------------------------------------------------------------------
static void player_fresh(Player *p, const char *name, int color, float x, float y){
    int was_conn = p->connected;
    smemset(p, 0, sizeof(*p));
    p->connected = was_conn; p->alive = 1;
    scpy(p->name, name[0]? name : "DEV", 13);
    p->x=x; p->y=y; p->aim=-PI/2; p->cur_aim=-PI/2;
    p->maxhp=100; p->hp=100; p->weapon=0; p->owned[0]=1;
    p->dmg_mul=1; p->rate_mul=1; p->color=color;
    p->on_wall=-1; p->station=-1;
}
static void reset_world(void){
    smemset(G.bugs, 0, sizeof(G.bugs));
    smemset(G.projs, 0, sizeof(G.projs));
    smemset(G.walls, 0, sizeof(G.walls));
    for (int i=0;i<MAXWALLS;i++){ G.walls[i].grp=-1; G.walls[i].grp_old=-1; grp_n[i]=0; grp_pool[i]=0; grp_cap[i]=0; grp_dsb[i]=0; }
    smemset(G.parts, 0, sizeof(G.parts));
    smemset(G.floats, 0, sizeof(G.floats));
    smemset(G.fx, 0, sizeof(G.fx));
    smemset(G.bud, 0, sizeof(G.bud));
    smemset(G.bowl, 0, sizeof(G.bowl));
    smemset(G.blg, 0, sizeof(G.blg));   // BUG BOWL LEAGUE (feat 10): a fresh world starts a fresh book; Continue re-applies it from the pl-lines
    smemset(wall_build_n, 0, sizeof(wall_build_n));
    G.time=0; G.total_kills=0; G.total_earned=0;
    G.spawn_t=1; G.autosave_t=5; pay_active=0;
    rng_seed(0xC0FFEEu ^ (u32)(plat_now()*1000));
}
// local co-op store sessions (0 = P1 browsing, 1 = P2 browsing)
static int lstore[2];
static int help_page=0;   // HOW TO PLAY flips to a second page for the mall & friends
static int lstation[2];   // which mall station each shopper is browsing
static void coop_reset_views(void){ lstore[0]=lstore[1]=0; lstation[0]=lstation[1]=0; }
// local co-op: bring in player 2 (second controller, same machine).
// The run still lives in ONE save slot — P2 is saved and restored as a
// player line in the same blob, exactly like a LAN guest would be, and
// claimed back by name on load. P2 drops in beside P1.
static void coop_spawn_p2(float x, float y){
    if (G.p2_slot>=0) return;
    for (int i=1;i<MAXP;i++) if (!G.p[i].connected){
        player_fresh(&G.p[i], "P2", 1, x, y);
        G.p[i].connected=1;
        G.p[i].whale_own=G.whale_own; G.p[i].whale_on=G.whale_on;
        G.p2_slot=i;
        return;
    }
}
