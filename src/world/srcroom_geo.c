// THE SURVIVAL GAME — world/srcroom_geo.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
//
// THE SOURCE ROOM (v1.26 Item 1a) — a small room off the Bug Mall's
// north wall holding the source terminal (station 9). The door sits in
// the gap between counter 0 (-315) and counter 1 (-105): their
// collision boxes end at x -246 and -174, so the strip around
// SRC_DOOR_X (-210) is standable floor with no station disc in reach
// (both counter discs are ~105 away vs their r64). The gate is the
// Trophy Case earned count vs the SRC_DOOR_TROPHIES live variable
// (data/design.c, registered in data/livedata.c). Room state is
// file-local statics like the Arcade's — nothing joins the save struct;
// the trophy count derives from saved records, so the gate survives
// save/load on its own.
#define SRC_CY (-18000)          // free band: MALL_CY -6000, ARC_CY -12000
#define SRC_DOOR_X (-210)
static int in_srcroom[MAXP];
static int trophy_earned_count(Player *p);   // mall/trophy_case.c, below

static int src_gate_open(int pi){
    return trophy_earned_count(&G.p[pi]) >= SRC_DOOR_TROPHIES;
}
static int src_door_at(Player *p){
    return p->in_mall && !p->in_arcade && !in_srcroom[p-G.p]
        && p->x>SRC_DOOR_X-32 && p->x<SRC_DOOR_X+32
        && p->y>MALL_CY-204 && p->y<MALL_CY-156;
}
static int srcroom_exit_at(Player *p){
    return in_srcroom[p-G.p] && p->x>-40 && p->x<40
        && p->y>SRC_CY+168 && p->y<SRC_CY+212;
}
static void srcroom_enter(int pi){
    Player *p=&G.p[pi];
    in_srcroom[pi]=1; p->station=-1;
    p->x=0; p->y=SRC_CY+158; p->aim=-PI/2;
    splat(p->x, p->y, COL(120,255,170), 10, 90);
    sfx(19);
}
static void srcroom_exit(int pi){
    Player *p=&G.p[pi];
    in_srcroom[pi]=0; p->station=-1;
    p->x=SRC_DOOR_X; p->y=MALL_CY-140; p->aim=PI/2;
    splat(p->x, p->y, COL(120,255,170), 10, 90);
    sfx(19);
}
static void srcroom_reset_all(void){
    for (int i=0;i<MAXP;i++) in_srcroom[i]=0;
}
// the terminal counter's interact disc, mirroring the mall stations
static int src_station_at(Player *p){
    if (!in_srcroom[p-G.p]) return -1;
    float dx=p->x - 0, dy=p->y - (SRC_CY-148);
    if (dx*dx + dy*dy < 64*64) return 9;
    return -1;
}
// one collision box in the mall idiom (push out the shallow way)
static void src_box(Player *p, float cx, float hx, float hy0, float hy1){
    float rad=11;
    if (p->x > cx-hx-rad && p->x < cx+hx+rad && p->y > hy0-rad && p->y < hy1+rad){
        float dyr = (hy1+rad) - p->y, dyl = p->y - (hy0-rad);
        float dxr = (cx+hx+rad) - p->x, dxl = p->x - (cx-hx-rad);
        float m=dyr; int side=0;
        if (dyl<m){ m=dyl; side=1; } if (dxr<m){ m=dxr; side=2; } if (dxl<m){ side=3; }
        if (side==0) p->y = hy1+rad; else if (side==1) p->y = hy0-rad;
        else if (side==2) p->x = cx+hx+rad; else p->x = cx-hx+rad;
    }
}
static void srcroom_collide(Player *p){
    if (p->x < -384) p->x = -384; if (p->x > 384) p->x = 384;
    if (p->y < SRC_CY-204) p->y = SRC_CY-204;
    if (p->y > SRC_CY+198) p->y = SRC_CY+198;
    src_box(p, 0, 58, SRC_CY-198, SRC_CY-166);        // the terminal counter
    src_box(p, -262, 30, SRC_CY-84, SRC_CY+4);        // server rack, west
    src_box(p, 262, 30, SRC_CY-84, SRC_CY+4);         // server rack, east
}
// the sealed door's answer to Interact: a floater with the live count
static void src_door_sealed(int pi){
    Player *p=&G.p[pi];
    char b[64]; char *q=b;
    q=sappend(q, "THE SOURCE IS SEALED - TROPHIES ");
    q=sapp_int(q, trophy_earned_count(p));
    q=sappend(q, "/"); q=sapp_int(q, SRC_DOOR_TROPHIES); *q=0;
    spawn_floater(p->x, p->y-24, b, COL(255,170,120));
    sfx(16);
}
