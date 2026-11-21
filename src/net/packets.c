// THE SURVIVAL GAME — net/packets.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 1438-1532 in the Nov 2026 fine-grained refactor; code unchanged.

static int pkt_src_token[MAXP];       // last transport token per slot (host)
static float net_send_t=0;
// event queue for snapshots (splats, purchases, wall builds...)
typedef struct { u8 type; float x, y; } NetEv;
static NetEv ev_queue[32]; static int ev_n=0;
static void push_ev(int type, float x, float y){
    if (G.net_mode!=NET_HOST) return;
    if (ev_n<32){ ev_queue[ev_n].type=(u8)type; ev_queue[ev_n].x=x; ev_queue[ev_n].y=y; ev_n++; }
}
static void wr_u8(int *o, u8 v){ pkt[(*o)++]=v; }
static void wr_u16(int *o, u16 v){ pkt[(*o)++]=v&255; pkt[(*o)++]=(v>>8)&255; }
static void wr_flt(int *o, float f){ union{float f; u8 b[4];} u; u.f=f;
    for (int i=0;i<4;i++) pkt[(*o)++]=u.b[i]; }
static u8 rd_u8(const u8 *b, int *o){ return b[(*o)++]; }
static u16 rd_u16(const u8 *b, int *o){ u16 v=b[*o] | (b[*o+1]<<8); *o+=2; return v; }
static float rd_flt(const u8 *b, int *o){ union{float f; u8 b[4];} u;
    for (int i=0;i<4;i++) u.b[i]=b[(*o)++]; return u.f; }

static void host_send_roster(int dest){
    int o=0; wr_u8(&o, 'R');
    int n=0; for (int i=0;i<MAXP;i++) if (G.p[i].connected) n++;
    wr_u8(&o, (u8)n);
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        wr_u8(&o, (u8)i); wr_u8(&o, (u8)G.p[i].color);
        for (int k=0;k<12;k++) wr_u8(&o, (u8)G.p[i].name[k]);
    }
    plat_net_send(pkt, o, dest);
}
static void host_send_walls(int dest){
    int o=0; wr_u8(&o, 'W');
    int n=0; for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1) n++;
    wr_u16(&o, (u16)n);
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){
        Wall *w=&G.walls[i];
        wr_flt(&o, w->x); wr_flt(&o, w->y); wr_flt(&o, w->hp); wr_flt(&o, w->maxhp);
        wr_u8(&o, (u8)w->horiz); wr_u8(&o, (u8)(w->stack>0? w->stack : 1));
    }
    plat_net_send(pkt, o, dest);
}
static void host_snapshot(void){
    int o=0; wr_u8(&o, 'S');
    wr_flt(&o, G.time);
    wr_u16(&o, (u16)G.total_kills);
    wr_u8(&o, (u8)G.diff_run);
    for (int i=0;i<MAXP;i++){
        Player *p=&G.p[i];
        wr_u8(&o, p->connected?1:0);
        if (!p->connected) continue;
        wr_flt(&o, p->x); wr_flt(&o, p->y); wr_flt(&o, p->aim);
        wr_u16(&o, (u16)(p->hp<0?0:p->hp)); wr_u16(&o, (u16)p->shield);
        wr_u8(&o, (u8)p->weapon); wr_u8(&o, p->alive?1:0);
        wr_u16(&o, (u16)p->stack_n); wr_flt(&o, p->carried); wr_flt(&o, p->heaviest);
        wr_u16(&o, (u16)p->kills);
        wr_u8(&o, (u8)p->rollback);
        for (int w=0;w<NWEAP;w++) wr_u16(&o, (u16)p->owned[w]);
        for (int u=0;u<5;u++) wr_u16(&o, (u16)p->upg[u]);
        wr_flt(&o, p->banked); wr_u8(&o, (u8)p->hatstand); wr_u8(&o, (u8)p->in_mall);
        wr_u8(&o, p->on_wall>=0? 1:0);
        wr_u8(&o, (u8)p->whale_own); wr_u8(&o, (u8)p->whale_on);
        wr_u8(&o, (u8)(p->whale_own>>8)); wr_u8(&o, (u8)(p->whale_on>>8));   // round 2 vanities (SKUs 7-14)
        wr_u8(&o, (u8)p->top_worn);   // HAT TOPPERS (feat 5): guests see the worn trophy
    }
    int bn=0; for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive) bn++;
    wr_u16(&o, (u16)bn);
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
        Bug *b=&G.bugs[i];
        wr_flt(&o, b->x); wr_flt(&o, b->y); wr_flt(&o, b->w);
        wr_u8(&o, (u8)(255*b->hp/b->maxhp)); wr_u8(&o, (u8)b->kind);
        wr_u8(&o, b->bp_t>0?1:0); wr_u8(&o, b->hit_t>0?1:0);
        wr_flt(&o, b->leg_ph);
        wr_u8(&o, (u8)b->elite);   // NAMED ELITES (feat 8): guests see the nameplate
    }
    wr_u8(&o, (u8)ev_n);
    for (int i=0;i<ev_n;i++){ wr_u8(&o, ev_queue[i].type); wr_flt(&o, ev_queue[i].x); wr_flt(&o, ev_queue[i].y); }
    ev_n=0;
    wr_u8(&o, shield_active()?1:0);
    wr_u8(&o, 0);   // agents retired (Nov 2): the count byte stays, always zero
    int bn2=0; for (int i=0;i<MAXP;i++) if (G.bud[i].active) bn2++;
    wr_u8(&o, (u8)bn2);
    for (int i=0;i<MAXP;i++) if (G.bud[i].active){
        Buddy *bd=&G.bud[i];
        wr_u8(&o, (u8)i); wr_u8(&o, (u8)bd->species);
        wr_flt(&o, bd->x); wr_flt(&o, bd->y);
        wr_u8(&o, (u8)(bd->maxhp>0? 255.0f*bd->hp/bd->maxhp : 0));
        wr_u8(&o, bd->lunge_t>0? 1:0);
        wr_u16(&o, (u16)(bd->xp>60000? 60000 : bd->xp));
        wr_u8(&o, (u8)bd->up_hp); wr_u8(&o, (u8)bd->up_atk); wr_u8(&o, (u8)bd->up_def); wr_u8(&o, (u8)bd->skills);
        wr_u8(&o, bd->fainted? 1:0);
        wr_flt(&o, bd->walk_ph);
        wr_u8(&o, (u8)bd->name_idx); wr_u8(&o, (u8)bd->color);
        wr_u8(&o, (u8)bd->energy);
        wr_u8(&o, (u8)(bd->on_wall+1));
    }
    plat_net_send(pkt, o, -1);
}
