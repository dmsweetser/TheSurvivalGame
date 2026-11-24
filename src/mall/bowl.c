// THE SURVIVAL GAME — mall/bowl.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 879-1129 in the Nov 2026 fine-grained refactor; code unchanged.

static void bowl_rack(Bowl *bw){ for (int i=0;i<10;i++){ bw->standing[i]=1; bw->pft[i]=0; } }
static int bowl_pins_up(Bowl *bw){ int n=0; for (int i=0;i<10;i++) n+=bw->standing[i]; return n; }
static void bowl_new_game(int pi){
    Bowl *bw=&G.bowl[pi];
    smemset(bw, 0, sizeof(*bw));
    bowl_rack(bw);
    bw->active=1; bw->phase=0; bw->angle_dir=1; bw->power_dir=1;
    scpy(bw->msg, "FRAME 1 - YOUR THROW!", 40); bw->msg_t=1.4f;
    sfx(19);
}
// cumulative score through frame f, or -1 if it can't be known yet
static int bowl_cum(Bowl *bw, int f){
    int ri=0, tot=0;
    for (int fr=0; fr<=f; fr++){
        if (fr<9){
            if (ri>=bw->nrolls) return -1;
            if (bw->rolls[ri]==10){
                if (ri+2>=bw->nrolls) return -1;
                tot += 10 + bw->rolls[ri+1] + bw->rolls[ri+2]; ri+=1;
            } else {
                if (ri+1>=bw->nrolls) return -1;
                if (bw->rolls[ri]+bw->rolls[ri+1]==10){
                    if (ri+2>=bw->nrolls) return -1;
                    tot += 10 + bw->rolls[ri+2];
                } else tot += bw->rolls[ri] + bw->rolls[ri+1];
                ri+=2;
            }
        } else {
            if (!bw->game_over) return -1;
            while (ri<bw->nrolls){ tot += bw->rolls[ri]; ri++; }
        }
    }
    return tot;
}
// the roll index where frame f's rolls begin (for the scoresheet)
static int bowl_frame_ri(Bowl *bw, int f){
    int ri=0;
    for (int fr=0; fr<f && ri<bw->nrolls; fr++)
        ri += (fr<9 && bw->rolls[ri]==10)? 1 : 2;
    return ri;
}
// ---------------------------------------------------------------------------
// BUG BOWL LEAGUE (v1.25 feat 10) — a six-night season at the lanes
// against three house rivals. Every completed non-Zen game IS the
// player's next league night; standings are cumulative pins. The
// rivals' scores are a pure function of (season, night, rival) via
// hash2 — never the gameplay RNG — so they are identical on every
// install, are posted on the card BEFORE the player bowls, and need
// no storage. Calibration is derived from the game's own scoring
// landmarks (all gutters 0, all open 9s ~90, all spares 150, the
// BOWL PIN line 200): GUTTER BUG is the open-frame bowler, SPARE
// CHANGE the spare shooter, KINGPIN above all-spares pace. A flat
// ±30 spread, no tails. See v125-feat10-spec.md.
// ---------------------------------------------------------------------------
#define LG_NIGHTS 6
#define LG_XP 50   // a season's crown: the Weekly's price for a week-sized arc
static const char *LG_RIVAL_NAME[3] = { "KINGPIN", "SPARE CHANGE", "GUTTER BUG" };
static const int LG_RIVAL_MEAN[3] = { 158, 128, 96 };
static int lg_rival_score(int season, int night, int r){
    int v = LG_RIVAL_MEAN[r&3] + (int)(hash2(season*7 + night, (r&3)*7919 + 13, 0xB0B1E5u) % 61u) - 30;
    return v<0? 0 : v>300? 300 : v;
}
static int lg_rival_total(int season, int r, int nights){
    int t=0; for (int n=0;n<nights;n++) t += lg_rival_score(season, n, r);
    return t;
}
// place (1..4) of a score against the three rivals: a rival must
// score STRICTLY more to stand above the player — ties go to the
// player, nightly and seasonally (the house is kind).
static int lg_place_vs(int score, int season, int night){
    int pl=1; for (int r=0;r<3;r++) if (lg_rival_score(season, night, r) > score) pl++;
    return pl;
}
static void league_finalize(int pi, Bowl *bw){
    BowlLeague *L=&G.blg[pi];
    if (L->season<1){ L->season=1; L->night=0; L->spins=0; }
    // career book
    L->games++; L->pins += bw->final; L->strikes += bw->strikes;
    if (bw->final > L->best) L->best = bw->final;
    // this game is night (night+1) of the season
    int season=L->season, night=L->night;
    L->last_score = bw->final;
    L->last_place = lg_place_vs(bw->final, season, night);
    L->last_night = night+1; L->last_season = season;
    L->spins += bw->final;
    L->night++;
    if (L->night>=LG_NIGHTS){
        // the season closes: cumulative pins decide, ties to the player
        int place=1;
        for (int r=0;r<3;r++) if (lg_rival_total(season, r, LG_NIGHTS) > L->spins) place++;
        L->fin_place = place; L->fin_season = season;
        if (place==1){
            L->trophies++;
            if (G.bud[pi].active) buddy_award_xp(pi, LG_XP);
            else G.fs_pool[pi] += LG_XP;   // the tuition pool flushes it on adoption (the wk pattern)
            if (pi==G.my_id || pi==G.p2_slot){ spawn_floater(G.p[pi].x, G.p[pi].y-46, "LEAGUE CHAMPION! +50 XP", COL(255,205,40)); sfx(11); }
        }
        L->season++; L->night=0; L->spins=0;
    }
    // a league night is on disk the moment it lands (the booth-snap
    // pattern); a LAN guest's finalize runs on their own machine and
    // persists only in their own slot's saves — the host keeps none.
    if (G.net_mode!=NET_CLIENT && (pi==G.my_id || pi==G.p2_slot)) do_save();
}
static void bowl_record(Bowl *bw, int knocked){
    if (knocked==10 && bw->pins_before==10) bw->strikes++;   // BUG BOWL LEAGUE: a strike is 10 on a full rack (tenth-frame fresh racks included)
    char m = knocked==0? '-' : (knocked==bw->pins_before? (bw->pins_before==10? 'X' : '/') : (char)('0'+knocked));
    if (bw->nrolls<21){ bw->rolls[bw->nrolls]=knocked; bw->marks[bw->nrolls]=m; bw->nrolls++; }
    int f=bw->frame;
    if (f<9){
        if (bw->ball==0 && knocked<10){ bw->ball=1; }
        else { bw->frame++; bw->ball=0; bw->ri_frame=bw->nrolls; bowl_rack(bw); }
    } else {
        int r1 = bw->rolls[bw->ri_frame];
        if (bw->ball==0){
            if (knocked==10) bowl_rack(bw);
            bw->ball=1;
        } else if (bw->ball==1){
            int r2 = knocked;
            if (r1==10){ if (r2==10) bowl_rack(bw); bw->ball=2; }
            else if (r1+r2==10){ bowl_rack(bw); bw->ball=2; }
            else bw->game_over=1;
        } else bw->game_over=1;
    }
    if (bw->game_over){
        bw->phase=5;
        bw->final = bowl_cum(bw, 9);
        if (bw->final > G.rec_bowl) G.rec_bowl = bw->final;
        // HAT TOPPERS (feat 5): the BOWL PIN is awarded HERE, at the
        // finalize, because the lanes keep only a house best — there
        // is no per-player record to derive it from later. It follows
        // the records rule (save_records): Zen sets no records, so a
        // Zen 200 earns no pin.
        if (bw->final>=200 && !G.zen_run){
            int pi=(int)(bw-G.bowl); Buddy *bd=&G.bud[pi];
            if (bd->active && !(bd->top_earned & TOP_BIT(TOP_PIN))){
                bd->top_earned |= TOP_BIT(TOP_PIN);
                if (pi==G.my_id || pi==G.p2_slot){ spawn_floater(G.p[pi].x, G.p[pi].y-46, "BOWL PIN EARNED!", COL(255,205,40)); sfx(11); }
            }
        }
        // BUG BOWL LEAGUE (feat 10): the per-player book is kept
        // HERE, at the same finalize, under the same records rule
        // as the pin — Zen games are practice and move nothing.
        if (!G.zen_run) league_finalize((int)(bw-G.bowl), bw);
    } else bw->phase=0;
}
static void bowl_knock(Bowl *bw, int i, float dx, float dy, float speed){
    float len = fsqrt(dx*dx + dy*dy);
    if (len<0.01f){ dx=0; dy=1; len=1; }
    bw->standing[i]=0;
    bw->pfx[i]=PIN_X[i]; bw->pfy[i]=PIN_Y[i];
    bw->pfvx[i]=dx/len*speed + rng_range(-36.0f, 36.0f);
    bw->pfvy[i]=dy/len*speed;
    bw->pft[i]=0.30f;
}
static void bowl_throw(int pi){
    Bowl *bw=&G.bowl[pi]; Player *p=&G.p[pi];
    if (p->stack_n<=0){
        scpy(bw->msg, "NO CARCASSES ON YOUR HEAD!", 40); bw->msg_t=1.6f;
        sfx(16); bw->phase=0; return;
    }
    if (G.net_mode==NET_CLIENT){
        // the guest's game is local; the host just takes the carcass
        bw->bl_kind = ((int)p->stack[p->stack_n-1])%4;
        int o=0; wr_u8(&o,'M'); wr_u8(&o,4); wr_u8(&o,77); plat_net_send(pkt, o, 0);
    } else {
        float w = hat_pop(p);
        bw->bl_kind = ((int)w)%4;
    }
    bw->pins_before = bowl_pins_up(bw);
    float sp = 260.0f + bw->power*3.0f;
    bw->bx = bw->thrower_x; bw->by = 6;
    bw->bvy = fcos(bw->angle)*sp;
    bw->bvx = fsin(bw->angle)*sp*0.6f;
    bw->bl_live=1; bw->bl_gutter=0; bw->phase=3;
    sfx(15);
}
static int bowl_axis(int pi){
    if (G.p2_slot>=0 && pi==G.p2_slot)
        return (act2_down(ACT_RIGHT)?1:0) - (act2_down(ACT_LEFT)?1:0);
    int k = IN.keys | IN.touch_keys;
    return ((k&K_RIGHT)?1:0) - ((k&K_LEFT)?1:0);
}
static void bowl_update(int pi, float dt){
    Bowl *bw=&G.bowl[pi];
    int act = bw->act; bw->act=0;
    // (bw->msg_t is NOT decremented here: it decays in frame() for
    // every connected player, panel open or not — see the note there.
    // When it ticked only inside bowl_update, closing the lanes panel
    // within the message's 1.4-1.8s froze the countdown, and the next
    // visit opened on a stale "FRAME 1 - YOUR THROW!" hanging over
    // the lanes indefinitely.)
    // toppling pins fly, and can take their neighbours down with them
    for (int i=0;i<10;i++) if (bw->pft[i]>0){
        bw->pft[i]-=dt;
        bw->pfx[i]+=bw->pfvx[i]*dt; bw->pfy[i]+=bw->pfvy[i]*dt;
        for (int j=0;j<10;j++) if (bw->standing[j]){
            float dx=PIN_X[j]-bw->pfx[i], dy=PIN_Y[j]-bw->pfy[i];
            if (dx*dx+dy*dy < 15*15)
                bowl_knock(bw, j, bw->pfvx[i], bw->pfvy[i],
                           fsqrt(bw->pfvx[i]*bw->pfvx[i]+bw->pfvy[i]*bw->pfvy[i])*0.75f);
        }
    }
    if (!bw->active){ if (act) bowl_new_game(pi); return; }
    if (bw->game_over){ if (act) bowl_new_game(pi); return; }
    switch (bw->phase){
    case 0: { int ax=bowl_axis(pi);
              bw->thrower_x = fclamp(bw->thrower_x + ax*150.0f*dt, -52.0f, 52.0f);
              // v1.30 (audit D1): touch aim — a held finger drags the
              // thrower along the lane. The lanes run in GS_STORE, where
              // no touch zones are claimed, so read the raw points and
              // invert the draw's own mapping (centre + thrower_x*KX).
              if (pi==G.my_id && G.p2_slot<0){
                  for (int i=0;i<6;i++) if (IN.tp[i].active){
                      int bcx = G.view_x0 + (G.view_w>0? G.view_w : VW)/2;
                      float bkx = G.ui_compact? 1.15f : 1.5f;
                      bw->thrower_x = fclamp((IN.tp[i].x - bcx)/bkx, -52.0f, 52.0f);
                      break;
                  }
              }
              if (act){ bw->phase=1; bw->angle=-0.42f; bw->angle_dir=1; sfx(6); } } break;
    case 1: bw->angle += bw->angle_dir*1.15f*dt;   // kid-paced meters: the aim
            if (bw->angle>0.42f){ bw->angle=0.42f; bw->angle_dir=-1; }   // sweep and the
            if (bw->angle<-0.42f){ bw->angle=-0.42f; bw->angle_dir=1; }  // power bar both
            if (act){ bw->phase=2; bw->power=0; bw->power_dir=1; sfx(6); } break;  // run slower
    case 2: bw->power += bw->power_dir*130.0f*dt;
            if (bw->power>100){ bw->power=100; bw->power_dir=-1; }
            if (bw->power<0){ bw->power=0; bw->power_dir=1; }
            if (act) bowl_throw(pi); break;
    case 3: { float sp2 = fsqrt(bw->bvx*bw->bvx + bw->bvy*bw->bvy);
              bw->bx += bw->bvx*dt; bw->by += bw->bvy*dt;
              float fr = 1.0f - 0.10f*dt; bw->bvx*=fr; bw->bvy*=fr;
              if (!bw->bl_gutter && (bw->bx<-60 || bw->bx>60)){
                  // the ball drops in when its CENTER crosses the lane's
                  // edge — the forgiving rule, so a throw from the far
                  // side of the lane isn't born in the gutter
                  bw->bl_gutter=1; bw->bx = bw->bx<0? -67 : 67; bw->bvx=0; sfx(16);
              }
              if (bw->bl_gutter) bw->bx = bw->bx<0? -67 : 67;
              else for (int i=0;i<10;i++) if (bw->standing[i]){
                  float dx=PIN_X[i]-bw->bx, dy=PIN_Y[i]-bw->by;
                  if (dx*dx+dy*dy < (BOWL_BALL_R+BOWL_PIN_R)*(BOWL_BALL_R+BOWL_PIN_R)){
                      bowl_knock(bw, i, dx, dy, 230.0f + sp2*0.28f);
                      bw->bvx = bw->bvx*0.90f + dx*0.5f; bw->bvy *= 0.90f;
                      sfx(3);
                  }
              }
              if (bw->by>415 || sp2<50){ bw->phase=4; bw->settle_t=0.85f; bw->bl_live=0; } } break;
    case 4: bw->settle_t-=dt;
            if (bw->settle_t<=0){
                int knocked = bw->pins_before - bowl_pins_up(bw);
                int was_frame=bw->frame, was_ball=bw->ball, was_gutter=bw->bl_gutter;
                bowl_record(bw, knocked);
                if (bw->game_over){ scpy(bw->msg, "GAME OVER!", 40); sfx(11); }
                else if (knocked==10 && bw->pins_before==10){ scpy(bw->msg, "STRIKE!", 40); sfx(11); }
                else if (was_ball==1 && was_frame<9 && knocked>0 && knocked==bw->pins_before){ scpy(bw->msg, "SPARE!", 40); sfx(11); }
                else if (was_gutter && knocked==0){ scpy(bw->msg, "INTO THE GUTTER...", 40); }
                else if (knocked==0){ scpy(bw->msg, "A COMPLETE WHIFF!", 40); }
                else { char *q=bw->msg; q=sapp_int(q, knocked); q=sappend(q, knocked==1? " PIN!" : " PINS!"); *q=0; }
                bw->msg_t=1.8f;
            } break;
    }
}
