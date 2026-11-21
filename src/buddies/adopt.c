// THE SURVIVAL GAME — buddies/adopt.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 1020-1042 in the Nov 2026 fine-grained refactor; code unchanged.

static void buddy_adopt(int pi, int species){
    Buddy *b=&G.bud[pi];
    smemset(b, 0, sizeof(*b));
    b->active=1; b->species=species; b->on_wall=-1;
    b->name_idx = bud_dname[pi]; b->color = bud_color_draft[pi];
    buddy_refresh(pi);
    b->hp = b->maxhp;
    if (G.fs_pool[pi] > 0){   // v1.25: First Shift tuition lands
        buddy_award_xp(pi, G.fs_pool[pi]);
        G.fs_pool[pi] = 0;
    }
    b->energy = BUD_ENERGY_MAX;
    b->x = G.p[pi].x + 26; b->y = G.p[pi].y + 12;
    b->aim = -PI/2;
    splat(b->x, b->y, COL(255,214,120), 12, 100);
}

// pending store payment (one at a time; the till is not self-service)
static int   store_open_local=0;      // client-side store UI flag
static int   client_station=0;        // client-side: which mall station panel is open
static int   shield_remote=0;         // client copy: shield state from snapshots
static int   pay_active=0, pay_player=0, pay_kind=0, pay_id=0;
static float pay_left=0, pay_t=0, pay_total=0;
