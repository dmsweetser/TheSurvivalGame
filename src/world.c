// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: world.c — the Bug Mall and Arcade rooms, cartoony world rendering, EDDIE MODE sprites, HUD / leaderboard / store UI.
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
// ---------------------------------------------------------------------------
// THE BUG MALL — the old single-panel Bug Bounty grew into a walk-in
// mall (Daniel's design): a shop counter at the entrance with everything
// the Bounty always sold, then the Gallery (Psychic Hat Stand), the Bug
// Bank, and the Banzai Buddies store. The interior lives far south of
// the world at (0, MALL_CY); walking in is a step through the hut door.
// No bugs inside. Ever. House rules.
// ---------------------------------------------------------------------------
#define MALL_CY (-6000)
static const int MALL_SX[4] = { -225, -75, 75, 225 };   // station counter centres
static const char *MALL_STATION[5] = { "BUG BOUNTY", "WHALES ONLY", "BUG BANK", "BANZAI BUDDIES", "BUG BOWL" };
#define BOWL_SX (-170)                                   // the lanes counter, south-west
static int station_at(Player *p){
    if (!p->in_mall) return -1;
    for (int s=0;s<4;s++){
        float dx=p->x - MALL_SX[s], dy=p->y - (MALL_CY-108);
        if (dx*dx + dy*dy < 64*64) return s;
    }
    { float dx=p->x - BOWL_SX, dy=p->y - (MALL_CY+92);   // the bowling counter
      if (dx*dx + dy*dy < 64*64) return 4; }
    return -1;
}
#define BW_SX 185                                    // the Banzai World door, south-east
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
static int cabinet_at(Player *p){
    if (!p->in_arcade) return -1;
    for (int s=0;s<7;s++){
        float dx=p->x - ARC_SX[s], dy=p->y - (ARC_CY + ARC_ROWY[s] + 58);
        if (dx*dx + dy*dy < 74*74) return s;
    }
    return -1;
}
static int arcade_door_at(Player *p){   // the mall's east-wall door
    return p->in_mall && !p->in_arcade && p->x>192 && p->y>MALL_CY-14 && p->y<MALL_CY+66;
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
    p->x=196; p->y=MALL_CY+24; p->aim=PI;
    splat(p->x, p->y, COL(140,240,255), 10, 90);
    sfx(19);
}
static void arc_gate_try(int pi);                      // cabinet checks + panel, below
static void arc_gate_draw(int pi, int vx, int vy, int vw2, int vh2);   // ditto
static void arc_gate_tap(int pi, int vx, int vy, int vw2, int vh2);    // ditto
static void arc_gate_pick(int pi);                     // ditto
static void draw_arcade(void);                         // ditto (with the mall art)
static void bw_gate_try(int pi);                     // gate checks + mode panel, below
static void bw_enter_mode(int pi, int mode);         // defined with Banzai World, below
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
    float dx=p->x - BW_SX, dy=p->y - (MALL_CY+92);
    // 100, not 64: the gate block itself holds you ~69 units off the
    // door point from the south and the west — at 64 the two most
    // natural approaches (up the door column from the entrance, east
    // along the mat row) stopped a few pixels short, and Interact did
    // nothing at all, not even the denial floaters (Daniel, Nov 3:
    // "wouldn't even let me in")
    return dx*dx + dy*dy < 100*100;
}
static int on_exit_mat(Player *p){
    return p->in_mall && p->x>-40 && p->x<40 && p->y>MALL_CY+128 && p->y<MALL_CY+172;
}
static void mall_enter(int pi){
    Player *p=&G.p[pi];
    p->in_mall=1; p->in_arcade=0; p->bean_jet=0; p->on_wall=-1; p->station=-1;
    p->x=0; p->y=MALL_CY+118; p->aim=-PI/2;
    splat(p->x, p->y, COL(255,214,120), 10, 90);
    sfx(19);
}
static void mall_exit(int pi){
    Player *p=&G.p[pi];
    p->in_mall=0; p->in_arcade=0; p->station=-1;
    p->x=0; p->y=58; p->aim=PI/2;
    splat(p->x, p->y, COL(255,214,120), 10, 90);
    sfx(19);
}
// what INTERACT would do right here (the HUD hint). neutral=1 swaps the
// key name for a plain INTERACT (P2's half of a co-op screen)
static const char *interact_prompt(Player *p, char *buf, int neutral){
    char *q=buf;
    if (!p->alive) return 0;
    const char *key = neutral? "INTERACT" : (pad_mode()? "B" : (IN.touch_seen? "TAP" : key_name(G.bind[ACT_INTERACT])));
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
    if (p->x < -284) p->x = -284; if (p->x > 284) p->x = 284;
    if (p->y < MALL_CY-164) p->y = MALL_CY-164;
    if (p->y > MALL_CY+158) p->y = MALL_CY+158;
    // the Banzai World gate against the south wall (east twin of the bowl)
    { float hx=52, hy0=MALL_CY+118, hy1=MALL_CY+150, rad=11;
      if (p->x > BW_SX-hx-rad && p->x < BW_SX+hx+rad && p->y > hy0-rad && p->y < hy1+rad){
          float dyr = (hy1+rad) - p->y, dyl = p->y - (hy0-rad);
          float dxr = (BW_SX+hx+rad) - p->x, dxl = p->x - (BW_SX-hx-rad);
          float m=dyr; int side=0;
          if (dyl<m){ m=dyl; side=1; } if (dxr<m){ m=dxr; side=2; } if (dxl<m){ side=3; }
          if (side==0) p->y = hy1+rad; else if (side==1) p->y = hy0-rad;
          else if (side==2) p->x = BW_SX+hx+rad; else p->x = BW_SX-hx-rad;
      } }
    // the bowling counter against the south wall
    { float hx=52, hy0=MALL_CY+118, hy1=MALL_CY+150, rad=11;
      if (p->x > BOWL_SX-hx-rad && p->x < BOWL_SX+hx+rad && p->y > hy0-rad && p->y < hy1+rad){
          float dyr = (hy1+rad) - p->y, dyl = p->y - (hy0-rad);
          float dxr = (BOWL_SX+hx+rad) - p->x, dxl = p->x - (BOWL_SX-hx-rad);
          float m=dyr; int side=0;
          if (dyl<m){ m=dyl; side=1; } if (dxr<m){ m=dxr; side=2; } if (dxl<m){ side=3; }
          if (side==0) p->y = hy1+rad; else if (side==1) p->y = hy0-rad;
          else if (side==2) p->x = BOWL_SX+hx+rad; else p->x = BOWL_SX-hx-rad;
      } }
    for (int s=0;s<4;s++){
        float hx=58, hy0=MALL_CY-158, hy1=MALL_CY-126, rad=11;
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

static void player_fire(Player *p, int pi, float dt){
    if (p->in_mall){ p->fire_cd = 0; return; }   // house rules: no shooting in the mall
    p->fire_cd -= dt;
    if (!(p->cur_keys & K_FIRE)) return;
    int w = p->weapon;
    if (!p->owned[w]) return;
    float mx = p->x + fcos(p->aim)*16, my = p->y + fsin(p->aim)*16;
    float dm = p->dmg_mul, rm = p->rate_mul;
    if (w==4){ // THE DEBUGGER: one pull = one sweep. All your beams
        // extend together, spin one revolution together, then close
        // together (the sweep itself lives in update_beam). Stacked
        // copies don't widen one beam — they add beams to the ring:
        // 2 = front + back, 3 = every 120°, and so on, each one also
        // reaching a little farther.
        if (p->fire_cd<=0 && beam_phase[pi]==0){
            int copies = p->owned[4]; if (copies<1) copies=1;
            beam_n[pi] = copies>MAXBEAMS? MAXBEAMS : copies;
            beam_len[pi] = 105 + 10*(copies-1);
            beam_base[pi] = p->aim;
            beam_phase[pi] = 1; beam_t[pi] = 0; beam_tick[pi] = 0;
            beam_frac[pi] = 0;
            p->fire_cd = (BEAM_EXT + BEAM_SPIN + BEAM_RET + BEAM_REST)/rm;
        }
        return;
    }
    float cd = (w==0?0.115f : w==1?0.38f : w==2?0.30f : 1.05f);
    if (p->fire_cd>0) return;
    p->fire_cd = cd/rm;
    float spread = (w==0?0.055f : w==1?0.03f : w==2?0.045f : 0.02f);
    float dmg = (w==0?6.0f : w==1?24.0f : w==2?15.0f : 65.0f) * dm;
    // stacked copies fan out: one copy fires straight ahead, two fire
    // at two angles, three at three — a row of shots across your front.
    // Stacks are uncapped; the fan itself tops out at 40 projectiles
    // per volley (the projectile pool is only so deep).
    int n = p->owned[w]; if (n<1) n=1; if (n>40) n=40;
    for (int i=0;i<n;i++){
        float off = ((float)i - (float)(n-1)/2.0f) * 0.11f;
        float ang = p->aim + off + rng_range(-spread, spread);
        fire_proj(pi, w, mx, my, ang, dmg, -1);
    }
    sfx(w==0?1 : w==1?2 : w==2?3 : 4);
    if (w==3){ G.shake = 2.0f; }
}

// one damage tick of every live beam: same cone test the held beam used
// (nearest bugs inside reach and ±0.55 rad of the beam's current angle
// get breakpoint-frozen and burned)
static void beam_hit_tick(int pi){
    Player *p=&G.p[pi];
    int nb = beam_n[pi]; if (nb<1) nb=1;
    float bl = beam_len[pi] * beam_frac[pi];
    float dm = p->dmg_mul;
    for (int k=0;k<nb;k++){
        float ba = beam_ang[pi][k];
        for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
            float dx=G.bugs[i].x-p->x, dy=G.bugs[i].y-p->y;
            float d2=dx*dx+dy*dy;
            if (d2 < bl*bl){
                float da = wrap_pi(fatan2(dy,dx) - ba);
                if (ffabs(da) < 0.55f){
                    G.bugs[i].bp_t = 0.35f;          // BREAKPOINT: frozen
                    hurt_bug(i, 8.0f*dm, pi, fcos(ba)*30, fsin(ba)*30);
                }
            }
        }
    }
    sfx(14);
}

// advance one player's Debugger sweep: extend (beams grow, damage live),
// spin (one full revolution, damage live), retract (beams close, no
// damage), done. Writes the render state (angles, reach fraction, vis)
// every frame; the ring rotates as a whole from the captured aim.
static void update_beam(int pi, float dt){
    if (beam_phase[pi]==0){ beam_frac[pi]=1; return; }
    Player *p=&G.p[pi];
    if (!p->alive || !p->owned[4]){ beam_phase[pi]=0; beam_vis[pi]=0; beam_frac[pi]=1; return; }
    beam_t[pi]+=dt;
    int ph = beam_phase[pi];
    float dur = ph==1? BEAM_EXT : ph==2? BEAM_SPIN : BEAM_RET;
    if (beam_t[pi]>=dur){
        beam_t[pi]=0;
        beam_phase[pi] = ph==3? 0 : ph+1;
        if (beam_phase[pi]==0){ beam_vis[pi]=0; beam_frac[pi]=1; return; }
        ph = beam_phase[pi];
        dur = ph==1? BEAM_EXT : ph==2? BEAM_SPIN : BEAM_RET;
    }
    float f = beam_t[pi]/dur;
    float rot = 0;
    if (ph==1) beam_frac[pi]=f;
    else if (ph==2){ beam_frac[pi]=1; rot = TAU*f; }
    else { beam_frac[pi]=1-f; rot = TAU; }
    int nb = beam_n[pi]; if (nb<1) nb=1;
    for (int k=0;k<nb;k++) beam_ang[pi][k] = beam_base[pi] + rot + TAU*(float)k/(float)nb;
    beam_vis[pi]=0.09f;
    if (ph==1 || ph==2){
        beam_tick[pi]+=dt;
        while (beam_tick[pi]>=0.07f){ beam_tick[pi]-=0.07f; beam_hit_tick(pi); }
    }
}

// THE BACKFIRE BEAN (Krista's idea, Daniel's build, Nov 5): a small
// green fellow with goggles and a jetpack he fuels entirely himself —
// beans are famous for exactly one thing, and he has weaponized it,
// tastefully, in little green puffs. Squeeze him (key 6, or tap BEAN
// in the status card) anywhere out in the wild and he flies you —
// and your hat, and your buddy — straight home to the Epicenter.
// One bean, one ride. He regrets nothing.
//
// INSIDE the epicenter there is no home to fly to, so the bean does
// the other thing he knows (Daniel, Nov 10): he becomes a JETPACK.
// You fly where you steer, over props, walls and swarms alike, for
// as long as you keep your nerve — the ride ends the moment you
// FIRE, or the moment anything lands ONE HIT on you, and he drops
// you right where you are. A launch pad for the far country.
#define BEAN_EPI_R 280.0f
static void bean_use(int pi){
    Player *p=&G.p[pi];
    if (G.state!=GS_PLAY || p->beans<=0 || p->bean_t>0 || p->bean_jet>0 || !p->alive) return;
    if (p->in_mall || p->station>=0 || G.bud[pi].in_bw) return;
    float dx=p->x, dy=p->y-70;
    if (dx*dx+dy*dy < BEAN_EPI_R*BEAN_EPI_R){
        p->beans--; p->bean_jet=1; p->jet_fall=0;
        spawn_floater(p->x, p->y-30, "JETPACK AWAY!", COL(150,220,120));
        sfx(23);
        if (pi==G.my_id) do_save();
        return;
    }
    p->beans--; p->bean_t=1.6f;
    spawn_floater(p->x, p->y-30, "BEAN POWER!", COL(150,220,120));
    sfx(23);
    if (pi==G.my_id) do_save();
}
static void update_players(float dt){
    for (int i=0;i<MAXP;i++){
        Player *p=&G.p[i];
        if (!p->connected) continue;
        if (p->fire_cd>0 && p->weapon!=4) { /* handled in fire */ }
        if (p->wall_cd>0) p->wall_cd-=dt;
        if (p->shed_cd>0) p->shed_cd-=dt;
        if (p->wof>0) p->wof-=dt;
        p->idle_t += dt;                       // movement zeroes this (idle anim)
        if (p->hurt_t>0) p->hurt_t-=dt;
        if (p->revive_t>0) p->revive_t-=dt;
        if (beam_vis[i]>0) beam_vis[i]-=dt;
        update_beam(i, dt);
        if (!p->alive){ p->dead_t+=dt; continue; }
        if (p->shield_regen_t>0) p->shield_regen_t-=dt;
        else if (p->shield < p->shield_max){
            // 14 points a second, accumulated fractionally: the old
            // line added (int)(14*dt+0.5) a frame, which truncates to
            // 0 at 30-60 fps — the shield silently never recharged.
            shield_frac[i] += 14.0f*dt;
            int whole = (int)shield_frac[i];
            if (whole>0){ p->shield += whole; shield_frac[i] -= (float)whole;
                          if (p->shield>p->shield_max){ p->shield=p->shield_max; shield_frac[i]=0; } }
        }
        // THE BACKFIRE BEAN ride: sputter (0–0.45s), the BLAST teleports
        // you home (0.45s), then you drop out of the sky onto the plaza
        // (to 1.05s) and stand there dizzy (to 1.6s). A passenger does
        // not move or fire — the bean is driving.
        if (p->bean_t>0){
            float eb = 1.6f - p->bean_t;          // elapsed at frame start
            p->bean_t -= dt; if (p->bean_t<0) p->bean_t=0;
            float e = 1.6f - p->bean_t;           // elapsed now
            if (eb<0.45f && e>=0.45f){
                p->x=0; p->y=70;
                Buddy *bb=&G.bud[i];
                if (bb->active && !bb->in_bw){ bb->x=p->x+26; bb->y=p->y+12; }
                splat(p->x, p->y+6, COL(168,216,120), 22, 130);
                splat(p->x, p->y+6, COL(240,248,230), 12, 90);
                spawn_floater(p->x, p->y-34, "PBBBT!", COL(150,220,120));
                sfx(22);
                if (i==G.my_id) do_save();
            } else if (e<0.45f){
                if ((int)(e*14)!=(int)(eb*14))
                    spawn_particle(p->x+rng_range(-6,6), p->y+8, COL(168,216,120), 14, 0.4f, 3);
            } else if (e<1.05f){
                if ((int)(e*20)!=(int)(eb*20))
                    spawn_particle(p->x+rng_range(-5,5), p->y-14, COL(240,248,230), 12, 0.35f, 2);
            } else if (eb<1.05f){
                splat(p->x, p->y+8, COL(168,216,120), 16, 110);
                splat(p->x, p->y+8, COL(240,248,230), 10, 80);
                sfx(5);
            }
            continue;
        }
        // THE BEAN JETPACK (epicenter launches): steer freely, fly
        // over everything at jet speed; firing or one landed hit
        // ends it where you are (see bean_jet_end). No hat burden,
        // no prop collision — you're above all of it.
        if (p->jet_fall>0){
            p->jet_fall -= dt;
            if ((int)(G.time*20)!=(int)((G.time-dt)*20))
                spawn_particle(p->x+rng_range(-5,5), p->y-8, COL(240,248,230), 10, 0.3f, 2);
            continue;
        }
        if (p->bean_jet>0){
            if (p->cur_keys & K_FIRE){ bean_jet_end(i, 1); continue; }
            float jx=0, jy=0;
            if (p->cur_keys & K_UP) jy-=1;
            if (p->cur_keys & K_DOWN) jy+=1;
            if (p->cur_keys & K_LEFT) jx-=1;
            if (p->cur_keys & K_RIGHT) jx+=1;
            float jl = fsqrt(jx*jx+jy*jy);
            if (jl>0){
                jx/=jl; jy/=jl;
                p->aim = fatan2(jy, jx);
                p->x += jx*335.0f*dt; p->y += jy*335.0f*dt;
                p->walk_ph += dt*6; p->idle_t = 0;
            }
            p->on_wall=-1;
            if ((int)(G.time*24)!=(int)((G.time-dt)*24))
                spawn_particle(p->x+rng_range(-4,4), p->y+10, COL(168,216,120), 12, 0.35f, 3);
            continue;
        }
        // movement
        float mx=0, my=0;
        if (p->cur_keys & K_UP) my-=1;
        if (p->cur_keys & K_DOWN) my+=1;
        if (p->cur_keys & K_LEFT) mx-=1;
        if (p->cur_keys & K_RIGHT) mx+=1;
        float ml = fsqrt(mx*mx+my*my);
        if (ml>0){
            mx/=ml; my/=ml;
            // Cyberdogs rules: you face (and therefore fire) the way you move
            p->aim = fatan2(my, mx);
            float sp = player_speed(p);
            float nx = p->x + mx*sp*dt, ny = p->y + my*sp*dt;
            // The mall interior stands on world chunks the junkyard
            // generator also fills — its solid crates and fax machines
            // were colliding in the mall even though draw_mall never
            // draws them (Daniel's "invisible impediments", Nov 3).
            // Inside, only mall_collide governs.
            if (p->in_mall){ p->x = nx; p->y = ny; }
            else {
                if (!prop_collide(nx, p->y, 11)) p->x = nx;
                if (!prop_collide(p->x, ny, 11)) p->y = ny;
            }
            wall_step(p);
            if (p->in_arcade) arcade_collide(p);
            else if (p->in_mall) mall_collide(p);
            p->walk_ph += dt*sp*0.055f;
            p->idle_t = 0;
            if ((p->whale_on & 8) && !p->in_mall){   // Whales Only: sparkle trail
                whale_spark_t[i] -= dt*sp;
                if (whale_spark_t[i]<=0){ whale_spark_t[i]=13;
                    spawn_particle(p->x + (float)rng_range(-8,8), p->y + (float)rng_range(0,9),
                                   COL(255,230,140), 26, 0.7f, 2); }
            }
            if (p->in_mall && !p->in_arcade && speed_curve(p) <= 40.0f){
                // WALK OF FAME (Daniel, Nov 14, 2026): crawling through
                // the mall under a big hat is a parade, not a slog —
                // confetti rains and the marquee flashes while you creep.
                // Gate tightened later the same day at Daniel's word:
                // ONLY at the minimum speed — the burden curve pinned
                // to the 40 px/s floor (a ~150KG+ hat at level 0). A
                // merely slow walk (curve between 40 and the old 64
                // line) earns no parade. Sprinting while pinned still
                // counts: the curve is what is pinned, not the stride.
                p->wof = 0.3f;
                wof_spark_t[i] -= dt*sp;
                if (wof_spark_t[i]<=0){ wof_spark_t[i]=2.2f;   // MORE confetti (Daniel, Nov 14): bursts ~3x as often,
                    for (int k=0;k<6;k++)                        // 3x the pieces, wider and taller
                        spawn_particle(p->x + (float)rng_range(-32,32), p->y - (float)rng_range(4,66),
                                       WOF_COL[(int)(rng_f()*5.99f)], 40, 1.3f, 2 + (int)(rng_f()*2.99f)); }
            }
        } else if (i!=G.my_id && G.net_mode==NET_HOST){
            p->aim = p->net_aim;      // remote player standing still: keep
                                      // the facing they last sent us
        }
        // WHALES ONLY: PARADE DUCK — it follows. It does nothing else.
        if ((p->whale_on & 256) && p->connected && p->alive){
            float ddx = p->x - duck_x[i], ddy = p->y - duck_y[i];
            if (!duck_on[i] || ddx*ddx + ddy*ddy > 300.0f*300.0f){
                duck_x[i] = p->x - fcos(p->aim)*30; duck_y[i] = p->y - fsin(p->aim)*30; duck_on[i] = 1;
            }
            float tx = p->x - fcos(p->aim)*30, ty = p->y - fsin(p->aim)*30;
            float dx = tx - duck_x[i], dy = ty - duck_y[i], dd = fsqrt(dx*dx + dy*dy);
            if (dd > 4){
                float dsp = 165.0f*dt; if (dsp > dd) dsp = dd;
                duck_x[i] += dx/dd*dsp; duck_y[i] += dy/dd*dsp;
                duck_ph[i] += dt*11; if (dx < -1) duck_face[i] = -1; else if (dx > 1) duck_face[i] = 1;
            }
        } else duck_on[i] = 0;
        if (!p->in_mall && p->on_wall<0) collide_hut(&p->x, &p->y, 11);
        if (G.state==GS_PLAY) player_fire(p, i, dt);
        else if (p->fire_cd>0) p->fire_cd-=dt;
        if (p->want_wall){ p->want_wall=0; if (G.state==GS_PLAY) place_wall(i); }
    }
}

static void update_projectiles(float dt){
    for (int i=0;i<MAXPROJ;i++){
        Proj *q=&G.projs[i];
        if (!q->alive) continue;
        q->life -= dt;
        q->x += q->vx*dt; q->y += q->vy*dt; q->spin += dt*9;
        int dead = (q->life<=0);
        // shots fly OVER carcass walls: the player can fire across a
        // wall line, while bugs still have to chew through it — there
        // are no enemy projectiles, so nothing hostile crosses a wall
        // without eating it first. PMs now detonate on a bug or when
        // their life runs out instead of on the wall.
        if (!dead && q->life<=0 && q->kind==3){ explode_pm(q->x, q->y, q->dmg, q->owner, q->agent); dead=1; }
        if (dead){ q->alive=0; continue; }
        // bug hits
        for (int bi=0;bi<MAXBUGS;bi++){
            Bug *b=&G.bugs[bi];
            if (!b->alive) continue;
            float dx=b->x-q->x, dy=b->y-q->y, rr=bug_radius(b->w)+5;
            if (dx*dx+dy*dy > rr*rr) continue;
            if (q->kind==3){ explode_pm(q->x, q->y, q->dmg, q->owner, q->agent); q->alive=0; }
            else if (q->kind==2){ // duck: hit and bounce to a neighbour
                hurt_bug(bi, q->dmg, q->owner, q->vx*0.2f, q->vy*0.2f);
                if (q->agent==-2) b->btarget=q->owner;   // a buddy's hit draws its ire
                sfx(3);
                q->bounces++;
                int best=-1; float bd=180*180;
                for (int j=0;j<MAXBUGS;j++) if (j!=bi && G.bugs[j].alive){
                    float ddx=G.bugs[j].x-q->x, ddy=G.bugs[j].y-q->y, d2=ddx*ddx+ddy*ddy;
                    if (d2<bd){ bd=d2; best=j; }
                }
                if (best>=0 && q->bounces<=3){
                    float d=fsqrt(bd)+0.01f, sp=360;
                    q->vx=(G.bugs[best].x-q->x)/d*sp; q->vy=(G.bugs[best].y-q->y)/d*sp;
                    q->life = q->life>0.5f? q->life : 0.5f;
                } else q->alive=0;
            } else {
                hurt_bug(bi, q->dmg, q->owner, q->vx*0.35f, q->vy*0.35f);
                if (q->agent==-2) b->btarget=q->owner;
                if (q->kind==1){ // mouse splash
                    for (int j=0;j<MAXBUGS;j++) if (j!=bi && G.bugs[j].alive){
                        float ddx=G.bugs[j].x-q->x, ddy=G.bugs[j].y-q->y;
                        if (ddx*ddx+ddy*ddy < 36*36){ hurt_bug(j, q->dmg*0.4f, q->owner, ddx, ddy); if (q->agent==-2) G.bugs[j].btarget=q->owner; }
                    }
                }
                q->alive=0;
            }
            break;
        }
    }
}

// The Epicenter shield: while ANY living player is anywhere in the
// Bug Mall (or has a shop panel open), the plaza is sealed — bugs
// cannot cross the ring until the last shopper comes back out.
// Shopping is sacred.
static int shield_active(void){
    for (int i=0;i<MAXP;i++)
        if (G.p[i].connected && G.p[i].alive && (G.p[i].in_store || G.p[i].in_mall)) return 1;
    return 0;
}
static int any_remote_connected(void){
    // a LAN guest is remote; the local co-op P2 sits on this machine
    for (int i=1;i<MAXP;i++) if (G.p[i].connected && i!=G.p2_slot) return 1;
    return 0;
}
static void update_bugs(float dt){
    int shield = shield_active();
    for (int i=0;i<MAXBUGS;i++){
        Bug *b=&G.bugs[i];
        if (!b->alive) continue;
        if (b->hit_t>0) b->hit_t-=dt;
        b->leg_ph += dt*(2.0f + b->speed*0.08f);
        // knockback decay
        b->x += b->vx*dt; b->y += b->vy*dt;
        b->vx *= (1.0f - 6.0f*dt); b->vy *= (1.0f - 6.0f*dt);
        if (b->bp_t>0){ b->bp_t-=dt; continue; }         // frozen at a breakpoint
        // who does this bug hunt? A player, normally. But the first
        // time a Banzai Buddy's shot lands on it, the bug turns on
        // that buddy and keeps coming for it until one of them is
        // down (the same rule the retired robot clones used to get).
        int tp=-1; float td=1e18f;
        if (b->btarget>=0 && G.bud[b->btarget].active && G.bud[b->btarget].hp>0 && !G.bud[b->btarget].fainted){
            float dx=G.bud[b->btarget].x-b->x, dy=G.bud[b->btarget].y-b->y;
            td=dx*dx+dy*dy; tp=-2;   // tp -2: the quarry is a buddy (index in btarget)
        } else {
            b->btarget=-1;   // its foe is gone — back to players
            for (int j=0;j<MAXP;j++) if (G.p[j].connected && G.p[j].alive && !G.p[j].in_mall){
                float dx=G.p[j].x-b->x, dy=G.p[j].y-b->y, d2=dx*dx+dy*dy;
                if (d2<td){ td=d2; tp=j; }
            }
        }
        if (tp<0 && tp!=-2) continue;
        float txp = tp==-2? G.bud[b->btarget].x : G.p[tp].x;
        float typ = tp==-2? G.bud[b->btarget].y : G.p[tp].y;
        float dist = fsqrt(td);
        if (dist > 1750){ b->alive=0; continue; }        // left behind
        float dx=txp-b->x, dy=typ-b->y;
        float inv = dist>0.01f? 1.0f/dist : 0;
        float mx = dx*inv, my = dy*inv;
        // GREASE (Daniel, Nov 10): a bug stuck behind a prop used to
        // hug it forever. While greased, its heading bends hard to one
        // side — chosen by probing which side is open — so it slides
        // around the obstacle instead of pressing into it.
        if (b->grease_t>0){
            b->grease_t -= dt;
            float gx2 = mx*0.22f + (-my)*b->grease_s, gy2 = my*0.22f + mx*b->grease_s;
            float gl = fsqrt(gx2*gx2 + gy2*gy2);
            if (gl>0.01f){ mx = gx2/gl; my = gy2/gl; }
        }
        float ox0 = b->x, oy0 = b->y;
        float nx = b->x + mx*b->speed*dt, ny = b->y + my*b->speed*dt;
        float rad = bug_radius(b->w);
        // chew through walls that block the way
        int blocked=-1;
        for (int wi=0;wi<MAXWALLS;wi++)
            if (G.walls[wi].alive==1 && wall_box_hit(nx, ny, rad*0.8f, &G.walls[wi])){ blocked=wi; break; }
        if (blocked>=0){
            damage_wall(blocked, b->dmg*3.0f*dt);         // pooled health for interlocked runs
            b->leg_ph += dt*6;                            // chewing scramble
            b->stuck_t = 0;                               // chewing is a choice, not a stuck bug
        } else {
            if (!prop_collide(nx, b->y, rad*0.7f)) b->x = nx;
            if (!prop_collide(b->x, ny, rad*0.7f)) b->y = ny;
            // stuck? it wanted a full step and barely moved, and it is
            // not yet in biting range — start the grease clock
            float stepw = b->speed*dt;
            float mvx = b->x-ox0, mvy = b->y-oy0;
            if (stepw>0.4f && dist > rad+16 && mvx*mvx+mvy*mvy < stepw*stepw*0.0625f){
                b->stuck_t += dt;
                if (b->stuck_t>0.30f && b->grease_t<=0){
                    float px2=-my, py2=mx, pr = rad+14;
                    int lf = !prop_collide(b->x+px2*pr, b->y+py2*pr, rad*0.7f);
                    int rf = !prop_collide(b->x-px2*pr, b->y-py2*pr, rad*0.7f);
                    b->grease_s = (lf && !rf)? 1 : (rf && !lf)? -1 : (b->grease_s? -b->grease_s : 1);
                    b->grease_t = 0.55f; b->stuck_t = 0;
                }
            } else if (b->stuck_t>0){ b->stuck_t -= dt*2; if (b->stuck_t<0) b->stuck_t=0; }
        }
        collide_hut(&b->x, &b->y, bug_radius(b->w));   // the mall is solid to bugs too
        // the shield ring: bugs inside get shoved out, bugs outside stay out
        if (shield){
            float bd = fsqrt(b->x*b->x + b->y*b->y);
            float rr = 152.0f + rad;
            if (bd < rr){
                if (bd < 0.01f){ b->x = rr; b->y = 0; }
                else {
                    float step = rr - bd; if (step > 260.0f*dt) step = 260.0f*dt;
                    float s = (bd + step)/bd; b->x *= s; b->y *= s;
                }
                // re-measure so a shoved-out bug can't bite through the ring
                float ddx = txp - b->x, ddy = typ - b->y;
                dist = fsqrt(ddx*ddx + ddy*ddy);
            }
        }
        // bite whatever it's chasing. A jetpacking player is ABOVE
        // the swarm: only a near-direct meeting connects (and one
        // landed hit ends the jet — see hurt_player).
        float brange = (tp>=0 && G.p[tp].bean_jet>0)? rad*0.6f+8.0f : rad+13.0f;
        if (dist < brange){
            b->chew_t -= dt;
            if (b->chew_t<=0){ b->chew_t = 0.8f;
                if (tp==-2){
                    Buddy *tb=&G.bud[b->btarget];
                    if (tb->on_wall<0) hurt_buddy(b->btarget, b->dmg);
                    else {   // the buddy's up on a run too: chew it down
                        int wi=-1; float bd2=1e18f;
                        for (int j=0;j<MAXWALLS;j++) if (G.walls[j].alive==1 && G.walls[j].grp==tb->on_wall){
                            float ddx=G.walls[j].x-b->x, ddy=G.walls[j].y-b->y, d2=ddx*ddx+ddy*ddy;
                            if (d2<bd2){ bd2=d2; wi=j; }
                        }
                        if (wi>=0) damage_wall(wi, b->dmg*2.4f);
                    }
                }
                else if (G.p[tp].on_wall<0) hurt_player(tp, b->dmg);   // up on a wall run: out of biting reach — they have to chew him down
                else {
                    // mounted at the very lip of the run, a small bug can
                    // reach the player's tile without ever entering the
                    // wall box — left alone it would vibrate there forever,
                    // neither biting nor chewing. So: chew the segment
                    // of his run nearest to hand, at the usual chew rate.
                    int wi=-1; float bd2=1e18f;
                    for (int j=0;j<MAXWALLS;j++) if (G.walls[j].alive==1 && G.walls[j].grp==G.p[tp].on_wall){
                        float ddx=G.walls[j].x-b->x, ddy=G.walls[j].y-b->y, d2=ddx*ddx+ddy*ddy;
                        if (d2<bd2){ bd2=d2; wi=j; }
                    }
                    if (wi>=0) damage_wall(wi, b->dmg*2.4f);
                }
            }
        }
    }
    // gentle separation so they don't stack into one mega-bug
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive)
        for (int j=i+1;j<MAXBUGS;j++) if (G.bugs[j].alive){
            Bug *a=&G.bugs[i], *c=&G.bugs[j];
            float dx=c->x-a->x, dy=c->y-a->y;
            float rr=(bug_radius(a->w)+bug_radius(c->w))*0.75f;
            float d2=dx*dx+dy*dy;
            if (d2>0.01f && d2<rr*rr){
                float d=fsqrt(d2), push=(rr-d)*0.5f/d;
                a->x-=dx*push*0.5f; a->y-=dy*push*0.5f;
                c->x+=dx*push*0.5f; c->y+=dy*push*0.5f;
            }
        }
    // expulsion: while the shield is up, EVERY bug inside the ring is
    // put outside it — instantly, not shoved. The in-loop shove never
    // reaches bugs with no living target outside (everyone is inside
    // shopping: they `continue` before it) or breakpoint-frozen bugs,
    // so entering the mall left the plaza still crawling (Daniel).
    if (shield){
        for (int i=0;i<MAXBUGS;i++){
            Bug *b=&G.bugs[i];
            if (!b->alive) continue;
            float rad = bug_radius(b->w);
            float bd = fsqrt(b->x*b->x + b->y*b->y);
            float rr = 152.0f + rad;
            if (bd < rr){
                if (bd < 0.01f){ b->x = rr; b->y = 0; }
                else { float sc = rr/bd; b->x *= sc; b->y *= sc; }
            }
        }
    }
}

static void update_fx(float dt){
    for (int i=0;i<(int)(sizeof(G.fx)/sizeof(G.fx[0]));i++){
        CarcassFx *f=&G.fx[i];
        if (!f->alive) continue;
        if (f->mode==0){ f->x1=G.p[f->player].x; f->y1=G.p[f->player].y-24; }
        if (f->mode==1){ f->x1=0; f->y1=-30; }
        f->t += dt/f->dur;
        if (f->t>=1){
            f->alive=0;
            if (f->mode==0){ hat_push(&G.p[f->player], f->w); sfx(6); }
            if (f->mode==2 && f->wall_slot>=0){
                int s=f->wall_slot;
                if (++wall_build_n[s]>=3 && G.walls[s].alive==2){
                    G.walls[s].alive=1;
                    splat(G.walls[s].x, G.walls[s].y, COL(170,120,70), 8, 90);
                    walls_recompute();
                }
            }
        }
    }
    for (int i=0;i<MAXPART;i++) if (G.parts[i].alive){
        Particle *q=&G.parts[i];
        q->life-=dt; if (q->life<=0){ q->alive=0; continue; }
        q->x+=q->vx*dt; q->y+=q->vy*dt; q->vx*=(1-3*dt); q->vy*=(1-3*dt);
    }
    for (int i=0;i<MAXFLOAT;i++) if (G.floats[i].alive){
        Floater *q=&G.floats[i];
        q->life-=dt; q->y-=22*dt; if (q->life<=0) q->alive=0;
    }
}

static void hurt_buddy(int owner, float dmg){
    Buddy *b=&G.bud[owner];
    if (!b->active || b->fainted) return;
    if (b->steady_t>0) return;                       // ROCK STEADY: not today
    float f = (1.0f - 0.12f*b->up_def) * BUD_DEF[b->species%BUDN];
    if (f<0.2f) f=0.2f;                              // training + species toughness
    b->hp -= dmg*f; b->hurt_t = 0.2f; b->calm = 0;
    if (b->hp<=0){
        // buddies FAINT — they never die (Daniel's rule). A fainted
        // buddy stays where it fell, out of the fight, until its
        // owner hauls back to the Banzai Buddies shop for a revive.
        b->hp=0; b->fainted=1;
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].btarget==owner) G.bugs[j].btarget=-1;
        splat(b->x, b->y, COL(255,150,150), 14, 110);
        spawn_floater(b->x, b->y-20, "FAINTED!", COL(255,120,120));
        // and at the owner, who may be looking the other way entirely —
        // a missed faint reads exactly like a vanished buddy (Nov 3)
        spawn_floater(G.p[owner].x, G.p[owner].y-26, "YOUR BUDDY FAINTED!", COL(255,150,150));
        sfx(17);
    }
}
static void buddy_hit(int pi, int bi, float dmg, float kb){
    Bug *t=&G.bugs[bi];
    Buddy *b=&G.bud[pi];
    if (!t->alive) return;
    float dx=t->x-b->x, dy=t->y-b->y, d=fsqrt(dx*dx+dy*dy)+0.01f;
    hurt_bug(bi, dmg, pi, dx/d*kb, dy/d*kb);
    if (!t->alive) b->xp += 2 + (int)(t->w*0.6f);        // its kill, its lesson
    else t->btarget = pi;                                 // and now it's personal
}
static void buddy_move2(int pi, int bi){
    Buddy *b=&G.bud[pi]; Bug *t=&G.bugs[bi];
    if (!t->alive) return;
    b->energy -= BUD_EN_MOVE2; if (b->energy<0) b->energy=0;
    b->move_cd = (b->species==2 || b->species==4)? 5.5f : 4.0f;
    b->lunge_t = 0.22f;
    if (b->species==0){            // POUNCE: land on it, splash the neighbours
        b->x = t->x + rng_range(-8,8); b->y = t->y + rng_range(-8,8);
        splat(b->x, b->y, COL(240,200,120), 8, 90);
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 46*46) buddy_hit(pi, j, buddy_dmg(b)*1.6f, 90);
        }
        sfx(3);
    } else if (b->species==1){     // RAKE: three slashes, one breath
        for (int k=0;k<3;k++) if (t->alive) buddy_hit(pi, bi, buddy_dmg(b)*0.7f, 30);
        sfx(3);
    } else if (b->species==3){     // SPUR DIVE: a platypus lands, venom first
        b->x = t->x + rng_range(-8,8); b->y = t->y + rng_range(-8,8);
        splat(b->x, b->y, COL(150,220,170), 8, 90);
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 44*44){ buddy_hit(pi, j, buddy_dmg(b)*1.7f, 70); if (G.bugs[j].alive) G.bugs[j].bp_t = 0.9f; }
        }
        sfx(3);
    } else if (b->species==4){     // ROLL OUT: a rock goes bowling
        float dx=t->x-b->x, dy=t->y-b->y, d=fsqrt(dx*dx+dy*dy)+0.01f;
        float ux=dx/d, uy=dy/d;
        float sx=b->x, sy=b->y;
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float rx=G.bugs[j].x-sx, ry=G.bugs[j].y-sy;
            float along = rx*ux + ry*uy, side = -rx*uy + ry*ux;
            if (along>0 && along<d+26 && side>-22 && side<22) buddy_hit(pi, j, buddy_dmg(b)*1.5f, 320);
        }
        b->x = t->x + ux*14; b->y = t->y + uy*14;
        b->roll += d/8.0f;
        splat(b->x, b->y, COL(170,166,158), 10, 100);
        sfx(7);
    } else if (b->species==5){     // SPLASH DOWN: the sink objects, wetly
        splat(b->x, b->y, COL(120,190,255), 14, 110);
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 56*56) buddy_hit(pi, j, buddy_dmg(b)*1.4f, 150);
        }
        spawn_floater(b->x, b->y-22, "SPLASH!", COL(170,220,255));
        sfx(12);
    } else {                       // CHARGE: plow the line, end at the target
        float dx=t->x-b->x, dy=t->y-b->y, d=fsqrt(dx*dx+dy*dy)+0.01f;
        float ux=dx/d, uy=dy/d;
        float sx=b->x, sy=b->y;
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float rx=G.bugs[j].x-sx, ry=G.bugs[j].y-sy;
            float along = rx*ux + ry*uy, side = -rx*uy + ry*ux;
            if (along>0 && along<d+20 && side>-24 && side<24) buddy_hit(pi, j, buddy_dmg(b)*1.8f, 240);
        }
        b->x = t->x - ux*20; b->y = t->y - uy*20;
        splat(b->x, b->y, COL(200,170,130), 10, 100);
        sfx(7);
    }
}
static void buddy_special(int pi, int bi){
    { Buddy *bb=&G.bud[pi]; bb->energy -= BUD_EN_SPEC; if (bb->energy<0) bb->energy=0; }
    Buddy *b=&G.bud[pi];
    b->spec_cd = 9.0f;
    b->lunge_t = 0.25f;
    if (b->species==0){            // BARK: a freezing shout
        spawn_floater(b->x, b->y-22, "WOOF!", COL(255,240,180));
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 95*95){ G.bugs[j].bp_t = 1.2f; buddy_hit(pi, j, buddy_dmg(b)*0.5f, 40); }
        }
        sfx(12);
    } else if (b->species==1){     // HAIRBALL: three wet ones (duck-chained shots)
        Bug *t=&G.bugs[bi];
        if (t->alive){
            float base = fatan2(t->y-b->y, t->x-b->x);
            for (int k=-1;k<=1;k++)
                fire_proj(pi, 2, b->x, b->y-4, base + k*0.16f, buddy_dmg(b)*1.2f, -2);
        }
        spawn_floater(b->x, b->y-22, "HACK!", COL(200,255,200));
        sfx(3);
    } else if (b->species==3){     // VENOM STING: one bug, all the venom
        Bug *t=&G.bugs[bi];
        spawn_floater(b->x, b->y-22, "STUNG!", COL(170,255,190));
        if (t->alive){ buddy_hit(pi, bi, buddy_dmg(b)*3.2f, 60); if (t->alive) t->bp_t = 1.6f; }
        sfx(3);
    } else if (b->species==4){     // ROCK STEADY: a rock cannot be moved
        b->steady_t = 5.0f;
        spawn_floater(b->x, b->y-22, "STEADY.", COL(220,216,205));
        splat(b->x, b->y+6, COL(170,166,158), 10, 90);
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 82*82) buddy_hit(pi, j, buddy_dmg(b)*1.0f, 280);
        }
        sfx(7);
    } else if (b->species==5){     // OVERFLOW: the sink gives everything back
        spawn_floater(b->x, b->y-24, "OVERFLOW!", COL(170,220,255));
        splat(b->x, b->y, COL(120,190,255), 20, 140);
        if (G.shake<2) G.shake=2;
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 106*106){ if (G.bugs[j].alive) G.bugs[j].bp_t = 0.5f; buddy_hit(pi, j, buddy_dmg(b)*1.6f, 230); }
        }
        sfx(12);
    } else {                       // STOMP: the ground itself objects
        spawn_floater(b->x, b->y-24, "STOMP!", COL(255,210,150));
        splat(b->x, b->y+8, COL(190,160,120), 16, 130);
        if (G.shake<2) G.shake=2;
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 74*74){ G.bugs[j].bp_t = 0.7f; buddy_hit(pi, j, buddy_dmg(b)*2.0f, 160); }
        }
        sfx(7);
    }
}

// MOVE 3 (skills bit2, 60 XP): a second taught trick per species.
static void buddy_move3(int pi, int bi){
    Buddy *b=&G.bud[pi]; Bug *t=&G.bugs[bi];
    if (!t->alive) return;
    b->energy -= BUD_EN_MOVE3; if (b->energy<0) b->energy=0;
    b->move3_cd = (b->species==2 || b->species==4)? 8.0f : 7.0f;
    b->lunge_t = 0.22f;
    if (b->species==0){            // FETCH: the far one. There and back again.
        int far=bi; float fd=-1;
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y, d2=dx*dx+dy*dy;
            if (d2<280*280 && d2>fd){ fd=d2; far=j; }
        }
        Bug *f=&G.bugs[far]; float sx=b->x, sy=b->y;
        b->x = f->x; b->y = f->y;
        splat(b->x, b->y, COL(240,200,120), 8, 90);
        buddy_hit(pi, far, buddy_dmg(b)*2.2f, 120);
        spawn_floater(b->x, b->y-22, "FETCHED!", COL(255,235,180));
        b->x = sx; b->y = sy;      // and back, pleased with itself
        sfx(3);
    } else if (b->species==1){     // FLYING CLAWS: a fan of five
        float base = fatan2(t->y-b->y, t->x-b->x);
        for (int k=-2;k<=2;k++)
            fire_proj(pi, 2, b->x, b->y-4, base + k*0.15f, buddy_dmg(b)*0.85f, -2);
        spawn_floater(b->x, b->y-22, "CLAWS OUT!", COL(255,215,235));
        sfx(3);
    } else if (b->species==3){     // BILL SLAM: one bug, thoroughly billed
        buddy_hit(pi, bi, buddy_dmg(b)*2.6f, 90);
        if (t->alive) t->bp_t = 1.2f;
        splat(t->x, t->y, COL(150,220,170), 10, 100);
        spawn_floater(b->x, b->y-22, "BILLED!", COL(190,255,205));
        sfx(7);
    } else if (b->species==4){     // SEDIMENT SLAM: a short, heavy roll
        float dx=t->x-b->x, dy=t->y-b->y, d=fsqrt(dx*dx+dy*dy)+0.01f;
        float ux=dx/d, uy=dy/d, sx=b->x, sy=b->y;
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float rx=G.bugs[j].x-sx, ry=G.bugs[j].y-sy;
            float along = rx*ux + ry*uy, side = -rx*uy + ry*ux;
            if (along>0 && along<110 && side>-26 && side<26) buddy_hit(pi, j, buddy_dmg(b)*2.0f, 340);
        }
        b->x = sx + ux*44; b->y = sy + uy*44; b->roll += 2.2f;
        splat(b->x, b->y, COL(170,166,158), 10, 100);
        spawn_floater(b->x, b->y-22, "SLAM!", COL(225,220,205));
        sfx(7);
    } else if (b->species==5){     // DISPOSAL: the weakest link, ground up
        int weak=bi; float wh=1e18f;
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 160*160 && G.bugs[j].hp<wh){ wh=G.bugs[j].hp; weak=j; }
        }
        buddy_hit(pi, weak, buddy_dmg(b)*3.5f, 40);
        splat(G.bugs[weak].x, G.bugs[weak].y, COL(150,150,160), 12, 110);
        spawn_floater(b->x, b->y-22, "DISPOSED!", COL(200,225,255));
        sfx(12);
    } else {                       // ANTLER SWEEP: a wide, branching arc
        float base = fatan2(t->y-b->y, t->x-b->x);
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y, d2=dx*dx+dy*dy;
            if (d2 < 98*98){
                float da = fatan2(dy, dx) - base;
                while (da>3.14159f) da -= 6.28318f; while (da<-3.14159f) da += 6.28318f;
                if (da>-1.1f && da<1.1f) buddy_hit(pi, j, buddy_dmg(b)*1.8f, 200);
            }
        }
        splat(b->x + fcos(base)*40, b->y + fsin(base)*40, COL(200,170,130), 10, 100);
        spawn_floater(b->x, b->y-24, "SWEEP!", COL(255,220,170));
        sfx(7);
    }
}
// MOVE 4 (skills bit3, 120 XP): the big one. Rare, pricey, worth it.
static void buddy_move4(int pi, int bi){
    Buddy *b=&G.bud[pi]; Bug *t=&G.bugs[bi];
    if (!t->alive) return;
    b->energy -= BUD_EN_MOVE4; if (b->energy<0) b->energy=0;
    b->move4_cd = (b->species==4)? 14.0f : 12.0f;
    b->lunge_t = 0.26f;
    if (b->species==0){            // PUPPY EYES: nobody can hit a puppy
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 132*132){ if (G.bugs[j].alive) G.bugs[j].bp_t = 2.5f; buddy_hit(pi, j, buddy_dmg(b)*0.5f, 0); }
        }
        spawn_floater(b->x, b->y-26, "PUPPY EYES!", COL(255,200,225));
        sfx(11);
    } else if (b->species==1){     // AMBUSH: behind the biggest one
        int big=bi; float bw2=-1;
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y, d2=dx*dx+dy*dy;
            if (d2<320*320 && G.bugs[j].w>bw2){ bw2=G.bugs[j].w; big=j; }
        }
        Bug *f=&G.bugs[big];
        b->x = f->x - fcos(b->aim)*18; b->y = f->y - fsin(b->aim)*18;
        buddy_hit(pi, big, buddy_dmg(b)*3.0f, 80);
        for (int j=0;j<MAXBUGS;j++) if (j!=big && G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 52*52) buddy_hit(pi, j, buddy_dmg(b)*1.2f, 60);
        }
        splat(b->x, b->y, COL(255,215,235), 10, 100);
        spawn_floater(b->x, b->y-22, "AMBUSH!", COL(255,200,235));
        sfx(3);
    } else if (b->species==3){     // SIXTH SENSE: the chain finds four
        int done[4] = { -1, -1, -1, -1 }, hitn=0;
        for (int j=0;j<4;j++){
            int best=-1; float bd2=170*170;
            for (int k2=0;k2<MAXBUGS;k2++) if (G.bugs[k2].alive){
                int seen=0; for (int m=0;m<4;m++) if (done[m]==k2) seen=1;
                if (seen) continue;
                float dx=G.bugs[k2].x-b->x, dy=G.bugs[k2].y-b->y, d2=dx*dx+dy*dy;
                if (d2<bd2){ bd2=d2; best=k2; }
            }
            if (best<0) break;
            done[hitn++]=best;
            splat(G.bugs[best].x, G.bugs[best].y, COL(170,255,230), 6, 80);
            buddy_hit(pi, best, buddy_dmg(b)*1.5f, 30);
        }
        spawn_floater(b->x, b->y-22, "IT KNOWS!", COL(190,255,230));
        sfx(11);
    } else if (b->species==4){     // AVALANCHE: the hill comes down
        int drops=0;
        for (int j=0;j<MAXBUGS && drops<6;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 195*195){
                splat(G.bugs[j].x, G.bugs[j].y, COL(170,166,158), 8, 100);
                buddy_hit(pi, j, buddy_dmg(b)*1.3f, 120);
                drops++;
            }
        }
        if (G.shake<2) G.shake=2;
        spawn_floater(b->x, b->y-24, "AVALANCHE!", COL(225,220,205));
        sfx(7);
    } else if (b->species==5){     // EVERYTHING: including the sink
        int shots=0;
        for (int j=0;j<MAXBUGS && shots<8;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 265*265){
                fire_proj(pi, 2, b->x, b->y-4, fatan2(dy, dx), buddy_dmg(b)*0.7f, -2);
                shots++;
            }
        }
        if (!shots){ float base=b->aim; for (int k2=0;k2<5;k2++) fire_proj(pi, 2, b->x, b->y-4, base + (k2-2)*0.18f, buddy_dmg(b)*0.7f, -2); }
        spawn_floater(b->x, b->y-24, "EVERYTHING!", COL(200,225,255));
        sfx(12);
    } else {                       // TIMBER!: the line comes down
        float dx=t->x-b->x, dy=t->y-b->y, d=fsqrt(dx*dx+dy*dy)+0.01f;
        float ux=dx/d, uy=dy/d;
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float rx=G.bugs[j].x-b->x, ry=G.bugs[j].y-b->y;
            float along = rx*ux + ry*uy, side = -rx*uy + ry*ux;
            if (along>0 && along<145 && side>-34 && side<34) buddy_hit(pi, j, buddy_dmg(b)*2.4f, 380);
        }
        if (G.shake<3) G.shake=3;
        splat(b->x + ux*70, b->y + uy*70, COL(170,140,100), 14, 120);
        spawn_floater(b->x, b->y-26, "TIMBER!", COL(255,220,170));
        sfx(7);
    }
}
static void update_buddies(float dt){
    for (int i=0;i<MAXP;i++){
        Buddy *b=&G.bud[i];
        if (!b->active) continue;
        if (b->in_bw) continue;   // in Banzai World: bw_update drives it
        Player *o=&G.p[i];
        if (b->mv_t>0) b->mv_t-=dt;
        if (b->atk_cd>0) b->atk_cd-=dt;
        if (b->move_cd>0) b->move_cd-=dt;
        if (b->spec_cd>0) b->spec_cd-=dt;
        if (b->move3_cd>0) b->move3_cd-=dt;
        if (b->move4_cd>0) b->move4_cd-=dt;
        if (!b->fainted && b->energy<BUD_ENERGY_MAX){   // energy comes back as you go
            b->energy += BUD_EN_REGEN*dt; if (b->energy>BUD_ENERGY_MAX) b->energy=BUD_ENERGY_MAX;
        }
        if (b->lunge_t>0) b->lunge_t-=dt;
        if (b->hurt_t>0) b->hurt_t-=dt;
        if (b->steady_t>0) b->steady_t-=dt;
        b->calm += dt;              // a quiet stretch and a buddy catches its breath
        if (!b->fainted && b->calm>4.0f && b->hp<b->maxhp){
            b->hp += 2.2f*dt; if (b->hp>b->maxhp) b->hp=b->maxhp;   // weakened Nov 2 (was 3.5/s after 3s): buddies should faint sometimes
        }
        if (b->fainted) continue;   // out cold where it fell — only the shop brings it back
        if (!o->connected) continue;
        // zones: 0 the field, 1 the mall, 2 the arcade — the buddy
        // trots through whichever door you used to stay in your zone
        int owner_zone = o->in_arcade? 2 : o->in_mall? 1 : 0;
        int buddy_zone = (b->y > ARC_CY-300 && b->y < ARC_CY+300)? 2
                       : (b->y > MALL_CY-300 && b->y < MALL_CY+300)? 1 : 0;
        int owner_mall = owner_zone!=0;
        if (owner_zone != buddy_zone){   // trotting through the door after you
            b->x = o->x+26; b->y = o->y+14;
            splat(b->x, b->y, COL(255,240,180), 6, 70);
        }
        int tgt=-1; float td=300*300;
        if (!owner_mall)
            for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
                float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y, d2=dx*dx+dy*dy;
                if (d2<td){ td=d2; tgt=j; }
            }
        float gx, gy;
        if (tgt>=0){
            Bug *t=&G.bugs[tgt];
            float dx=t->x-b->x, dy=t->y-b->y, d=fsqrt(dx*dx+dy*dy)+0.01f;
            b->aim = fatan2(dy, dx);
            float rr = bug_radius(t->w) + 15;
            if (d > rr){ gx=t->x; gy=t->y; }
            else {
                gx=b->x; gy=b->y;
                if (b->atk_cd<=0){
                    b->atk_cd = BUD_CD0[b->species%BUDN];
                    b->lunge_t = 0.16f;
                    buddy_hit(i, tgt, buddy_dmg(b), 60);
                }
                if (t->alive && (b->skills&1) && b->move_cd<=0 && b->energy>=BUD_EN_MOVE2) buddy_move2(i, tgt);
                { int near=0;
                  for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
                      float ddx=G.bugs[j].x-b->x, ddy=G.bugs[j].y-b->y;
                      if (ddx*ddx+ddy*ddy<110*110) near++;
                  }
                  if (t->alive && (b->skills&2) && b->spec_cd<=0 && b->energy>=BUD_EN_SPEC && (near>=2 || t->w>25)) buddy_special(i, tgt);
                  if (t->alive && (b->skills&4) && b->move3_cd<=0 && b->energy>=BUD_EN_MOVE3 && (near>=2 || t->w>30)) buddy_move3(i, tgt);
                  if (t->alive && (b->skills&8) && b->move4_cd<=0 && b->energy>=BUD_EN_MOVE4 && (near>=3 || t->w>45)) buddy_move4(i, tgt);
                }
            }
            if (d>60 && d<170 && (b->skills&1) && b->move_cd<=0 && b->energy>=BUD_EN_MOVE2) buddy_move2(i, tgt);   // gap-close
        } else {
            gx = o->x - fcos(o->aim)*30 + 12;
            gy = o->y - fsin(o->aim)*30 + 16;
            float dx=gx-b->x, dy=gy-b->y, d=fsqrt(dx*dx+dy*dy);
            if (d>4) b->aim = fatan2(dy, dx);
            if (d>420){ b->x=gx; b->y=gy; b->on_wall=-1; splat(b->x, b->y, COL(255,240,180), 6, 70); }
        }
        float dx=gx-b->x, dy=gy-b->y, d=fsqrt(dx*dx+dy*dy);
        if (d>2){
            float sp=BUD_SPD[b->species%BUDN];
            if (b->species==4){
                // an oblong rock rolls awkwardly: it crawls as it
                // climbs up onto its end, then lurches over the flop
                float ph = b->roll; ph -= (int)(ph/3.14159f)*3.14159f; if (ph<0) ph+=3.14159f;
                float tri = ph<1.5708f? ph/1.5708f : (3.14159f-ph)/1.5708f;
                sp *= 0.68f + 0.6f*tri;
            }
            float step=sp*dt; if (step>d) step=d;
            if (b->species==4) b->roll += step/20.0f;   // the rock rolls
            float hx = dx/d, hy = dy/d;
            // GREASE (Daniel, Nov 10): same medicine as the bugs — a
            // buddy pinned behind a prop bends its heading to the open
            // side and slides around instead of hugging it forever
            if (b->grease_t>0){
                b->grease_t -= dt;
                float gx2 = hx*0.22f + (-hy)*b->grease_s, gy2 = hy*0.22f + hx*b->grease_s;
                float gl = fsqrt(gx2*gx2 + gy2*gy2);
                if (gl>0.01f){ hx = gx2/gl; hy = gy2/gl; }
            }
            float nx=b->x + hx*step, ny=b->y + hy*step;
            if (owner_mall){
                float zcy = o->in_arcade? ARC_CY : MALL_CY;
                b->x=nx; b->y=ny;
                if (b->x<-282) b->x=-282; if (b->x>282) b->x=282;
                if (b->y<zcy-160) b->y=zcy-160; if (b->y>zcy+154) b->y=zcy+154;
            } else {
                float ox0=b->x, oy0=b->y;
                if (!prop_collide(nx, b->y, 9)) b->x=nx;
                if (!prop_collide(b->x, ny, 9)) b->y=ny;
                // walls resolve in buddy_wall_step below: a climbable
                // run is a floor to him, not a wall (Daniel)
                float mvx=b->x-ox0, mvy=b->y-oy0;
                if (step>0.4f && d>26 && mvx*mvx+mvy*mvy < step*step*0.0625f){
                    b->stuck_t += dt;
                    if (b->stuck_t>0.30f && b->grease_t<=0){
                        float px2=-hy, py2=hx, pr=24;
                        int lf = !prop_collide(b->x+px2*pr, b->y+py2*pr, 9);
                        int rf = !prop_collide(b->x-px2*pr, b->y-py2*pr, 9);
                        b->grease_s = (lf && !rf)? 1 : (rf && !lf)? -1 : (b->grease_s? -b->grease_s : 1);
                        b->grease_t = 0.55f; b->stuck_t = 0;
                    }
                } else if (b->stuck_t>0){ b->stuck_t -= dt*2; if (b->stuck_t<0) b->stuck_t=0; }
            }
            b->walk_ph += dt*sp*0.06f;
            b->mv_t = 0.15f;
        }
        if (!owner_mall) buddy_wall_step(b);
    }
}

static void update_sim(float dt){
    G.time += dt;
    { static int prev_shield = 0; int sh = shield_active();
      if (sh && !prev_shield) sfx(8);                 // shield rises: firewall ping
      prev_shield = sh; }
    update_players(dt);
    update_projectiles(dt);
    update_bugs(dt);
    // Bug Bowl games tick for whoever is standing at the lanes
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].in_store && G.p[i].station==4)
        bowl_update(i, dt);
    update_buddies(dt);
    update_fx(dt);
    update_payment(dt);
    spawner(dt);
    // game over: everyone connected is dead
    int any_conn=0, any_alive=0;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){ any_conn=1; if (G.p[i].alive) any_alive=1; }
    if (any_conn && !any_alive && (G.state==GS_PLAY || G.state==GS_STORE)){
        G.state=GS_OVER; G.gameover_t=0; G.has_save=0;
        // the run is finished: empty its slot so CONTINUE can't
        // resurrect it (records are banked by save_records below)
        if (G.slot>=1 && G.slot<=3 && G.slot_has[G.slot]){
            plat_save_slot(G.slot, "TSG3\n", 5);
            G.slot_has[G.slot]=0; refresh_has_save();
        }
        save_records();
        sfx(18);
    }
    // autosave
    G.autosave_t -= dt;
    if (G.autosave_t<=0){ G.autosave_t=5.0f; if (G.state==GS_PLAY||G.state==GS_STORE) do_save(); }
}

// ---------------------------------------------------------------------------
// Rendering — the cartoony world
// ---------------------------------------------------------------------------
static int SX(float wx){ float z = G.zoom>0.01f? G.zoom : 1.0f; int vw = G.view_w>0? G.view_w : VW; return (int)((wx - G.camx)*z + G.view_x0 + vw/2); }
static int SY(float wy){ float z = G.zoom>0.01f? G.zoom : 1.0f; int vh = G.view_h>0? G.view_h : VH; return (int)((wy - G.camy)*z + G.view_y0 + vh/2); }

// ---------------------------------------------------------------------------
// EDDIE MODE sprites: every bug is a fuzzy orange puffball — round fur,
// cream belly, two shuffling feet and one very big shiny red nose. An
// original generic puff-critter design (Daniel's mode, Daniel's name).
// ---------------------------------------------------------------------------
static void draw_eddie_carcass(int x, int y, int s, float tilt){
    int o = (int)(tilt*3);
    ellipse_fill(x+o, y, s+6, s+1, OUTLINE);
    ellipse_shade(x+o, y, s+4, s-1>2? s-1:2, COL(243,146,44));
    for (int i=0;i<5;i++){   // flattened fuzz bumps along the top
        int fx = x-s+2 + i*(s/2) + o;
        circle_fill(fx, y-s+1, 2, COL(243,146,44));
    }
    ellipse_fill(x+o, y, s/2+2, 2, OUTLINE);                 // squished nose
    ellipse_shade(x+o, y, s/2, 1, COL(228,54,46));
    line_thick(x-5+o, y-2, x-2+o, y+1, 1, OUTLINE); line_thick(x-5+o, y+1, x-2+o, y-2, 1, OUTLINE);
    line_thick(x+3+o, y-2, x+6+o, y+1, 1, OUTLINE); line_thick(x+3+o, y+1, x+6+o, y-2, 1, OUTLINE);
    line_thick(x-3+o, y-s+1, x-3+o, y-s-3, 1, OUTLINE);      // feet in the air
    line_thick(x+4+o, y-s+1, x+4+o, y-s-3, 1, OUTLINE);
}
static void draw_eddie_full(Bug *b, int x, int y, int r){
    int flash = b->hit_t>0;
    u32 fur  = flash? COL(255,255,255) : COL(243,146,44);
    u32 fsh  = flash? COL(230,230,230) : COL(214,116,32);
    u32 belly= flash? COL(255,255,255) : COL(255,224,168);
    u32 nose = flash? COL(255,255,255) : COL(228,54,46);
    shadow_ellipse(x, y+r-1, r+3, r/3+1);
    // two little feet, shuffling with the leg phase
    int fo = (int)(fsin(b->leg_ph)*2.0f);
    ellipse_fill(x-r/2+fo, y+r-2, r/3+2, 3, OUTLINE);
    ellipse_fill(x+r/2-fo, y+r-2, r/3+2, 3, OUTLINE);
    ellipse_shade(x-r/2+fo, y+r-2, r/3, 2, COL(146,84,30));
    ellipse_shade(x+r/2-fo, y+r-2, r/3, 2, COL(146,84,30));
    // fuzz: bumps all around the rim, jittered by the bug's own seed
    for (int i=0;i<12;i++){
        float a = i*0.5236f + b->seed*3.0f;
        int bx = x + (int)(fcos(a)*(r+1)), by = y + (int)(fsin(a)*(r*3/4+1));
        circle_fill(bx, by, r/6+2, fur);
    }
    // round body + cream belly
    ellipse_fill(x, y, r+4, r*3/4+4, OUTLINE);
    ellipse_shade(x, y, r+2, r*3/4+2, fur);
    ellipse_shade(x, y, r, r*3/4, fsh);
    ellipse_shade(x, y+r/3, r*3/5, r/3, belly);
    gloss(x, y, r+2);
    // googly eyes above the nose, tracking like any bug's
    int look = (int)(fsin(G.anim_t*2 + b->seed)*1.5f);
    int ey = y - r/3, ex1 = x - r/3, ex2 = x + r/3, er = r/5+2;
    circle_fill(ex1, ey, er+1, OUTLINE); circle_fill(ex2, ey, er+1, OUTLINE);
    circle_fill(ex1, ey, er, COL(255,255,255)); circle_fill(ex2, ey, er, COL(255,255,255));
    circle_fill(ex1+look, ey, er/2+1, OUTLINE); circle_fill(ex2+look, ey, er/2+1, OUTLINE);
    pset(ex1+look-1, ey-1, COL(255,255,255)); pset(ex2+look-1, ey-1, COL(255,255,255));
    // THE nose: one big shiny red button right in the middle
    int nr = r*55/100; if (nr<3) nr=3;
    circle_fill(x, y+2, nr+2, OUTLINE);
    circle_shade(x, y+2, nr, nose);
    pset_blend(x-nr/3, y+2-nr/3, COL(255,255,255), 200);
    pset_blend(x-nr/3+1, y+2-nr/3, COL(255,255,255), 140);
    pset_blend(x-nr/3, y+2-nr/3+1, COL(255,255,255), 140);
    // hp bar on big bugs + breakpoint symbol, same as any bug
    if (b->maxhp>=40 && b->hp<b->maxhp){
        int wpx = r*2;
        rect_fill(x-r, y-r-10, wpx, 3, COL(40,30,40));
        rect_fill(x-r, y-r-10, (int)(wpx*b->hp/b->maxhp), 3, COL(120,230,90));
    }
    if (b->bp_t>0){
        circle_out(x+r-2, y-r-4, 7, COL(70,140,255));
        rect_fill(x+r-5, y-r-8, 2, 7, COL(255,255,255));
        rect_fill(x+r-1, y-r-8, 2, 7, COL(255,255,255));
    }
}
static int g_gold_carcass = 0;   // Whales Only: the hat being drawn is golden
static void draw_carcass(int x, int y, int s, int kind, float tilt){
    if (G.eddie){ draw_eddie_carcass(x, y, s, tilt); return; }
    // a dead bug, belly-up: squished body, legs in the air, X eyes
    u32 col = g_gold_carcass? COL(226,168,44) : BUG_COL[kind&3];
    int o = (int)(tilt*3);
    ellipse_fill(x+o, y, s+5, s+2, OUTLINE);
    ellipse_shade(x+o, y, s+3, s, g_gold_carcass? COL(255,224,130) : BUG_BELLY[kind&3]);
    ellipse_shade(x+o, y-1, s, s-2>2? s-2:2, col);   // a patch of shell color
    pset_blend(x-2+o, y-3, COL(255,255,255), 120);   // waxy sheen
    pset_blend(x-1+o, y-3, COL(255,255,255), 90);
    for (int l=0;l<3;l++){
        int lx = x - s + l*(s) + o;
        line_thick(lx, y-2, lx-3, y-7-(l&1)*2, 1, OUTLINE);
        line_thick(lx+4, y-2, lx+7, y-7-((l+1)&1)*2, 1, OUTLINE);
    }
    // X eyes
    line_thick(x-4+o, y-3, x-1+o, y, 1, OUTLINE); line_thick(x-4+o, y, x-1+o, y-3, 1, OUTLINE);
    line_thick(x+2+o, y-3, x+5+o, y, 1, OUTLINE); line_thick(x+2+o, y, x+5+o, y-3, 1, OUTLINE);
}
static void draw_bug_sprite(Bug *b){
    int x=SX(b->x), y=SY(b->y);
    if (x<-70||x>VW+70||y<-70||y>VH+70) return;
    int r=(int)(bug_radius(b->w)*(G.zoom>0.01f? G.zoom : 1.0f)); if (r<3) r=3;
    if (G.eddie){ draw_eddie_full(b, x, y, r); return; }
    u32 col = b->hit_t>0? COL(255,255,255) : BUG_COL[b->kind];
    u32 dark = col_shade(col, 0.72f);
    shadow_ellipse(x, y+r-1, r+3, r/3+1);
    // legs: 6 (spiders 8), two-segment with a knee, scuttling with leg_ph
    int legs = b->kind==2? 8 : 6;
    for (int l=0;l<legs;l++){
        float side = (l%2==0)? -1.0f: 1.0f;
        float row = (float)(l/2)/(legs/2-1) - 0.5f;      // -0.5..0.5 along body
        float wig = fsin(b->leg_ph + l*1.7f)*3.0f;
        int lx = x + (int)(row*r*1.2f);
        int ly = y + (int)(side*r*0.55f);
        int kx = lx + (int)(wig*0.6f + row*3), ky = ly + (int)(side*(r*0.38f));
        int fx2 = lx + (int)(wig + row*5), fy2 = ly + (int)(side*(r*0.68f));
        line_thick(lx, ly, kx, ky, 1, OUTLINE);
        line_thick(kx, ky, fx2, fy2, 1, OUTLINE);
        pset(fx2, fy2, dark);
    }
    if (b->kind==3){ // moth wings: shaded, spotted, gently translucent tips
        int flap = (int)(fsin(b->leg_ph*1.6f)*4);
        ellipse_fill(x-r-4, y-4-flap, r/2+5, r/3+4, OUTLINE);
        ellipse_fill(x+r+4, y-4-flap, r/2+5, r/3+4, OUTLINE);
        ellipse_shade(x-r-4, y-4-flap, r/2+3, r/3+2, BUG_BELLY[3]);
        ellipse_shade(x+r+4, y-4-flap, r/2+3, r/3+2, BUG_BELLY[3]);
        circle_fill(x-r-6, y-6-flap, 2, col_shade(BUG_BELLY[3],0.8f));
        circle_fill(x+r+2, y-6-flap, 2, col_shade(BUG_BELLY[3],0.8f));
    }
    if (b->kind==2){ // spider abdomen bulging out the back
        circle_fill(x, y+r/2, r*3/4+2, OUTLINE);
        circle_shade(x, y+r/2, r*3/4, dark);
        circle_fill(x-3, y+r/2-2, 2, col); circle_fill(x+4, y+r/2+3, 2, col);
    }
    // body: outlined, sphere-shaded, glossed
    ellipse_fill(x, y, r+4, r*3/4+4, OUTLINE);
    ellipse_shade(x, y, r+2, r*3/4+2, col);
    gloss(x, y, r+2);
    // shell markings, per kind
    if (b->kind==0){ // roach: two darker bands across the shell
        line_thick(x-r*2/3, y-r/4, x+r*2/3, y-r/4, 2, dark);
        line_thick(x-r*3/4, y+r/6, x+r*3/4, y+r/6, 2, dark);
    } else if (b->kind==1){ // beetle: dorsal seam + rivet dots
        line_thick(x, y-r*2/3, x, y+r*2/3, 1, dark);
        circle_fill(x-r/3, y-r/5, 1, dark); circle_fill(x+r/3, y-r/5, 1, dark);
        circle_fill(x-r/3, y+r/4, 1, dark); circle_fill(x+r/3, y+r/4, 1, dark);
    } else if (b->kind==2){ // spider: warning chevrons
        line_thick(x-4, y-3, x, y+1, 1, COL(255,220,90));
        line_thick(x, y+1, x+4, y-3, 1, COL(255,220,90));
    }
    ellipse_shade(x, y+2, r*3/5, r/2, BUG_BELLY[b->kind]);
    // head + googly eyes that track the wobble
    int hx = x + (int)(fcos(b->wobble)*2), hy = y - r/3;
    circle_fill(hx, hy, r/2+3, OUTLINE);
    circle_shade(hx, hy, r/2+1, col);
    int look = (int)(fsin(G.anim_t*2 + b->seed)*1.5f);
    circle_fill(hx-r/4, hy-1, r/4+1, COL(255,255,255));
    circle_fill(hx+r/4, hy-1, r/4+1, COL(255,255,255));
    circle_fill(hx-r/4+look, hy-1, r/8+1, OUTLINE);
    circle_fill(hx+r/4+look, hy-1, r/8+1, OUTLINE);
    pset(hx-r/4+look-1, hy-2, COL(255,255,255));
    pset(hx+r/4+look-1, hy-2, COL(255,255,255));
    // antennae: jointed, with shaded tips
    line_thick(hx-3, hy-r/3, hx-5, hy-r/3-3, 1, OUTLINE);
    line_thick(hx-5, hy-r/3-3, hx-7, hy-r/3-6, 1, OUTLINE);
    line_thick(hx+3, hy-r/3, hx+5, hy-r/3-3, 1, OUTLINE);
    line_thick(hx+5, hy-r/3-3, hx+7, hy-r/3-6, 1, OUTLINE);
    circle_fill(hx-7, hy-r/3-6, 2, dark); circle_fill(hx+7, hy-r/3-6, 2, dark);
    gloss(hx, hy, r/2+1);   // shiny little head
    // a determined little mouth
    line_thick(hx-3, hy+r/4+2, hx, hy+r/4, 1, dark);
    line_thick(hx, hy+r/4, hx+3, hy+r/4+2, 1, dark);
    // hp bar on big bugs
    if (b->maxhp>=40 && b->hp<b->maxhp){
        int wpx = r*2;
        rect_fill(x-r, y-r-10, wpx, 3, COL(40,30,40));
        rect_fill(x-r, y-r-10, (int)(wpx*b->hp/b->maxhp), 3, COL(120,230,90));
    }
    if (b->bp_t>0){ // breakpoint: paused, with the universal symbol
        circle_out(x+r-2, y-r-4, 7, COL(70,140,255));
        rect_fill(x+r-5, y-r-8, 2, 7, COL(255,255,255));
        rect_fill(x+r-1, y-r-8, 2, 7, COL(255,255,255));
    }
}
// Hat shape: the stack grows UP to HAT_ROWS rows — then it stops getting
// taller and starts getting WIDER, spreading sideways row by row until it
// is four carcasses across. A wide hat sways much more: properly unwieldy.
#define HAT_ROWS 14
#define HAT_VIS_MAX MAXSTACK
// the hat grows UP to HAT_ROWS first — then it stops being polite and
// spreads SIDEWAYS, wider and wider, rows creeping up only slowly.
// At 420 carcasses it is a 14-wide, 30-row monument.
static void hat_shape_c(int n, int cap, int rowcap, int *rows, int *max_across){
    int vis = n>cap? cap : n;
    if (vis<=0){ if (rows) *rows=0; if (max_across) *max_across=0; return; }
    int r = vis<=HAT_ROWS? vis : HAT_ROWS + (vis-HAT_ROWS)/24;
    if (r>rowcap) r=rowcap;
    int base = vis/r, extra = vis%r;
    if (rows) *rows = r;
    if (max_across) *max_across = base + (extra? 1:0);
}
static void hat_shape(int n, int *rows, int *max_across){ hat_shape_c(n, HAT_VIS_MAX, 30, rows, max_across); }
static int hat_across_c(int n, int row, int cap, int rowcap){
    // carcasses across on a given row (row 0 = bottom); bottom rows widen first
    int vis = n>cap? cap : n;
    if (vis<=0) return 0;
    int r = vis<=HAT_ROWS? vis : HAT_ROWS + (vis-HAT_ROWS)/24;
    if (r>rowcap) r=rowcap;
    if (row>=r) return 0;
    int base = vis/r, extra = vis%r;
    return base + (row<extra? 1:0);
}
static int hat_across(int n, int row){ return hat_across_c(n, row, HAT_VIS_MAX, 30); }
static void draw_hat(Player *p, int x, int y){
    // the carcass stack on the player's head, growing through tier shapes
    int n = p->stack_n;
    if (n<=0) return;
    g_gold_carcass = (p->whale_on & 1) != 0;   // Whales Only: golden hat
    // (The Psychic Hat Stand that once floated the pile beside its
    // owner — bonus height and all — was removed with the Gallery
    // on Nov 2. The hat is worn, and shows what you carry.)
    int vis_n = n;
    int cap = HAT_VIS_MAX;
    int rowcap = 30;
    int hx2 = x, hy2 = y;
    if (0){
        ellipse_fill(hx2, hy2+1, 10, 3, COL(190,150,255));
        ellipse_fill(hx2, hy2, 7, 2, COL(230,210,255));
    }
    int rows, maxa; hat_shape_c(vis_n, cap, rowcap, &rows, &maxa);
    int wide = maxa>1;
    float sway_amp = 0.6f + (wide? (maxa-1)*0.55f : 0.0f);   // width = wobble
    // the hat lives in the world, so it shrinks with the camera zoom
    float z = G.zoom>0.01f? G.zoom : 1.0f;
    int stepx = (int)(10*z+0.5f); if (stepx<5) stepx=5;
    int stepy = (int)(7*z+0.5f);  if (stepy<4) stepy=4;
    int cs = (int)(6*z+0.5f);     if (cs<3) cs=3; if (maxa>10 && cs>5) cs=5;
    int idx = 0;
    int top_sway = 0;
    for (int i=0;i<rows;i++){
        int a = hat_across_c(vis_n, i, cap, rowcap);
        int cy = hy2 - 16 - i*stepy;
        // each row sways a little more than the one below; a wide stack
        // also leans slowly side to side like it's about to go over
        float sway = fsin(G.anim_t*2.2f + p->walk_ph + i*0.35f) * (sway_amp + i*0.30f);
        float lean = wide? fsin(G.anim_t*0.8f + p->walk_ph*0.5f) * (maxa-1) * ((float)i/rows) * 1.6f : 0.0f;
        int rowx = hx2 + (int)(sway + lean);
        if (i==rows-1) top_sway = (int)(sway+lean);
        for (int k=0;k<a;k++){
            int off = (int)((k - (a-1)*0.5f)*stepx);
            draw_carcass(rowx + off + ((idx%2)? 1 : -1), cy, cs,
                         (idx < p->stack_n? (int)(p->stack[idx]) : idx*7)%4, fsin(idx*2.3f));
            idx++;
        }
    }
    int top_y = hy2 - 16 - rows*stepy;
    if (vis_n>=19){ // CROWN: a golden bug reigns
        draw_carcass(hx2+top_sway, top_y-4, 7, 1, 0);
        tri_fill(hx2-6+top_sway, top_y-12, hx2+top_sway, top_y-18, hx2+6+top_sway, top_y-12, COL(255,205,40));
    }
    if (vis_n>=36){ // LEGACY SYSTEM: scaffolding + warning light, sized to the stack
        int hw = maxa*stepx/2 + 8;
        line_thick(hx2-hw, hy2-14, hx2-hw, top_y, 1, COL(120,120,130));
        line_thick(hx2+hw, hy2-14, hx2+hw, top_y, 1, COL(120,120,130));
        if (((int)(G.anim_t*2))%2) circle_fill(hx2+top_sway, top_y-14, 3, COL(255,60,60));
    }
    if (n>=4){ // weight label (the REAL weight, stand or no stand)
        char b[20]; fmt_int(b, "", (int)p->carried, "KG");
        draw_text_center(hx2, top_y - (vis_n>=19? 26:12), b, 1, COL(255,240,180));
    }
    g_gold_carcass = 0;
}
static const u32 HAIR_COL[MAXP] = { COL(96,64,40), COL(38,34,38), COL(158,116,62), COL(72,50,34) };
static const u32 SKIN = COL(245,205,160);
static int g_bud_monocle = 0;   // WHALES ONLY: this buddy's owner bought the monocle
static int g_bud_crown = 0;     // WHALES ONLY: ROYAL BUDDY
static int g_bud_space = 0;     // WHALES ONLY: SPACE BUDDY
// buddies wear the same comically big nerd glasses as their DEV
// (Daniel, Nov 2): pale sheen, chunky frames, wider than the face.
static void draw_bud_glasses(int cx, int cy, int lw, int lh){
    int gl = cx - lw - 1, gr = cx + 1, gy = cy - lh/2;
    blend_rect(gl, gy, lw, lh, COL(200,230,255), 60);
    blend_rect(gr, gy, lw, lh, COL(200,230,255), 60);
    for (int s2=0;s2<2;s2++){
        rect_line(gl-s2, gy-s2, lw+2*s2, lh+2*s2, OUTLINE);
        rect_line(gr-s2, gy-s2, lw+2*s2, lh+2*s2, OUTLINE);
    }
    rect_fill(gl+lw, cy-1, gr-(gl+lw), 2, OUTLINE);
    pset_blend(gl+2, gy+lh-2, COL(255,255,255), 170);
    pset_blend(gl+3, gy+lh-3, COL(255,255,255), 140);
    pset_blend(gr+2, gy+lh-2, COL(255,255,255), 170);
    pset_blend(gr+3, gy+lh-3, COL(255,255,255), 140);
    if (g_bud_space){   // WHALES ONLY: SPACE BUDDY — a bubble over the whole head
        int br = lw + 8;
        for (int a2=0;a2<28;a2++){
            float an = (float)a2 * (6.2831853f/28.0f);
            pset_blend(cx + (int)(fcos(an)*(float)br), cy - 2 + (int)(fsin(an)*(float)(br*3/4)), COL(200,235,255), 190);
        }
        line_thick(cx-br+4, cy-2-br/2, cx-br+9, cy-2-br/4, 1, COL(240,250,255));   // the shine
    }
    if (g_bud_crown){   // WHALES ONLY: ROYAL BUDDY — three gold points on a band
        int cw2 = lw + 4, cx0 = cx - cw2/2, cy0 = cy - lh - 5;
        rect_fill(cx0, cy0+4, cw2, 4, COL(232,184,60));
        tri_fill(cx0, cy0+4, cx0+5, cy0+4, cx0+2, cy0-3, COL(255,205,60));
        tri_fill(cx0+cw2/2-3, cy0+4, cx0+cw2/2+3, cy0+4, cx0+cw2/2, cy0-4, COL(255,205,60));
        tri_fill(cx0+cw2-5, cy0+4, cx0+cw2, cy0+4, cx0+cw2-2, cy0-3, COL(255,205,60));
        pset(cx0+cw2/2, cy0+5, COL(230,60,90));
    }
    if (g_bud_monocle){   // WHALES ONLY: THE FORBIDDEN MONOCLE, on a chain
        int mx2 = gr + lw/2, my2 = gy + lh/2;
        for (int a2=0;a2<20;a2++){
            float an = (float)a2 * (6.2831853f/20.0f);
            pset(mx2 + (int)(fcos(an)*(float)(lw/2+2)), my2 + (int)(fsin(an)*(float)(lh/2+2)), COL(232,184,60));
        }
        line_thick(mx2+lw/2, my2+lh/2, mx2+lw/2+3, gy+lh+7, 1, COL(232,184,60));
    }
}
static void draw_buddy(Buddy *b){
    int owner = (int)(b - G.bud);
    g_bud_monocle = (owner>=0 && owner<MAXP && (G.p[owner].whale_on & 16) && !b->fainted);
    g_bud_crown   = (owner>=0 && owner<MAXP && (G.p[owner].whale_on & 128) && !b->fainted);
    g_bud_space   = (owner>=0 && owner<MAXP && (G.p[owner].whale_on & 4096) && !b->fainted);
    int x=SX(b->x), y=SY(b->y);
    if (x<-50||x>VW+50||y<-60||y>VH+50) return;
    u32 collar = (owner>=0 && owner<MAXP)? HOODIE[G.p[owner].color&3] : COL(200,60,60);
    float fx = fcos(b->aim), fy = fsin(b->aim);
    int hx = (int)(fx*8), hy = (int)(fy*5);            // head leads toward facing
    int lx = (int)(fx * b->lunge_t * 34), ly = (int)(fy * b->lunge_t * 34);   // the lunge
    int flash = (b->hurt_t>0 && ((int)(G.anim_t*20))%2);
    shadow_ellipse(x, y+10, 12, 3);
    if (b->on_wall>=0) y-=8;          // up on top of an interlocked wall run, like his DEV
    if (b->fainted){
        // flat out, seeing stars — fainted, not gone. The Banzai
        // Buddies shop revives it; until then it naps right here.
        if (b->species==4){   // a fainted rock is a flat cracked slab
            rect_fill(x-13, y+2, 26, 5, COL(120,110,110));
            rect_fill(x-10, y-1, 19, 3, COL(120,110,110));
            rect_fill(x-5, y-3, 8, 2, COL(120,110,110));
            line_thick(x-7, y+1, x-1, y+5, 1, OUTLINE);
            line_thick(x-1, y+5, x+5, y+2, 1, OUTLINE);
            line_thick(x+2, y+2, x+4, y+7, 1, OUTLINE);
        } else {
            ellipse_fill(x, y+4, 13, 6, COL(120,110,110));
            circle_fill(x-8, y+2, 5, COL(120,110,110));
        }
        for (int s=0;s<3;s++){
            float a = G.anim_t*2.4f + s*(TAU/3);
            disc_fill(x + (int)(fcos(a)*10), y-8 + (int)(fsin(a)*3), 1, COL(255,230,120));
        }
        draw_text_center(x, y-18, "FAINTED", 1, COL(255,150,150));
        return;
    }
    int bob = (int)(fsin(G.anim_t*3.0f + owner*1.7f)*1.0f);          // breathing
    int step = (int)(fsin(b->walk_ph)*3);
    // THE WALK (Daniel, Nov 6): no wiggle. While actually moving,
    // the body F L O P S — a rigid square-wave lean locked to the
    // same phase that steps the feet: the head tips the full flop,
    // the body half of it, the shadow stays planted. Breathing
    // pauses mid-stride. The pet rock is exempt: it already flops
    // end over end on its own roll.
    int walking = (b->mv_t>0 && b->species!=4);
    int wflop = walking? ((fsin(b->walk_ph)>=0)? 3 : -3) : 0;
    if (walking){ bob = 0; hx += wflop; lx += wflop/2; }
    if (b->species==0){          // PUPPY: tan, floppy ears, eternal tail
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(199,152,94));
        u32 dark = col_shade(body, 0.7f);
        // tail: wagging is a full-time job
        float wag = fsin(G.anim_t*11.0f + owner)*0.6f;
        int tx2 = x - hx - (int)(fx*6), ty2 = y - hy + 2;
        line_thick(tx2, ty2, tx2 + (int)(-fx*7 - fy*wag*7), ty2 + (int)(-fy*7 + fx*wag*7) - 3, 2, dark);
        ellipse_shade(x+lx, y+ly+bob, 12, 9, body);
        for (int lg=0;lg<4;lg++){   // trotters
            int ox = (lg%2? 6:-6), oy = (lg<2? 4:8);
            disc_fill(x+lx+ox, y+ly+oy + ((lg+ (step>0?0:1))%2? 0:1), 2, dark);
        }
        rect_fill(x+lx-5, y+ly-3+bob, 10, 4, collar);                // collar band
        int hxx = x+lx+hx, hyy = y+ly+hy+bob;
        circle_fill(hxx, hyy, 8, OUTLINE); circle_shade(hxx, hyy, 7, body); gloss(hxx, hyy, 7);
        // floppy ears, mid-flop (at a walk they ride the head rigidly)
        int flop = walking? 0 : (int)(fsin(b->walk_ph*1.4f)*1.5f);
        ellipse_fill(hxx-6, hyy-3+flop, 3, 5, dark);
        ellipse_fill(hxx+6, hyy-3-flop, 3, 5, dark);
        circle_fill(hxx + (int)(fx*5), hyy + (int)(fy*3) + 2, 2, COL(60,40,36));   // snout nose
        circle_fill(hxx-3, hyy-2, 1, OUTLINE); circle_fill(hxx+3, hyy-2, 1, OUTLINE);
        draw_bud_glasses(hxx + (int)(fx*3), hyy-2, 8, 8);
        if (b->lunge_t>0){   // jaw snap
            line_thick(hxx + (int)(fx*6), hyy+2, hxx + (int)(fx*10), hyy+4, 1, OUTLINE);
            line_thick(hxx + (int)(fx*6), hyy+4, hxx + (int)(fx*10), hyy+2, 1, OUTLINE);
        }
    } else if (b->species==1){   // KITTY: orange, striped, unbothered
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(228,152,72));
        u32 dark = col_shade(body, 0.72f);
        // tail: a slow question mark
        float curl = fsin(G.anim_t*2.0f + owner*2.0f)*3.0f;
        int tx2 = x - hx - (int)(fx*7), ty2 = y - hy + 3;
        line_thick(tx2, ty2, tx2 - (int)(fx*6), ty2 - 5, 2, dark);
        line_thick(tx2 - (int)(fx*6), ty2 - 5, tx2 - (int)(fx*6) + (int)curl, ty2 - 10, 2, dark);
        ellipse_shade(x+lx, y+ly+bob, 11, 8, body);
        line_thick(x+lx-6, y+ly-4+bob, x+lx-4, y+ly+5+bob, 1, dark);   // tabby stripes
        line_thick(x+lx+1, y+ly-5+bob, x+lx+3, y+ly+5+bob, 1, dark);
        for (int lg=0;lg<4;lg++){
            int ox = (lg%2? 5:-5), oy = (lg<2? 3:7);
            disc_fill(x+lx+ox, y+ly+oy + ((lg + (step>0?0:1))%2? 0:1), 2, dark);
        }
        rect_fill(x+lx-5, y+ly-3+bob, 10, 4, collar);
        int hxx = x+lx+hx, hyy = y+ly+hy+bob;
        circle_fill(hxx, hyy, 7, OUTLINE); circle_shade(hxx, hyy, 6, body); gloss(hxx, hyy, 6);
        tri_fill(hxx-6, hyy-4, hxx-4, hyy-10, hxx-1, hyy-5, dark);     // ears: triangles, obviously
        tri_fill(hxx+6, hyy-4, hxx+4, hyy-10, hxx+1, hyy-5, dark);
        circle_fill(hxx + (int)(fx*4), hyy + (int)(fy*2) + 1, 1, COL(200,90,90));
        circle_fill(hxx-2, hyy-1, 1, OUTLINE); circle_fill(hxx+2, hyy-1, 1, OUTLINE);
        draw_bud_glasses(hxx + (int)(fx*2), hyy-1, 7, 7);
        line_thick(hxx-8, hyy+1, hxx-4, hyy+1, 1, dark);               // whiskers
        line_thick(hxx+4, hyy+1, hxx+8, hyy+1, 1, dark);
        if (b->lunge_t>0){   // paw swipe, alternating
            int sw = ((int)(b->walk_ph*2))%2? 1 : -1;
            int sxp = hxx + (int)(fx*8) - (int)(fy*sw*5), syp = hyy + (int)(fy*8) + (int)(fx*sw*5);
            line_thick(sxp-3, syp-3, sxp+3, syp+3, 1, COL(255,255,255));
            line_thick(sxp-3, syp+1, sxp+3, syp-3, 1, COL(255,255,255));
        }
    } else if (b->species==2){   // MOOSE: a walking wall with antlers
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(143,106,68));
        u32 dark = col_shade(body, 0.7f);
        int sway = walking? 0 : (int)(fsin(G.anim_t*1.1f + owner)*1.2f);   // the great sway (idle only — walking is the flop)
        ellipse_shade(x+lx, y+ly+bob, 15, 11, body);
        for (int lg=0;lg<4;lg++){   // sturdy legs
            int ox = (lg%2? 8:-8), oy = (lg<2? 5:10);
            rect_fill(x+lx+ox-2, y+ly+oy-2, 4, 5, dark);
        }
        rect_fill(x+lx-6, y+ly-4+bob, 12, 4, collar);
        int hxx = x+lx+hx+sway, hyy = y+ly+hy-2+bob;
        ellipse_fill(hxx, hyy, 9, 7, OUTLINE); ellipse_shade(hxx, hyy, 8, 6, body);
        ellipse_fill(hxx + (int)(fx*6), hyy + (int)(fy*3) + 2, 5, 3, dark);    // the great snout
        circle_fill(hxx-2, hyy-1, 1, OUTLINE); circle_fill(hxx+3, hyy-1, 1, OUTLINE);
        draw_bud_glasses(hxx + (int)(fx*3), hyy-1, 9, 7);
        // antlers: two racks of pure punctuation
        u32 ant = COL(226,204,160);
        for (int s=-1;s<=1;s+=2){
            int ax = hxx + s*7, ay = hyy - 5;
            line_thick(ax, ay, ax + s*5, ay - 6, 2, ant);
            line_thick(ax + s*2, ay - 3, ax + s*2, ay - 8, 1, ant);
            line_thick(ax + s*4, ay - 5, ax + s*7, ay - 9, 1, ant);
        }
        if (b->lunge_t>0){   // the head swing behind every swipe
            int ax = hxx + (int)(fx*8), ay = hyy + (int)(fy*6);
            line_thick(ax-6, ay, ax+6, ay, 2, ant);
        }
    }
    else if (b->species==3){   // PLATYPUS: brown, billed, gloriously odd
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(128,92,58));
        u32 dark = col_shade(body, 0.7f);
        int sway = walking? 0 : (int)(fsin(G.anim_t*4.0f + owner*1.3f)*1.0f);   // a swimmy wiggle (idle only — walking is the flop)
        // the flat paddle tail, behind everything
        int tx2 = x - hx - (int)(fx*9), ty2 = y - hy + 3;
        ellipse_fill(tx2, ty2, 8, 4, dark); ellipse_fill(tx2, ty2, 7, 3, body);
        ellipse_shade(x+lx, y+ly+bob+sway/2, 12, 8, body);
        for (int lg=0;lg<4;lg++){   // webbed feet, roughly
            int ox = (lg%2? 6:-6), oy = (lg<2? 4:8);
            disc_fill(x+lx+ox, y+ly+oy + ((lg + (step>0?0:1))%2? 0:1), 2, dark);
        }
        rect_fill(x+lx-5, y+ly-3+bob, 10, 4, collar);
        int hxx = x+lx+hx, hyy = y+ly+hy+bob;
        circle_fill(hxx, hyy, 7, OUTLINE); circle_shade(hxx, hyy, 6, body); gloss(hxx, hyy, 6);
        // THE bill: wide, flat, faintly smug
        int bx2 = hxx + (int)(fx*7), by2 = hyy + (int)(fy*3) + 2;
        ellipse_fill(bx2, by2, 7, 3, OUTLINE); ellipse_fill(bx2, by2, 6, 2, COL(216,186,128));
        circle_fill(hxx-3, hyy-2, 1, OUTLINE); circle_fill(hxx+3, hyy-2, 1, OUTLINE);
        draw_bud_glasses(hxx + (int)(fx*2), hyy-2, 8, 7);
        if (b->lunge_t>0){   // the spur flashes
            int sxp = x+lx - (int)(fx*10), syp = y+ly + 6;
            line_thick(sxp, syp, sxp - (int)(fx*5), syp+3, 1, COL(190,255,200));
        }
    } else if (b->species==4){   // PET ROCK: a flat-bottomed crag
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(148,144,138));
        u32 dark = col_shade(body, 0.7f);
        // Daniel's spec (Nov 2): flat bottom, jagged hilly top, and
        // it FLOPS over as it travels — the slab tips end over end,
        // its peaks shearing across the base, with a little hop at
        // each flop. tip runs -1..1 over the roll cycle.
        float ph = b->roll; ph -= (int)(ph/6.28318f)*6.28318f; if (ph<0) ph+=6.28318f;
        float tip = fsin(ph);
        int lift = -(int)((tip>0? tip : -tip)*3.0f);
        int cx = x+lx + (int)(tip*2.0f);
        int base = y+ly+bob+lift + 9;                 // the flat bottom line
        int shear = (int)(tip*5.0f);                  // top leans into the flop
        // the silhouette in horizontal slices, bottom to top: full
        // width at the flat base, stepping in jaggedly to two humps
        static const int PROF[9] = { 15, 15, 14, 13, 12, 12, 9, 8, 5 };
        for (int r2=0;r2<9;r2++){
            int hw = PROF[r2];
            int off = (shear * r2) / 8;               // upper slices lean most
            int yy = base - r2*2;
            rect_fill(cx - hw + off, yy-1, hw*2, 2, r2==0? dark : body);
            if (r2==6) rect_fill(cx - hw + off - 3, yy-1, 4, 2, body);   // the left shoulder juts
            if (r2==8) rect_fill(cx + off + 1, yy-3, 5, 2, body);        // the tall peak
        }
        // speckles riding the slab as it flops
        for (int s2=0;s2<5;s2++){
            float a = b->roll + s2*2.4f + (float)(s2%3)*0.7f;
            int sx2 = cx + (int)(fcos(a)*9.0f) + (shear * (s2%3)) / 2;
            int sy2 = base - 3 - (s2%4)*3;
            disc_fill(sx2, sy2, 1, dark);
        }
        // googly eyes up on the ridge, glasses over them, both
        // shearing with the flop
        int ey = base - 15 + (int)(tip*1.0f), exx = (int)(fx*2), eoff = (shear * 7) / 8;
        for (int s2=-1;s2<=1;s2+=2){
            circle_fill(cx+eoff+s2*5, ey, 3, OUTLINE);
            circle_fill(cx+eoff+s2*5, ey, 2, COL(246,246,246));
            disc_fill(cx+eoff+s2*5+exx, ey + (int)(fy*1), 1, OUTLINE);
        }
        draw_bud_glasses(cx+eoff, ey, 9, 8);
        line_thick(cx-2, base-3, cx+2, base-3, 1, dark);   // a determined mouth, low on the slab
        if (b->steady_t>0) circle_out(cx, base-8, 19, COL(255,214,64));   // ROCK STEADY ring
        if (b->lunge_t>0) circle_out(cx, base-8, 17, COL(230,226,218));
    } else if (b->species==5){   // KITCHEN SINK: porcelain, plumbed, present
        u32 body = flash? COL(255,255,255) : bud_coat(b, COL(238,240,242));
        u32 dark = col_shade(body, 0.8f);
        u32 chrome = COL(168,174,182);
        int hop = walking? 0 : (int)(fsin(b->walk_ph*1.4f)*1.2f);   // no hopping at a walk — the whole sink tips instead
        // stubby pipe legs
        rect_fill(x+lx-9, y+ly+8, 4, 5, chrome); rect_fill(x+lx+5, y+ly+8, 4, 5, chrome);
        // the basin
        rect_fill(x+lx-14, y+ly-1+bob+hop, 28, 11, body); rect_line(x+lx-14, y+ly-1+bob+hop, 28, 11, dark);
        ellipse_fill(x+lx, y+ly-2+bob+hop, 14, 5, body); ellipse_fill(x+lx, y+ly-2+bob+hop, 13, 4, dark);
        // water, sloshing as it goes
        int slosh = walking? wflop : (int)(fsin(b->walk_ph*1.6f)*2.0f);   // at a walk the water tips rigidly with the basin
        ellipse_fill(x+lx+slosh, y+ly-2+bob+hop, 10, 3, COL(126,192,242));
        disc_fill(x+lx+slosh+3, y+ly-3+bob+hop, 1, COL(220,245,255));
        // backsplash with twin faucets and cross handles
        rect_fill(x+lx-11, y+ly-11+bob+hop, 22, 6, body); rect_line(x+lx-11, y+ly-11+bob+hop, 22, 6, dark);
        for (int s2=-1;s2<=1;s2+=2){
            rect_fill(x+lx+s2*6-1, y+ly-14+bob+hop, 2, 4, chrome);
            line_thick(x+lx+s2*6-3, y+ly-14+bob+hop, x+lx+s2*6+3, y+ly-14+bob+hop, 1, chrome);
            line_thick(x+lx+s2*6, y+ly-16+bob+hop, x+lx+s2*6, y+ly-12+bob+hop, 1, chrome);
        }
        // centre spout + the occasional drip of destiny
        rect_fill(x+lx-1, y+ly-13+bob+hop, 2, 3, chrome);
        if (((int)(G.anim_t*0.9f + owner))%3==0) disc_fill(x+lx, y+ly-6+bob+hop, 1, COL(126,192,242));
        // a face on the porcelain front, drain for a nose
        circle_fill(x+lx-5, y+ly+3+bob+hop, 1, OUTLINE); circle_fill(x+lx+5, y+ly+3+bob+hop, 1, OUTLINE);
        circle_fill(x+lx, y+ly+5+bob+hop, 2, dark);
        draw_bud_glasses(x+lx, y+ly+3+bob+hop, 8, 7);
        if (b->lunge_t>0){   // it splashes toward the fight
            for (int s2=0;s2<3;s2++)
                disc_fill(x+lx + (int)(fx*14) + s2*2 - 2, y+ly + (int)(fy*10) + (s2%2)*3 - 1, 1, COL(126,192,242));
        }
    }
    draw_text_center(x, y+16, BUD_ELF[b->name_idx%BUD_NAMES_N], 1, COL(255,244,210));
    if (b->hp < b->maxhp){
        float frac = b->maxhp>0? b->hp/b->maxhp : 0; if (frac<0) frac=0;
        rect_fill(x-12, y-24, 24, 3, COL(40,30,40));
        rect_fill(x-12, y-24, (int)(24*frac), 3, frac>0.35f? COL(120,230,90) : COL(255,170,60));
    }
    if (b->energy < BUD_ENERGY_MAX - 0.5f){   // the blue thread under the health
        float ef = b->energy/BUD_ENERGY_MAX; if (ef<0) ef=0;
        rect_fill(x-12, y-20, 24, 2, COL(40,30,40));
        rect_fill(x-12, y-20, (int)(24*ef), 2, COL(120,200,255));
    }
}

static void draw_player_sprite(Player *p){
    { int pi = (int)(p - G.p);   // WHALES ONLY: PARADE DUCK, drawn at its own spot
      if (pi>=0 && pi<MAXP && duck_on[pi] && p->alive){
        int dx2 = SX(duck_x[pi]), dy2 = SY(duck_y[pi]);
        if (dx2>-30 && dx2<VW+30 && dy2>-30 && dy2<VH+30){
            float dhop = fsin(duck_ph[pi]); if (dhop<0) dhop = -dhop;
            int hop = (int)(dhop*3.0f), fc = duck_face[pi]? duck_face[pi] : 1;
            shadow_ellipse(dx2, dy2+6, 8, 2);
            ellipse_fill(dx2, dy2-hop, 9, 6, COL(250,215,80));
            tri_fill(dx2-fc*7, dy2-2-hop, dx2-fc*12, dy2-5-hop, dx2-fc*7, dy2+1-hop, COL(240,200,70));
            disc_fill(dx2+fc*7, dy2-5-hop, 4, COL(250,220,95));
            tri_fill(dx2+fc*10, dy2-1-hop, dx2+fc*15, dy2+1-hop, dx2+fc*10, dy2+3-hop, COL(240,130,40));
            pset(dx2+fc*8, dy2-6-hop, COL(30,26,44));
        }
      } }
    int x=SX(p->x), y=SY(p->y);
    if (x<-90||x>VW+90||y<-190||y>VH+40) return;
    if ((p->whale_on & 2048) && p->alive){   // WHALES ONLY: AURA OF IMPORTANCE
        int pr = 17 + (int)(fsin(G.anim_t*3.0f + (float)(p->color&3))*2);
        ellipse_fill(x, y+11, pr, 5, COL(240,196,80));
        ellipse_fill(x, y+11, pr-5, 3, COL(255,226,130));
    }
    // bean-borne: dropping out of the sky onto the plaza — the body
    // rides high and settles; the shadow stays on the ground
    int bean_lift=0;
    if (p->bean_t>0){ float e=1.6f-p->bean_t;
        if (e>=0.45f && e<1.05f){ float k=(e-0.45f)/0.6f; bean_lift=(int)((1.0f-k)*(1.0f-k)*90.0f); } }
    if (p->bean_jet>0) bean_lift = 13 + (int)(fsin(G.anim_t*7.0f)*2.0f);   // the jetpack hover
    else if (p->jet_fall>0) bean_lift = (int)(p->jet_fall*30.0f);          // tumbling back down
    shadow_ellipse(x, y+11, 13, 4);
    if (bean_lift>0) y-=bean_lift;
    if (p->on_wall>=0) y-=8;          // up on top of an interlocked wall run
    if ((p->whale_on & 4) && p->alive){   // WHALES ONLY: the ROYAL CAPE, worn behind everything
        int bx2 = x - (int)(fcos(p->aim)*9), by2 = y + 2 - (int)(fsin(p->aim)*5);
        int sway = (int)(fsin(G.anim_t*2.4f + (float)(p->color&3))*3);
        tri_fill(x-7, y-10, x+7, y-10, bx2-11+sway, by2+22, COL(150,30,44));
        tri_fill(x+7, y-10, bx2+11+sway, by2+22, bx2-11+sway, by2+22, COL(150,30,44));
        tri_fill(x-7, y-10, bx2-11+sway, by2+22, bx2, by2+16, COL(120,22,36));
        disc_fill(x, y-10, 2, COL(255,214,90));   // the gold clasp
    }
    int ibob=0, iblink=0;             // idle breathing/blink (set in the body draw)
    if (!p->alive){
        draw_carcass(x, y+4, 9, 0, 0.4f);                          // knocked out
        draw_text_center(x, y-18, "DOWN", 1, COL(255,120,120));
        return;
    }
    if (p->hurt_t>0 && ((int)(G.anim_t*20))%2) { /* flash: skip body this blink */ }
    else {
        u32 hood = HOODIE[p->color&3];
        u32 hair = HAIR_COL[p->color&3];
        // the face leans toward the facing direction
        int fx = (int)(fcos(p->aim)*3), fy = (int)(fsin(p->aim)*2);
        // idle life: standing still, the DEV breathes (a slow bob that
        // carries the head and the whole hat with it), blinks every few
        // seconds, and after a while starts glancing around for bugs
        if (p->idle_t>0.25f){
            ibob = (int)(fsin(G.anim_t*2.1f + (float)(p->color&3)*1.7f)*1.2f);
            float ph = G.anim_t + (float)(p->color&3)*1.31f; ph -= (int)(ph/3.4f)*3.4f;
            iblink = (ph < 0.11f);
            if (p->idle_t>1.5f) fx += (int)(fsin(G.anim_t*0.55f + (float)(p->color&3)*2.3f)*2.0f);
        }
        // legs and animated feet: alternating stride along the facing
        // direction, and each shoe lifts as it swings forward
        int lp = (int)(fsin(p->walk_ph)*4);
        float sax = fcos(p->aim), say = fsin(p->aim);
        float swl = fsin(p->walk_ph), swr = fsin(p->walk_ph + PI);
        float cl = fcos(p->walk_ph), cr = fcos(p->walk_ph + PI);
        int lfx = x-5 + (int)(sax*swl*4), lfy = y+11 + (int)(say*swl*3) - (cl>0? (int)(cl*2):0);
        int rfx = x+4 + (int)(sax*swr*4), rfy = y+11 + (int)(say*swr*3) - (cr>0? (int)(cr*2):0);
        line_thick(x-5, y+3, lfx, lfy-1, 5, COL(64,64,104));   // jeans down to each shoe
        line_thick(x+4, y+3, rfx, rfy-1, 5, COL(64,64,104));
        ellipse_fill(lfx, lfy, 5, 3, COL(42,36,46));           // shoes
        ellipse_fill(rfx, rfy, 5, 3, COL(42,36,46));
        line_thick(lfx-4, lfy+3, lfx+4, lfy+3, 1, COL(205,200,210));  // soles
        line_thick(rfx-4, rfy+3, rfx+4, rfy+3, 1, COL(205,200,210));
        // torso: shaded hoodie, pocket, zipper
        ellipse_fill(x, y, 13, 14, OUTLINE);
        ellipse_shade(x, y, 11, 12, hood);
        rect_fill(x-6, y+5, 12, 4, col_shade(hood, 0.8f));
        line_thick(x, y-10, x, y+4, 1, col_shade(hood, 0.68f));
        // hoodie drawstrings with little aglets
        line_thick(x-3, y-8, x-3, y-3, 1, COL(235,235,240));
        line_thick(x+3, y-8, x+3, y-3, 1, COL(235,235,240));
        circle_fill(x-3, y-2, 1, COL(235,235,240));
        circle_fill(x+3, y-2, 1, COL(235,235,240));
        // free arm, swinging against the stride
        line_thick(x-9, y-3, x-12, y+3+(lp>0?-1:1), 3, col_shade(hood, 0.9f));
        circle_fill(x-12, y+4+(lp>0?-1:1), 2, SKIN);
        // ---- the person: neck, ears, shaded head, hair, and the glasses
        // (drawn before the aim arm so the weapon stays on top)
        rect_fill(x-3, y-11+ibob, 6, 4, col_shade(SKIN, 0.94f));
        circle_fill(x-8, y-14+ibob, 2, SKIN);
        circle_fill(x+8, y-14+ibob, 2, SKIN);
        circle_fill(x, y-15+ibob, 9, OUTLINE);
        circle_shade(x, y-15+ibob, 8, SKIN);
        gloss(x, y-15+ibob, 8);
        // hair: a cap swept across the top, thinning at the temples
        ellipse_fill(x-fx/2, y-20+ibob, 8, 4, hair);
        rect_fill(x-8-fx/2, y-20+ibob, 3, 5, hair);
        rect_fill(x+5-fx/2, y-20+ibob, 3, 4, hair);
        // COMICALLY BIG NERD GLASSES: the lenses are wider than the head.
        // Chunky triple frames, pale lenses with a glint, eyes behind them.
        // The whole face rides a few px toward the facing direction.
        int gw2 = (p->whale_on & 2)? 19 : 13, gh2 = (p->whale_on & 2)? 16 : 12;   // WHALES ONLY: ENORMOUS-ER
        int gy = y-22+fy+ibob - (gh2-12)/2;
        int gl = x-16+fx - (gw2-13), gr = x+3+fx;       // lens box origins, pulled apart so the two lenses read as separate glasses
        line_thick(gl+1, gy+4, x-9, y-14+ibob, 2, OUTLINE);  // temple arms
        line_thick(gr+gw2-1, gy+4, x+9, y-14+ibob, 2, OUTLINE);
        blend_rect(gl, gy, gw2, gh2, COL(200,230,255), 60); // lens sheen
        blend_rect(gr, gy, gw2, gh2, COL(200,230,255), 60);
        for (int s=0;s<3;s++){                          // triple-drawn frames
            rect_line(gl-s, gy-s, gw2+2*s, gh2+2*s, OUTLINE);
            rect_line(gr-s, gy-s, gw2+2*s, gh2+2*s, OUTLINE);
        }
        rect_fill(gl+gw2, gy+3, gr-(gl+gw2), 2, OUTLINE); // slim bridge, clear space between the lenses
        // diagonal glints on each lens
        pset_blend(gl+3, gy+8, COL(255,255,255), 170); pset_blend(gl+4, gy+7, COL(255,255,255), 170);
        pset_blend(gl+5, gy+6, COL(255,255,255), 170); pset_blend(gl+6, gy+5, COL(255,255,255), 140);
        pset_blend(gr+3, gy+8, COL(255,255,255), 170); pset_blend(gr+4, gy+7, COL(255,255,255), 170);
        pset_blend(gr+5, gy+6, COL(255,255,255), 170); pset_blend(gr+6, gy+5, COL(255,255,255), 140);
        if (iblink){                                    // closed for a blink
            line_thick(x-9+fx, gy+6, x-5+fx, gy+6, 1, OUTLINE);
            line_thick(x+5+fx, gy+6, x+9+fx, gy+6, 1, OUTLINE);
        } else {
            circle_fill(x-7+fx, gy+6, 2, OUTLINE);          // eyes, magnified by the lenses
            circle_fill(x+7+fx, gy+6, 2, OUTLINE);
            pset(x-8+fx, gy+5, COL(255,255,255));
            pset(x+6+fx, gy+5, COL(255,255,255));
        }
        line_thick(x-11+fx, gy-3, x-5+fx, gy-3, 1, hair);  // brows peek over the top
        line_thick(x+5+fx, gy-3, x+11+fx, gy-3, 1, hair);
        line_thick(x-2+fx, y-8+fy+ibob, x, y-7+fy+ibob, 1, COL(196,128,104));  // smile below the frames
        line_thick(x, y-7+fy+ibob, x+2+fx, y-8+fy+ibob, 1, COL(196,128,104));
        // aim arm: hoodie sleeve to the elbow, skin to the grip
        int hx = x + (int)(fcos(p->aim)*13), hy = y-4 + (int)(fsin(p->aim)*10);
        int ex = (x+hx)/2, ey = (y-3+hy)/2;
        line_thick(x, y-3, ex, ey, 4, col_shade(hood, 0.92f));
        line_thick(ex, ey, hx, hy, 3, SKIN);
        int w = p->weapon;
        if (w==0){ rect_fill(hx-6, hy-3, 13, 7, OUTLINE); rect_fill(hx-5, hy-2, 11, 5, COL(200,200,215));
                   for (int k=0;k<3;k++) rect_fill(hx-3+k*4, hy-1, 2, 2, COL(90,90,120)); }
        else if (w==1){ ellipse_out(hx, hy, 5, 4, COL(170,170,190)); circle_fill(hx+1, hy-1, 1, COL(240,80,80)); }
        else if (w==2){ circle_out(hx, hy, 5, COL(255,215,60)); circle_fill(hx+4, hy-2, 2, COL(255,150,40)); }
        else if (w==3){ rect_fill(hx-4, hy-5, 10, 8, COL(120,80,40)); rect_fill(hx-4, hy-5, 10, 3, COL(90,60,30));
                        draw_text_plain(hx-3, hy-4, "PM", 1, COL(255,230,150)); }
        else { rect_fill(hx-5, hy-2, 11, 5, COL(60,60,80)); circle_fill(hx+6, hy, 2, COL(255,60,60)); }
        if (w==4 && beam_vis[p-G.p]>0){
            int nb = beam_n[p-G.p]; if (nb<1) nb=1;
            float bf = beam_frac[p-G.p]; if (bf<=0) bf=1;
            int bl = (int)(beam_len[p-G.p] * bf); if (bl<2) bl=2;
            for (int k=0;k<nb;k++){
                float ba = beam_ang[p-G.p][k];
                int bx = x + (int)(fcos(ba)*bl), by = y-4 + (int)(fsin(ba)*bl);
                line_thick(x, y-4, bx, by, 3, COL(120,200,255));
                line_thick(x, y-4, bx, by, 1, COL(230,250,255));
                circle_fill(bx, by, 4, COL(160,220,255));
            }
        }
        // shield bubble: a ring of firewall sparks orbiting the player
        if (p->shield>0){
            for (int a=0;a<14;a++){
                float t = G.anim_t*0.9f + a*(TAU/14);
                int px2 = x+(int)(fcos(t)*19), py2 = y-6+(int)(fsin(t)*22);
                pset(px2, py2, COL(170,220,255)); pset(px2+1, py2, COL(120,190,255));
            }
        }
        // rollback grace: a green time-ring while you're untouchable
        if (p->revive_t>0){
            for (int a=0;a<14;a++){
                float t = -G.anim_t*2.4f + a*(TAU/14);
                int px2 = x+(int)(fcos(t)*20), py2 = y-6+(int)(fsin(t)*23);
                disc_fill(px2, py2, 2, COL(120,255,160));
            }
        }
    }
    draw_hat(p, x, y + ibob);
    // name tag
    draw_text_center(x, y+18, p->name, 1, COL(255,255,255));
    if (p->whale_on & 64)   // WHALES ONLY: the title, floating in gold
        draw_text_center(x, y+29, "~ WHALE ~", 1, COL(255,205,40));
    if (p->whale_on & 32){  // WHALES ONLY: DRAMATIC RAIN — on this player, only this player
        int fall = (int)(G.anim_t*95);
        for (int i2=0;i2<14;i2++){
            int rx = x - 17 + (int)(((unsigned)(i2*2654435761u)>>13) % 35u);
            int ry = y - 36 + (i2*29 + fall) % 48;
            line_thick(rx, ry, rx+2, ry+7, 1, COL(150,200,255));
        }
    }
    // hp pips over head when hurt
    if (p->hp < p->maxhp){
        rect_fill(x-11, y-26, 22, 3, COL(40,30,40));
        rect_fill(x-11, y-26, 22*p->hp/p->maxhp, 3, COL(120,230,90));
    }
    // THE BACKFIRE BEAN, riding the shoulder through his own ride:
    // goggles down, cheeks puffed, entirely pleased with himself.
    if (p->bean_t>0 && p->alive){
        float e = 1.6f - p->bean_t;
        int jx = (e<0.45f && ((int)(G.anim_t*40))%2)? 1:0;   // the sputter shake
        int bx2 = x+11+jx, by2 = y-8+jx;
        ellipse_fill(bx2, by2+6, 6, 3, COL(240,248,230));        // his little cloud seat
        ellipse_fill(bx2, by2, 6, 7, COL(126,196,86));            // bean body
        ellipse_fill(bx2-2, by2-2, 2, 3, COL(168,226,120));      // the shine
        rect_fill(bx2-6, by2-6, 12, 3, COL(90,70,50));           // goggle strap
        disc_fill(bx2-3, by2-4, 2, COL(220,240,255));            // goggles
        disc_fill(bx2+3, by2-4, 2, COL(220,240,255));
        pset(bx2-3, by2-4, COL(30,26,44)); pset(bx2+3, by2-4, COL(30,26,44));
        if (e>=0.45f && e<1.05f){   // mid-flight: the exhaust does the talking
            disc_fill(bx2, by2+11, 4, COL(220,240,200));
            disc_fill(bx2-3, by2+15, 3, COL(190,225,150));
            disc_fill(bx2+3, by2+18, 2, COL(168,216,120));
        }
        if (e>=1.05f){   // touchdown: stars, his and yours alike
            int s1 = (int)(fsin(G.anim_t*5.0f)*8), s2 = (int)(fsin(G.anim_t*5.0f+2.0f)*8);
            rect_fill(x+s1-1, y-27, 3, 1, COL(255,220,90)); rect_fill(x+s1, y-28, 1, 3, COL(255,220,90));
            rect_fill(x-s2-1, y-23, 3, 1, COL(255,220,90)); rect_fill(x-s2, y-24, 1, 3, COL(255,220,90));
        }
    }
}
static void draw_store_building(void){
    int x0=SX(-84), y0=SY(-108), x1=SX(84), y1=SY(-38);
    if (x1<-48||x0>VW+48||y1<-48||y0>VH+48) return;
    int cx=(x0+x1)/2, w=x1-x0;
    u32 wood = COL(122,80,44), wood_d = COL(94,60,32), plank = COL(238,220,180);
    shadow_ellipse(cx, y1+3, w/2, 6);
    // back wall: warm planks with seams
    rect_fill(x0, y0+18, w, y1-y0-18, plank);
    for (int py=y0+27; py<y1-4; py+=9) rect_fill(x0+2, py, w-4, 1, COL(216,194,150));
    rect_fill(x0, y1-6, w, 6, COL(226,204,162));
    // a little lit window high on the left wall, clear of the board
    rect_fill(x0+9, y0+27, 20, 14, wood_d); rect_line(x0+9, y0+27, 20, 14, OUTLINE);
    rect_fill(x0+12, y0+30, 14, 8, COL(255,214,120));
    rect_fill(x0+18, y0+30, 2, 8, wood_d); rect_fill(x0+12, y0+33, 14, 2, wood_d);
    // corner posts + top beam (the stall frame)
    rect_fill(x0-2, y0+14, 7, y1-y0-6, wood);  rect_line(x0-2, y0+14, 7, y1-y0-6, OUTLINE);
    rect_fill(x1-5, y0+14, 7, y1-y0-6, wood);  rect_line(x1-5, y0+14, 7, y1-y0-6, OUTLINE);
    rect_fill(x0-3, y0+16, w+6, 8, wood);      rect_line(x0-3, y0+16, w+6, 8, OUTLINE);
    // lanterns hanging off the posts
    for (int s=0;s<2;s++){
        int lx = s? x1-10 : x0+10;
        disc_fill(lx, y0+33, 6, COL(255,196,96));
        rect_fill(lx-3, y0+28, 7, 9, wood_d); rect_line(lx-3, y0+28, 7, 9, OUTLINE);
        disc_fill(lx, y0+32, 2, COL(255,240,190));
    }
    // striped awning with a scalloped hem, over the beam
    for (int i=0;i<8;i++){
        int ax = x0 + i*w/8; u32 c = (i%2)? COL(220,70,70) : COL(245,245,245);
        rect_fill(ax, y0+4, w/8, 13, c);
        disc_fill(ax + w/16, y0+17, w/16, c);
    }
    rect_line(x0, y0+4, w, 13, OUTLINE);
    rect_fill(x0, y0+2, w, 2, COL(198,164,84));
    // bunting strung between the posts
    { static const u32 flags[5] = { COL(220,70,70), COL(255,205,40), COL(110,190,255), COL(120,220,120), COL(180,120,255) };
      int bx0=x0+6, by0=y0+26, bx1=x1-6, sag=9;
      for (int i=0;i<14;i++){
          float t0=i/14.0f, t1=(i+1)/14.0f;
          int ax=bx0+(int)((bx1-bx0)*t0), ay=by0+(int)(sag*4*t0*(1-t0));
          int ex=bx0+(int)((bx1-bx0)*t1), ey=by0+(int)(sag*4*t1*(1-t1));
          line_thick(ax, ay, ex, ey, 1, OUTLINE);
          if (i%2==0) tri_fill(ax, ay, ax+8, ay, ax+4, ay+8, flags[(i/2)%5]);
      } }
    // the hanging sign (sways a touch) on two stubs off the awning
    { int sway = (int)(fsin(G.anim_t*0.8f)*1.5f);
      int sx0=x0+18+sway, sw=x1-x0-36;
      rect_fill(sx0+8, y0-14, 4, 18, wood_d); rect_fill(sx0+sw-12, y0-14, 4, 18, wood_d);
      rect_fill(sx0, y0-16, sw, 22, wood_d); rect_line(sx0, y0-16, sw, 22, OUTLINE);
      rect_line(sx0+2, y0-14, sw-4, 18, COL(198,164,84));
      rect_fill(sx0+22, y0-11, sw-44, 1, COL(122,88,52));
      draw_text_center(cx+sway, y0-9, "BUG MALL", 2, COL(255,220,90)); }
    // "we buy" board on the wall — two lines, so the whole phrase fits
    rect_fill(cx-52, y0+42, 104, 24, COL(250,242,218)); rect_line(cx-52, y0+42, 104, 24, wood_d);
    draw_text_center(cx, y0+45, "WE BUY BUGS", 1, COL(90,60,30));
    draw_text_center(cx, y0+55, "MALL: 5 SHOPS", 1, COL(90,60,30));
    // counter, crates of stock, a sack, and the golden scale
    int cy=SY(-26);
    rect_fill(cx-62, cy-8, 17, 17, wood); rect_line(cx-62, cy-8, 17, 17, OUTLINE);
    rect_fill(cx-60, cy-17, 14, 9, wood);  rect_line(cx-60, cy-17, 14, 9, OUTLINE);
    ellipse_fill(cx-53, cy-19, 6, 3, COL(234,130,46));          // today's catch, on top
    rect_fill(cx+48, cy-9, 13, 9, COL(206,182,138)); rect_line(cx+48, cy-9, 13, 9, OUTLINE);
    rect_fill(cx+52, cy-12, 5, 3, wood_d);
    rect_fill(cx-32, cy, 64, 10, COL(140,95,55)); rect_line(cx-32, cy, 64, 10, OUTLINE);
    rect_fill(cx-32, cy, 64, 2, COL(172,124,78));
    { int gx=cx+19;   // the scale: post, beam, two pans — a bug being weighed
      rect_fill(gx, cy-13, 2, 13, COL(198,164,84));
      rect_fill(gx-11, cy-14, 24, 2, COL(198,164,84));
      line_thick(gx-9, cy-12, gx-13, cy-6, 1, COL(198,164,84)); line_thick(gx-9, cy-12, gx-5, cy-6, 1, COL(198,164,84));
      line_thick(gx+11, cy-12, gx+7, cy-6, 1, COL(198,164,84)); line_thick(gx+11, cy-12, gx+15, cy-6, 1, COL(198,164,84));
      ellipse_fill(gx-9, cy-5, 6, 2, COL(216,180,80)); ellipse_fill(gx+11, cy-5, 6, 2, COL(216,180,80));
      disc_fill(gx-9, cy-8, 3, COL(98,192,92)); }
    draw_text_center(cx-14, cy-6, pad_mode()? "B" : key_name(G.bind[ACT_INTERACT]), 1, COL(255,255,255));
    // welcome rug in front of the counter
    int ry=SY(-14);
    rect_fill(cx-34, ry, 68, 9, COL(178,60,54)); rect_line(cx-34, ry, 68, 9, COL(120,40,38));
    rect_line(cx-30, ry+2, 60, 5, COL(240,200,120));
}
static void draw_prop(int type, int x, int y, u32 seed){
    u32 edge = COL(52,50,48);
    int j1 = (int)(seed%5) - 2, j2 = (int)((seed>>3)%5) - 2;   // per-prop jitter
    switch (type){
    case 1: // shattered PCB + parts spill — flat debris, walkable
        ellipse_fill(x, y+5, 18, 6, COL(96,80,60));                 // scuffed dirt patch
        tri_fill(x-14, y+6, x+2, y+7, x-6, y-4, COL(38,118,60));    // main board shard
        tri_fill(x-14, y+6, x-6, y-4, x-13, y-3, COL(48,140,72));
        rect_fill(x-4, y+1, 12, 6, COL(42,128,64)); rect_line(x-4, y+1, 12, 6, COL(24,74,38));
        rect_fill(x-3, y+5, 10, 1, COL(216,178,64));                 // gold edge pins
        rect_fill(x-1, y-2, 5, 3, COL(28,28,34));                   // a loose chip
        rect_fill(x+5, y+2, 4, 2, COL(28,28,34));
        circle_fill(x-11+j1, y+2, 3, COL(178,52,44)); circle_out(x-11+j1, y+2, 3, edge);  // capacitor
        circle_out(x+11, y+3, 6, COL(52,102,200));                   // coiled CAT6
        circle_out(x+11, y+3, 4, COL(52,102,200));
        rect_fill(x+15, y+1, 4, 3, COL(196,202,210));                // its RJ45 plug
        break;
    case 2: // a crate with a CRT monitor in it — solid
        shadow_ellipse(x, y+13, 22, 5);
        rect_fill(x-11, y-17, 22, 15, COL(206,198,178)); rect_line(x-11, y-17, 22, 15, edge);  // CRT body
        rect_fill(x-8, y-14, 13, 9, COL(58,76,86));                  // screen
        tri_fill(x-8, y-14, x+1, y-14, x-8, y-7, COL(112,140,150));  // screen glare
        rect_fill(x+6, y-13, 2, 7, COL(160,152,134));                // side vents
        line_thick(x+8, y-3, x+12, y+3, 1, COL(30,30,34));           // power cord into the crate
        rect_fill(x-17, y-2, 34, 16, COL(146,104,58)); rect_line(x-17, y-2, 34, 16, COL(84,58,32));  // crate
        line_thick(x-17, y-2, x+17, y+14, 2, COL(120,84,44));        // cross planks
        line_thick(x+17, y-2, x-17, y+14, 2, COL(120,84,44));
        rect_fill(x-17, y-2, 34, 3, COL(164,120,68));
        break;
    case 3: // DRAM sticks + a coax coil, lying flat — walkable
        for (int k=0;k<2;k++){                                       // two RAM sticks
            int sx = x-13+k*7, sy = y-3+k*7;
            rect_fill(sx, sy, 20, 5, COL(34,110,58)); rect_line(sx, sy, 20, 5, COL(20,64,34));
            rect_fill(sx, sy+4, 20, 1, COL(216,178,64));             // gold contacts
            rect_fill(sx+2, sy+1, 4, 3, COL(26,26,32));
            rect_fill(sx+8, sy+1, 4, 3, COL(26,26,32));
            rect_fill(sx+14, sy+1, 4, 3, COL(26,26,32));
        }
        circle_out(x+11+j2, y-4, 7, COL(26,26,30));                  // coax coil
        circle_out(x+11+j2, y-4, 4, COL(26,26,30));
        rect_fill(x+16+j2, y-8, 4, 4, COL(178,182,188));             // F-connector
        rect_fill(x+17+j2, y-7, 2, 2, COL(120,124,130));
        circle_fill(x-9+j1, y+6, 2, COL(216,178,64));                // a stray gold pin
        break;
    case 4: // fax machine with a modem perched on top — solid
        shadow_ellipse(x, y+12, 20, 5);
        rect_fill(x-7, y-13, 12, 10, COL(238,236,228));              // paper sticking out the top
        rect_fill(x-5, y-11, 8, 1, COL(168,166,158));
        rect_fill(x-5, y-8, 8, 1, COL(168,166,158));
        rect_fill(x-15, y-3, 30, 14, COL(202,194,176)); rect_line(x-15, y-3, 30, 14, edge);   // fax body
        rect_fill(x-15, y-3, 30, 3, COL(178,170,152));               // feed slot shadow
        rect_fill(x-13, y+3, 6, 6, COL(160,152,134));                // handset rest
        for (int r2=0;r2<2;r2++) for (int c2=0;c2<3;c2++)            // keypad
            rect_fill(x+4+c2*4, y+3+r2*4, 2, 2, COL(90,88,84));
        rect_fill(x+3, y-8, 13, 6, COL(40,42,50)); rect_line(x+3, y-8, 13, 6, edge);          // modem
        line_thick(x+13, y-8, x+16, y-15, 1, COL(30,30,34));         // its antenna
        circle_fill(x+6, y-5, 1, COL(90,220,110));                   // modem LEDs
        circle_fill(x+10, y-5, 1, COL(240,180,60));
        break;
    case 5: // a game console tipped into the dirt, cables spilled — walkable
        ellipse_fill(x-2, y+8, 19, 5, COL(96,78,54));               // dirt mound it sank into
        tri_fill(x-15, y+7, x+11, y+7, x+15, y-6, COL(58,54,74));   // console, leaning
        tri_fill(x-15, y+7, x+15, y-6, x+5, y-9, COL(76,70,96));     // its top face
        rect_fill(x-2, y-9, 7, 6, COL(150,148,158));                 // cartridge, half out
        rect_fill(x-2, y-9, 7, 2, COL(196,84,60));                   // cartridge label stripe
        line_thick(x-13, y+4, x+12, y+4, 1, COL(38,36,48));          // slot shadow line
        circle_fill(x+9, y+1, 1, COL(232,64,54));                    // power LED, still on somehow
        line_thick(x+13, y+3, x+20, y+7, 2, COL(52,102,200));        // spilled CAT6...
        line_thick(x+20, y+7, x+26, y+4, 2, COL(52,102,200));
        rect_fill(x+25, y+1, 5, 4, COL(196,202,210));                // ...and its plug
        line_thick(x+11, y+6, x+19, y+11, 3, COL(24,24,28));         // spilled coax...
        rect_fill(x+18, y+10, 5, 4, COL(178,182,188));               // ...and its F-connector
        break;
    case 6: // KERNEL BADLANDS: a burnt server husk — solid
        shadow_ellipse(x, y+13, 20, 5);
        rect_fill(x-12, y-26, 24, 38, COL(46,40,38)); rect_line(x-12, y-26, 24, 38, COL(22,19,18));
        rect_fill(x-12, y-26, 24, 4, COL(64,54,48));                 // scorched crown
        for (int r2=0;r2<4;r2++) rect_fill(x-8, y-18+r2*8, 16, 3, COL(28,24,23));  // dead drive bays
        line_thick(x-12, y-26, x+3, y-12, 1, COL(16,14,13));         // the crack that killed it
        line_thick(x+3, y-12, x+1, y-2, 1, COL(16,14,13));
        circle_fill(x-6+j1, y+6, 1, COL(255,120,40));                // embers that never got the memo
        circle_fill(x+5, y+2, 1, COL(255,160,60));
        circle_fill(x+8+j2, y+8, 1, COL(230,80,30));
        break;
    case 7: // KERNEL BADLANDS: a cracked heat vent — walkable
        ellipse_fill(x, y+4, 17, 6, COL(60,38,28));                 // scorch ring
        ellipse_fill(x, y+4, 11, 4, COL(30,20,16));                  // the vent mouth
        rect_fill(x-8, y-6, 4, 8, COL(52,46,44)); rect_fill(x-8, y-7, 4, 2, COL(255,120,40));
        rect_fill(x+4, y-9, 4, 11, COL(52,46,44)); rect_fill(x+4, y-10, 4, 2, COL(255,150,50));
        circle_fill(x+j1, y+3, 1, COL(255,170,70)); circle_fill(x-4+j2, y+6, 1, COL(255,120,40));
        break;
    case 8: // FROZEN CACHE FIELDS: a frost-locked drive bank — solid
        shadow_ellipse(x, y+12, 21, 5);
        rect_fill(x-15, y-14, 30, 26, COL(150,172,190)); rect_line(x-15, y-14, 30, 26, COL(96,118,140));
        rect_fill(x-15, y-14, 30, 5, COL(232,242,250));              // snow cap
        tri_fill(x-10, y-9, x-6, y-9, x-8, y-3, COL(214,232,244));   // icicles
        tri_fill(x+2, y-9, x+6, y-9, x+4, y-4, COL(214,232,244));
        for (int r2=0;r2<3;r2++){ rect_fill(x-11, y-5+r2*7, 22, 4, COL(120,142,164)); rect_line(x-11, y-5+r2*7, 22, 4, COL(88,108,130)); }
        circle_fill(x+9, y+8, 1, COL(120,255,230));                  // one teal LED, frozen mid-blink
        break;
    case 9: // FROZEN CACHE FIELDS: RAM sticks in a snow drift — walkable
        ellipse_fill(x, y+6, 18, 6, COL(226,238,248));
        ellipse_fill(x-4, y+4, 10, 4, COL(240,248,252));
        rect_fill(x-12, y-2, 17, 4, COL(120,160,150)); rect_fill(x-12, y+2, 17, 1, COL(216,178,64));  // a stick, half out
        rect_fill(x-10, y-1, 3, 2, COL(60,90,86)); rect_fill(x-5, y-1, 3, 2, COL(60,90,86));
        rect_fill(x+2, y+3, 15, 4, COL(108,148,140)); rect_fill(x+2, y+7, 15, 1, COL(216,178,64));
        pset(x-6+j1, y-3, COL(255,255,255)); pset(x+9, y+1, COL(255,255,255)); pset(x+3+j2, y-2, COL(200,230,245));
        break;
    case 10: // NEON RAM FLATS: a memory tower, still lit — solid
        shadow_ellipse(x, y+12, 17, 5);
        rect_fill(x-8, y-30, 16, 42, COL(30,26,58)); rect_line(x-8, y-30, 16, 42, COL(70,64,140));
        line_thick(x-4, y-26, x-4, y+8, 2, COL(64,240,255));         // glowing traces up the spine
        line_thick(x+3, y-26, x+3, y+8, 2, COL(255,84,200));
        for (int r2=0;r2<4;r2++){ rect_fill(x-6, y-22+r2*9, 3, 3, COL(64,240,255)); rect_fill(x+3, y-18+r2*9, 3, 3, COL(255,84,200)); }
        circle_fill(x, y-33, 2, COL(190,250,255));                   // the beacon on top
        break;
    case 11: // NEON RAM FLATS: a circuit bloom on the ground — walkable
        ellipse_fill(x, y+3, 16, 6, COL(36,32,74));
        line_thick(x-12, y+3, x+12, y+3, 1, COL(64,240,255));
        line_thick(x, y-4, x, y+9, 1, COL(64,240,255));
        line_thick(x-8, y-2, x+7, y+8, 1, COL(255,84,200));
        circle_fill(x-12, y+3, 2, COL(64,240,255)); circle_fill(x+12, y+3, 2, COL(255,84,200));
        circle_fill(x, y-4, 2, COL(190,250,255)); circle_fill(x+j1, y+9, 1, COL(64,240,255));
        break;
    case 12: // CABLE JUNGLE: a cable tree — solid
        shadow_ellipse(x, y+12, 19, 5);
        rect_fill(x-4, y-16, 8, 28, COL(94,70,46)); rect_line(x-4, y-16, 8, 28, COL(60,44,28));
        disc_fill(x, y-24, 15, COL(44,92,48)); disc_fill(x-11, y-18, 10, COL(52,104,54)); disc_fill(x+11, y-19, 10, COL(40,84,44));
        disc_fill(x+j1, y-27, 7, COL(64,120,60));
        line_thick(x-9, y-20, x-11, y-2, 2, COL(26,26,30));          // hanging cables...
        rect_fill(x-13, y-1, 4, 4, COL(178,182,188));                // ...with the plugs still on
        line_thick(x+8, y-22, x+12, y-6, 2, COL(52,102,200));
        rect_fill(x+11, y-5, 4, 3, COL(196,202,210));
        break;
    case 13: // CABLE JUNGLE: a mossy keyboard slab — walkable
        rect_fill(x-16, y-4, 32, 12, COL(120,118,110)); rect_line(x-16, y-4, 32, 12, COL(80,78,70));
        for (int r2=0;r2<2;r2++) for (int c2=0;c2<6;c2++)
            rect_fill(x-13+c2*5, y-1+r2*5, 3, 3, COL(168,164,152));
        disc_fill(x-9, y-2, 4, COL(70,130,66)); disc_fill(x+11, y+6, 5, COL(62,120,60)); disc_fill(x+3+j1, y-4, 3, COL(80,140,72));
        break;
    case 14: // THE DEPTHS: a half-deleted file monolith — solid
        shadow_ellipse(x, y+12, 18, 5);
        rect_fill(x-11, y-28, 22, 40, COL(96,90,112)); rect_line(x-11, y-28, 22, 40, COL(58,54,74));
        rect_fill(x-11, y-28, 22, 6, COL(120,112,138));              // the header that survived
        rect_fill(x+2, y-14, 9, 10, COL(80,72,96));                  // a chunk that didn't (hole)
        rect_fill(x-11, y+2, 8, 8, COL(80,72,96));
        for (int r2=0;r2<3;r2++) rect_fill(x-7, y-18+r2*7, 10, 2, COL(140,132,158));   // faded file lines
        rect_fill(x-13, y-6, 4, 12, COL(110,90,140));                // a glitch slice, offset from the body
        pset(x+7+j1, y-24, COL(255,84,200)); pset(x-5, y+9, COL(64,240,255));
        break;
    case 15: // THE DEPTHS: glitch shards — walkable
        rect_fill(x-10+j1, y-2, 8, 4, COL(255,84,200));
        rect_fill(x-2, y+3, 10, 3, COL(64,240,255));
        rect_fill(x+4+j2, y-6, 6, 4, COL(150,144,168));
        rect_fill(x-13, y+5, 5, 3, COL(110,104,130));
        pset(x+11, y, COL(255,84,200)); pset(x-6, y-6, COL(64,240,255)); pset(x+1, y+8, COL(190,250,255));
        break;
    }
}
static void draw_buddy(Buddy *b);   // Banzai Buddies, drawn further down
// the Bug Mall interior: plank floors, a red carpet up the middle, four
// counters with a keeper bug behind each, and NO hostile anything

// the mall keepers (Daniel, Nov 2): five variations on a certain
// bearded, bespectacled co-founder of a fruit company — round face,
// full beard, glasses, big friendly smile — each in his station's
// outfit, head and shoulders clear above the counter with a warm
// lantern glow behind him so he actually reads at game scale.
static void draw_keeper(int st, float wx, float wy){
    float cz = G.zoom; if (cz<0.6f) cz=0.6f;
    int x = SX(wx), y = SY(wy) + (int)(fsin(G.anim_t*1.3f + st*1.7f)*cz);
    u32 skin = COL(240,200,164), beard = COL(128,106,88), hair = COL(160,148,134);
    // (the lantern-glow discs behind the head are GONE — Nov 5,
    // Daniel: "get rid of the halos behind the shopkeepers". The
    // warmth moved to light pools on the floor instead.)
    // shoulders and the station outfit (the counter covers the rest)
    u32 shirt = st==0? COL(206,74,60) : st==1? COL(238,236,230)
              : st==2? COL(238,236,230) : st==3? COL(186,164,110) : COL(230,230,234);
    ellipse_fill(x, y + (int)(17*cz), (int)(17*cz), (int)(10*cz), shirt);
    if (st==0){          // Hawaiian: little flowers on the chest band
        disc_fill(x-(int)(9*cz), y+(int)(14*cz), (int)(1.6*cz), COL(255,240,200));
        disc_fill(x+(int)(2*cz), y+(int)(15*cz), (int)(1.6*cz), COL(255,214,90));
        disc_fill(x+(int)(11*cz), y+(int)(13*cz), (int)(1.6*cz), COL(255,240,200));
        tri_fill(x-(int)(3*cz), y+(int)(10*cz), x+(int)(3*cz), y+(int)(10*cz), x, y+(int)(15*cz), skin);   // open collar
    } else if (st==1){   // whales only: black tux panels, a red bow tie, a gold whale pin
        rect_fill(x-(int)(16*cz), y+(int)(11*cz), (int)(7*cz), (int)(9*cz), COL(28,26,34));
        rect_fill(x+(int)(9*cz), y+(int)(11*cz), (int)(7*cz), (int)(9*cz), COL(28,26,34));
        tri_fill(x-(int)(4*cz), y+(int)(11*cz), x, y+(int)(13*cz), x-(int)(4*cz), y+(int)(15*cz), COL(160,30,40));
        tri_fill(x+(int)(4*cz), y+(int)(11*cz), x, y+(int)(13*cz), x+(int)(4*cz), y+(int)(15*cz), COL(160,30,40));
        disc_fill(x+(int)(6*cz), y+(int)(16*cz), (int)(2*cz), COL(255,214,90));
    } else if (st==2){   // banker: green vest + a gold tie
        rect_fill(x-(int)(16*cz), y+(int)(11*cz), (int)(7*cz), (int)(9*cz), COL(56,116,78));
        rect_fill(x+(int)(9*cz), y+(int)(11*cz), (int)(7*cz), (int)(9*cz), COL(56,116,78));
        rect_fill(x-(int)(1.5*cz), y+(int)(10*cz), (int)(3*cz), (int)(9*cz), COL(226,178,60));
    } else if (st==3){   // buddies: khaki collar + a paw-print badge
        rect_fill(x-(int)(8*cz), y+(int)(11*cz), (int)(16*cz), (int)(3*cz), COL(150,128,84));
        disc_fill(x+(int)(7*cz), y+(int)(15*cz), (int)(2*cz), COL(120,84,50));
        disc_fill(x+(int)(4*cz), y+(int)(12.5*cz), 1, COL(120,84,50));
        disc_fill(x+(int)(10*cz), y+(int)(12.5*cz), 1, COL(120,84,50));
    } else {             // bowling: blue shoulders + a red neckerchief
        ellipse_fill(x-(int)(11*cz), y+(int)(14*cz), (int)(7*cz), (int)(5*cz), COL(70,130,220));
        ellipse_fill(x+(int)(11*cz), y+(int)(14*cz), (int)(7*cz), (int)(5*cz), COL(70,130,220));
        tri_fill(x-(int)(3*cz), y+(int)(10*cz), x+(int)(3*cz), y+(int)(10*cz), x, y+(int)(15*cz), COL(200,50,50));
    }
    // neck, beard mass, then the face over it so the beard wraps
    // the chin and cheeks
    rect_fill(x-(int)(3*cz), y+(int)(8*cz), (int)(6*cz), (int)(5*cz), col_shade(skin, 0.92f));
    ellipse_fill(x, y+(int)(4*cz), (int)(10.5*cz), (int)(9.5*cz), beard);
    circle_fill(x, y, (int)(11*cz), OUTLINE);
    circle_shade(x, y-(int)(1*cz), (int)(10*cz), skin);
    ellipse_fill(x, y-(int)(9*cz), (int)(8.5*cz), (int)(3.5*cz), hair);   // thin grey cap, forehead bare
    ellipse_fill(x, y+(int)(5.5*cz), (int)(4.5*cz), (int)(3*cz), skin);   // mouth patch over the beard
    line_thick(x-(int)(2.5*cz), y+(int)(5*cz), x, y+(int)(7*cz), 1, OUTLINE);   // the smile
    line_thick(x, y+(int)(7*cz), x+(int)(2.5*cz), y+(int)(5*cz), 1, OUTLINE);
    disc_fill(x, y+(int)(2*cz), (int)(1.6*cz)+1, col_shade(skin, 0.9f));        // nose
    // the glasses: thin dark frames, eyes inside, brows over the top
    { int gy2 = y-(int)(2*cz), lw2 = (int)(7.5*cz), lh2 = (int)(6*cz);
      rect_line(x-lw2-(int)(1*cz), gy2, lw2, lh2, OUTLINE);
      rect_line(x+(int)(1*cz), gy2, lw2, lh2, OUTLINE);
      rect_fill(x-(int)(1*cz), gy2+(int)(2*cz), (int)(2*cz), 1, OUTLINE);
      disc_fill(x-(int)(4.5*cz), gy2+(int)(3*cz), 1, OUTLINE);
      disc_fill(x+(int)(4.5*cz), gy2+(int)(3*cz), 1, OUTLINE);
      line_thick(x-(int)(7*cz), gy2-(int)(2*cz), x-(int)(2*cz), gy2-(int)(2.5*cz), 1, beard);
      line_thick(x+(int)(2*cz), gy2-(int)(2.5*cz), x+(int)(7*cz), gy2-(int)(2*cz), 1, beard); }
}
static void draw_mall(void){
    rect_fill(0, 0, VW, VH, COL(22,18,32));
    float cz = G.zoom>0.01f? G.zoom : 1.0f;
    int tlx = (int)(G.camx - (VW/2)/cz) - 64, tly = (int)(G.camy - (VH/2)/cz) - 64;
    int gx0 = tlx/64 - (tlx<0?1:0), gy0 = tly/64 - (tly<0?1:0);
    int gw = (int)(VW/cz/64) + 4, gh = (int)(VH/cz/64) + 4, ts = (int)(64*cz) + 1;
    for (int gy=gy0; gy<gy0+gh; gy++) for (int gx=gx0; gx<gx0+gw; gx++){
        float wx = gx*64+32, wy = gy*64+32;
        if (wx<-330 || wx>330 || wy<MALL_CY-230 || wy>MALL_CY+230) continue;
        u32 c = col_shade(((gx+gy)&1)? COL(124,94,62) : COL(116,88,58),
                          0.96f + hash_f(gx, gy, 91)*0.07f);
        { Player *vp = &G.p[G.view_pi>=0? G.view_pi : 0];
          if (vp->whale_on & 512){   // WHALES ONLY: DISCO MALL — the floor cycles for the disco owner
              static const u32 DISCO[4] = { COL(150,80,140), COL(70,130,170), COL(170,130,60), COL(80,150,110) };
              c = col_shade(DISCO[(gx+gy+(int)(G.time*2.0f))&3], 0.96f + hash_f(gx, gy, 91)*0.07f);
          } }
        int tx = SX(gx*64), tyy = SY(gy*64);
        rect_fill(tx, tyy, ts, ts, c);
        rect_fill(tx, tyy+ts-2, ts, 1, col_shade(c, 0.88f));   // plank seam
    }
    // the red carpet: entrance to counters
    { int cx0=SX(-37), cy0=SY(MALL_CY-118), cw=(int)(74*cz), ch=(int)(272*cz);
      rect_fill(cx0, cy0, cw, ch, COL(148,46,50));
      rect_line(cx0, cy0, cw, ch, COL(198,164,84));
      rect_line(cx0+3, cy0+3, cw-6, ch-6, COL(120,36,40)); }
    // the mall's name, painted on the floor inside the entrance. It
    // used to hang on the north wall — where the HUD's zone pill slid
    // in front of it every time you walked north. Down here, under
    // your feet as you come in, nothing can cover it.
    draw_text_center(SX(0), SY(MALL_CY+58), "THE BUG MALL", 2, COL(255,220,130));
    // room walls: wood panelling with a gold chair rail, door gap south
    { u32 wallc = COL(84,58,38);
      rect_fill(SX(-308), SY(MALL_CY-192), (int)(616*cz), (int)(26*cz), wallc);
      rect_fill(SX(-308), SY(MALL_CY-170), (int)(616*cz), 2, COL(198,164,84));
      rect_fill(SX(-308), SY(MALL_CY+162), (int)(268*cz), (int)(26*cz), wallc);
      rect_fill(SX(40), SY(MALL_CY+162), (int)(268*cz), (int)(26*cz), wallc);
      rect_fill(SX(-308), SY(MALL_CY+162), (int)(616*cz), 2, COL(198,164,84));
      rect_fill(SX(-308), SY(MALL_CY-192), (int)(24*cz), (int)(380*cz), wallc);
      rect_fill(SX(284), SY(MALL_CY-192), (int)(24*cz), (int)(380*cz), wallc); }
    // exit mat
    { int mx0=SX(-38), my0=SY(MALL_CY+132), mw=(int)(76*cz), mh=(int)(34*cz);
      rect_fill(mx0, my0, mw, mh, COL(52,96,60)); rect_line(mx0, my0, mw, mh, COL(198,164,84));
      draw_text_center(SX(0), SY(MALL_CY+143), "EXIT", 2, COL(240,255,240)); }
    // the Banzai World gate, south-east: a grass patch, a red torii,
    // and a sign. Through it, you play as your buddy.
    { int cx = SX(BW_SX);
      int gx0=SX(BW_SX-56), gy0=SY(MALL_CY+110), gw=(int)(112*cz), gh=(int)(48*cz);
      rect_fill(gx0, gy0, gw, gh, COL(86,150,66)); rect_line(gx0, gy0, gw, gh, COL(60,110,48));
      for (int gt=0; gt<7; gt++){ int tx2 = SX(BW_SX-48 + (gt*37)%96), ty2 = SY(MALL_CY+118 + (gt*23)%34);
          line_thick(tx2, ty2, tx2-1, ty2-4, 1, COL(60,120,45)); line_thick(tx2+3, ty2, tx2+4, ty2-3, 1, COL(60,120,45)); }
      // torii: two posts and two lintels, vermilion
      int px1=SX(BW_SX-26), px2=SX(BW_SX+26), py0=SY(MALL_CY+118), py1=SY(MALL_CY+152);
      rect_fill(px1-3, py0, 6, py1-py0, COL(196,64,44)); rect_line(px1-3, py0, 6, py1-py0, OUTLINE);
      rect_fill(px2-3, py0, 6, py1-py0, COL(196,64,44)); rect_line(px2-3, py0, 6, py1-py0, OUTLINE);
      rect_fill(px1-10, py0-4, (px2-px1)+20, 6, COL(196,64,44)); rect_line(px1-10, py0-4, (px2-px1)+20, 6, OUTLINE);
      rect_fill(px1-6, py0+8, (px2-px1)+12, 4, COL(170,52,38));
      int sx0=SX(BW_SX-52), sy0=SY(MALL_CY+96), sw=(int)(104*cz), sh=(int)(13*cz);
      rect_fill(sx0, sy0, sw, sh, COL(46,40,34)); rect_line(sx0, sy0, sw, sh, COL(198,164,84));
      draw_text_center(cx, sy0+3, "BANZAI WORLD", 1, COL(255,240,210)); }
    // the Arcade door, east wall: a dark opening in the panelling, a
    // violet mat, and a buzzing cyan sign. The games live through here.
    { int dx0=SX(266), dy0=SY(MALL_CY+2), dw=(int)(18*cz), dh=(int)(44*cz);
      rect_fill(dx0, dy0, dw, dh, COL(16,12,26)); rect_line(dx0, dy0, dw, dh, COL(120,240,255));
      int mx0=SX(214), my0=SY(MALL_CY+8), mw=(int)(50*cz), mh=(int)(34*cz);
      rect_fill(mx0, my0, mw, mh, COL(74,40,110)); rect_line(mx0, my0, mw, mh, COL(120,240,255));
      int sx0=SX(196), sy0=SY(MALL_CY-26), sw=(int)(76*cz), sh=(int)(13*cz);
      rect_fill(sx0, sy0, sw, sh, COL(24,20,36)); rect_line(sx0, sy0, sw, sh, COL(120,240,255));
      draw_text_center(SX(234), sy0+3, "ARCADE", 1, COL(160,255,255)); }
    // (two potted shrubs stood here while the Gallery slot was empty;
    // Whales Only took the counter over on Nov 2 and the shrubs went)
    // hanging lanterns down the carpet
    for (int i=0;i<3;i++){
        int lx=SX(-150 + i*150), ly=SY(MALL_CY-30);
        disc_fill(lx, ly, 13, COL(255,196,96));
        disc_fill(lx, ly, 8, COL(255,226,150));
        rect_fill(lx-3, ly-6, 6, 8, COL(94,60,32));
    }
    // the four counters, each with its keeper behind it
    static const u32 SBAND[4] = { COL(190,60,50), COL(198,164,84), COL(70,170,90), COL(240,140,50) };
    for (int s=0;s<4;s++){
        int cx = SX(MALL_SX[s]);
        // warm light pooled on the floorboards in front of the
        // counter (the glow-up's answer to the old head halos)
        { int py2 = SY(MALL_CY-112);
          ellipse_fill(cx, py2, (int)(58*cz), (int)(17*cz), COL(140,110,70));
          ellipse_fill(cx, py2, (int)(40*cz), (int)(11*cz), COL(152,122,78)); }
        // keeper first, so the counter hides his lower half; his
        // head rides well clear of the counter top
        draw_keeper(s, MALL_SX[s], MALL_CY-176);
        int x0=SX(MALL_SX[s]-60), y0=SY(MALL_CY-158), cw=(int)(120*cz), ch=(int)(34*cz);
        shadow_ellipse(cx, y0+ch+2, cw/2, 4);
        rect_fill(x0, y0, cw, ch, COL(122,80,44)); rect_line(x0, y0, cw, ch, OUTLINE);
        rect_fill(x0, y0, cw, 6, COL(150,104,58));
        rect_fill(x0, y0+8, cw, 4, SBAND[s]);               // the station's colour stripe
        // the station's name, lettered on the counter front where the
        // shopper actually stands. (There used to be hanging signs at
        // double-size letters, but the boards were wider than the
        // counters' spacing — the names ran into each other — and from
        // anywhere but the counter itself they sat off-screen or
        // behind the HUD. The plate is always readable.)
        draw_text_center(cx, y0+20, MALL_STATION[s], 1, COL(255,240,210));
    }
    // the bowling counter, south-west: a keeper, three pins, and a
    // little lane rug pointing at the real lanes (behind the counter)
    { int cx = SX(BOWL_SX);
      { int rx0=SX(BOWL_SX-24), ry0=SY(MALL_CY+26), rw=(int)(48*cz), rh=(int)(82*cz);
        rect_fill(rx0, ry0, rw, rh, COL(216,186,130)); rect_line(rx0, ry0, rw, rh, COL(150,120,80));
        for (int pn=0;pn<3;pn++){ int px2 = SX(BOWL_SX + (pn-1)*13), py2 = SY(MALL_CY+40);
            ellipse_fill(px2, py2, 3, 5, COL(245,242,238)); rect_fill(px2-3, py2-2, 6, 2, COL(200,60,50)); } }
      draw_keeper(4, BOWL_SX, MALL_CY+100);
      int x0=SX(BOWL_SX-52), y0=SY(MALL_CY+118), cw=(int)(104*cz), ch=(int)(34*cz);
      shadow_ellipse(cx, y0+ch+2, cw/2, 4);
      rect_fill(x0, y0, cw, ch, COL(122,80,44)); rect_line(x0, y0, cw, ch, OUTLINE);
      rect_fill(x0, y0+ch-6, cw, 6, COL(150,104,58));
      rect_fill(x0, y0+ch-10, cw, 4, COL(80,130,220));         // bowling blue stripe
      draw_carcass(cx-30, y0+16, 8, 1, 0.6f);                  // the house ball: a carcass, of course
      { int sw2 = text_w(MALL_STATION[4], 2) + 16;
        int sx0 = cx - sw2/2, sy0 = SY(MALL_CY+68);
        rect_fill(sx0, sy0, sw2, 20, COL(94,60,32)); rect_line(sx0, sy0, sw2, 20, OUTLINE);
        rect_line(sx0+2, sy0+2, sw2-4, 16, COL(198,164,84));
        draw_text_center(cx, sy0+3, MALL_STATION[4], 2, COL(255,220,130)); } }
    // (the Gallery's display pedestals — "yesterday's legends" —
    // went out with the Gallery itself on Nov 2)
    // the bank's vault door, gleaming behind the counter, beside
    // the keeper (it used to sit dead centre — where he now stands)
    { int vx=SX(MALL_SX[2]+44), vy=SY(MALL_CY-176);
      circle_fill(vx, vy, 15, COL(120,124,134)); circle_out(vx, vy, 15, COL(198,164,84));
      circle_out(vx, vy, 9, COL(90,94,104));
      for (int k=0;k<4;k++){ float a=k*TAU/4 + G.anim_t*0.1f;
          line_thick(vx, vy, vx+(int)(fcos(a)*7), vy+(int)(fsin(a)*7), 2, COL(198,164,84)); } }
    // the buddy pen: a little fence in front of the Banzai counter
    for (int fx2=MALL_SX[3]-56; fx2<=MALL_SX[3]+56; fx2+=14){
        int px=SX(fx2), py=SY(MALL_CY-92);
        rect_fill(px-2, py-10, 4, 12, COL(150,110,64)); rect_line(px-2, py-10, 4, 12, OUTLINE);
    }
    rect_fill(SX(MALL_SX[3]-58), SY(MALL_CY-100), (int)(116*cz), 3, COL(150,110,64));
    ui_vignette(clip_x0, clip_y0, clip_x1-clip_x0, clip_y1-clip_y0);
    // entities: buddies, then shoppers, then the usual sparkle
    for (int i=0;i<MAXP;i++) if (G.bud[i].active){
        float bx=G.bud[i].x, by=G.bud[i].y;
        if (!G.bud[i].in_bw && by>MALL_CY-300 && by<MALL_CY+300) draw_buddy(&G.bud[i]);
    }
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].in_mall) draw_player_sprite(&G.p[i]);
    for (int i=0;i<MAXPART;i++) if (G.parts[i].alive){
        Particle *q=&G.parts[i];
        if (q->y>MALL_CY-300 && q->y<MALL_CY+300) rect_fill(SX(q->x), SY(q->y), q->size, q->size, q->col);
    }
    for (int i=0;i<MAXFLOAT;i++) if (G.floats[i].alive){
        Floater *q=&G.floats[i];
        if (q->y>MALL_CY-300 && q->y<MALL_CY+300) draw_text_center(SX(q->x), SY(q->y), q->text, 1, q->col);
    }
    // WALK OF FAME marquee: flashes through the confetti colours while
    // the viewed player is mid-crawl under a heavy hat
    { Player *vp=&G.p[G.view_pi>=0? G.view_pi : 0];
      if (vp->wof>0)
          draw_text_center((clip_x0+clip_x1)/2, clip_y0+150, "WALK OF FAME", 3,
                           WOF_COL[(int)(G.anim_t*8)%6]); }
}
// THE ARCADE room (Nov 10): dark carpet, neon trim, and four cabinets
// against the north wall — one per game. West wall leads back to the mall.
static void draw_arcade(void){
    rect_fill(0, 0, VW, VH, COL(10,8,20));
    float cz = G.zoom>0.01f? G.zoom : 1.0f;
    int tlx = (int)(G.camx - (VW/2)/cz) - 64, tly = (int)(G.camy - (VH/2)/cz) - 64;
    int gx0 = tlx/64 - (tlx<0?1:0), gy0 = tly/64 - (tly<0?1:0);
    int gw = (int)(VW/cz/64) + 4, gh = (int)(VH/cz/64) + 4, ts = (int)(64*cz) + 1;
    for (int gy=gy0; gy<gy0+gh; gy++) for (int gx=gx0; gx<gx0+gw; gx++){
        float wx = gx*64+32, wy = gy*64+32;
        if (wx<-410 || wx>410 || wy<ARC_CY-230 || wy>ARC_CY+430) continue;
        u32 c = col_shade(((gx+gy)&1)? COL(52,40,84) : COL(46,36,74), 0.96f + hash_f(gx, gy, 91)*0.07f);
        int tx = SX(gx*64), tyy = SY(gy*64);
        rect_fill(tx, tyy, ts, ts, c);
        rect_fill(tx, tyy+ts-2, ts, 1, col_shade(c, 0.85f));
    }
    draw_text_center(SX(0), SY(ARC_CY+210), "THE ARCADE", 2, COL(140,255,255));
    { u32 wallc = COL(30,24,48);
      rect_fill(SX(-386), SY(ARC_CY-192), (int)(772*cz), (int)(26*cz), wallc);
      rect_fill(SX(-386), SY(ARC_CY-170), (int)(772*cz), 2, COL(120,240,255));
      rect_fill(SX(-386), SY(ARC_CY+332), (int)(772*cz), (int)(26*cz), wallc);
      rect_fill(SX(-386), SY(ARC_CY+332), (int)(772*cz), 2, COL(240,120,220));
      rect_fill(SX(-386), SY(ARC_CY-192), (int)(24*cz), (int)(160*cz), wallc);
      rect_fill(SX(-386), SY(ARC_CY+52), (int)(24*cz), (int)(306*cz), wallc);
      rect_fill(SX(362), SY(ARC_CY-192), (int)(24*cz), (int)(550*cz), wallc); }
    { int mx0=SX(-360), my0=SY(ARC_CY+6), mw=(int)(40*cz), mh=(int)(38*cz);
      rect_fill(mx0, my0, mw, mh, COL(52,96,60)); rect_line(mx0, my0, mw, mh, COL(120,240,255));
      draw_text_center(SX(-340), SY(ARC_CY+20), "EXIT", 1, COL(240,255,240)); }
    draw_text_center(SX(0), SY(ARC_CY-186), "ARCADE", 2, COL(255,140,230));
    static const u32 CABCOL[7] = { COL(240,140,40), COL(84,190,84), COL(70,130,230), COL(200,70,60), COL(120,220,130), COL(250,200,50), COL(216,64,54) };
    for (int s=0;s<7;s++){
        int cx = SX(ARC_SX[s]);
        { int py2 = SY(ARC_CY + ARC_ROWY[s] + 54);
          ellipse_fill(cx, py2, (int)(68*cz), (int)(17*cz), COL(60,50,96));
          ellipse_fill(cx, py2, (int)(48*cz), (int)(11*cz), COL(72,60,110)); }
        int x0=SX(ARC_SX[s]-64), y0=SY(ARC_CY + ARC_ROWY[s]), cw=(int)(128*cz), ch=(int)(98*cz);
        shadow_ellipse(cx, y0+ch+2, cw/2, 4);
        rect_fill(x0, y0, cw, ch, COL(24,22,38)); rect_line(x0, y0, cw, ch, CABCOL[s]);
        rect_fill(x0, y0, cw, (int)(16*cz), CABCOL[s]);
        draw_text_center(cx, y0+4, ARC_NAME[s], 1, COL(16,14,24));
        for (int li=0; li<6; li++){ int lx = x0 + (int)((8+li*17)*cz), ly = y0 + (int)(19*cz);
            disc_fill(lx, ly, 2, ((li + (int)(G.anim_t*3)) & 1)? COL(255,230,140) : CABCOL[s]); }
        int sx0 = x0+(int)(12*cz), sy0 = y0+(int)(26*cz), sw = (int)(104*cz), sh = (int)(44*cz);
        rect_fill(sx0, sy0, sw, sh, COL(10,14,20)); rect_line(sx0, sy0, sw, sh, COL(90,100,120));
        if (s==0){
            rect_fill(sx0+sw/2-(int)(9*cz), sy0+(int)(6*cz), (int)(18*cz), (int)(32*cz), COL(120,84,44));
            rect_fill(sx0+sw/2+(int)(2*cz), sy0+(int)(20*cz), (int)(3*cz), (int)(3*cz), COL(255,220,120));
        } else if (s==1){
            rect_fill(sx0, sy0+sh-(int)(8*cz), sw, (int)(8*cz), COL(70,150,70));
            rect_fill(sx0+sw-(int)(14*cz), sy0+(int)(8*cz), (int)(2*cz), (int)(28*cz), COL(220,220,220));
            tri_fill(sx0+sw-(int)(12*cz), sy0+(int)(8*cz), sx0+sw-(int)(12*cz), sy0+(int)(16*cz), sx0+sw-(int)(3*cz), sy0+(int)(12*cz), COL(240,90,80));
            disc_fill(sx0+(int)(16*cz), sy0+sh-(int)(14*cz), (int)(5*cz), COL(255,220,120));
        } else if (s==2){
            rect_fill(sx0+(int)(12*cz), sy0+(int)(20*cz), (int)(6*cz), (int)(16*cz), COL(120,84,44));
            disc_fill(sx0+(int)(15*cz), sy0+(int)(14*cz), (int)(10*cz), COL(70,160,80));
            rect_fill(sx0+sw-(int)(24*cz), sy0+sh-(int)(14*cz), (int)(13*cz), (int)(13*cz), COL(150,104,58)); rect_line(sx0+sw-(int)(24*cz), sy0+sh-(int)(14*cz), (int)(13*cz), (int)(13*cz), OUTLINE);
        } else if (s==3) {
            rect_fill(sx0+sw/2-(int)(13*cz), sy0+(int)(16*cz), (int)(26*cz), (int)(16*cz), COL(96,110,60));
            rect_fill(sx0+sw/2-(int)(2*cz), sy0+(int)(4*cz), (int)(4*cz), (int)(14*cz), COL(70,80,44));
            disc_fill(sx0+sw/2, sy0+(int)(20*cz), (int)(6*cz), COL(70,80,44));
            rect_fill(sx0+sw/2-(int)(17*cz), sy0+(int)(14*cz), (int)(4*cz), (int)(20*cz), COL(50,54,40));
            rect_fill(sx0+sw/2+(int)(13*cz), sy0+(int)(14*cz), (int)(4*cz), (int)(20*cz), COL(50,54,40));
        } else if (s==4) {
            // CASTLE: a little stone tower with a flag, DOS-green
            rect_fill(sx0+sw/2-(int)(11*cz), sy0+(int)(16*cz), (int)(22*cz), (int)(22*cz), COL(96,110,96));
            rect_fill(sx0+sw/2-(int)(13*cz), sy0+(int)(12*cz), (int)(5*cz), (int)(6*cz), COL(96,110,96));
            rect_fill(sx0+sw/2-(int)(3*cz), sy0+(int)(12*cz), (int)(5*cz), (int)(6*cz), COL(96,110,96));
            rect_fill(sx0+sw/2+(int)(8*cz), sy0+(int)(12*cz), (int)(5*cz), (int)(6*cz), COL(96,110,96));
            rect_fill(sx0+sw/2-(int)(1*cz), sy0+(int)(24*cz), (int)(3*cz), (int)(14*cz), COL(30,36,30));
            rect_fill(sx0+sw/2-(int)(1*cz), sy0+(int)(2*cz), (int)(2*cz), (int)(10*cz), COL(200,200,210));
            tri_fill(sx0+sw/2+(int)(1*cz), sy0+(int)(2*cz), sx0+sw/2+(int)(1*cz), sy0+(int)(8*cz), sx0+sw/2+(int)(11*cz), sy0+(int)(5*cz), COL(120,220,130));
        } else if (s==5) {
            // SUPER BUDDY CART: a little kart under a checkered flag
            rect_fill(sx0, sy0+sh-(int)(9*cz), sw, (int)(9*cz), COL(52,52,60));
            rect_fill(sx0, sy0+sh-(int)(5*cz), sw, (int)(2*cz), COL(240,240,240));
            rect_fill(sx0+sw/2-(int)(16*cz), sy0+(int)(18*cz), (int)(32*cz), (int)(13*cz), COL(230,60,50));
            disc_fill(sx0+sw/2-(int)(13*cz), sy0+(int)(20*cz), (int)(4*cz), COL(16,14,24));
            disc_fill(sx0+sw/2+(int)(13*cz), sy0+(int)(20*cz), (int)(4*cz), COL(16,14,24));
            disc_fill(sx0+sw/2-(int)(13*cz), sy0+(int)(32*cz), (int)(4*cz), COL(16,14,24));
            disc_fill(sx0+sw/2+(int)(13*cz), sy0+(int)(32*cz), (int)(4*cz), COL(16,14,24));
            disc_fill(sx0+sw/2, sy0+(int)(22*cz), (int)(6*cz), COL(250,200,50));
            rect_fill(sx0+sw-(int)(10*cz), sy0+(int)(4*cz), (int)(2*cz), (int)(16*cz), COL(200,200,210));
            rect_fill(sx0+sw-(int)(8*cz), sy0+(int)(4*cz), (int)(4*cz), (int)(4*cz), COL(240,240,240));
            rect_fill(sx0+sw-(int)(4*cz), sy0+(int)(4*cz), (int)(4*cz), (int)(4*cz), COL(16,14,24));
            rect_fill(sx0+sw-(int)(8*cz), sy0+(int)(8*cz), (int)(4*cz), (int)(4*cz), COL(16,14,24));
            rect_fill(sx0+sw-(int)(4*cz), sy0+(int)(8*cz), (int)(4*cz), (int)(4*cz), COL(240,240,240));
        } else {
            // WAR!: a gold bit-node, a buddy and a bug squaring off
            rect_fill(sx0, sy0+sh-(int)(7*cz), sw, (int)(7*cz), COL(120,108,86));
            tri_fill(sx0+(int)(16*cz), sy0+(int)(24*cz), sx0+(int)(22*cz), sy0+(int)(10*cz), sx0+(int)(28*cz), sy0+(int)(24*cz), COL(255,205,40));
            disc_fill(sx0+(int)(22*cz), sy0+(int)(22*cz), (int)(3*cz), COL(255,240,180));
            disc_fill(sx0+(int)(42*cz), sy0+(int)(24*cz), (int)(7*cz), COL(216,64,54));
            rect_fill(sx0+(int)(40*cz), sy0+(int)(12*cz), (int)(2*cz), (int)(5*cz), COL(216,64,54));
            rect_fill(sx0+(int)(46*cz), sy0+(int)(12*cz), (int)(2*cz), (int)(5*cz), COL(216,64,54));
            ellipse_fill(sx0+(int)(76*cz), sy0+(int)(24*cz), (int)(10*cz), (int)(7*cz), COL(150,90,200));
            disc_fill(sx0+(int)(70*cz), sy0+(int)(20*cz), (int)(2*cz), COL(255,240,240));
            disc_fill(sx0+(int)(82*cz), sy0+(int)(20*cz), (int)(2*cz), COL(255,240,240));
            rect_fill(sx0+(int)(64*cz), sy0+(int)(30*cz), (int)(3*cz), (int)(6*cz), COL(90,60,120));
            rect_fill(sx0+(int)(88*cz), sy0+(int)(30*cz), (int)(3*cz), (int)(6*cz), COL(90,60,120));
        }
        rect_fill(x0+(int)(8*cz), y0+(int)(76*cz), cw-(int)(16*cz), (int)(14*cz), COL(38,34,56)); rect_line(x0+(int)(8*cz), y0+(int)(76*cz), cw-(int)(16*cz), (int)(14*cz), OUTLINE);
        disc_fill(x0+(int)(24*cz), y0+(int)(83*cz), 3, COL(240,90,80));
        disc_fill(x0+(int)(38*cz), y0+(int)(83*cz), 3, COL(255,220,120));
        rect_fill(x0+cw-(int)(22*cz), y0+(int)(79*cz), (int)(6*cz), (int)(8*cz), COL(200,200,210));
    }
    ui_vignette(clip_x0, clip_y0, clip_x1-clip_x0, clip_y1-clip_y0);
    for (int i=0;i<MAXP;i++) if (G.bud[i].active){
        float by=G.bud[i].y;
        if (!G.bud[i].in_bw && by>ARC_CY-300 && by<ARC_CY+430) draw_buddy(&G.bud[i]);
    }
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].in_arcade) draw_player_sprite(&G.p[i]);
    for (int i=0;i<MAXPART;i++) if (G.parts[i].alive){
        Particle *q=&G.parts[i];
        if (q->y>ARC_CY-300 && q->y<ARC_CY+430) rect_fill(SX(q->x), SY(q->y), q->size, q->size, q->col);
    }
    for (int i=0;i<MAXFLOAT;i++) if (G.floats[i].alive){
        Floater *q=&G.floats[i];
        if (q->y>ARC_CY-300 && q->y<ARC_CY+430) draw_text_center(SX(q->x), SY(q->y), q->text, 1, q->col);
    }
}
static void render_world(void){
    // camera zoom: the bigger your hat, the wider your view of the world
    // (in a split-screen pass the caller owns cam/zoom — cam_lock — and
    // view_pi says whose hat would set the zoom)
    if (!G.cam_lock)
    { Player *me=&G.p[G.view_pi>=0? G.view_pi : 0];
      float zt = zoom_target_of(me);
      if (G.zoom<0.01f) G.zoom=1.0f;
      G.zoom += (zt-G.zoom)*0.045f; }
    { Player *viewp=&G.p[G.view_pi>=0? G.view_pi : 0];
      if (viewp->in_arcade){ draw_arcade(); return; }
      if (viewp->in_mall){ draw_mall(); return; } }
    float cz = G.zoom>0.01f? G.zoom : 1.0f;
    // ground: 64px checker tiles tinted by zone band
    int tlx = (int)(G.camx - (VW/2)/cz) - 64, tly = (int)(G.camy - (VH/2)/cz) - 64;
    int gx0 = tlx/64 - (tlx<0?1:0), gy0 = tly/64 - (tly<0?1:0);
    int gw = (int)(VW/cz/64) + 4, gh = (int)(VH/cz/64) + 4, ts = (int)(64*cz) + 1;
    for (int gy=gy0; gy<gy0+gh; gy++) for (int gx=gx0; gx<gx0+gw; gx++){
        float wx = gx*64+32, wy = gy*64+32;
        float d = fsqrt(wx*wx+wy*wy);
        int chk = ((gx+gy)&1);
        u32 c = col_shade(zone_ground(d, chk), 0.97f + hash_f(gx, gy, 77)*0.05f);
        // region borders dither instead of seaming (Nov 9): within
        // 70px of a boundary a hashed share of tiles already wears the
        // country across the line — half and half right on the line,
        // thinning to nothing at the band's edge
        { int r = region_at(d);
          float e = -1.0f; int other = r;
          if (r < NREGIONS-1 && REGION_EDGE[r] - d < 70.0f){ e = REGION_EDGE[r]; other = r+1; }
          else if (r > 0 && d - REGION_EDGE[r-1] < 70.0f){ e = REGION_EDGE[r-1]; other = r-1; }
          if (e > 0.0f){ float f = 0.5f - (d>e? d-e : e-d)/140.0f;
              if (hash_f(gx, gy, 314) < f)
                  c = col_shade(chk? REGION_GA[other] : REGION_GB[other], 0.97f + hash_f(gx, gy, 77)*0.05f); } }
        int tx = SX(gx*64), tyy = SY(gy*64);
        rect_fill(tx, tyy, ts, ts, c);
        // soft grout seams so the checker reads clean instead of harsh
        rect_fill(tx+64, tyy, 1, 65, col_shade(c, 0.93f));
        rect_fill(tx, tyy+64, 65, 1, col_shade(c, 0.93f));
        // faint deterministic speckles for texture
        float sp = hash_f(gx, gy, 43);
        if (sp>0.45f){
            u32 sc = col_shade(c, 0.90f);
            pset(tx+(int)(sp*57)+3, tyy+(int)(sp*41)+5, sc);
            pset(tx+(int)(sp*29)+11, tyy+(int)(sp*53)+7, sc);
            // regional sparkle: lit traces in the RAM Flats, glitch
            // flecks in the Depths
            int rg = region_at(d);
            if (rg==5 && sp>0.86f) pset(tx+(int)(sp*47)+7, tyy+(int)(sp*37)+9, COL(110,235,255));
            if (rg==7 && sp>0.88f) pset(tx+(int)(sp*43)+9, tyy+(int)(sp*31)+13, COL(255,84,200));
        }
        // sprinkle: deterministic pebbles/tufts, tinted by the region
        float r = hash_f(gx, gy, 42);
        if (r>0.6f){ int px=SX(gx*64+(int)(r*50)), py=SY(gy*64+(int)(r*40));
            u32 tc = REGION_TUFT[region_at(d)];
            line_thick(px, py, px-2, py-4, 1, tc); line_thick(px+3, py, px+4, py-5, 1, tc);
            // the Meadows bloom: two-petal flowers close to home
            if (d<500 && r>0.86f){
                u32 fc = ((gx*7+gy*13)&1)? COL(255,170,200) : COL(255,240,200);
                disc_fill(px+8, py-3, 1, fc); disc_fill(px+10, py-2, 1, fc); pset(px+9, py-2, COL(255,205,40));
            }
        }
    }
    // the Epicenter plaza — the firewall is gone; it's just pavement now,
    // and it shows the wear: scuffs, stains and cracks
    int pcx=SX(0), pcy=SY(0);
    circle_fill(pcx, pcy, 150, COL(196,190,178));
    circle_fill(pcx, pcy, 138, COL(208,201,186));
    circle_fill(pcx, pcy, 96, COL(219,211,193));   // sun-warmed heart of the plaza (glow-up)
    circle_out(pcx, pcy, 121, COL(206,172,92));   // a gold inlay ring, mall money well spent
    circle_out(pcx, pcy, 118, COL(196,190,178));
    ellipse_fill(pcx-52, pcy+38, 30, 10, COL(188,182,168));
    ellipse_fill(pcx+64, pcy-30, 22, 8, COL(190,184,170));
    ellipse_fill(pcx+18, pcy+74, 16, 6, COL(186,180,166));
    line_thick(pcx-96, pcy-58, pcx-70, pcy-38, 1, COL(170,164,152));
    line_thick(pcx-70, pcy-38, pcx-78, pcy-18, 1, COL(170,164,152));
    line_thick(pcx+84, pcy+52, pcx+104, pcy+66, 1, COL(170,164,152));
    draw_text_center(pcx, pcy+118, "THE EPICENTER", 1, COL(120,110,95));
    draw_store_building();
    // the Epicenter shield: a shimmering ring, up while anyone is in the
    // store. When the last shopper leaves it fades out over ~0.8s (visual
    // only — the barrier itself drops the moment the store empties).
    if (G.state==GS_PLAY || G.state==GS_STORE){
        static float shield_seen = -10.0f;
        int show_shield = (G.net_mode==NET_CLIENT)? (shield_remote || store_open_local)
                                                  : shield_active();
        if (show_shield) shield_seen = G.anim_t;
        float fade = show_shield? 1.0f : 1.0f - (G.anim_t - shield_seen)/0.8f;
        if (fade > 0.0f){
            if (fade > 1.0f) fade = 1.0f;
            float pulse = (0.72f + 0.28f*fsin(G.anim_t*3.0f)) * fade;
            for (int ring=0; ring<2; ring++){
                float rr = ring? 146.0f : 152.0f;
                int alpha = (int)((ring? 90:170)*pulse);
                for (float a=0; a<TAU; a+=TAU/110){
                    if (((int)(a*9 - G.anim_t*1.5f))%2==0) continue;   // rotating dashes
                    pset_blend(pcx+(int)(fcos(a)*rr), pcy+(int)(fsin(a)*rr), COL(150,215,255), alpha);
                }
            }
            if (fade > 0.5f)
                for (int k=0;k<4;k++){ // bright nodes drifting around the ring
                    float a = G.anim_t*0.5f + k*(TAU/4);
                    circle_fill(pcx+(int)(fcos(a)*152), pcy+(int)(fsin(a)*152), 2, COL(190,235,255));
                }
        }
    }
    // props
    int cx0=(int)((G.camx-(VW/2)/cz)/CHUNK)-1, cy0=(int)((G.camy-(VH/2)/cz)/CHUNK)-1;
    int cw = (int)(VW/cz/CHUNK) + 3, chh = (int)(VH/cz/CHUNK) + 3;
    for (int cy=cy0; cy<cy0+chh; cy++) for (int cx=cx0; cx<cx0+cw; cx++)
        for (int s=0;s<2;s++){
            float px,py; int t,sol; chunk_prop(cx, cy, s, &px,&py,&t,&sol);
            if (t) draw_prop(t, SX(px), SY(py), hash2(cx,cy,7+(u32)s));
        }
    // walls: a proper barricade of carcasses — shaded mound, five bugs
    // deep, and it visibly falls apart as it gets chewed through
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive){
        Wall *w=&G.walls[i]; int x=SX(w->x), y=SY(w->y);
        if (x<-40||x>VW+40||y<-40||y>VH+40) continue;
        int hw = w->horiz? 26:9, hh = w->horiz? 9:26;
        float frac = w->hp/w->maxhp;
        u32 col = frac>0.5f? COL(168,118,72) : (frac>0.25f? COL(150,100,60) : COL(128,82,50));
        shadow_ellipse(x, y+hh+1, hw+3, 3);
        rect_fill(x-hw-2, y-hh-2, hw*2+4, hh*2+4, OUTLINE);
        rect_fill(x-hw, y-hh, hw*2, hh*2, col_shade(col, 0.82f));            // base in shade
        if (w->horiz) rect_fill(x-hw, y-hh, hw*2, hh, col);                   // lit top half
        else          rect_fill(x-hw, y-hh, hw, hh*2, col);
        rect_fill(x-hw, y-hh, w->horiz? hw*2:2, w->horiz? 2:hh*2, col_shade(col, 1.18f)); // top/left highlight
        // the carcasses themselves: five across (or down), each different,
        // slumping lower as the wall's HP drains away
        int slump = frac>0.5f? 0 : (frac>0.25f? 1:2);
        for (int k=0;k<5;k++){
            float t = (k-2)/2.0f;                          // -1..1 along the wall
            int cx2 = w->horiz? x + (int)(t*(hw-6)) : x + ((k%2)? 2:-2);
            int cy2 = w->horiz? y + ((k%2)? 2:-2) : y + (int)(t*(hh-6));
            int kind = (i*3 + k*7)%4;
            float tilt = ((i*13 + k*29)%10 - 5)/8.0f + (k==4 && frac<0.25f? 0.9f:0.0f); // one slides off when nearly dead
            draw_carcass(cx2, cy2 + slump + (k==4 && frac<0.25f? 3:0), 4 + (k%2), kind, tilt);
        }
        // exposed legs poking out of the mound
        line_thick(x-hw+4, y-hh+1, x-hw+1, y-hh-4, 1, OUTLINE);
        line_thick(x+3, y-hh+1, x+5, y-hh-4, 1, OUTLINE);
        if (w->horiz) line_thick(x+hw-5, y+hh-1, x+hw-2, y+hh+3, 1, OUTLINE);
        // reinforcement made visible (restored Nov 7): every stacked
        // layer piles another course of carcasses on top — the heap
        // grows taller with each reinforcement, capped for the eye
        int pile_h = 0;
        { int st = w->stack>0? w->stack : 1;
          int layers = st-1; if (layers>4) layers=4;
          pile_h = layers*5;
          for (int L=1; L<=layers; L++){
              int ly = y - hh - L*5 + slump;
              int inset = L*2; if (inset>hw-8) inset=hw-8; if (inset<0) inset=0;
              // shaded under-course so the layer reads as sitting ON the heap
              if (w->horiz) rect_fill(x-hw+inset, ly+3, (hw-inset)*2, 3, col_shade(col, 0.55f));
              else          rect_fill(x-4, ly+3, 8, 3, col_shade(col, 0.55f));
              int nk = (L>=3)? 3 : 4;
              for (int k=0;k<nk;k++){
                  float t = (k-(nk-1)/2.0f)/((nk-1)/2.0f);   // -1..1 across the course
                  int cx2 = w->horiz? x + (int)(t*(hw-6-inset)) : x + ((k%2)? 2:-2);
                  int cy2 = w->horiz? ly + ((k%2)? 1:-1) : ly + (int)(t*3);
                  int kind = (i*5 + L*3 + k*7)%4;
                  float tilt = ((i*17 + L*11 + k*29)%10 - 5)/8.0f;
                  draw_carcass(cx2, cy2, 4 + (k%2), kind, tilt);
              }
          }
          if (st>5){ // a proper tower: say how deep the pile goes
              char nb[8]; char *nq=nb; nq=sappend(nq, "x"); nq=sapp_int(nq, st); *nq=0;
              draw_text_center(x, y-hh-pile_h-13, nb, 1, COL(255,230,150));
          }
        }
        // cracks as it gets chewed
        if (frac<0.75f){
            line_thick(x-6, y-hh+2, x-2, y, 1, OUTLINE);
            line_thick(x-2, y, x-5, y+hh-3, 1, OUTLINE);
        }
        if (frac<0.45f){
            line_thick(x+8, y-hh+3, x+4, y+1, 1, OUTLINE);
            line_thick(x+4, y+1, x+7, y+hh-2, 1, OUTLINE);
            line_thick(x-1, y-2, x+3, y+3, 1, OUTLINE);
        }
        if (frac<0.25f) // scorch tint: it's barely holding together
            blend_rect(x-hw, y-hh, hw*2, hh*2, COL(40,26,20), 60);
        // interlock tiers: the bigger the run, the more engineered it
        // looks — and at 5+ you can walk on top of it
        { int tier = (w->grp>=0)? grp_tier(grp_n[w->grp]) : 0;
          if (tier>=1){ // plank cap rail + bolts
              u32 wood2 = COL(140,96,52);
              if (w->horiz){ rect_fill(x-hw, y-hh-3, hw*2, 3, wood2); rect_fill(x-hw, y-hh-3, hw*2, 1, COL(172,128,76)); }
              else { rect_fill(x-3, y-hh, 3, hh*2, wood2); rect_fill(x-3, y-hh, 1, hh*2, COL(172,128,76)); }
              disc_fill(x-hw+4, y, 1, OUTLINE); disc_fill(x+hw-4, y, 1, OUTLINE);
          }
          if (tier>=2){ // iron straps with rivets
              u32 iron = COL(74,74,88);
              if (w->horiz){ for (int s2=-1;s2<=1;s2+=2){ int sx2=x+s2*hw/2; rect_fill(sx2-1, y-hh, 3, hh*2, iron); disc_fill(sx2, y-hh+3, 1, COL(150,150,165)); disc_fill(sx2, y+hh-3, 1, COL(150,150,165)); } }
              else { for (int s2=-1;s2<=1;s2+=2){ int sy2=y+s2*hh/2; rect_fill(x-hw, sy2-1, hw*2, 3, iron); disc_fill(x-hw+3, sy2, 1, COL(150,150,165)); disc_fill(x+hw-3, sy2, 1, COL(150,150,165)); } }
          }
          if (tier>=3){ // CASTLE WALL (Daniel): at 20+ segments the run is
              // faced in stone — block courses, joints, and battlements.
              u32 stone = COL(148,146,142), stone_hi = COL(172,170,164), stone_lo = COL(118,116,112);
              rect_fill(x-hw, y-hh, hw*2, hh*2, stone);
              if (w->horiz) rect_fill(x-hw, y-hh, hw*2, hh, stone_hi);
              else          rect_fill(x-hw, y-hh, hw, hh*2, stone_hi);
              rect_line(x-hw, y-hh, hw*2, hh*2, OUTLINE);
              // block courses: a mid seam plus staggered joints
              if (w->horiz){
                  rect_fill(x-hw, y-1, hw*2, 1, stone_lo);
                  for (int jx=-hw+9; jx<hw-2; jx+=13) rect_fill(x+jx, y-hh, 1, hh, stone_lo);
                  for (int jx=-hw+15; jx<hw-2; jx+=13) rect_fill(x+jx, y, 1, hh, stone_lo);
              } else {
                  rect_fill(x-1, y-hh, 1, hh*2, stone_lo);
                  for (int jy=-hh+9; jy<hh-2; jy+=13) rect_fill(x-hw, y+jy, hw, 1, stone_lo);
                  for (int jy=-hh+15; jy<hh-2; jy+=13) rect_fill(x, y+jy, hw, 1, stone_lo);
              }
              // battlements: stone teeth along the outer edge
              if (w->horiz){ for (int tx2=x-hw+2; tx2<x+hw-5; tx2+=11){
                  rect_fill(tx2, y-hh-5, 7, 5, stone); rect_fill(tx2, y-hh-5, 7, 1, stone_hi);
                  rect_line(tx2, y-hh-5, 7, 5, OUTLINE); } }
              else { for (int ty2=y-hh+2; ty2<y+hh-5; ty2+=11){
                  rect_fill(x-hw-5, ty2, 5, 7, stone); rect_fill(x-hw-5, ty2, 1, 7, stone_hi);
                  rect_line(x-hw-5, ty2, 5, 7, OUTLINE); } }
              if (frac<0.6f){ // even castles crack
                  line_thick(x-5, y-hh+2, x-1, y+1, 1, OUTLINE);
                  line_thick(x+6, y, x+3, y+hh-2, 1, OUTLINE);
              }
          }
          if (tier>=4){ // GRAND CASTLE: taller teeth, a cap course, gold trim
              u32 stone = COL(148,146,142), stone_hi = COL(178,176,170);
              if (w->horiz){ for (int tx2=x-hw+2; tx2<x+hw-5; tx2+=11) rect_fill(tx2, y-hh-7, 7, 2, stone_hi);
                             rect_fill(x-hw-2, y+hh, hw*2+4, 2, stone); }
              else { for (int ty2=y-hh+2; ty2<y+hh-5; ty2+=11) rect_fill(x-hw-7, ty2, 2, 7, stone_hi);
                     rect_fill(x+hw, y-hh-2, 2, hh*2+4, stone); }
              rect_line(x-hw-3, y-hh-3, hw*2+6, hh*2+6, COL(255,205,40));
          }
          if (tier>=1 && w->endpt){ // step stones at the end of a climbable run + a pennant
              int ndx=0, ndy=0;
              for (int j=0;j<MAXWALLS;j++) if (j!=i && G.walls[j].alive==1 && G.walls[j].grp==w->grp && wall_adjacent(w, &G.walls[j])){
                  ndx = G.walls[j].x > w->x+2? 1 : (G.walls[j].x < w->x-2? -1:0);
                  ndy = G.walls[j].y > w->y+2? 1 : (G.walls[j].y < w->y-2? -1:0);
                  break;
              }
              int sxp = x - ndx*(hw+5), syp = y - ndy*(hh+5);
              rect_fill(sxp-5, syp+1, 10, 5, COL(150,146,140)); rect_line(sxp-5, syp+1, 10, 5, OUTLINE);
              rect_fill(sxp-3, syp-3, 6, 4, COL(168,164,158)); rect_line(sxp-3, syp-3, 6, 4, OUTLINE);
              if (tier>=4){ // a round stone turret caps the run's end,
                  // and the pennant flies from its top
                  circle_fill(x, y, 11, OUTLINE);
                  circle_fill(x, y, 10, COL(148,146,142));
                  circle_fill(x-3, y-3, 3, COL(172,170,164));
                  for (int ts=0; ts<6; ts++){ float ta=ts*1.0472f;
                      disc_fill(x + (int)(fcos(ta)*9), y + (int)(fsin(ta)*9), 1, COL(118,116,112)); }
                  line_thick(x, y-9, x, y-22, 1, OUTLINE);
                  tri_fill(x, y-22, x+8, y-19, x, y-16, (i%2)? COL(220,70,70) : COL(255,205,40));
              }
          }
        }
        if (w->alive==2) draw_text_center(x, y-16, "BUILDING...", 1, COL(255,240,180));
        if (frac<0.99f){ rect_fill(x-14, y-hh-pile_h-8, 28, 3, COL(40,30,40));
            rect_fill(x-14, y-hh-pile_h-8, (int)(28*frac), 3, COL(255,170,80)); }
    }
    // bugs
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive) draw_bug_sprite(&G.bugs[i]);
    // Banzai Buddies trot with their owners
    for (int i=0;i<MAXP;i++) if (G.bud[i].active && !G.bud[i].in_bw) draw_buddy(&G.bud[i]);
    // players
    for (int i=0;i<MAXP;i++) if (G.p[i].connected) draw_player_sprite(&G.p[i]);
    // projectiles
    for (int i=0;i<MAXPROJ;i++) if (G.projs[i].alive){
        Proj *q=&G.projs[i]; int x=SX(q->x), y=SY(q->y);
        if (x<-20||x>VW+20||y<-20||y>VH+20) continue;
        if (q->kind==0){ rect_fill(x-3, y-2, 6, 4, OUTLINE); rect_fill(x-2, y-1, 4, 2, COL(230,230,240)); }
        else if (q->kind==1){ ellipse_out(x, y, 5, 4, COL(180,180,200)); }
        else if (q->kind==2){ circle_out(x, y, 5, COL(255,215,60)); circle_fill(x+3, y-3, 2, COL(255,150,40)); }
        else { // a tiny project manager, tumbling through the air, briefcase first
            circle_out(x, y-3, 4, COL(245,205,160));
            rect_fill(x-3, y+1, 7, 6, COL(50,50,90));
            rect_fill(x+3, y+2, 4, 3, COL(120,80,40));
        }
    }
    // carcass flights
    for (int i=0;i<(int)(sizeof(G.fx)/sizeof(G.fx[0]));i++) if (G.fx[i].alive){
        CarcassFx *f=&G.fx[i];
        float t=f->t; if (t>1) t=1;
        float mx=(f->x0+f->x1)/2, my=(f->y0+f->y1)/2 - 70;
        float ix=(1-t)*(1-t)*f->x0 + 2*(1-t)*t*mx + t*t*f->x1;
        float iy=(1-t)*(1-t)*f->y0 + 2*(1-t)*t*my + t*t*f->y1;
        draw_carcass(SX(ix), SY(iy), 5, (int)f->w%4, t*5);
    }
    ui_vignette(clip_x0, clip_y0, clip_x1-clip_x0, clip_y1-clip_y0);
    // particles + floaters
    for (int i=0;i<MAXPART;i++) if (G.parts[i].alive){
        Particle *q=&G.parts[i];
        rect_fill(SX(q->x), SY(q->y), q->size, q->size, q->col);
    }
    for (int i=0;i<MAXFLOAT;i++) if (G.floats[i].alive){
        Floater *q=&G.floats[i];
        draw_text_center(SX(q->x), SY(q->y), q->text, 1, q->col);
    }
}

// ---------------------------------------------------------------------------
// HUD, leaderboard, store UI
// ---------------------------------------------------------------------------
static void draw_bar(int x, int y, int w, int h, float frac, u32 col){
    rect_fill(x-1, y-1, w+2, h+2, OUTLINE);
    rect_fill(x, y, w, h, COL(46,40,58));
    if (frac>0){ int fw = (int)(w*fclamp(frac,0,1));
        rect_fill(x, y, fw, h, col);
        if (h>=4) blend_rect(x, y, fw, h/2, COL(255,255,255), 42);   // gloss band (glow-up)
    }
}
// pretty HUD panel: soft shadow, slate body, pale top light, gold corners
static void ui_panel(int x, int y, int w, int h){
    rect_fill(x+3, y+4, w, h, COL(10,9,18));
    rect_fill(x, y, w, h, COL(24,22,38));
    rect_line(x, y, w, h, COL(104,98,138));
    rect_fill(x+1, y+1, w-2, 1, COL(92,86,122));
    u32 g = COL(255,205,40);
    rect_fill(x, y, 9, 2, g);         rect_fill(x, y, 2, 9, g);
    rect_fill(x+w-9, y, 9, 2, g);     rect_fill(x+w-2, y, 2, 9, g);
    rect_fill(x, y+h-2, 9, 2, g);     rect_fill(x, y+h-9, 2, 9, g);
    rect_fill(x+w-9, y+h-2, 9, 2, g); rect_fill(x+w-2, y+h-9, 2, 9, g);
}
// soft vignette for world views (Nov 5 glow-up): the edges of the
// view fall off into a gentle shade so the world feels lit instead of
// flat. Drawn AFTER the world and its entities but BEFORE any HUD or
// panel, so text and bars stay at full brightness — legibility first.
static void ui_vignette(int x0, int y0, int w, int h){
    const int D = 15;
    for (int i=0;i<D;i++){
        int a = (D-i)*3;
        blend_rect(x0, y0+i, w, 1, COL(6,6,14), a);
        blend_rect(x0, y0+h-1-i, w, 1, COL(6,6,14), a);
        blend_rect(x0+i, y0, 1, h, COL(6,6,14), a);
        blend_rect(x0+w-1-i, y0, 1, h, COL(6,6,14), a);
    }
}
// chunky touch button base: drop shadow, cartoon outline, shaded face
static void tbtn_base(int cx, int cy, int r, u32 face){
    disc_fill(cx+2, cy+3, r+2, COL(10,9,18));
    circle_out(cx, cy, r, face);
    circle_shade(cx, cy, r-3, face);
}
// little item glyphs (store rows + weapon chips), drawn in a ~18px box
static void draw_item_icon(int sel, int cx, int cy){
    u32 ink = COL(40,36,50);
    switch (sel){
    case 0:  // mechanical keyboard
        rect_fill(cx-9, cy-5, 18, 11, COL(240,238,228)); rect_line(cx-9, cy-5, 18, 11, ink);
        for (int i=0;i<3;i++){ disc_fill(cx-4+i*4, cy-1, 1, ink); disc_fill(cx-4+i*4, cy+3, 1, ink); }
        break;
    case 1:  // mouse tosser
        ellipse_fill(cx, cy+1, 7, 9, ink); ellipse_fill(cx, cy+1, 6, 8, COL(240,238,228));
        rect_fill(cx, cy-6, 1, 6, ink);
        break;
    case 2:  // rubber duck cannon
        ellipse_fill(cx+1, cy+3, 9, 5, COL(246,204,64));
        disc_fill(cx-4, cy-3, 4, COL(246,204,64));
        tri_fill(cx-10, cy-4, cx-5, cy-5, cx-5, cy-1, COL(232,128,44));
        disc_fill(cx-5, cy-4, 1, ink);
        break;
    case 3:  // PM launcher: a little rocket
        tri_fill(cx, cy-10, cx-3, cy-4, cx+3, cy-4, COL(198,60,50));
        rect_fill(cx-3, cy-4, 7, 10, COL(226,230,238)); rect_line(cx-3, cy-4, 7, 10, ink);
        tri_fill(cx-3, cy+6, cx-7, cy+10, cx-3, cy+10, COL(198,60,50));
        tri_fill(cx+4, cy+6, cx+4, cy+10, cx+7, cy+10, COL(198,60,50));
        disc_fill(cx, cy+9, 2, COL(255,170,60));
        break;
    case 4:  // the debugger: bug in crosshairs
        for (int s=-1;s<=1;s+=2){ rect_fill(cx+s*9-1, cy-1, 3, 2, COL(216,70,60)); rect_fill(cx-1, cy+s*9-1, 2, 3, COL(216,70,60)); }
        disc_fill(cx, cy, 6, ink); disc_fill(cx, cy, 5, COL(98,192,92));
        disc_fill(cx-2, cy-1, 1, ink); disc_fill(cx+2, cy-1, 1, ink);
        break;
    case 5:  // overclock: lightning bolt
        tri_fill(cx+3, cy-9, cx-4, cy+2, cx+0, cy+2, COL(255,205,40));
        tri_fill(cx+0, cy+2, cx-3, cy+9, cx+5, cy-2, COL(255,205,40));
        break;
    case 6:  // caffeine drip: coffee cup
        rect_fill(cx-6, cy-2, 11, 9, COL(240,238,228)); rect_line(cx-6, cy-2, 11, 9, ink);
        rect_fill(cx-6, cy-2, 11, 3, COL(120,72,40));
        rect_line(cx+5, cy+0, 4, 5, ink);
        break;
    case 7:  // firewall shield
        rect_fill(cx-6, cy-8, 13, 10, COL(110,190,255));
        tri_fill(cx-6, cy+2, cx+7, cy+2, cx, cy+10, COL(110,190,255));
        rect_fill(cx-1, cy-5, 3, 9, COL(226,240,255));
        break;
    case 8:  // hoodie armor: a heart
        disc_fill(cx-3, cy-3, 4, COL(226,80,90)); disc_fill(cx+4, cy-3, 4, COL(226,80,90));
        tri_fill(cx-7, cy-2, cx+8, cy-2, cx, cy+9, COL(226,80,90));
        break;
    case 9:  // espresso legs: speed chevrons
        tri_fill(cx-7, cy-5, cx-7, cy+5, cx-1, cy, COL(110,220,235));
        tri_fill(cx+1, cy-5, cx+1, cy+5, cx+7, cy, COL(110,220,235));
        break;
    case 10: // rollback token: circular arrow
        for (int a=0;a<10;a++){ float t = -0.6f + a*0.58f;
            disc_fill(cx+(int)(fcos(t)*6.5f), cy+(int)(fsin(t)*6.5f), 2, COL(110,220,140)); }
        tri_fill(cx+5, cy-9, cx+10, cy-4, cx+2, cy-3, COL(110,220,140));
        break;
    case 11: // hotfix: medical cross
        disc_fill(cx, cy, 9, COL(240,238,228));
        rect_fill(cx-2, cy-6, 5, 12, COL(216,70,60)); rect_fill(cx-6, cy-2, 12, 5, COL(216,70,60));
        break;
    default: // THE BACKFIRE BEAN: a green fellow, goggles on, ready
        ellipse_fill(cx, cy+2, 7, 8, COL(126,196,86));
        ellipse_fill(cx-2, cy-1, 2, 3, COL(168,226,120));
        rect_fill(cx-7, cy-5, 14, 3, COL(90,70,50));
        disc_fill(cx-3, cy-3, 2, COL(220,240,255)); disc_fill(cx+3, cy-3, 2, COL(220,240,255));
        pset(cx-3, cy-3, ink); pset(cx+3, cy-3, ink);
        disc_fill(cx-6, cy+9, 2, COL(190,225,150)); disc_fill(cx+6, cy+10, 2, COL(168,216,120));
        break;
    }
}
// weapon chips: a vertical strip under the status card, one chip per
// owned weapon (only shown once you own 2+). Tappable on touch.
static int owned_count(Player *me){ int n=0; for (int w=0;w<NWEAP;w++) if (me->owned[w]) n++; return n; }
static int chip_rect(Player *me, int w, int *x, int *y){
    int idx=0, found=0;
    for (int i=0;i<NWEAP;i++) if (me->owned[i]){ if (i==w){ *x=8; *y=94+idx*32; found=1; } idx++; }
    return (idx>=2 && found)? 1 : 0;
}
static int chip_at(Player *me, int tx, int ty){
    for (int w=0;w<NWEAP;w++){ int x,y; if (chip_rect(me,w,&x,&y) && tx>=x && tx<x+52 && ty>=y && ty<y+26) return w; }
    return -1;
}
static void render_leaderboard(int x, int y){
    // CONSTANT LEADERBOARD: heaviest hat wins. Always on screen.
    int idx[MAXP], n=0;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected) idx[n++]=i;
    for (int i=0;i<n;i++) for (int j=i+1;j<n;j++)
        if (G.p[idx[j]].carried > G.p[idx[i]].carried){ int t=idx[i]; idx[i]=idx[j]; idx[j]=t; }
    int wpx=150, hpx=26+n*11+14;
    ui_panel(x, y, wpx, hpx);
    draw_text(x+8, y+5, "HEAVIEST HATS", 1, COL(255,205,40));
    for (int i=0;i<n;i++){
        Player *p=&G.p[idx[i]];
        char b[32]; char *q=b;
        q=sapp_int(q, i+1); q=sappend(q, ". ");
        { int ni=0; while (p->name[ni] && ni<8){ *q++=p->name[ni++]; } *q=0; }  // truncated: the KG column must never be overrun
        draw_text(x+8, y+16+i*11, b, 1, i==0? COL(255,240,180):COL(210,210,220));
        fmt_int(b, "", (int)p->carried, "KG");
        draw_text(x+wpx-8-text_w(b,1), y+16+i*11, b, 1, COL(255,240,180));
        if (!p->alive) draw_text(x+wpx-30, y+16+i*11, "DOWN", 1, COL(255,110,110));
    }
    char b[40]; char *q=b;
    q=sappend(q, "BEST: "); q=sappend(q, G.rec_name); q=sappend(q, " ");
    q=sapp_int(q, (int)G.rec_hat); q=sappend(q, "KG");
    draw_text_fit(x+8, y+16+n*11+2, wpx-16, b, 1, COL(150,200,255));
}
// the way-back arrow: orbits (cx, cy) — normally the player's screen
// position — pointing home to the Epicenter, distance underneath.
// Hidden once you're close. Drawn per view in split-screen co-op so
// BOTH players get their own.
static void draw_home_arrow(Player *p, int cx, int cy){
    if (p->in_mall) return;              // inside the mall you can see the door
    float dx = -p->x, dy = -p->y; float d = fsqrt(dx*dx + dy*dy);
    if (d <= 190.0f) return;
    float ca = dx/d, sa = dy/d;
    int ax = cx + (int)(ca*46.0f), ay = cy + (int)(sa*46.0f);
    int tx = ax + (int)(ca*13), ty = ay + (int)(sa*13);
    int bx = ax - (int)(ca*7), by = ay - (int)(sa*7);
    int px = (int)(-sa*9), py = (int)(ca*9);
    tri_fill(tx, ty, bx+px, by+py, bx-px, by-py, COL(30,26,44));
    int t2x = ax + (int)(ca*10), t2y = ay + (int)(sa*10);
    int b2x = ax - (int)(ca*5), b2y = ay - (int)(sa*5);
    int qx = (int)(-sa*6), qy = (int)(ca*6);
    tri_fill(t2x, t2y, b2x+qx, b2y+qy, b2x-qx, b2y-qy, COL(255,214,64));
    char b[24]; char *q=b; q=sapp_int(q, (int)(d/50)); q=sappend(q, "M");
    draw_text_center(ax, ay+17, b, 1, COL(255,240,170));
}
static void render_hud(void){
    Player *me=&G.p[G.my_id];
    // top-left status card (wide enough for the longest hat tier name)
    ui_panel(8, 8, 224, 78);
    draw_bar(14, 14, 120, 8, (float)me->hp/me->maxhp, COL(120,230,90));
    { char b[24]; fmt_int(b, "HP ", me->hp, ""); draw_text(140, 14, b, 1, COL(255,255,255)); }
    if (me->shield_max>0) draw_bar(14, 26, 120, 6, me->shield_max? (float)me->shield/me->shield_max:0, COL(110,190,255));
    { char b[40]; char *q=b; q=sappend(q, WEAPON_NAME[me->weapon]);
      if (me->owned[me->weapon]>1){ q=sappend(q, " X"); q=sapp_int(q, me->owned[me->weapon]); }
      draw_text_fit(14, 38, 204, b, 1, COL(255,220,120)); }
    { char b[24]; char *q=b;
      q=sappend(q, "HAT: "); q=sapp_int(q, (int)me->carried); q=sappend(q, "KG");
      draw_text(14, 50, b, 1, COL(255,240,180)); }
    draw_text_fit(14, 61, 134, hat_tier_name(me->stack_n, me->carried), 1, COL(190,180,150));
    if (me->rollback>0){   // held revive tokens: know you've got one
        char b[20]; char *q=b; q=sappend(q, "TOKEN X"); q=sapp_int(q, me->rollback); *q=0;
        draw_text(226-text_w(b,1), 61, b, 1, COL(120,255,160));
    }
    if (me->beans>0){   // Backfire Beans held: the ride home (tap it, or key 6)
        char b[20]; char *q=b; q=sappend(q, "BEAN X"); q=sapp_int(q, me->beans); *q=0;
        draw_text(226-text_w(b,1), 70, b, 1, COL(150,230,120));
    }
    draw_bar(14, 72, 120, 4, me->carried/200.0f>1?1:me->carried/200.0f, COL(255,190,80));
    // weapon chips under the card: tap one to switch (touch), or use the
    // number keys. Gold frame = what you're holding; XN = stacked copies.
    for (int w=0;w<NWEAP;w++){ int cx, cy;
        if (!chip_rect(me, w, &cx, &cy)) continue;
        int cur = (me->weapon==w);
        rect_fill(cx+2, cy+3, 52, 26, COL(10,9,18));
        rect_fill(cx, cy, 52, 26, cur? COL(66,56,28) : COL(24,22,38));
        rect_line(cx, cy, 52, 26, cur? COL(255,205,40) : COL(104,98,138));
        draw_item_icon(w, cx+13, cy+13);
        if (me->owned[w]>1){ char b[8]; char *q=b; q=sappend(q, "X"); q=sapp_int(q, me->owned[w]);
            draw_text(cx+25, cy+9, b, 1, COL(255,240,200)); }
        else { char b[8]; char *q=b; q=sapp_int(q, w+1);
            draw_text(cx+25, cy+9, b, 1, COL(165,160,185)); }
    }
    // top-center: zone + distance, centered in the free band between the
    // status card (ends x=232) and the leaderboard (starts x=482) so long
    // zone names can never run into either panel
    float d = fsqrt(me->x*me->x + me->y*me->y);
    rect_fill(254, 5, 214, 46, COL(16,14,28));          // readability pill
    rect_line(254, 5, 214, 46, COL(84,78,112));
    if (me->in_mall && me->in_arcade){
        draw_text_center(361, 7, "THE ARCADE", 1, COL(255,255,255));
        draw_text_center(361, 19, "SEVEN FINE GAMES", 1, COL(200,210,230));
        draw_text_center(361, 31, "SAFE ZONE - NO BUGS", 1, COL(150,230,150));
    } else if (me->in_mall){
        draw_text_center(361, 7, "THE BUG MALL", 1, COL(255,255,255));
        draw_text_center(361, 19, "FIVE FINE SHOPS", 1, COL(200,210,230));
        draw_text_center(361, 31, "SAFE ZONE - NO BUGS", 1, COL(150,230,150));
    } else {
    draw_text_center_fit(361, 7, 200, zone_name(d), 1, COL(255,255,255));
    { char b[32]; char *q=b; q=sapp_int(q, (int)(d/50)); q=sappend(q, "M FROM EPICENTER");
      draw_text_center(361, 19, b, 1, COL(200,210,230)); }
    { compute_threat();
      char b[40]; char *q=b; q=sappend(q, "THREAT X"); q=sapp_int(q, (int)G.threat);
      *q++='.'; *q++=(char)('0'+((int)(G.threat*10)%10)); *q=0;
      if (G.zen_run) q=sappend(q, " - ZEN MODE");
      else if (G.diff_run) { q=sappend(q, " - "); q=sappend(q, DIFF_NAME[G.diff_run&3]); }
      draw_text_center_fit(361, 31, 200, b, 1, COL(255,150,90)); }
    }
    render_leaderboard(VW-158, 8);
    // bottom prompts (key names follow the player's bindings)
    { char pb[64]; const char *pr = interact_prompt(me, pb, 0);
      if (pr) draw_text_center(VW/2, VH-34, pr, 1, COL(255,240,150));
      else if (me->stack_n>=3 && !me->in_mall)
        { char b[64]; char *q=b; if (pad_mode()) q=sappend(q, "X"); else if (mobile_ui()) q=sappend(q, "WALL"); else q=sappend(q, key_name(G.bind[ACT_WALL])); q=sappend(q, " - THROW DOWN 3 CARCASSES AS A WALL");
          draw_text_center(VW/2, VH-34, b, 1, COL(220,220,200)); } }
    // (no bottom control line any more — controls live in the pause
    // menu's CONTROLS panel now; Daniel, Nov 6: the line overlapped)
    // run stats live under the player panel now, not at the bottom
    // of the screen (Daniel, Nov 6: the bottom stays for prompts)
    { char b[40]; char *q=b; int t=(int)G.time;
      q=sappend(q, "TIME "); q=sapp_int(q, t/60); q=sappend(q, ":");
      if (t%60<10) q=sappend(q, "0"); q=sapp_int(q, t%60);
      q=sappend(q, " - SQUASHED: "); q=sapp_int(q, me->kills);
      draw_text(14, 92, b, 1, COL(190,195,210)); }
    // way back: a gold arrow orbiting the player, pointing at the
    // Epicenter, with the distance under it (hidden once you're close)
    draw_home_arrow(me, VW/2, VH/2);
    // a partner is down: a red arrow points at the body, however far
    // it is — getting to them is the whole game now. The viewer is
    // whoever is still standing (in co-op both players watch the
    // same merged screen, centered on the rescuer).
    { Player *viewer = me->alive? me : 0;
      if (!viewer) for (int j=0;j<MAXP;j++) if (G.p[j].connected && G.p[j].alive){ viewer=&G.p[j]; break; }
      if (viewer) for (int j=0;j<MAXP;j++){ Player *dp=&G.p[j];
        if (dp==viewer || !dp->connected || dp->alive) continue;
        float dx = dp->x-viewer->x, dy = dp->y-viewer->y; float d = fsqrt(dx*dx+dy*dy);
        if (d > 60.0f){
            float ca = dx/d, sa = dy/d;
            int cx2 = VW/2, cy2 = VH/2;
            // a wider orbit than the gold home arrow (46), so the two
            // never sit on top of each other when a partner went down
            // near the Epicenter — exactly where rescues point home
            int ax = cx2 + (int)(ca*66.0f), ay = cy2 + (int)(sa*66.0f);
            int tx = ax + (int)(ca*13), ty = ay + (int)(sa*13);
            int bx = ax - (int)(ca*7), by = ay - (int)(sa*7);
            int px = (int)(-sa*9), py = (int)(ca*9);
            tri_fill(tx, ty, bx+px, by+py, bx-px, by-py, COL(30,26,44));
            int t2x = ax + (int)(ca*10), t2y = ay + (int)(sa*10);
            int b2x = ax - (int)(ca*5), b2y = ay - (int)(sa*5);
            int qx = (int)(-sa*6), qy = (int)(ca*6);
            tri_fill(t2x, t2y, b2x+qx, b2y+qy, b2x-qx, b2y-qy, COL(255,70,60));
            char b[40]; char *s=b; s=sappend(s, dp->name); s=sappend(s, " DOWN - ");
            s=sapp_int(s, (int)(d/50)); s=sappend(s, "M"); *s=0;
            draw_text_center(ax, ay+17, b, 1, COL(255,150,140));
        }
        break;
      } }
    // WALL button (both touch schemes): throws down 3 carcasses as a wall.
    // It pulses once you're carrying enough to build one.
    if (mobile_ui() && !pad_mode()){
        int pulse = (me->stack_n>=3) && (fsin(G.anim_t*4.0f)>0.0f);
        tbtn_base(TP_WALL_CX, TP_WALL_CY, 24, pulse? COL(158,128,54) : COL(62,56,86));
        u32 br = COL(232,186,108);   // little brick wall glyph
        rect_fill(TP_WALL_CX-10, TP_WALL_CY-11, 9, 4, br); rect_fill(TP_WALL_CX+1, TP_WALL_CY-11, 9, 4, br);
        rect_fill(TP_WALL_CX-10, TP_WALL_CY-6, 6, 4, br);  rect_fill(TP_WALL_CX-3, TP_WALL_CY-6, 6, 4, br);
        rect_fill(TP_WALL_CX+4, TP_WALL_CY-6, 6, 4, br);
        draw_text(TP_WALL_CX-text_w("WALL",1)/2, TP_WALL_CY+4, "WALL", 1, COL(255,244,200));
    }
    // drag scheme still needs a weapon cycler (stick mode draws its own below)
    if (G.touch_ctl==0 && mobile_ui()){
        tbtn_base(TP_WPN_CX, TP_WPN_CY, 18, COL(66,60,96));
        draw_text(TP_WPN_CX-text_w("WPN",1)/2, TP_WPN_CY-4, "WPN", 1, COL(225,230,245));
    }
    // stick controls (touch scheme 1): floating joystick + fixed buttons
    if (G.touch_ctl==1 && mobile_ui()){
        tbtn_base(TP_FIRE_CX, TP_FIRE_CY, 34, (IN.tp_held&32)? COL(224,88,62):COL(158,54,46));
        { u32 lc = COL(255,240,230); int gy = TP_FIRE_CY-6;   // crosshair glyph
          rect_fill(TP_FIRE_CX-1, gy-10, 3, 6, lc); rect_fill(TP_FIRE_CX-1, gy+5, 3, 6, lc);
          rect_fill(TP_FIRE_CX-10, gy-1, 6, 3, lc); rect_fill(TP_FIRE_CX+5, gy-1, 6, 3, lc);
          disc_fill(TP_FIRE_CX, gy, 2, lc); }
        draw_text(TP_FIRE_CX-text_w("FIRE",1)/2, TP_FIRE_CY+14, "FIRE", 1, COL(255,240,230));
        tbtn_base(TP_ACT_CX, TP_ACT_CY, 22, COL(56,92,62));
        { u32 lc = COL(230,255,235); int gy = TP_ACT_CY-3;    // up-arrow glyph
          tri_fill(TP_ACT_CX, gy-8, TP_ACT_CX-6, gy+1, TP_ACT_CX+6, gy+1, lc);
          rect_fill(TP_ACT_CX-2, gy+1, 5, 5, lc); }
        draw_text(TP_ACT_CX-text_w("USE",1)/2, TP_ACT_CY+9, "USE", 1, COL(230,255,235));
        tbtn_base(TP_WPN_CX, TP_WPN_CY, 18, COL(66,60,96));
        draw_text(TP_WPN_CX-text_w("WPN",1)/2, TP_WPN_CY-4, "WPN", 1, COL(225,230,245));
        if (IN.joy_on){
            circle_out(IN.joy_ax, IN.joy_ay, 42, COL(34,31,52));
            circle_shade(IN.joy_ax, IN.joy_ay, 39, COL(40,36,58));
            disc_fill(IN.joy_ax, IN.joy_ay, 25, COL(30,27,46));
            circle_out(IN.joy_kx, IN.joy_ky, 17, COL(198,204,220));
            circle_shade(IN.joy_kx, IN.joy_ky, 14, COL(198,204,220));
            disc_fill(IN.joy_kx-5, IN.joy_ky-5, 4, COL(244,246,250));
        } else {
            // faint home base so players know where the stick lives
            disc_fill(TP_DPAD_CX, TP_DPAD_CY, 42, COL(24,22,38));
            circle_out(TP_DPAD_CX, TP_DPAD_CY, 15, COL(52,48,72));
        }
    }
    // damage vignette
    if (me->hurt_t>0){
        rect_fill(0,0,VW,3,COL(230,60,60)); rect_fill(0,VH-3,VW,3,COL(230,60,60));
        rect_fill(0,0,3,VH,COL(230,60,60)); rect_fill(VW-3,0,3,VH,COL(230,60,60));
    }
}
// store layout shared by render + input — rows are tall and the text is
// double-size so phone fingers can hit them
#define STORE_X 70
#define STORE_Y 4
#define STORE_W 500
#define STORE_H 352
#define STORE_ROW0 54
#define STORE_ROWH 18
#define STORE_UPGOFF 18
#define STORE_ROWS 13
static int store_item_price(int sel, Player *p, int *owned_out){
    // sel: 0..4 weapons, 5..12 upgrades; returns price or -1 (owned/maxed)
    *owned_out=0;
    if (sel<5){
        if (p->owned[sel]>=MAXCOPIES){ *owned_out=1; return -1; }
        // stacking escalates: every copy costs more than the one before
        // (copy N costs N x the base price; the free starter keyboard
        // escalates from a 10 KG unit so its stack isn't free either)
        int unit = WEAPON_PRICE[sel]? WEAPON_PRICE[sel] : 10;
        return unit * (p->owned[sel]+1);
    }
    int u=sel-5;
    if (u==5){ return UPG_PRICE[5][0]; }                  // rollback: always buyable
    if (u==7){ return UPG_PRICE[7][0]; }                  // THE BACKFIRE BEAN: always buyable, never cheap
    if (u==6){                                            // hotfix: heal, unless full
        if (p->hp>=p->maxhp){ *owned_out=1; return -1; }
        return UPG_PRICE[6][0];
    }
    if (p->upg[u]>=UPG_MAXLV[u]){ *owned_out=1; return -1; }
    return upg_price(u, p->upg[u]);
}
static const char *store_item_name(int sel){
    return sel<5? WEAPON_NAME[sel] : UPG_NAME[sel-5];
}
static const char *store_item_blurb(int sel, Player *p){
    if (sel<5){
        static char bb[96];
        if (p->owned[sel]>=MAXCOPIES){ char *q=bb; q=sappend(q, "FULLY STACKED - MAXIMUM FAN!"); *q=0; return bb; }
        if (p->owned[sel]>0){
            int ow2; int nx = store_item_price(sel, p, &ow2);
            char *q=bb; q=sappend(q, "STACKED X"); q=sapp_int(q, p->owned[sel]);
            q=sappend(q, sel==4? " - BEAMS RING OUT - NEXT COPY " : " - SHOTS FAN OUT - NEXT COPY ");
            q=sapp_int(q, nx); q=sappend(q, " KG");
            *q=0; return bb;
        }
        return WEAPON_BLURB[sel];
    }
    switch (sel-5){
        case 0: return "All weapons hit 25% harder per level.";
        case 1: return "All weapons fire 18% faster per level.";
        case 2: return "A firewall bubble eats damage. Regenerates.";
        case 3: return "+25 max HP per level. Comfy AND protective.";
        case 4: return "Heavy hats slow you less per level. A bit quicker bare, too.";
        case 5: return "Death? Rolled back. Revive at half HP.";
        case 7: return "Squeeze him: PBBBT - straight home to the Epicenter. (KEY 6 / TAP)";
        default: return "A hotfix, applied directly to you. +50 HP.";
    }
}
// the Bug Bounty panel for whoever is shopping (G.store_pi). Solo it
// covers the screen over a dim; in split-screen co-op it squeezes into
// the shopper's half (ui_compact) with the living world — shield and
// all — still visible around it.
static void draw_store_panel(void){
    Player *me=&G.p[G.store_pi];
    int sel = (G.p2_slot>=0 && G.store_pi==G.p2_slot)? G.store_sel2 : G.store_sel;
    int y=STORE_Y, h=STORE_H;
    int w = G.ui_compact? 308 : STORE_W;
    int x = G.ui_compact? G.view_x0 + (G.view_w - w)/2 : STORE_X;
    if (!G.ui_compact) rect_fill(0, 0, VW, VH, COL(16,14,28));   // dim (solid, drawn as overlay panels)
    else rect_fill(G.view_x0, G.view_y0, G.view_w, G.view_h>0? G.view_h : VH, COL(16,14,28));   // split: dim the shopper's whole half, so no world (or bug) peeks around the panel
    rect_fill(x, y+1, w+8, h+8, COL(8,6,14));            // drop shadow
    rect_fill(x-4, y-4, w+8, h+8, OUTLINE);
    rect_fill(x, y, w, h, COL(250,244,225));
    rect_line(x+3, y+3, w-6, h-6, COL(198,164,84));      // gold inner trim
    rect_fill(x, y, w, 32, COL(74,36,34));               // title band
    rect_fill(x, y+31, w, 2, COL(198,164,84));
    draw_text_center(x+w/2, y+3, "THE BUG BOUNTY", 2, COL(255,226,150));
    { char b[56]; char *q=b;
      q=sappend(q, "ON YOUR HEAD: "); q=sapp_int(q, (int)me->carried); q=sappend(q, " KG");
      if (!G.ui_compact){ q=sappend(q, "  ("); q=sappend(q, hat_tier_name(me->stack_n, me->carried)); q=sappend(q, ")"); }
      draw_text_center(x+w/2, y+20, b, 1, COL(255,240,205)); }
    rect_fill(x+14, STORE_ROW0-11, 6, 6, COL(190,60,40));
    if (G.ui_compact){
        draw_text(x+24, STORE_ROW0-13, "WEAPONS - STACK FOREVER", 1, COL(160,60,40));
    } else {
        draw_text(x+24, STORE_ROW0-16, "WEAPONS - STACK FOREVER", 2, COL(160,60,40));
        rect_fill(x+308, STORE_ROW0-9, w-322, 2, COL(216,200,160));
    }
    if (!G.ui_compact)
    { const char *sh = "SHIELD: UP";   // you're in here, so it's up — sits on the title band
      draw_text(x+w-14-text_w(sh,1), y+8, sh, 1, COL(150,220,255)); }
    rect_fill(x+14, STORE_ROW0+5*STORE_ROWH+5, 6, 6, COL(190,60,40));
    if (G.ui_compact){
        draw_text(x+24, STORE_ROW0+5*STORE_ROWH+3, "UPGRADES - STACK FOREVER", 1, COL(160,60,40));
    } else {
        draw_text(x+24, STORE_ROW0+5*STORE_ROWH, "UPGRADES - STACK FOREVER", 2, COL(160,60,40));
        rect_fill(x+320, STORE_ROW0+5*STORE_ROWH+7, w-334, 2, COL(216,200,160));
    }
    for (int i=0;i<STORE_ROWS;i++){
        int ry = STORE_ROW0 + (i<5? i*STORE_ROWH : i*STORE_ROWH+STORE_UPGOFF);
        int owned; int price = store_item_price(i, me, &owned);
        if (i%2==0) rect_fill(x+8, ry-2, w-16, STORE_ROWH, COL(243,236,210));   // zebra
        rect_line(x+8, ry-2, w-16, STORE_ROWH, COL(228,219,192));               // row card edge
        if (i==sel){
            rect_fill(x+8, ry-2, w-16, STORE_ROWH, COL(255,224,138));
            rect_fill(x+8, ry-2, 3, STORE_ROWH, COL(190,60,40));                // selection tab
            rect_line(x+8, ry-2, w-16, STORE_ROWH, COL(198,164,84));
        }
        rect_fill(x+30, ry+1, 16, 16, i==sel? COL(255,240,196) : COL(255,252,240));
        rect_line(x+30, ry+1, 16, 16, COL(198,164,84));
        draw_item_icon(i, x+38, ry+9);
        if (G.ui_compact) draw_text(x+56, ry+5, store_item_name(i), 1, COL(40,36,50));
        else draw_text(x+56, ry+2, store_item_name(i), 2, COL(40,36,50));
        // stack count as plain text — X1, X2, X3... — not pips.
        // Rollback shows tokens held.
        { int cnt = -1;
          if (i<5) cnt = me->owned[i];
          else if (i-5==5) cnt = me->rollback;
          else if (i==12) cnt = me->beans;
          else if (i-5!=6) cnt = me->upg[i-5];
          if (cnt>0){ char b[16]; char *q=b; *q++='X'; q=sapp_int(q, cnt); *q=0;
              // the count lives in its own column LEFT of the icon
              // (Daniel, Nov 5): one fixed place the eye learns,
              // instead of at the end of names of random length
              draw_text(x+28-text_w(b,1), ry+5, b, 1, COL(150,95,30)); } }
        char b[16];
        if (owned) scpy(b, i==11? "FULL HP" : "MAXED", 16);
        else fmt_int(b, "", price, " KG");
        u32 pc = owned? COL(120,150,110) : ((int)me->carried>=price? COL(150,95,30) : COL(186,128,116));
        if (G.ui_compact) draw_text(x+w-14-text_w(b,1), ry+5, b, 1, pc);
        else draw_text(x+w-14-text_w(b,2), ry+2, b, 2, pc);
    }
    // selected blurb + status
    int by = y+h-40;
    int bh = G.ui_compact? 24 : 20;
    rect_fill(x+8, by-4, w-16, bh, COL(240,232,205));
    rect_line(x+8, by-4, w-16, bh, COL(198,164,84));
    rect_fill(x+8, by-4, 3, bh, COL(190,60,40));
    if (G.ui_compact){
        // narrow panel: wrap the blurb onto two lines at a word break
        char bb[96]; scpy(bb, store_item_blurb(sel, me), 96);
        int cut=-1; for (int i=0; bb[i] && i<34; i++) if (bb[i]==' ') cut=i;
        if (cut>0 && bb[cut+1]){ bb[cut]=0;
            draw_text(x+14, by+1, bb, 1, COL(80,66,44));
            draw_text(x+14, by+12, bb+cut+1, 1, COL(80,66,44));
        } else draw_text(x+14, by+2, bb, 1, COL(80,66,44));
    }
    else draw_text(x+14, by+2, store_item_blurb(sel, me), 1, COL(80,66,44));
    if (pay_active) draw_text_center(x+w/2, y+h-13, "COUNTING CARCASSES...", 1, COL(190,60,40));
    else if (G.store_msg_t>0) draw_text_center(x+w/2, y+h-13, G.store_msg, 1, COL(60,140,60));
    else if (G.ui_compact) draw_text_center(x+w/2, y+h-13, "SHIELD UP - FIRE BUYS - BACK LEAVES", 1, COL(88,76,54));
    else if (mobile_ui()) draw_text_center(x+w/2, y+h-13, "TAP A ROW - TAP AGAIN TO BUY - TAP OUTSIDE TO LEAVE", 1, COL(88,76,54));
    else if (pad_mode()) draw_text_center(x+w/2, y+h-13, "STICK/DPAD CHOOSE - A BUYS - B LEAVES", 1, COL(88,76,54));
    else { char b[80]; char *q=b; q=sappend(q, "ARROWS CHOOSE - "); q=sappend(q, key_name(G.bind[ACT_FIRE]));
           q=sappend(q, " BUYS - "); q=sappend(q, key_name(G.bind[ACT_INTERACT])); q=sappend(q, " LEAVES");
           draw_text_center(x+w/2, y+h-13, b, 1, COL(88,76,54)); }
}
static void render_store(void){
    G.store_pi = G.my_id; G.ui_compact = 0;
    render_world();
    draw_active_panel();
}

