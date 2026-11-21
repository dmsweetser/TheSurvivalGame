// THE SURVIVAL GAME — world/arcade_geo.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 128-516 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// THE ARCADE (Daniel, Nov 10): a whole room of its own off the mall's
// east wall. The Banzai World gate keeps STORY MODE and FREE PLAY;
// every other game that used to hide behind the gate now lives here,
// one arcade cabinet per game — ENTRANCES, SUPER BUDDY RUN, BUDDY
// CRAFT, and the new TANK RESCUE. The room sits far south of the mall
// in world space; in_mall stays 1 inside (same bug-safe zone), with
// in_arcade marking the room for collision, rendering and buddies.
// ---------------------------------------------------------------------------
#define ARC_CY (-12000)
// TWO ROWS of cabinets (Nov 14, Daniel — for SUPER BUDDY CART):
// three against the north wall, three on a freestanding island row
// mid-room, all facing south. ARC_ROWY is each cabinet's draw-top
// offset from ARC_CY; the collision band and the interact point
// derive from it (+8/+40 and +58), exactly as the single row did.
// A THIRD ROW (Nov 15, for WAR!): the room grew ~110px south (the
// south wall, the y-clamp and the floor text moved with it) and
// WAR! stands alone, centred, on its own row at ARC_ROWY +254.
static const int ARC_SX[7] = { -256, 0, 256, -256, 0, 256, 0 };
static const int ARC_ROWY[7] = { -166, -166, -166, 44, 44, 44, 254 };
static const char *ARC_NAME[7] = { "ENTRANCES", "SUPER BUDDY RUN", "BUDDY CRAFT", "TANK RESCUE", "CASTLE", "SUPER BUDDY CART", "WAR!" };
static const int ARC_MODE[7] = { 2, 3, 4, 5, 6, 7, 8 };         // the bw_mode each cabinet launches
static int arcgate[MAXP], arcgate_sel[MAXP], arc_cab[MAXP];   // the cabinet panel (per player)
static int bw_arc_src[MAXP], bw_arc_cab[MAXP];         // pending arcade origin for bw_enter_mode
static int bw_cas_shuffle_req[MAXP];                   // CASTLE SHUFFLE (v1.25 feat 7): the cabinet panel's pending mode-6 request
static int bw_ent_ch2_req[MAXP];                       // DEEPER DOORS (v1.25 feat 11): the cabinet panel's pending mode-2 chapter request
static int cabinet_at(Player *p){
    if (!p->in_arcade) return -1;
    for (int s=0;s<7;s++){
        float dx=p->x - ARC_SX[s], dy=p->y - (ARC_CY + ARC_ROWY[s] + 58);
        if (dx*dx + dy*dy < 74*74) return s;
    }
    return -1;
}
static int arcade_door_at(Player *p){   // the mall's east-wall door
    return p->in_mall && !p->in_arcade && p->x>312 && p->y>MALL_CY-14 && p->y<MALL_CY+66;
}
static int arcade_exit_at(Player *p){   // the arcade's west-wall mat
    return p->in_arcade && p->x<-300 && p->y>ARC_CY-14 && p->y<ARC_CY+66;
}
static void arcade_enter(int pi){
    Player *p=&G.p[pi];
    p->in_arcade=1; p->station=-1;
    p->x=-330; p->y=ARC_CY+24; p->aim=0;
    splat(p->x, p->y, COL(140,240,255), 10, 90);
    sfx(19);
}
static void arcade_exit(int pi){
    Player *p=&G.p[pi];
    p->in_arcade=0; p->station=-1;
    p->x=316; p->y=MALL_CY+24; p->aim=PI;
    splat(p->x, p->y, COL(140,240,255), 10, 90);
    sfx(19);
}
static void arc_gate_try(int pi);                      // cabinet checks + panel, below
static void arc_gate_draw(int pi, int vx, int vy, int vw2, int vh2);   // ditto
static void arc_gate_tap(int pi, int vx, int vy, int vw2, int vh2);    // ditto
static void arc_gate_pick(int pi);                     // ditto
static int arc_gate_rows(int pi);                      // CASTLE SHUFFLE (feat 7): 3 rows at the unlocked CASTLE panel, else 2
static void draw_arcade(void);                         // ditto (with the mall art)
static void bw_gate_try(int pi);                     // gate checks + mode panel, below
static void bw_enter_mode(int pi, int mode);         // defined with Banzai World, below
static int bw_qstate_g(Buddy *b, int q);             // ditto: any story quest's state, 0-9 (TROPHY CASE reads it)
static int trophy_earned_count(Player *p);           // ditto (THE SOURCE ROOM gate reads it)
static int crt_medal_count(Buddy *b, int tier);      // defined with the Cart medal table, below
static int plat_medal_count(Buddy *b, int tier);     // defined with the Run medal table, below
static int topper_refresh(int pi, int announce);     // HAT TOPPERS (feat 5): defined beside draw_hat, below
static void ent_reset(int pi);
static int ent_solid(int pi, float x, float y);
static void bw_render_ent(int pi);
static void bw_gate_draw(int pi, int vx, int vy, int vw2, int vh2);   // ditto
static void bw_gate_tap(int pi, int vx, int vy, int vw2, int vh2);    // ditto
static void p1_view_rect(int *x, int *y, int *w, int *h);             // P1's co-op view rect, below
static void bw_gate_pick(int pi);                    // ditto
static int bwgate[MAXP], bwgate_sel[MAXP];           // the gate's mode-choice panel
// Banzai World runs per-player: each frame the owning player's input is
// packed into a BwIn and handed to bw_update, wherever that player lives
// (solo full-screen, or their half of a co-op split).
typedef struct { int keys, interact, exit_, sel, up, down, wall, sel_held, pause; } BwIn;
static void ent_update(int pi, float dt, const BwIn *in);
static void plat_enter(int pi);
static void plat_update(int pi, float dt, const BwIn *in);
static void plat_render(int pi);
static void glow_ellipse(int cx, int cy, int rx, int ry, u32 c, int alpha);
static void craft_enter(int pi);
static void craft_update(int pi, float dt, const BwIn *in);
static void craft_render(int pi);
static void tank_enter(int pi);
static void tank_update(int pi, float dt, const BwIn *in);
static void tank_render(int pi);
static void cas_enter(int pi);
static void cas_update(int pi, float dt, const BwIn *in);
static void cas_render(int pi);
static int cas_solid(int pi, float x, float y);
static void crt_enter(int pi);
static void crt_update(int pi, float dt, const BwIn *in);
static void crt_render(int pi);
static void war_enter(int pi);
static void war_update(int pi, float dt, const BwIn *in);
static void war_render(int pi);
static const char *war_snap_for_save(int pi);   // v1.17: WAR! battle snapshot for the save writer (defined with the WAR! block, below)
static void bw_draw_note(int pi);
static void bw_update(int pi, float dt, const BwIn *in);   // ditto, below
static void bw_fill_p1(BwIn *bin, float dt);   // P1 input pack (touch-unified), below
static void bw_battle_tap(int pi, int vx, int vy, int vw2, int vh2);   // ditto
static void bw_render(int pi);                       // ditto
static int bw_door_at(Player *p){
    if (!p->in_mall) return 0;
    float dx=p->x - BW_SX, dy=p->y - (MALL_CY+132);
    // 100, not 64: the gate block itself holds you ~69 units off the
    // door point from the south and the west — at 64 the two most
    // natural approaches (up the door column from the entrance, east
    // along the mat row) stopped a few pixels short, and Interact did
    // nothing at all, not even the denial floaters (Daniel, Nov 3:
    // "wouldn't even let me in")
    return dx*dx + dy*dy < 100*100;
}
static int on_exit_mat(Player *p){
    return p->in_mall && p->x>-40 && p->x<40 && p->y>MALL_CY+168 && p->y<MALL_CY+212;
}
static void mall_enter(int pi){
    Player *p=&G.p[pi];
    p->in_mall=1; p->in_arcade=0; in_srcroom[pi]=0; p->bean_jet=0; p->on_wall=-1; p->station=-1;
    p->x=0; p->y=MALL_CY+158; p->aim=-PI/2;
    splat(p->x, p->y, COL(255,214,120), 10, 90);
    sfx(19);
}
static void mall_exit(int pi){
    Player *p=&G.p[pi];
    p->in_mall=0; p->in_arcade=0; in_srcroom[pi]=0; p->station=-1;
    p->x=0; p->y=58; p->aim=PI/2;
    splat(p->x, p->y, COL(255,214,120), 10, 90);
    sfx(19);
}
// what INTERACT would do right here (the HUD hint). neutral=1 swaps the
// key name for a plain INTERACT — retained for any future shared-screen
// line; both current callers pass 0 and let the view's device context
// name the viewing player's own control (v1.25).
static const char *interact_prompt(Player *p, char *buf, int neutral){
    char *q=buf;
    if (!p->alive) return 0;
    const char *key = neutral? "INTERACT" : lbla(H_INT);
    { Player *down = downed_near(p);
      if (down){ q=sappend(q, key); q=sappend(q, " - REVIVE "); q=sappend(q, down->name);
                 q=sappend(q, " - "); q=sapp_int(q, revive_price(down)); q=sappend(q, " KG");
                 *q=0; return buf; } }
    if (!p->in_mall){
        if (!near_store(p)) return 0;
        q=sappend(q, key); q=sappend(q, " - ENTER THE BUG MALL"); *q=0; return buf;
    }
    int st = station_at(p);
    if (st>=0){
        q=sappend(q, key); q=sappend(q, " - "); q=sappend(q, MALL_STATION[st]); *q=0; return buf;
    }
    if (src_door_at(p)){
        if (src_gate_open(p-G.p)){ q=sappend(q, key); q=sappend(q, " - THE SOURCE ROOM"); }
        else { q=sappend(q, "THE SOURCE - SEALED - TROPHIES "); q=sapp_int(q, trophy_earned_count(p));
               q=sappend(q, "/"); q=sapp_int(q, SRC_DOOR_TROPHIES); }
        *q=0; return buf;
    }
    if (srcroom_exit_at(p)){
        q=sappend(q, key); q=sappend(q, " - BACK TO THE MALL"); *q=0; return buf;
    }
    // the arcade door is checked BEFORE the Banzai gate: the gate's
    // generous trigger disc (r=100 around its front) overlaps the
    // door's approach strip on the east wall, and the wall door is
    // the more specific address — the gate's own stands (south of
    // the block, west of it) are outside the door strip, so the
    // gate loses nothing.
    if (arcade_door_at(p)){
        q=sappend(q, key); q=sappend(q, " - THE ARCADE"); *q=0; return buf;
    }
    if (bw_door_at(p) && G.net_mode!=NET_CLIENT){
        q=sappend(q, key); q=sappend(q, " - BANZAI WORLD"); *q=0; return buf;
    }
    { int cab = cabinet_at(p);
      if (cab>=0 && G.net_mode!=NET_CLIENT){
          q=sappend(q, key); q=sappend(q, " - PLAY "); q=sappend(q, ARC_NAME[cab]); *q=0; return buf;
      } }
    if (arcade_exit_at(p)){
        q=sappend(q, key); q=sappend(q, " - BACK TO THE MALL"); *q=0; return buf;
    }
    if (on_exit_mat(p)){
        q=sappend(q, key); q=sappend(q, " - LEAVE THE MALL"); *q=0; return buf;
    }
    return 0;
}
// mall interior collision: stay in the room, don't walk through counters
static void mall_collide(Player *p){
    if (p->x < -384) p->x = -384; if (p->x > 384) p->x = 384;
    if (p->y < MALL_CY-204) p->y = MALL_CY-204;
    if (p->y > MALL_CY+198) p->y = MALL_CY+198;
    // the Banzai World gate against the south wall (east twin of the bowl)
    { float hx=52, hy0=MALL_CY+158, hy1=MALL_CY+190, rad=11;
      if (p->x > BW_SX-hx-rad && p->x < BW_SX+hx+rad && p->y > hy0-rad && p->y < hy1+rad){
          float dyr = (hy1+rad) - p->y, dyl = p->y - (hy0-rad);
          float dxr = (BW_SX+hx+rad) - p->x, dxl = p->x - (BW_SX-hx-rad);
          float m=dyr; int side=0;
          if (dyl<m){ m=dyl; side=1; } if (dxr<m){ m=dxr; side=2; } if (dxl<m){ side=3; }
          if (side==0) p->y = hy1+rad; else if (side==1) p->y = hy0-rad;
          else if (side==2) p->x = BW_SX+hx+rad; else p->x = BW_SX-hx-rad;
      } }
    // the bowling counter against the south wall
    { float hx=52, hy0=MALL_CY+158, hy1=MALL_CY+190, rad=11;
      if (p->x > BOWL_SX-hx-rad && p->x < BOWL_SX+hx+rad && p->y > hy0-rad && p->y < hy1+rad){
          float dyr = (hy1+rad) - p->y, dyl = p->y - (hy0-rad);
          float dxr = (BOWL_SX+hx+rad) - p->x, dxl = p->x - (BOWL_SX-hx-rad);
          float m=dyr; int side=0;
          if (dyl<m){ m=dyl; side=1; } if (dxr<m){ m=dxr; side=2; } if (dxl<m){ side=3; }
          if (side==0) p->y = hy1+rad; else if (side==1) p->y = hy0-rad;
          else if (side==2) p->x = BOWL_SX+hx+rad; else p->x = BOWL_SX-hx-rad;
      } }
    // the trophy case against the south wall (v1.25 feat 4): a
    // narrower box than the bowl counter's, tucked east of the
    // Banzai gate's box (ends x 278) so the entry-mat <-> gate
    // corridor west of it stays walkable end to end
    { float hx=28, hy0=MALL_CY+158, hy1=MALL_CY+190, rad=11;
      if (p->x > CASE_SX-hx-rad && p->x < CASE_SX+hx+rad && p->y > hy0-rad && p->y < hy1+rad){
          float dyr = (hy1+rad) - p->y, dyl = p->y - (hy0-rad);
          float dxr = (CASE_SX+hx+rad) - p->x, dxl = p->x - (CASE_SX-hx-rad);
          float m=dyr; int side=0;
          if (dyl<m){ m=dyl; side=1; } if (dxr<m){ m=dxr; side=2; } if (dxl<m){ side=3; }
          if (side==0) p->y = hy1+rad; else if (side==1) p->y = hy0-rad;
          else if (side==2) p->x = CASE_SX+hx+rad; else p->x = CASE_SX-hx-rad;
      } }
    // the challenge board against the south wall's west end
    // (v1.25 feat 6): the case's mirror — a box in the same idiom,
    // west of the bowling counter's box (starts x -233), against
    // the west wall. Nothing routes past it; it guards no corridor.
    { float hx=26, hy0=MALL_CY+158, hy1=MALL_CY+190, rad=11;
      if (p->x > BOARD_SX-hx-rad && p->x < BOARD_SX+hx+rad && p->y > hy0-rad && p->y < hy1+rad){
          float dyr = (hy1+rad) - p->y, dyl = p->y - (hy0-rad);
          float dxr = (BOARD_SX+hx+rad) - p->x, dxl = p->x - (BOARD_SX-hx-rad);
          float m=dyr; int side=0;
          if (dyl<m){ m=dyl; side=1; } if (dxr<m){ m=dxr; side=2; } if (dxl<m){ side=3; }
          if (side==0) p->y = hy1+rad; else if (side==1) p->y = hy0-rad;
          else if (side==2) p->x = BOARD_SX+hx+rad; else p->x = BOARD_SX-hx-rad;
      } }
    // the photo booth island, west-centre (v1.25 MALL ENLARGEMENT):
    // a freestanding box in the same idiom — the lanes around it stay
    // wide (107 px west to the clamp, ~80 px north to the counters)
    { float hx=34, hy0=BOOTH_CY-34, hy1=BOOTH_CY+30, rad=11;
      if (p->x > BOOTH_SX-hx-rad && p->x < BOOTH_SX+hx+rad && p->y > hy0-rad && p->y < hy1+rad){
          float dyr = (hy1+rad) - p->y, dyl = p->y - (hy0-rad);
          float dxr = (BOOTH_SX+hx+rad) - p->x, dxl = p->x - (BOOTH_SX-hx-rad);
          float m=dyr; int side=0;
          if (dyl<m){ m=dyl; side=1; } if (dxr<m){ m=dxr; side=2; } if (dxl<m){ side=3; }
          if (side==0) p->y = hy1+rad; else if (side==1) p->y = hy0-rad;
          else if (side==2) p->x = BOOTH_SX+hx+rad; else p->x = BOOTH_SX-hx-rad;
      } }
    for (int s=0;s<4;s++){
        float hx=58, hy0=MALL_CY-198, hy1=MALL_CY-166, rad=11;
        if (p->x > MALL_SX[s]-hx-rad && p->x < MALL_SX[s]+hx+rad && p->y > hy0-rad && p->y < hy1+rad){
            // push out the shallow way (almost always south, off the counter front)
            float dyr = (hy1+rad) - p->y, dyl = p->y - (hy0-rad);
            float dxr = (MALL_SX[s]+hx+rad) - p->x, dxl = p->x - (MALL_SX[s]-hx-rad);
            float m=dyr; int side=0;
            if (dyl<m){ m=dyl; side=1; } if (dxr<m){ m=dxr; side=2; } if (dxl<m){ side=3; }
            if (side==0) p->y = hy1+rad; else if (side==1) p->y = hy0-rad;
            else if (side==2) p->x = MALL_SX[s]+hx+rad; else p->x = MALL_SX[s]-hx-rad;
        }
    }
}

// arcade interior collision: same room shape as the mall, with the
// four cabinets standing along the north wall where counters would be
static void arcade_collide(Player *p){
    if (p->x < -356) p->x = -356; if (p->x > 356) p->x = 356;
    if (p->y < ARC_CY-164) p->y = ARC_CY-164;
    if (p->y > ARC_CY+328) p->y = ARC_CY+328;   // the room grew south for the WAR! row (Nov 15)
    for (int s=0;s<7;s++){
        float hx=66, hy0=ARC_CY + ARC_ROWY[s] + 8, hy1=ARC_CY + ARC_ROWY[s] + 40, rad=11;
        if (p->x > ARC_SX[s]-hx-rad && p->x < ARC_SX[s]+hx+rad && p->y > hy0-rad && p->y < hy1+rad){
            float dyr = (hy1+rad) - p->y, dyl = p->y - (hy0-rad);
            float dxr = (ARC_SX[s]+hx+rad) - p->x, dxl = p->x - (ARC_SX[s]-hx-rad);
            float m=dyr; int side=0;
            if (dyl<m){ m=dyl; side=1; } if (dxr<m){ m=dxr; side=2; } if (dxl<m){ side=3; }
            if (side==0) p->y = hy1+rad; else if (side==1) p->y = hy0-rad;
            else if (side==2) p->x = ARC_SX[s]+hx+rad; else p->x = ARC_SX[s]-hx-rad;
        }
    }
}

static void place_wall(int pi){
    Player *p=&G.p[pi];
    if (p->in_mall) return;                               // the mall is a no-build zone
    if (p->stack_n<3 || p->wall_cd>0) { if (pi==G.my_id) sfx(16); return; }
    float wx = p->x + fcos(p->aim)*36, wy = p->y + fsin(p->aim)*36;
    int horiz = (ffabs(fsin(p->aim)) > ffabs(fcos(p->aim))) ? 1 : 0;
    // Forgiving interlock (Daniel): a wall placed NEAR an existing one
    // snaps into line with it — same row, end to end, or a clean
    // corner — so extending a run doesn't take pixel-perfect
    // shuffling. The nearest good snap wins; the loop rule and the
    // too-close rule below judge the SNAPPED spot, not the raw one.
    { float best=1e18f, sx=wx, sy=wy;
      for (int i=0;i<MAXWALLS;i++){ Wall *o=&G.walls[i];
        if (!o->alive) continue;
        if (o->horiz == horiz){
            // collinear: my line is close to theirs, my center is
            // somewhere past their end — slide me to exactly end-to-end
            float dl = horiz? wy-o->y : wx-o->x; if (dl<0) dl=-dl;
            float da = horiz? wx-o->x : wy-o->y;
            float ada = da<0? -da:da;
            if (dl<=20 && ada>20 && ada<=104){
                float tx = horiz? o->x + (da<0? -56:56) : o->x;
                float ty = horiz? o->y : o->y + (da<0? -56:56);
                float d2 = (tx-wx)*(tx-wx) + (ty-wy)*(ty-wy);
                if (d2<best){ best=d2; sx=tx; sy=ty; }
            }
        } else {
            // a corner: my center belongs at (±26, ±26) from theirs,
            // where my end lands exactly on their end
            for (int a=-1;a<=1;a+=2) for (int b=-1;b<=1;b+=2){
                float tx = o->x + 26*a, ty = o->y + 26*b;
                float d2 = (tx-wx)*(tx-wx) + (ty-wy)*(ty-wy);
                if (d2 < 24*24 && d2<best){ best=d2; sx=tx; sy=ty; }
            }
        }
      }
      wx=sx; wy=sy; }
    // Stacking (Daniel): building right on top of an existing wall is
    // allowed — it reinforces that spot, and the bugs have to chew
    // through both. A same-direction stack can't change the run's
    // outline, so it skips the circuit rule; there is no too-close
    // rejection anymore.
    int stacking = 0, stack_i = -1;
    for (int i=0;i<MAXWALLS;i++){ Wall *o=&G.walls[i];
        if (o->alive){ float dx=o->x-wx, dy=o->y-wy;
            if (dx*dx+dy*dy < 36){ stacking = (o->horiz==horiz)? 1 : 2; stack_i = i; break; } } }
    // Daniel's circuit rule: a wall that would entirely close a loop
    // is never allowed — the run must always stay open somewhere.
    if (stacking!=1 && placement_closes_loop(wx, wy, horiz)){
        spawn_floater(wx, wy-16, "THE LOOP MUST STAY OPEN", COL(255,120,110));
        if (pi==G.my_id) sfx(16);
        p->wall_cd = 0.35f;
        return;
    }
    // Absorb (Daniel, Nov 6): stacking onto a same-direction wall no
    // longer spawns a twin entry on the same spot — every twin was a
    // full extra wall to draw, collide and chew, and a tall stack
    // lagged the game. The new carcasses melt into the wall that is
    // already there: same total strength, one wall entry. Nov 7:
    // the pile is still a PILE — the entry counts its layers
    // (o->stack), the renderer draws them heaped up, the run counts
    // them toward its size/tiers, and chewing sheds them one at a
    // time (break_wall), like the separate walls they used to be.
    if (stacking==1 && stack_i>=0){
        Wall *o=&G.walls[stack_i];
        float sum=0; float ws[3];
        for (int k=0;k<3;k++){ ws[k]=hat_pop(p); sum+=ws[k]; }
        float add = 45.0f + sum*2.2f;
        // the run's health fraction BEFORE the layer lands: the
        // recompute's share carry credits the new layer at this
        // fraction, but a fresh layer arrives at FULL strength —
        // the pool must include ALL of the reinforcement (Daniel,
        // Nov 15), so the un-credited part is topped up after.
        int oldL = o->grp; float ofrac = 1.0f;
        if (oldL>=0 && grp_cap[oldL]>0){ ofrac = grp_pool[oldL]/grp_cap[oldL];
            if (ofrac<0) ofrac=0; if (ofrac>1) ofrac=1; }
        o->maxhp += add; o->hp += add; if (o->hp>o->maxhp) o->hp=o->maxhp;
        o->stack = (o->stack>0? o->stack : 1) + 1; if (o->stack>999) o->stack=999;
        for (int k=0;k<3;k++){
            CarcassFx *f = spawn_fx(2, p->x, p->y-24-k*4, o->x, o->y, ws[k], pi);
            if (f) f->wall_slot=stack_i;   // lands on it; an alive wall ignores the build count
        }
        walls_recompute();
        { int L2 = o->grp;
          if (L2>=0 && grp_n[L2]>=2 && ofrac<1.0f){
              grp_pool[L2] += add*1.1f*(1.0f-ofrac);
              if (grp_pool[L2]>grp_cap[L2]) grp_pool[L2]=grp_cap[L2];
              float fr2 = grp_cap[L2]>0? grp_pool[L2]/grp_cap[L2] : 0;
              for (int m=0;m<MAXWALLS;m++) if (G.walls[m].alive==1 && G.walls[m].grp==L2)
                  G.walls[m].hp = G.walls[m].maxhp * fr2;
          } }
        spawn_floater(o->x, o->y-16, "REINFORCED!", COL(255,230,150));
        p->wall_cd = 0.35f;
        sfx(15);
        return;
    }
    // walls may go anywhere now — the plaza needs defending like anywhere
    for (int i=0;i<MAXWALLS;i++) if (!G.walls[i].alive){
        Wall *w=&G.walls[i];
        float sum=0; float ws[3];
        for (int k=0;k<3;k++){ ws[k]=hat_pop(p); sum+=ws[k]; }
        w->alive=2; w->horiz = horiz;
        w->x=wx; w->y=wy; w->hp = w->maxhp = 45.0f + sum*2.2f;
        w->stack=1; w->grp=-1; w->grp_old=-1; w->endpt=0;
        wall_build_n[i]=0;
        for (int k=0;k<3;k++){
            CarcassFx *f = spawn_fx(2, p->x, p->y-24-k*4, wx, wy, ws[k], pi);
            if (f) f->wall_slot=i; else wall_build_n[i]++;
        }
        if (wall_build_n[i]>=3){ w->alive=1; walls_recompute(); }
        p->wall_cd = 0.35f;
        sfx(15);
        return;
    }
}

