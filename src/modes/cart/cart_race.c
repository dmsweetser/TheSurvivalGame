// THE SURVIVAL GAME — modes/cart/cart_race.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from cart.c lines 303-759 in the Nov 2026 fine-grained refactor; code unchanged.

static void crt_enter(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    S->crt_cup = 0; S->crt_race = 0; S->crt_track = 0;
    for (int i = 0; i < 6; i++){ S->crt_pts[i] = 0; S->crt_lastpl[i] = 0; S->crt_gprank[i] = 0; }
    crt_build(0);
    S->crt_state = 3; S->crt_t = 0; S->crt_count = 3.4f; S->crt_fint = 0;
    S->crt_fincount = 0; S->crt_player_place = 0;
    S->crt_duo = -1; S->crt_guest = 0;
    S->crt_hfin[0] = 0; S->crt_hfin[1] = 0; S->crt_endt = -1.0f;
    S->crt_colsel = (pi == G.p2_slot && G.my_id != pi)? 1 : 0;
    S->crt_colsel2 = 1; S->crt_ready = 0; S->crt_prevk = 0; S->crt_gprevk = 0;
    for (int i = 0; i < 6; i++) S->crt_kcol[i] = i;
    b->x = CRT_OX + 320; b->y = CRT_OY + 295;
    b->bw_x = b->x; b->bw_y = b->y;
}
// the waiting room is over: lay out the grid and start the countdown
static void crt_start_race(int pi){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    S->crt_state = 0; S->crt_t = 0; S->crt_count = 3.4f; S->crt_fint = 0;
    S->crt_fincount = 0; S->crt_player_place = 0;
    S->crt_hfin[0] = 0; S->crt_hfin[1] = 0; S->crt_endt = -1.0f;
    for (int q2 = 0; q2 < 20; q2++) S->crt_puffl[q2] = 0;
    S->crt_puffn = 0; S->crt_pufft = 0;
    S->crt_prevplace[0] = S->crt_prevplace[1] = 0;
    S->crt_prevlap[0] = S->crt_prevlap[1] = 0;
    static const float SKILL[5] = { 0.95f, 0.97f, 0.99f, 1.005f, 1.02f };
    static const int SLOT[6] = { 3, 0, 1, 2, 4, 5 };   // your buddy starts 4th: a race, not a parade
    for (int i = 0; i < 6; i++){
        int slot = SLOT[i];
        float back = 30.0f + (float)(slot / 2) * 26.0f;
        float lat = (slot % 2 == 0)? -15.0f : 15.0f;
        float x, y, hx, hy; crt_point_at(CRT_OFF - back, lat, &x, &y, &hx, &hy);
        S->crt_x[i] = x; S->crt_y[i] = y; S->crt_h[i] = fatan2(hy, hx);
        S->crt_v[i] = 0; S->crt_prog[i] = -back; S->crt_boost[i] = 0;
        S->crt_finplace[i] = 0; S->crt_place[i] = slot + 1;
        S->crt_seg[i] = crt_nearest_full(x, y);
        S->crt_off[i] = (i == 0)? 0 : ((i % 2 == 0)? -1 : 1) * (7.0f + (float)((i * 37) % 9));
        // the far country's rivals are quicker, like its bugs
        S->crt_skill[i] = (i == 0)? 1.0f : SKILL[i-1] * (0.99f + rng_f() * 0.02f) * (1.0f + 0.012f * (float)S->crt_track);
    }
    // colours: the humans wear their waiting-room picks, the rivals
    // take whatever is left in palette order
    { int used[6] = { 0, 0, 0, 0, 0, 0 };
      S->crt_kcol[0] = S->crt_colsel; used[S->crt_colsel] = 1;
      int start = 1;
      if (S->crt_duo >= 0){ S->crt_kcol[1] = S->crt_colsel2; used[S->crt_colsel2] = 1; start = 2; }
      int nxt = 0;
      for (int i = start; i < 6; i++){ while (used[nxt]) nxt++; S->crt_kcol[i] = nxt; used[nxt] = 1; } }
    // the DRIVERS: the humans race as their own buddies — species, coat
    // and name come straight off the Buddy. The AI seats are fellow
    // Banzai Buddies out for a spin, one of each species in the stable.
    { Buddy *hb = &G.bud[pi];
      S->crt_species[0] = hb->species; S->crt_dcolidx[0] = hb->color; S->crt_nameidx[0] = hb->name_idx;
      int start = 1;
      if (S->crt_duo >= 0){ Buddy *gb = &G.bud[S->crt_duo];
          S->crt_species[1] = gb->species; S->crt_dcolidx[1] = gb->color; S->crt_nameidx[1] = gb->name_idx; start = 2; }
      static const int AI_SP[6] = { 1, 2, 3, 5, 4, 0 };
      static const int AI_CL[6] = { 0, 3, 5, 0, 0, 2 };
      static const int AI_NM[6] = { 7, 22, 40, 51, 58, 17 };
      for (int i = start; i < 6; i++){ int a = (i - start) % 6;
          S->crt_species[i] = AI_SP[a]; S->crt_dcolidx[i] = AI_CL[a]; S->crt_nameidx[i] = AI_NM[a];
          S->crt_line[i] = S->crt_off[i]; S->crt_slidet[i] = 0; }
      S->crt_slidet[0] = 0; if (S->crt_duo >= 0) S->crt_slidet[1] = 0;
    }
    b->x = CRT_OX + S->crt_x[0]; b->y = CRT_OY + S->crt_y[0];
    b->bw_x = b->x; b->bw_y = b->y; b->aim = S->crt_h[0];
    { char *q = S->note; q = sappend(q, "RACE "); q = sapp_int(q, S->crt_race + 1); q = sappend(q, "/4 - "); q = sappend(q, REGION_NAME[S->crt_track]); q = sappend(q, ". HOLD "); q = sappend(q, lbla(H_FIRE)); q = sappend(q, " TO GO!"); *q = 0; S->note_t = 2.4f; }
    if (S->crt_duo >= 0){ Buddy *gb = &G.bud[S->crt_duo];
        gb->x = CRT_OX + S->crt_x[1]; gb->y = CRT_OY + S->crt_y[1];
        gb->bw_x = gb->x; gb->bw_y = gb->y; gb->aim = S->crt_h[1]; }
}

// one kart under human hands (host = kart 0, guest = kart 1)
static void crt_drive_kart(BwSim *S, int i, int keys, int go, float dt){
    int bi = crt_nearest_full(S->crt_x[i], S->crt_y[i]);
    float dxl = S->crt_x[i] - CRT_WX[bi], dyl = S->crt_y[i] - CRT_WY[bi];
    float hw1 = CRT_HW + 1.0f;
    int onroad = (dxl*dxl + dyl*dyl) <= hw1 * hw1;
    float vmax = onroad? 168.0f : 62.0f;
    if (S->crt_boost[i] > 0){ S->crt_boost[i] -= dt; vmax *= 1.38f; }
    float v = S->crt_v[i];
    // GO is the FIRE button held down (Nov 14, Daniel): the stick is
    // for steering only — up no longer accelerates. Down still brakes
    // and reverses; letting go of FIRE coasts.
    if (go){
        if (v > vmax){ v -= 150.0f * dt; if (v < vmax) v = vmax; }
        else { v += (v < 60.0f? 265.0f : 165.0f) * dt; if (v > vmax) v = vmax; }   // a proper launch, then the usual pull
    } else if (keys & K_DOWN){
        v -= 260.0f * dt; if (v < -58.0f) v = -58.0f;
    } else {
        if (v > 0){ v -= 90.0f * dt; if (v < 0) v = 0; }
        else if (v < 0){ v += 90.0f * dt; if (v > 0) v = 0; }
        if (v > vmax){ v -= 170.0f * dt; if (v < vmax) v = vmax; }
    }
    float steer = 0;
    if (keys & K_LEFT) steer -= 1;
    if (keys & K_RIGHT) steer += 1;
    float sf = v / 55.0f; if (sf > 1) sf = 1; if (sf < -1) sf = -1;
    S->crt_h[i] = crt_wrap(S->crt_h[i] + steer * 2.75f * sf * dt);
    // DRIFT (Nov 14): hold a hard steer at speed and the rear steps
    // out — the kart slides wide while the charge builds; let go
    // after a real slide and the stored-up grip fires you forward.
    if (steer != 0 && v > 118.0f){
        S->crt_slidet[i] += dt;
        if (S->crt_slidet[i] > 0.28f){
            float lx = fsin(S->crt_h[i]), ly = -fcos(S->crt_h[i]);   // leftward unit
            float mag = (v - 100.0f) * 0.38f * steer;
            S->crt_x[i] += lx * mag * dt;
            S->crt_y[i] += ly * mag * dt;
        }
    } else {
        if (S->crt_slidet[i] > 0.45f && S->crt_boost[i] < 0.38f) S->crt_boost[i] = 0.38f;   // mini-boost on release
        S->crt_slidet[i] = 0;
    }
    S->crt_v[i] = v;
    S->crt_x[i] += fcos(S->crt_h[i]) * v * dt;
    S->crt_y[i] += fsin(S->crt_h[i]) * v * dt;
}
static void crt_save_record(int who, int place, int tds, int track){
    if (place <= 0) return;
    Buddy *bb = &G.bud[who];
    if (!bb->crt_best_pos || place < bb->crt_best_pos) bb->crt_best_pos = place;
    if (!bb->crt_best_time || tds < bb->crt_best_time) bb->crt_best_time = tds;
    // MEDAL TIMES (feat 3): the per-track best rides the SAME shared
    // race clock for host and guest alike (the legacy co-op rule), but
    // — standing records rule — a Zen run sets no records.
    if (!G.zen_run && track >= 0 && track < 8)
        if (!bb->crt_tbest[track] || tds < bb->crt_tbest[track]) bb->crt_tbest[track] = tds;
}
static void crt_finish_floater(int who, int place){
    Player *p = &G.p[who]; char buf[64]; char *q = buf;
    q = sappend(q, "SUPER BUDDY CART: YOU FINISHED P");
    q = sapp_int(q, place); q = sappend(q, "!"); *q = 0;
    spawn_floater(p->x, p->y - 26, buf, COL(255, 220, 120));
}
// a cup begins: points wiped, race 1 on the cup's first region
static void crt_gp_begin(int pi){
    BwSim *S = &BWS[pi];
    S->crt_race = 0;
    for (int i = 0; i < 6; i++){ S->crt_pts[i] = 0; S->crt_lastpl[i] = 0; S->crt_gprank[i] = 0; }
    S->crt_track = S->crt_cup * 4;
    crt_build(S->crt_track);
    crt_start_race(pi);
}
// the cup ends: rank the field, and bank each human's best rank
static void crt_gp_finalize(int pi){
    BwSim *S = &BWS[pi];
    int ord[6]; crt_order(S, ord);
    for (int r = 0; r < 6; r++) S->crt_gprank[ord[r]] = r + 1;
    for (int h = 0; h < 2; h++){
        int who = (h == 0)? pi : S->crt_duo;
        if (who < 0) continue;
        Buddy *hb = &G.bud[who];
        int rank = S->crt_gprank[h];
        int cur = (S->crt_cup == 0)? hb->crt_cupbest % 8 : hb->crt_cupbest / 8;
        if (!cur || rank < cur){
            if (S->crt_cup == 0) hb->crt_cupbest = (hb->crt_cupbest / 8) * 8 + rank;
            else hb->crt_cupbest = (hb->crt_cupbest % 8) + rank * 8;
        }
    }
    do_save();
}
static void crt_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (S->crt_guest){
        // this player is riding in the host's race: ferry the input over,
        // and let them bail out on their own (their kart races on as a rival)
        int host = S->crt_guest - 1;
        crt_gin[host] = *in;
        if (in->exit_){
            BwSim *HS = &BWS[host];
            if (HS->crt_duo == pi){ HS->crt_duo = -1; HS->crt_ready &= ~2; }
            S->crt_guest = 0;
            bw_exit(pi, 0);
            Player *p = &G.p[pi];
            spawn_floater(p->x, p->y - 26, HS->crt_state == 3? "YOU LEFT THE WAITING ROOM" : "YOU LEFT THE RACE", COL(255, 200, 150));
        }
        return;
    }
    if (S->crt_state == 3){
        // ---- the WAITING ROOM: pick a colour; in local co-op, link with
        // the other player only when they have picked the cart too, and
        // start when both are ready. Solo, FIRE starts a solo race.
        // (crt_t is the room's grace clock: the cabinet-pick press must
        // never double as the ready press, so FIRE only counts once the
        // room is a third of a second old — or re-aged at the link.)
        S->crt_t += dt;
        if (S->crt_duo < 0 && (G.net_mode == NET_OFF || (G.net_mode == NET_HOST && !any_remote_connected()))){
            int other = (pi == G.my_id)? G.p2_slot : G.my_id;
            if (other >= 0 && other != pi){
                BwSim *OS = &BWS[other]; Buddy *ob = &G.bud[other];
                if (ob->active && ob->in_bw && ob->bw_mode == 7 && OS->crt_state == 3
                    && OS->crt_duo < 0 && OS->crt_guest == 0){
                    S->crt_duo = other; OS->crt_guest = pi + 1;
                    S->crt_colsel2 = OS->crt_colsel;
                    S->crt_ready = 0; S->crt_gprevk = 0; crt_gin[pi] = (BwIn){0};
                    S->crt_t = 0;   // restart the grace clock at the link
                    sfx(19);
                }
            }
        }
        int edges = in->keys & ~S->crt_prevk; S->crt_prevk = in->keys;
        if (edges & K_LEFT){ do { S->crt_colsel = (S->crt_colsel + 5) % 6; } while (S->crt_duo >= 0 && S->crt_colsel == S->crt_colsel2); sfx(6); }
        if (edges & K_RIGHT){ do { S->crt_colsel = (S->crt_colsel + 1) % 6; } while (S->crt_duo >= 0 && S->crt_colsel == S->crt_colsel2); sfx(6); }
        // the cup is the host's call: UP for the HOME CUP (regions
        // 1-4), DOWN for the FAR CUP (regions 5-8)
        if (edges & K_UP){ S->crt_cup = 0; S->crt_track = 0; crt_build(0); sfx(6); }
        if (edges & K_DOWN){ S->crt_cup = 1; S->crt_track = 4; crt_build(4); sfx(6); }
        if (in->exit_){
            if (S->crt_duo >= 0){
                // send the partner back to their own solo waiting room
                int guest = S->crt_duo; BwSim *GS2 = &BWS[guest];
                GS2->crt_colsel = S->crt_colsel2;
                GS2->crt_guest = 0; S->crt_duo = -1;
            }
            bw_exit(pi, 0);
            return;
        }
        if (S->crt_duo >= 0){
            BwIn *g = &crt_gin[pi];
            int gedges = g->keys & ~S->crt_gprevk; S->crt_gprevk = g->keys;
            if (gedges & K_LEFT){ do { S->crt_colsel2 = (S->crt_colsel2 + 5) % 6; } while (S->crt_colsel2 == S->crt_colsel); }
            if (gedges & K_RIGHT){ do { S->crt_colsel2 = (S->crt_colsel2 + 1) % 6; } while (S->crt_colsel2 == S->crt_colsel); }
            if (g->sel && S->crt_t > 0.35f) S->crt_ready |= 2;
            if (in->sel && S->crt_t > 0.35f) S->crt_ready |= 1;
            if (S->crt_ready == 3) crt_gp_begin(pi);
        } else if (in->sel && S->crt_t > 0.35f){
            crt_gp_begin(pi);
        }
        return;
    }
    if (S->crt_state == 2){
        S->crt_fint -= dt;
        if (S->crt_fint <= 0){
            int tds = (int)(S->crt_t * 10.0f);
            int guest = S->crt_duo, gplace = S->crt_hfin[1];
            crt_save_record(pi, S->crt_hfin[0], tds, S->crt_track);
            if (guest >= 0) crt_save_record(guest, gplace, tds, S->crt_track);
            // bank the Grand Prix points for the whole field
            static const int PTS[6] = { 10, 8, 6, 5, 4, 3 };
            for (int i = 0; i < 6; i++){
                int pl = S->crt_finplace[i]? S->crt_finplace[i] : S->crt_place[i];
                S->crt_pts[i] += PTS[pl - 1];
                S->crt_lastpl[i] = pl;
            }
            if (S->crt_race < 3){ S->crt_state = 4; S->crt_fint = 5.0f; }
            else { S->crt_state = 5; S->crt_fint = 7.0f; crt_gp_finalize(pi); }
        }
        return;
    }
    if (in->exit_){
        if (S->crt_duo >= 0){
            int guest = S->crt_duo; BwSim *GS2 = &BWS[guest];
            GS2->crt_guest = 0; S->crt_duo = -1;
            bw_exit(guest, 0);
            Player *gp = &G.p[guest];
            spawn_floater(gp->x, gp->y - 26, "THE HOST LEFT THE RACE", COL(255, 200, 150));
        }
        bw_exit(pi, 0); return;
    }
    if (S->crt_state == 4){
        // the standings between races: breathe, read the table,
        // then roll on to the next region (FIRE hurries it along)
        S->crt_fint -= dt;
        if (S->crt_fint <= 0 || in->sel){
            S->crt_race++;
            S->crt_track = S->crt_cup * 4 + S->crt_race;
            crt_build(S->crt_track);
            crt_start_race(pi);
        }
        return;
    }
    if (S->crt_state == 5){
        // the final standings: the cup is decided — linger, then
        // everyone goes back to the mall with their rank
        S->crt_fint -= dt;
        if (S->crt_fint <= 0 || in->sel){
            int guest = S->crt_duo;
            int hrank = S->crt_gprank[0], grank = S->crt_gprank[1];
            if (guest >= 0){
                BwSim *GS2 = &BWS[guest];
                GS2->crt_guest = 0; S->crt_duo = -1;
                if (grank == 1) buddy_award_xp(guest, 40);   // v1.19: a Grand Prix win pays the buddy an extra 40
                bw_exit(guest, 0);
                Player *gp = &G.p[guest];
                char buf[64]; char *q = buf;
                q = sappend(q, "GRAND PRIX: YOU RANKED P"); q = sapp_int(q, grank); q = sappend(q, " OF 6!"); if (grank == 1 && G.bud[guest].active) q = sappend(q, " +40 BUDDY XP"); *q = 0;
                spawn_floater(gp->x, gp->y - 26, buf, COL(255, 220, 120));
            }
            if (hrank == 1) buddy_award_xp(pi, 40);   // v1.19: a Grand Prix win pays the buddy an extra 40
            bw_exit(pi, 0);
            { Player *p = &G.p[pi];
              char buf[64]; char *q = buf;
              q = sappend(q, "GRAND PRIX: YOU RANKED P"); q = sapp_int(q, hrank); q = sappend(q, " OF 6!"); if (hrank == 1 && b->active) q = sappend(q, " +40 BUDDY XP"); *q = 0;
              spawn_floater(p->x, p->y - 26, buf, COL(255, 220, 120)); }
        }
        return;
    }
    if (S->crt_state == 0){
        float prev = S->crt_count; S->crt_count -= dt;
        if ((int)prev != (int)S->crt_count && S->crt_count > 0) sfx(6);
        if (S->crt_count <= 0){ S->crt_state = 1; sfx(19); }
        return;
    }
    S->crt_t += dt;
    // ---- the human karts: host on kart 0, the co-op guest on kart 1
    crt_drive_kart(S, 0, in->keys, in->sel_held || in->sel, dt);
    if (S->crt_duo >= 0) crt_drive_kart(S, 1, crt_gin[pi].keys, crt_gin[pi].sel_held || crt_gin[pi].sel, dt);
    // ---- the rivals (one fewer when a guest is driving kart 1)
    for (int i = (S->crt_duo >= 0? 2 : 1); i < 6; i++){
        float tx, ty, hx, hy, t2x, t2y, h2x, h2y;
        // the racing line: lean toward the inside of the bend ahead,
        // smoothed so the kart flows instead of twitching
        crt_point_at(CRT_OFF + S->crt_prog[i] + 118.0f, 0, &t2x, &t2y, &h2x, &h2y);
        crt_point_at(CRT_OFF + S->crt_prog[i] + 48.0f, 0, &tx, &ty, &hx, &hy);
        { float cross = hx * h2y - hy * h2x;
          float want = S->crt_off[i] * 0.3f + cross * 20.0f;
          float lc = CRT_HW - 10.0f; if (lc > 14.0f) lc = 14.0f;
          if (want > lc) want = lc; if (want < -lc) want = -lc;
          S->crt_line[i] += (want - S->crt_line[i]) * (dt * 2.2f > 1.0f? 1.0f : dt * 2.2f); }
        crt_point_at(CRT_OFF + S->crt_prog[i] + 48.0f, S->crt_line[i], &tx, &ty, &hx, &hy);
        // traffic sense: if a kart is right on your nose, shade away
        for (int j = 0; j < 6; j++){ if (j == i) continue;
            float ax = S->crt_x[j] - S->crt_x[i], ay = S->crt_y[j] - S->crt_y[i];
            float fwd = ax * fcos(S->crt_h[i]) + ay * fsin(S->crt_h[i]);
            float lat = ax * (-fsin(S->crt_h[i])) + ay * fcos(S->crt_h[i]);
            if (fwd > 2.0f && fwd < 36.0f && lat > -14.0f && lat < 14.0f){
                tx += (-fsin(S->crt_h[i])) * (lat > 0? -11.0f : 11.0f);
                ty += fcos(S->crt_h[i]) * (lat > 0? -11.0f : 11.0f);
            }
        }
        float des = fatan2(ty - S->crt_y[i], tx - S->crt_x[i]);
        float dh = crt_wrap(des - S->crt_h[i]);
        float turn = 3.3f * dt; if (dh > turn) dh = turn; if (dh < -turn) dh = -turn;
        S->crt_h[i] = crt_wrap(S->crt_h[i] + dh);
        float curve = ffabs(crt_wrap(fatan2(h2y, h2x) - fatan2(hy, hx)));
        float tv = 158.0f * S->crt_skill[i] * (1.0f - (curve > 0.5f? 0.30f : curve * 0.30f));
        float rubber = S->crt_prog[0] - S->crt_prog[i];
        float rbw = CRT_LEN * 0.178f;   // v1.23: the band stretches with the circuit
        tv *= (rubber > rbw)? 1.05f : (rubber < -rbw)? 0.95f : 1.0f;
        { int bi = crt_nearest_full(S->crt_x[i], S->crt_y[i]);
          float dxl = S->crt_x[i] - CRT_WX[bi], dyl = S->crt_y[i] - CRT_WY[bi];
          float hw1 = CRT_HW + 1.0f;
          if (dxl*dxl + dyl*dyl > hw1 * hw1) tv *= 0.45f; }
        if (S->crt_boost[i] > 0){ S->crt_boost[i] -= dt; tv *= 1.30f; }
        float v = S->crt_v[i];
        if (v < tv){ v += 130.0f * dt; if (v > tv) v = tv; }
        else { v -= 210.0f * dt; if (v < tv) v = tv; }
        S->crt_v[i] = v;
        S->crt_x[i] += fcos(S->crt_h[i]) * v * dt;
        S->crt_y[i] += fsin(S->crt_h[i]) * v * dt;
    }
    // ---- boost pads
    for (int i = 0; i < 6; i++) for (int pd = 0; pd < CRT_NPAD; pd++){
        float dx = S->crt_x[i] - CRT_PAD[pd][0], dy = S->crt_y[i] - CRT_PAD[pd][1];
        if (dx*dx + dy*dy < 19*19 && S->crt_boost[i] <= 0){
            S->crt_boost[i] = 0.9f;
            if (i == 0 || (i == 1 && S->crt_duo >= 0)){ qnote(0, 0, 76, 80, 3); qnote(0.07f, 0, 81, 80, 3); }
        }
    }
    // ---- kart vs kart: soft bumps, nobody spins out
    for (int i = 0; i < 6; i++) for (int j = i + 1; j < 6; j++){
        float dx = S->crt_x[j] - S->crt_x[i], dy = S->crt_y[j] - S->crt_y[i];
        float d2 = dx*dx + dy*dy;
        if (d2 < 21*21 && d2 > 0.01f){ float d = fsqrt(d2);
            float push = (21.0f - d) / 2.0f, ux = dx / d, uy = dy / d;
            S->crt_x[i] -= ux * push; S->crt_y[i] -= uy * push;
            S->crt_x[j] += ux * push; S->crt_y[j] += uy * push;
            // trade a little pace instead of both karts just dying:
            // the pair settles toward a shared speed, nobody stalls
            float dv = (S->crt_v[j] - S->crt_v[i]) * 0.12f;
            S->crt_v[i] += dv; S->crt_v[j] -= dv;
            S->crt_v[i] *= 0.995f; S->crt_v[j] *= 0.995f; }
    }
    // ---- tire-stack obstacles (v1.23, far country): a firm bounce
    // and a bite of speed for anyone who clips one, human or rival
    for (int i = 0; i < 6; i++) for (int ob = 0; ob < CRT_NOBS; ob++){
        float dx = S->crt_x[i] - CRT_OBS[ob][0], dy = S->crt_y[i] - CRT_OBS[ob][1];
        float d2 = dx*dx + dy*dy;
        if (d2 < 17.0f * 17.0f && d2 > 0.01f){ float d = fsqrt(d2);
            S->crt_x[i] = CRT_OBS[ob][0] + dx / d * 17.0f;
            S->crt_y[i] = CRT_OBS[ob][1] + dy / d * 17.0f;
            if (S->crt_v[i] > 50.0f && (i == 0 || (i == 1 && S->crt_duo >= 0))) sfx(17);
            S->crt_v[i] *= 0.55f; }
    }
    // ---- bounds, progress, finishes
    for (int i = 0; i < 6; i++){
        if (S->crt_x[i] < 18) S->crt_x[i] = 18; if (S->crt_x[i] > CRT_WW - 18) S->crt_x[i] = CRT_WW - 18;
        if (S->crt_y[i] < 18) S->crt_y[i] = 18; if (S->crt_y[i] > CRT_WH - 18) S->crt_y[i] = CRT_WH - 18;
        crt_track_prog(S, i);
        if (!S->crt_finplace[i] && S->crt_prog[i] >= 3.0f * CRT_LEN){
            S->crt_finplace[i] = ++S->crt_fincount;
            int human = (i == 0)? 0 : (i == 1 && S->crt_duo >= 0)? 1 : -1;
            if (human >= 0){
                S->crt_hfin[human] = S->crt_finplace[i];
                if (S->crt_endt < 0) S->crt_endt = 8.0f;   // first human home starts the clock
            }
        }
    }
    crt_places(S);
    // exhaust puffs from working engines: boosting karts, and the
    // humans whenever they're moving with intent
    S->crt_pufft -= dt;
    for (int q2 = 0; q2 < 20; q2++) if (S->crt_puffl[q2] > 0) S->crt_puffl[q2] -= dt;
    if (S->crt_pufft <= 0){
        S->crt_pufft = 0.07f;
        for (int i = 0; i < 6; i++){
            int human = (i == 0) || (i == 1 && S->crt_duo >= 0);
            if (S->crt_boost[i] > 0 || (human && S->crt_v[i] > 30.0f)){
                int s2 = S->crt_puffn; S->crt_puffn = (S->crt_puffn + 1) % 20;
                S->crt_puffx[s2] = S->crt_x[i] - fcos(S->crt_h[i]) * 12.0f;
                S->crt_puffy[s2] = S->crt_y[i] - fsin(S->crt_h[i]) * 12.0f;
                S->crt_puffl[s2] = 0.45f;
            }
        }
    }
    // race notes for the humans: a place gained, the final lap
    for (int h = 0; h < 2; h++){
        if (h == 1 && S->crt_duo < 0) break;
        if (S->crt_prevplace[h] && S->crt_place[h] < S->crt_prevplace[h] && !S->crt_finplace[h]){
            char buf[48]; char *q = buf;
            q = sappend(q, "UP TO P"); q = sapp_int(q, S->crt_place[h]); q = sappend(q, "!"); *q = 0;
            scpy(S->note, buf, 72); S->note_t = 1.6f;
        }
        S->crt_prevplace[h] = S->crt_place[h];
        { int lap = (int)(S->crt_prog[h] / CRT_LEN) + 1; if (lap > 3) lap = 3;
          if (S->crt_prevlap[h] && lap == 3 && S->crt_prevlap[h] < 3 && !S->crt_finplace[h]){
              scpy(S->note, "FINAL LAP!", 72); S->note_t = 1.8f;
          }
          S->crt_prevlap[h] = lap; }
    }
    // the race closes when every human is home, or the clock runs out
    if (S->crt_endt >= 0) S->crt_endt -= dt;
    { int all_home = S->crt_hfin[0] && (S->crt_duo < 0 || S->crt_hfin[1]);
      int timed_out = S->crt_endt <= 0 && S->crt_endt > -50.0f && (S->crt_hfin[0] || S->crt_hfin[1]);
      if (all_home || timed_out){
          if (!S->crt_hfin[0]) S->crt_hfin[0] = S->crt_place[0];
          if (S->crt_duo >= 0 && !S->crt_hfin[1]) S->crt_hfin[1] = S->crt_place[1];
          S->crt_player_place = S->crt_hfin[0];
          { static const int CRTXP[3] = { 40, 30, 20 };   // v1.19: a finish pays the buddy by placement — 1st/2nd/3rd per the table, any other finish 10
            int amt0 = (S->crt_hfin[0]>=1 && S->crt_hfin[0]<=3)? CRTXP[S->crt_hfin[0]-1] : 10;
            buddy_award_xp(pi, amt0);
            if (S->crt_duo >= 0){ int amt1 = (S->crt_hfin[1]>=1 && S->crt_hfin[1]<=3)? CRTXP[S->crt_hfin[1]-1] : 10; buddy_award_xp(S->crt_duo, amt1); }
            if (b->active){ char *q=S->note; q=sappend(q, "FINISH BONUS: +"); q=sapp_int(q, amt0); q=sappend(q, " BUDDY XP"); *q=0; S->note_t=3.4f; } }
          S->crt_state = 2; S->crt_fint = 3.4f;
          qnote(0, 0, 60, 90, 6); qnote(0.12f, 0, 64, 90, 6);
          qnote(0.24f, 0, 67, 90, 6); qnote(0.36f, 0, 72, 110, 10);
      } }
    b->x = CRT_OX + S->crt_x[0]; b->y = CRT_OY + S->crt_y[0];
    b->bw_x = b->x; b->bw_y = b->y; b->aim = S->crt_h[0];
    if (S->crt_duo >= 0){ Buddy *gb = &G.bud[S->crt_duo];
        gb->x = CRT_OX + S->crt_x[1]; gb->y = CRT_OY + S->crt_y[1];
        gb->bw_x = gb->x; gb->bw_y = gb->y; gb->aim = S->crt_h[1]; }
}
// the driver of a kart, drawn from behind at screen position (cx,
// seat_y) with the kart w px wide: the buddy's own back, head, the
// species giveaways — and, of course, the glasses.
