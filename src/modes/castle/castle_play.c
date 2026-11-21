// THE SURVIVAL GAME — modes/castle/castle_play.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from castle.c lines 199-341 in the Nov 2026 fine-grained refactor; code unchanged.

static void cas_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (S->cas_won){
        S->cas_wont -= dt;
        if (S->cas_wont <= 0){
            int sc = S->cas_score;
            if (sc > b->cas_best) b->cas_best = sc;
            buddy_award_xp(pi, 50);   // v1.19: an escape pays the buddy
            bw_exit(pi, 0);
            Player *p = &G.p[pi];
            char buf[64]; char *q = buf;
            q = sappend(q, "YOU ESCAPED THE CASTLE! SCORE ");
            q = sapp_int(q, sc); if (b->active) q = sappend(q, " +50 BUDDY XP"); *q = 0;
            spawn_floater(p->x, p->y - 26, buf, COL(255, 220, 120));
        }
        return;
    }
    if (in->exit_){
        // v1.22 (Daniel, Nov 17): the v1.11 "no leaving without the
        // key" gate was never his intent — CASTLE is a cabinet, not
        // a prison, and walking back to the Arcade ALWAYS works.
        // The key still gates the courtyard gate (the win) and
        // nothing else.
        bw_exit(pi, 0); return;
    }
    if (S->cas_hitcd > 0) S->cas_hitcd -= dt;
    if (S->cas_notecd > 0) S->cas_notecd -= dt;
    // movement at a fixed castle pace
    float mx = 0, my = 0; int k2 = in->keys;
    if (k2&K_UP) my -= 1; if (k2&K_DOWN) my += 1;
    if (k2&K_LEFT) mx -= 1; if (k2&K_RIGHT) mx += 1;
    if (mx != 0 || my != 0){
        float d = fsqrt(mx*mx + my*my); mx /= d; my /= d;
        b->aim = fatan2(my, mx);
        float nx = b->x + mx * 150.0f * dt, ny = b->y + my * 150.0f * dt;
        if (!bw_solid_b(b, nx, b->y)) b->x = nx;
        if (!bw_solid_b(b, b->x, ny)) b->y = ny;
        b->walk_ph += dt * 9;
    }
    b->bw_x = b->x; b->bw_y = b->y;
    float lx = b->x - CAS_OX, ly = b->y - CAS_OY;
    int room = S->cas_room, col = room % 5, row = room / 5, nroom = -1;
    if (lx < 8 && (CAS_DOORS[room]&8) && col > 0) nroom = room - 1;
    else if (lx > 632 && (CAS_DOORS[room]&2) && col < 4) nroom = room + 1;
    else if (ly < 36 && (CAS_DOORS[room]&1) && row > 0) nroom = room - 5;
    else if (ly > 352 && (CAS_DOORS[room]&4) && row < 3) nroom = room + 5;
    if (nroom >= 0){
        S->cas_room = nroom;
        if (nroom == room - 1){ b->x = CAS_OX + 596; b->y = CAS_OY + 194; }
        else if (nroom == room + 1){ b->x = CAS_OX + 44; b->y = CAS_OY + 194; }
        else if (nroom == room - 5){ b->x = CAS_OX + 320; b->y = CAS_OY + 316; }
        else { b->x = CAS_OX + 320; b->y = CAS_OY + 72; }
        b->bw_x = b->x; b->bw_y = b->y;
        cas_note(S, CAS_NAMES[nroom], 1.5f);
        sfx(19);
        return;
    }
    // the gate, in the courtyard where you started
    if (room == 17 && ly > 306 && lx >= 282 && lx <= 358){
        if (S->cas_key){
            S->cas_won = 1; S->cas_wont = 2.6f; S->cas_score += 100;
            cas_note(S, "THE GATE CREAKS OPEN... YOU ARE FREE!", 2.6f);
            qnote(0, 0, 60, 80, 6); qnote(0.12f, 0, 64, 80, 6); qnote(0.24f, 0, 67, 80, 6); qnote(0.36f, 0, 72, 90, 10);
            return;
        } else if (S->cas_notecd <= 0){
            S->cas_notecd = 2.0f;
            cas_note(S, "THE GATE IS LOCKED FAST. SOMEWHERE THERE IS A KEY.", 2.0f);
            sfx(16);
        }
    }
    // pickups: run into a thing to take it, like the original
    for (int id=0; id<17; id++){
        if (S->cas_taken[id] || S->cas_iroom[id] != room) continue;
        float ix, iy; cas_item_pos(pi, id, &ix, &iy);
        float dx = b->x - ix, dy = b->y - iy;
        if (dx*dx + dy*dy < 24*24){
            S->cas_taken[id] = 1;
            if (CAS_ITEM_KIND[id] == 0){
                S->cas_treasures++; S->cas_score += 100;
                char buf[48]; char *q = buf;
                q = sappend(q, "TREASURE! "); q = sapp_int(q, S->cas_treasures); q = sappend(q, " OF 13."); *q = 0;
                cas_note(S, buf, 1.8f);
                qnote(0, 0, 76, 70, 3); qnote(0.07f, 0, 81, 70, 3); qnote(0.14f, 0, 88, 80, 5);
            } else if (CAS_ITEM_KIND[id] == 1){
                S->cas_sword = 1; cas_note(S, "YOU TAKE THE SWORD. NOW YOUR BUMPS COUNT.", 2.2f); sfx(19);
            } else if (CAS_ITEM_KIND[id] == 2){
                S->cas_key = 1; cas_note(S, "THE GATE KEY! THE COURTYARD GATE WILL OPEN NOW.", 2.2f); sfx(19);
            } else {
                b->hp += 30; if (b->hp > b->maxhp) b->hp = b->maxhp;
                cas_note(S, "A TONIC! +30 HP.", 1.6f); sfx(19);
            }
            splat(ix, iy, COL(255, 220, 120), 8, 70);
        }
    }
    // the room's monsters
    for (int i=0;i<15;i++){
        if (!S->cas_malive[i] || S->cas_mroom[i] != room) continue;
        float *mxp = &S->cas_mx[i], *myp = &S->cas_my[i];
        float dx = b->x - *mxp, dy = b->y - *myp;
        float d2 = dx*dx + dy*dy;
        S->cas_mwt[i] -= dt;
        float vx2 = 0, vy2 = 0;
        if (d2 < 210*210 && d2 > 1){
            float d = fsqrt(d2); vx2 = dx/d; vy2 = dy/d;
        } else if (S->cas_mwt[i] <= 0){
            S->cas_mwt[i] = 1.2f + (float)((unsigned)(i*37 + (int)(G.anim_t*2.0f)) % 10) * 0.13f;
            unsigned h = (unsigned)i * 2654435761u + (unsigned)(int)(G.anim_t * 1.7f) * 40503u;
            vx2 = (float)((int)(h % 3) - 1); vy2 = (float)((int)((h >> 8) % 3) - 1);
        }
        if (vx2 != 0 || vy2 != 0){
            float sp = CAS_MSPD[S->cas_mtype[i]];
            float nx2 = *mxp + vx2 * sp * dt, ny2 = *myp + vy2 * sp * dt;
            if (!cas_solid(pi, nx2, *myp)) *mxp = nx2;
            if (!cas_solid(pi, *mxp, ny2)) *myp = ny2;
            dx = b->x - *mxp; dy = b->y - *myp; d2 = dx*dx + dy*dy;
        }
        // bump combat: an exchange, both ways, like the original
        if (d2 < 23*23 && S->cas_hitcd <= 0){
            S->cas_hitcd = 0.65f;
            if (S->cas_sword){
                S->cas_mhp[i]--;
                qnote(0, 3, 34, 80, 2);
                if (S->cas_mhp[i] <= 0){
                    S->cas_malive[i] = 0; S->cas_score += 10;
                    splat(*mxp, *myp, COL(200, 90, 90), 10, 80);
                    cas_note(S, "IT DIES. +10.", 1.2f);
                } else {
                    float kx = *mxp - dx * 0.5f, ky = *myp - dy * 0.5f;
                    if (kx < CAS_OX + 40) kx = CAS_OX + 40; if (kx > CAS_OX + 600) kx = CAS_OX + 600;
                    if (ky < CAS_OY + 68) ky = CAS_OY + 68; if (ky > CAS_OY + 320) ky = CAS_OY + 320;
                    if (!cas_solid(pi, kx, ky)){ *mxp = kx; *myp = ky; }
                    cas_note(S, "YOU HIT IT!", 0.9f);
                }
            } else {
                cas_note(S, "BARE HANDS DO NOTHING. YOU NEED A SWORD!", 1.6f);
            }
            b->hp -= CAS_MDMG[S->cas_mtype[i]];
            { float kx = b->x - dx * 0.35f, ky = b->y - dy * 0.35f;
              if (!cas_solid(pi, kx, ky)){ b->x = kx; b->y = ky; } }
            if (b->hp <= 0){ b->hp = 0; b->fainted = 1; bw_exit(pi, 1); return; }
        }
    }
}
