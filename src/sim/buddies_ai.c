// THE SURVIVAL GAME — sim/buddies_ai.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 1081-1507 in the Nov 2026 fine-grained refactor; code unchanged.

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
        fx_shake(2.0f);
        for (int j=0;j<MAXBUGS;j++) if (G.bugs[j].alive){
            float dx=G.bugs[j].x-b->x, dy=G.bugs[j].y-b->y;
            if (dx*dx+dy*dy < 106*106){ if (G.bugs[j].alive) G.bugs[j].bp_t = 0.5f; buddy_hit(pi, j, buddy_dmg(b)*1.6f, 230); }
        }
        sfx(12);
    } else {                       // STOMP: the ground itself objects
        spawn_floater(b->x, b->y-24, "STOMP!", COL(255,210,150));
        splat(b->x, b->y+8, COL(190,160,120), 16, 130);
        fx_shake(2.0f);
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
        fx_shake(2.0f);
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
        fx_shake(3.0f);
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
        int owner_zone = in_srcroom[o-G.p]? 3 : o->in_arcade? 2 : o->in_mall? 1 : 0;
        int buddy_zone = (b->y > SRC_CY-300 && b->y < SRC_CY+300)? 3
                       : (b->y > ARC_CY-300 && b->y < ARC_CY+300)? 2
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
                float zcy = in_srcroom[o-G.p]? SRC_CY : o->in_arcade? ARC_CY : MALL_CY;
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

