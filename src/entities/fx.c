// THE SURVIVAL GAME — entities/fx.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 792-839 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Entity helpers
// ---------------------------------------------------------------------------
// WHALES ONLY: PARADE DUCK — per-player follower state (purely visual)
static float duck_x[MAXP], duck_y[MAXP], duck_ph[MAXP];
static int duck_on[MAXP], duck_face[MAXP];
static void spawn_particle(float x, float y, u32 col, float spd, float life, int size){
    for (int i=0;i<MAXPART;i++) if (!G.parts[i].alive){
        Particle *q=&G.parts[i];
        float a = rng_f()*TAU;
        q->alive=1; q->x=x; q->y=y; q->vx=fcos(a)*spd*rng_f(); q->vy=fsin(a)*spd*rng_f();
        q->life=q->maxlife=life*(0.5f+rng_f()*0.5f); q->size=size; q->col=col; return;
    }
}
static void splat(float x, float y, u32 col, int n, float spd){
    for (int i=0;i<n;i++) spawn_particle(x, y, col, spd, 0.55f, 2+(int)(rng_f()*3));
}
static void spawn_floater(float x, float y, const char *txt, u32 col){
    for (int i=0;i<MAXFLOAT;i++) if (!G.floats[i].alive){
        Floater *f=&G.floats[i];
        f->alive=1; f->x=x; f->y=y; f->life=1.1f; scpy(f->text, txt, 24); f->col=col; return;
    }
}
static void floater_num(float x, float y, const char *pre, int v, const char *post, u32 col){
    char b[24]; fmt_int(b, pre, v, post); spawn_floater(x, y, b, col);
}
static CarcassFx *spawn_fx(int mode, float x0, float y0, float x1, float y1, float w, int player){
    for (int i=0;i<(int)(sizeof(G.fx)/sizeof(G.fx[0]));i++) if (!G.fx[i].alive){
        CarcassFx *f=&G.fx[i];
        f->alive=1; f->mode=mode; f->x0=x0; f->y0=y0; f->x1=x1; f->y1=y1;
        f->t=0; f->dur=0.42f; f->w=w; f->player=player; f->wall_slot=-1; return f;
    }
    return 0;
}
// hat stack ops — the stack is both hat and wallet
static void hat_push(Player *p, float w){
    if (p->stack_n < MAXSTACK) p->stack[p->stack_n++] = w;
    else p->stack[MAXSTACK-1] += w;                    // merge overflow
    p->carried += w;
    if (p->carried > p->heaviest) p->heaviest = p->carried;
}
static float hat_pop(Player *p){                        // returns weight or 0
    if (p->stack_n<=0) return 0;
    float w = p->stack[--p->stack_n];
    p->carried -= w; if (p->carried<0) p->carried=0;
    return w;
}

