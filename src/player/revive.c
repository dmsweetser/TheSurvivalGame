// THE SURVIVAL GAME — player/revive.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 1227-1271 in the Nov 2026 fine-grained refactor; code unchanged.

static int revive_price(Player *down){
    int v = 30 + (int)(player_dps(down)*0.25f) + down->maxhp/8;
    return v<15? 15 : v;
}
static Player *downed_near(Player *p){
    for (int j=0;j<MAXP;j++){ Player *q=&G.p[j];
        if (q==p || !q->connected || q->alive) continue;
        float dx=q->x-p->x, dy=q->y-p->y;
        if (dx*dx+dy*dy <= 46*46) return q;
    }
    return 0;
}
static int try_revive(int pi){
    Player *p=&G.p[pi];
    Player *q = downed_near(p);
    if (!q) return 0;
    int price = revive_price(q);
    if (p->carried < (float)price){
        char b[48]; char *s=b; s=sappend(s, "REVIVE NEEDS "); s=sapp_int(s, price); s=sappend(s, " KG"); *s=0;
        spawn_floater(p->x, p->y-30, b, COL(255,120,110));
        sfx(16);
        return 1;   // the press was spent on the attempt
    }
    float need = (float)price;
    while (need > 0 && p->stack_n>0) need -= hat_pop(p);
    q->alive=1; q->hp=q->maxhp/2; q->shield=q->shield_max/2;
    q->revive_t=3.0f; q->dead_t=0;
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
        float dx=G.bugs[i].x-q->x, dy=G.bugs[i].y-q->y, d2=dx*dx+dy*dy;
        if (d2 < 140*140 && d2>0.01f){ float d=fsqrt(d2); G.bugs[i].vx += dx/d*340; G.bugs[i].vy += dy/d*340; }
    }
    spawn_floater(q->x, q->y-34, "REVIVED!", COL(120,255,160));
    splat(q->x, q->y, COL(120,255,160), 22, 160);
    sfx(12);
    return 1;
}
// the owner's best gun for a bot to carry: highest per-shot damage among
// the projectile weapons (the Debugger counts as its keyboard fallback)
static int best_weapon_idx(Player *p, float *per_shot){
    int bw=0; float bd=0;
    for (int w=0;w<4;w++) if (p->owned[w]){ float d=weap_dmg(w)*p->dmg_mul; if (d>bd){ bd=d; bw=w; } }
    if (p->owned[4] && 30.0f*p->dmg_mul>bd){ bd=30.0f*p->dmg_mul; bw=0; }
    if (per_shot) *per_shot=bd;
    return bw;
}
