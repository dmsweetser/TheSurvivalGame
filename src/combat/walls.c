// THE SURVIVAL GAME — combat/walls.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 1590-1869 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Interlocked walls (Daniel's rules): a wall placed touching others joins
// their RUN. A run shares ONE health pool worth 1.1x the sum of its
// segments' individual health; bugs chewing anywhere drain the pool, and
// the segment being chewed gives way once the damage since the last break
// passes its own share. A placement that would close a complete circuit
// is refused. Runs of 5+ grow cosmetic tiers — and you can walk on top.
// ---------------------------------------------------------------------------
static float grp_pool[MAXWALLS];   // shared health remaining (leader-indexed)
static float grp_cap[MAXWALLS];    // 1.1 * sum of member base hp
static float grp_dsb[MAXWALLS];    // damage since the last segment broke
static int   grp_n[MAXWALLS];      // member count
static int grp_tier(int n){ return n>=100? 4 : n>=20? 3 : n>=10? 2 : n>=5? 1 : 0; }
static int wall_adjacent(Wall *a, Wall *b){
    float ahx = a->horiz? 26:9, ahy = a->horiz? 9:26;
    float bhx = b->horiz? 26:9, bhy = b->horiz? 9:26;
    float dx = a->x-b->x; if (dx<0) dx=-dx;
    float dy = a->y-b->y; if (dy<0) dy=-dy;
    return dx < ahx+bhx+10 && dy < ahy+bhy+10;
}
static int uf_find(int *par, int i){ while (par[i]!=i){ par[i]=par[par[i]]; i=par[i]; } return i; }
// rebuild the groups. Pool health carries across reshapes by share: a
// wall coming from an old group brings that group's remaining pool in
// proportion to its base; a loner brings what it had left, plus the
// interlock bonus on the remainder. fresh=1 (save load): pool = cap x
// the saved damage fraction instead.
static void walls_recompute_ex(int fresh){
    static int par[MAXWALLS], lead[MAXWALLS];
    // Nov 15: snapshot the OUTGOING pool state before anything is
    // rebuilt. The carry below used to test grp_n[grp_old] AFTER
    // grp_n had been recounted for the NEW grouping — when a run's
    // leader died, the survivors silently fell out of the share
    // branch, and a chewed loner's pool record (never updated by
    // loner damage) let it rejoin a run at health it no longer had.
    static int oldn[MAXWALLS]; static float oldcap[MAXWALLS], oldpool[MAXWALLS];
    for (int i=0;i<MAXWALLS;i++){ oldn[i]=grp_n[i]; oldcap[i]=grp_cap[i]; oldpool[i]=grp_pool[i]; }
    int sim = (G.net_mode!=NET_CLIENT);
    for (int i=0;i<MAXWALLS;i++){ par[i]=i; lead[i]=-1; }
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1)
        for (int j=i+1;j<MAXWALLS;j++) if (G.walls[j].alive==1 && wall_adjacent(&G.walls[i], &G.walls[j])){
            int a=uf_find(par,i), b=uf_find(par,j); if (a!=b) par[b]=a;
        }
    static float npool[MAXWALLS], ncap[MAXWALLS];
    for (int i=0;i<MAXWALLS;i++){ npool[i]=0; ncap[i]=0; grp_n[i]=0; }
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1)
        G.walls[i].grp = uf_find(par,i);   // root for now; leader fixed below
    // leader must be the smallest member index (stable identity)
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){
        int r = G.walls[i].grp;
        if (lead[r]<0 || i<lead[r]) lead[r]=i;
    }
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){
        int L = lead[G.walls[i].grp]; G.walls[i].grp = L;
        // a reinforced stack counts as the several walls it is: it
        // feeds the run's tiers, its walk-on-top size and its pool
        // membership exactly like the separate entries it replaced
        grp_n[L] += G.walls[i].stack>0? G.walls[i].stack : 1;
        ncap[L] += G.walls[i].maxhp;
    }
    if (sim){
        for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){
            Wall *w=&G.walls[i]; int L=w->grp;
            if (fresh) npool[L] += w->hp * 1.1f;                    // saved hp was a fraction of base; x1.1 rebuilds the pool share
            else if (w->grp_old>=0 && oldn[w->grp_old]>=2 && oldcap[w->grp_old]>0)
                npool[L] += oldpool[w->grp_old] * (w->maxhp*1.1f) / oldcap[w->grp_old];
            else npool[L] += w->hp + w->maxhp*0.1f;                 // loner/new: what it had + interlock bonus on the base
        }
        for (int i=0;i<MAXWALLS;i++) if (grp_n[i]>0){
            grp_cap[i] = ncap[i]*1.1f;
            grp_pool[i] = npool[i]; if (grp_pool[i]>grp_cap[i]) grp_pool[i]=grp_cap[i]; if (grp_pool[i]<0) grp_pool[i]=0;
            grp_dsb[i]=0;
            if (grp_n[i]>=2){
                float frac = grp_cap[i]>0? grp_pool[i]/grp_cap[i] : 0;
                for (int m=0;m<MAXWALLS;m++) if (G.walls[m].alive==1 && G.walls[m].grp==i)
                    G.walls[m].hp = G.walls[m].maxhp * frac;
            }
        }
        if (G.net_mode==NET_HOST) wall_send_t = 0.05f;
    }
    // endpoints: members with at most one neighbour inside the run
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1){
        int cnt=0;
        for (int j=0;j<MAXWALLS;j++) if (j!=i && G.walls[j].alive==1 && G.walls[j].grp==G.walls[i].grp && wall_adjacent(&G.walls[i], &G.walls[j])) cnt++;
        G.walls[i].endpt = (cnt<=1 && grp_n[G.walls[i].grp]>=2)? 1:0;
    }
    for (int i=0;i<MAXWALLS;i++) G.walls[i].grp_old = G.walls[i].grp;
}
static void walls_recompute(void){ walls_recompute_ex(0); }
static void break_wall(int wi){
    Wall *w=&G.walls[wi];
    // Reinforcement (restored Nov 7): a stacked wall gives way ONE
    // layer at a time — the top course crumbles off, and the rest of
    // the pile, with its remaining share of the run's pool, fights
    // on. Only a stack chewed down to its last layer, or one drained
    // dry (hp ~0), falls for good.
    if (w->stack>1 && w->hp>1.0f){
        float share = w->maxhp / w->stack;
        w->stack--;
        w->maxhp -= share; if (w->maxhp<1.0f) w->maxhp=1.0f;
        if (w->hp>w->maxhp) w->hp=w->maxhp;
        splat(w->x, w->y, COL(170,120,70), 10, 110);
        push_ev(2, w->x, w->y);
        if (G.net_mode==NET_HOST) wall_send_t = 0.05f;
        sfx(17);
        walls_recompute();
        return;
    }
    w->alive=0; w->grp=-1; w->grp_old=-1;
    splat(w->x, w->y, COL(170,120,70), 14, 120);
    push_ev(2, w->x, w->y);
    if (G.net_mode==NET_HOST) wall_send_t = 0.05f;
    sfx(17);
    walls_recompute();
}
static void damage_wall(int wi, float dmg){
    Wall *w=&G.walls[wi];
    if (w->alive!=1) return;
    int L = w->grp;
    if (L>=0 && grp_n[L]>=2){
        grp_pool[L] -= dmg; grp_dsb[L] += dmg;
        float frac = grp_cap[L]>0? grp_pool[L]/grp_cap[L] : 0; if (frac<0) frac=0;
        for (int m=0;m<MAXWALLS;m++) if (G.walls[m].alive==1 && G.walls[m].grp==L)
            G.walls[m].hp = G.walls[m].maxhp * frac;
        // the chewed segment gives way after taking 1.1x ONE layer's
        // share — for a reinforced stack that sheds a layer, not the pile
        float layer_hp = w->maxhp / (w->stack>0? w->stack : 1);
        if (grp_pool[L]<=0 || grp_dsb[L] >= 1.1f*layer_hp) break_wall(wi);
    } else {
        w->hp -= dmg;
        // keep a loner's pool record truthful (Nov 15): the record
        // is what the share carry reads when this wall later joins
        // a run — left stale, a chewed wall rejoined at the health
        // it had when the record was last written, not its own.
        if (w->grp>=0){ grp_cap[w->grp] = w->maxhp*1.1f;
            grp_pool[w->grp] = w->hp + w->maxhp*0.1f; if (grp_pool[w->grp]<0) grp_pool[w->grp]=0; }
        if (w->hp<=0) break_wall(wi);
    }
}
// would a wall at (wx,wy,horiz) close a circuit? For every pair of its
// neighbours already in one run, walk the existing path between them:
// if that path plus the new wall encloses real area, it's a loop.
static int placement_closes_loop(float wx, float wy, int horiz){
    Wall cand; cand.alive=1; cand.horiz=horiz; cand.x=wx; cand.y=wy;
    int nb[8], nn=0;
    for (int i=0;i<MAXWALLS && nn<8;i++)
        if (G.walls[i].alive==1 && wall_adjacent(&cand, &G.walls[i])) nb[nn++]=i;
    for (int a=0;a<nn;a++) for (int b=a+1;b<nn;b++){
        if (G.walls[nb[a]].grp != G.walls[nb[b]].grp || G.walls[nb[a]].grp<0) continue;
        // BFS the run from nb[a] to nb[b]
        static int par2[MAXWALLS]; static int qq[MAXWALLS];
        for (int i=0;i<MAXWALLS;i++) par2[i]=-2;
        int qh=0, qt=0; qq[qt++]=nb[a]; par2[nb[a]]=-1;
        while (qh<qt && par2[nb[b]]==-2){
            int cur=qq[qh++];
            for (int j=0;j<MAXWALLS;j++)
                if (G.walls[j].alive==1 && G.walls[j].grp==G.walls[cur].grp && par2[j]==-2 && wall_adjacent(&G.walls[cur], &G.walls[j])){
                    par2[j]=cur; qq[qt++]=j;
                }
        }
        if (par2[nb[b]]==-2) continue;
        // shoelace over the closed polygon: path points, then the
        // candidate, then back to the start — EVERY edge counts
        // (dropping the candidate's closing edge once silently
        // shrank the area and let loops through)
        float pts[440][2]; int np=0;
        for (int c=nb[b]; c!=-1; c=par2[c]){ pts[np][0]=G.walls[c].x; pts[np][1]=G.walls[c].y; np++; }
        pts[np][0]=wx; pts[np][1]=wy; np++;
        float area=0;
        for (int i=0;i<np;i++){
            int j=(i+1)%np;
            area += pts[i][0]*pts[j][1] - pts[j][0]*pts[i][1];
        }
        if (area<0) area=-area;
        if (area*0.5f > 2400.0f) return 1;
    }
    return 0;
}

static int wall_box_hit(float x, float y, float rad, Wall *w){
    float hx = w->horiz? 26.0f : 9.0f, hy = w->horiz? 9.0f : 26.0f;
    return (x > w->x-hx-rad && x < w->x+hx+rad && y > w->y-hy-rad && y < w->y+hy+rad);
}
// The Bug Mall is SOLID — the whole storefront: building, awning,
// counter and crates (the same footprint tap_on_store uses). Entry
// is by INTERACT from the plaza, so the collision needs no door
// gap. Applied to players, bugs, bots and buddies: nothing walks
// through the mall. (It lives inside collide_walls too, and is
// called directly by the movers that skip wall collision.)
static void collide_hut(float *x, float *y, float rad){
    if (*x > -92-rad && *x < 92+rad && *y > -116-rad && *y < -14+rad){
        float dyl = *y-(-116-rad), dyr = (-14+rad)-*y;
        float dxl = *x-(-92-rad), dxr = (92+rad)-*x;
        float m=dxl; int side=0;
        if (dxr<m){ m=dxr; side=1; } if (dyl<m){ m=dyl; side=2; } if (dyr<m){ side=3; }
        if (side==0) *x=-92-rad; else if (side==1) *x=92+rad;
        else if (side==2) *y=-116-rad; else *y=-14+rad;
    }
}
static void collide_walls(float *x, float *y, float rad){
    for (int i=0;i<MAXWALLS;i++){
        Wall *w=&G.walls[i];
        if (w->alive!=1) continue;
        float hx = w->horiz? 26.0f : 9.0f, hy = w->horiz? 9.0f : 26.0f;
        if (*x > w->x-hx-rad && *x < w->x+hx+rad && *y > w->y-hy-rad && *y < w->y+hy+rad){
            float dxl = *x-(w->x-hx-rad), dxr = (w->x+hx+rad)-*x;
            float dyl = *y-(w->y-hy-rad), dyr = (w->y+hy+rad)-*y;
            float m = dxl; int side=0;
            if (dxr<m){ m=dxr; side=1; } if (dyl<m){ m=dyl; side=2; } if (dyr<m){ side=3; }
            if (side==0) *x = w->x-hx-rad; else if (side==1) *x = w->x+hx+rad;
            else if (side==2) *y = w->y-hy-rad; else *y = w->y+hy+rad;
        }
    }
    collide_hut(x, y, rad);
}
// is the player still standing over his own run?
static int over_own_run(Player *p){
    if (p->on_wall<0) return 0;
    for (int i=0;i<MAXWALLS;i++)
        if (G.walls[i].alive==1 && G.walls[i].grp==p->on_wall && wall_box_hit(p->x, p->y, 11, &G.walls[i])) return 1;
    return 0;
}
// wall collision with a door in it: step INTO a cosmetically-merged run
// (5+ interlocked) and you're up on top of it, walking the wall. Step
// off the run anywhere and you drop back to the ground.
static void wall_step(Player *p){
    if (p->in_mall){ p->on_wall=-1; return; }
    int hit_grp=-1, hit_other=0;
    for (int i=0;i<MAXWALLS;i++){
        Wall *w=&G.walls[i];
        if (w->alive!=1) continue;
        if (!wall_box_hit(p->x, p->y, 11, w)) continue;
        if (w->grp>=0 && grp_n[w->grp]>=5){ if (hit_grp<0) hit_grp=w->grp; }
        else hit_other=1;
    }
    if (hit_grp>=0 && !hit_other){ p->on_wall=hit_grp; return; }   // up (or still up)
    if (hit_other){
        collide_walls(&p->x, &p->y, 11);
        if (p->on_wall>=0 && !over_own_run(p)) p->on_wall=-1;
        return;
    }
    if (p->on_wall>=0 && !over_own_run(p)) p->on_wall=-1;           // stepped off the end
}
// the buddy climbs exactly when the player can (Daniel): same runs of
// 5+ interlocked walls, same drop-off rule — wall_step, mirrored.
static int buddy_over_run(Buddy *b){
    if (b->on_wall<0) return 0;
    for (int i=0;i<MAXWALLS;i++)
        if (G.walls[i].alive==1 && G.walls[i].grp==b->on_wall && wall_box_hit(b->x, b->y, 9, &G.walls[i])) return 1;
    return 0;
}
static void buddy_wall_step(Buddy *b){
    int hit_grp=-1, hit_other=0;
    for (int i=0;i<MAXWALLS;i++){
        Wall *w=&G.walls[i];
        if (w->alive!=1) continue;
        if (!wall_box_hit(b->x, b->y, 9, w)) continue;
        if (w->grp>=0 && grp_n[w->grp]>=5){ if (hit_grp<0) hit_grp=w->grp; }
        else hit_other=1;
    }
    if (hit_grp>=0 && !hit_other){ b->on_wall=hit_grp; return; }
    if (hit_other){
        collide_walls(&b->x, &b->y, 9);
        if (b->on_wall>=0 && !buddy_over_run(b)) b->on_wall=-1;
        return;
    }
    if (b->on_wall>=0 && !buddy_over_run(b)) b->on_wall=-1;
}
static int near_store(Player *p){
    float dx=p->x, dy=p->y+10;
    return (dx*dx+dy*dy < 130*130);
}
// a tap only opens the store when it lands ON the store building —
// tapping anywhere else in the plaza is just a tap (mobile drag fix)
static int SX(float wx); static int SY(float wy);   // defined with the renderer
static float zoom_target_of(Player *p);             // defined with the renderer
static int tap_on_store(void){
    return IN.tap_x>=SX(-92) && IN.tap_x<=SX(92) &&
           IN.tap_y>=SY(-116) && IN.tap_y<=SY(-14);
}

