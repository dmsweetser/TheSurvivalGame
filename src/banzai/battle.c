// THE SURVIVAL GAME — banzai/battle.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 656-919 in the Nov 2026 fine-grained refactor; code unchanged.

// --- the battle ------------------------------------------------------------
static const char *BW_HACK_NAME[6] = { "SCRIPT KIDDIE", "PHISHER", "BOT HERDER", "SPAM LORD", "RANSOMWARE RANDY", "ZERO-DAY ZED" };
static const char *BW_HACK_MOVE[6] = { "BRUTE FORCE", "PHISH SPAM", "DDOS FLOOD", "ZERO-DAY", "RANSOMWARE", "STACK SMASH" };
static const char *bw_hacker_name(int lvl){
    int i = lvl<=1? 0 : lvl<=3? 1 : lvl<=5? 2 : lvl<=6? 3 : lvl<=8? 4 : 5;
    return BW_HACK_NAME[i];
}
static void bw_attack_info(Buddy *b, int mv, const char **nm, float *cost, float *mult){
    switch (mv){
        case 1: *nm=BUD_MOVE2[b->species%BUDN]; *cost=BUD_EN_MOVE2; *mult=1.6f; break;
        case 2: *nm=BUD_SPEC[b->species%BUDN];  *cost=BUD_EN_SPEC;  *mult=2.3f; break;
        case 3: *nm=BUD_MOVE3[b->species%BUDN]; *cost=BUD_EN_MOVE3; *mult=2.2f; break;
        case 4: *nm=BUD_MOVE4[b->species%BUDN]; *cost=BUD_EN_MOVE4; *mult=2.9f; break;
        case 5: *nm="RUN AWAY"; *cost=0; *mult=0; break;
        case 6: case 7: case 8: case 9: *nm=BW_ITEM_NAME[mv-6]; *cost=0; *mult=0; break;
        default: *nm=BUD_BASIC[b->species%BUDN]; *cost=0; *mult=1.0f; break;
    }
}
static void bw_battle_start(Buddy *b, BWHack *h){
    BwSim *S = &BWS[(int)(b - G.bud)];
    S->bat.active=1; S->bat.phase=5; S->bat.t=1.0f; S->bat.lvl=h->lvl; S->bat.cell=h->cell; S->bat.sel=0;
    S->bat.boss=0; S->bat.strikes=0; S->bat.zcharge=0; S->bat.zph=0; S->bat.lastdmg=0;
    S->bat.site = bw_site_at_xy(b, b->x, b->y);
    S->bat.hmax = 26 + 20*h->lvl + (int)(hash2(h->cell, 7, 3) % 10);
    S->bat.hhp = S->bat.hmax;
    S->bat.natt=0; S->bat.attacks[S->bat.natt++]=0;
    if (b->skills&1) S->bat.attacks[S->bat.natt++]=1;
    if (b->skills&2) S->bat.attacks[S->bat.natt++]=2;
    if (b->skills&4) S->bat.attacks[S->bat.natt++]=3;
    if (b->skills&8) S->bat.attacks[S->bat.natt++]=4;
    for (int it=0; it<4; it++) if (b->items[it]>0) S->bat.attacks[S->bat.natt++]=6+it;
    S->bat.attacks[S->bat.natt++]=5;
    { char *q=S->bat.msg; q=sappend(q, "A WILD "); q=sappend(q, bw_hacker_name(S->bat.lvl));
      q=sappend(q, " APPEARED!"); *q=0; scpy(S->bat.msg2, "IT WANTS YOUR PASSWORD.", 72); }
    sfx(7);
}
static void bw_boss_start(int pi, int boss){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    S->bat.active=1; S->bat.phase=5; S->bat.t=1.2f; S->bat.sel=0; S->bat.cell=0;
    S->bat.boss=boss; S->bat.strikes=0; S->bat.zcharge=0; S->bat.zph=0; S->bat.lastdmg=0;
    S->bat.site = bw_site_at_xy(b, b->x, b->y);
    if (boss==1){
        S->bat.lvl = 7; S->bat.hmax = 330;
        scpy(S->bat.msg, "BROKEN RAID ROLLS OUT OF THE DARK!", 96);
        scpy(S->bat.msg2, "EVERY BAD SECTOR YOU EVER MET. IN A TRENCHCOAT.", 72);
    } else if (boss==3){
        S->bat.lvl = 9; S->bat.hmax = 430;
        scpy(S->bat.msg, "THE CACHE HOARDER WAKES ON ITS HOARD!", 96);
        scpy(S->bat.msg2, "IT HAS BEEN SITTING ON THE GATE CRANK FOR YEARS.", 72);
    } else if (boss==4){
        // THE REBUILDING (feat 14): the rematch — the original's
        // mirror trick at a harder weight, between RAID and Zed.
        S->bat.lvl = 9; S->bat.hmax = 460;
        scpy(S->bat.msg, "BROKEN RAID, REASSEMBLED!", 96);
        scpy(S->bat.msg2, "IT KEPT THE RECEIPTS. IT REMEMBERS YOU.", 72);
    } else {
        S->bat.lvl = 12; S->bat.hmax = 560;
        scpy(S->bat.msg, "ZERO-DAY ZED. UNPATCHED. UNBOTHERED.", 96);
        scpy(S->bat.msg2, "THE FINAL BUG. PROBABLY.", 72);
    }
    S->bat.hhp = S->bat.hmax;
    S->bat.natt=0; S->bat.attacks[S->bat.natt++]=0;
    if (b->skills&1) S->bat.attacks[S->bat.natt++]=1;
    if (b->skills&2) S->bat.attacks[S->bat.natt++]=2;
    if (b->skills&4) S->bat.attacks[S->bat.natt++]=3;
    if (b->skills&8) S->bat.attacks[S->bat.natt++]=4;
    for (int it=0; it<4; it++) if (b->items[it]>0) S->bat.attacks[S->bat.natt++]=6+it;
    S->bat.attacks[S->bat.natt++]=5;
    sfx(7);
}
static void bw_battle_end(int pi, int won){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    if (won && S->bat.boss==0 && S->defeated_n<64) S->defeated[S->defeated_n++] = S->bat.cell;
    S->bat.active=0; S->grace_t = won? 1.2f : 2.5f;
    if (won){
        int noted = 0;
        if (S->bat.boss==1){
            b->story_bits |= (1<<16); b->xp += 60;
            b->items[2] += 2; if (b->items[2]>9) b->items[2]=9;
            scpy(S->note, "BROKEN RAID SCRAPPED. +60 XP. THE AISLE GOES QUIET.", 72); S->note_t=4.2f; noted=1;
        } else if (S->bat.boss==3){
            b->story_bits |= (1<<22) | (1<<23);   // hoarder down, crank carried
            if (bw_qstate_x(b,5)==1) bw_qset_x(b,5,2);
            b->xp += 100;
            for (int it=0; it<4; it++){ b->items[it] += 2; if (b->items[it]>9) b->items[it]=9; }
            scpy(S->note, "THE HOARDER CRACKS OPEN. THE GATE CRANK IS YOURS.", 72); S->note_t=5.0f; noted=1;
        } else if (S->bat.boss==2){
            b->story_bits |= (1<<17);
            if (bw_qstate(b,3)==1) bw_qset(b,3,2);
            b->xp += 150;
            for (int it=0; it<4; it++){ b->items[it] += 2; if (b->items[it]>9) b->items[it]=9; }
            scpy(S->note, "ZERO-DAY ZED HAS BEEN FUNERALED. THE MEADOW IS PATCHED.", 72); S->note_t=5.0f; noted=1;
        } else if (S->bat.boss==4){
            // THE REBUILDING (feat 14): the rematch is quest 12 —
            // winning it arms the festival report (quest 13) on the
            // spot. Story bit 16 (the ORIGINAL Raid) is not touched:
            // it was already set for anyone who can reach this fight.
            if (bw_qstate3(b,12)==1){ bw_qset3(b,12,2); bw_qset3(b,13,1); }
            b->xp += 80;
            for (int it=0; it<4; it++){ b->items[it] += 1; if (b->items[it]>9) b->items[it]=9; }
            scpy(S->note, "BROKEN RAID, SCRAPPED FOR GOOD. +80 XP. PATCHINA WILL WANT A FESTIVAL.", 72); S->note_t=5.0f; noted=1;
        }
        // story: clearing hackers AT the farm counts toward the quest
        if (b->bw_mode==1 && S->bat.site==1 && bw_qstate(b, 1)==1){
            int k = bw_farmkills(b) + 1; bw_farmkills_set(b, k);
            if (k>=3){
                bw_qset(b, 1, 2); b->xp += 30;
                scpy(S->note, "QUEST DONE: CLEAR THE SERVER FARM. REPORT TO PATCHINA.", 72); S->note_t = 4.5f; noted = 1;
            } else {
                char *q=S->note; q=sappend(q, "SERVER FARM CLEARED: "); q=sapp_int(q, k); q=sappend(q, "/3"); *q=0;
                S->note_t = 2.2f; noted = 1;
            }
        }
        // story: clearing hackers ON the frozen lake counts for Glitch
        if (b->bw_mode==1 && S->bat.site==2 && S->bat.boss==0 && bw_qstate2(b,6)==1){
            int k = bw_lakekills(b) + 1; if (k>2) k=2; bw_lakekills_set(b, k);
            if (k>=2){ scpy(S->note, "LAKE CLEARED: 2/2. GLITCH WILL WANT WORD.", 72); S->note_t = 3.6f; noted = 1; }
            else { char *q=S->note; q=sappend(q, "LAKE CLEARED: "); q=sapp_int(q, k); q=sappend(q, "/2"); *q=0; S->note_t = 2.2f; noted = 1; }
        }
        // hackers sometimes drop something useful (a BW-only consumable)
        if (rng_f()<0.25f){
            int it = (int)(rng_f()*3.99f); if (it>3) it=3;
            if (b->items[it]<9){
                b->items[it]++;
                if (!noted){ char *q=S->note; q=sappend(q, "THE HACKER DROPPED: "); q=sappend(q, BW_ITEM_NAME[it]); q=sappend(q, "!"); *q=0; S->note_t = 2.6f; }
            }
        }
    }
    if (b->hp<=0){ b->hp=0; b->fainted=1; bw_exit(pi, 1); }
}
static const char *bw_foe_name(BwSim *S){
    return S->bat.boss==1? "BROKEN RAID" : S->bat.boss==2? "ZERO-DAY ZED" : S->bat.boss==3? "CACHE HOARDER" : S->bat.boss==4? "BROKEN RAID MK. II" : bw_hacker_name(S->bat.lvl);
}
static void bw_foe_strike(int pi){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    const char *mv = BW_HACK_MOVE[(S->bat.lvl + (int)(G.anim_t*7)) % 6];
    float f = (1.0f - 0.12f*b->up_def) * BUD_DEF[b->species%BUDN];
    S->bat.strikes++;
    if ((S->bat.boss==1 || S->bat.boss==4) && S->bat.strikes%3==0){   // boss 4 (feat 14) kept the original's mirror trick
        float dmg = S->bat.lastdmg * 0.5f * f; if (dmg<3) dmg=3;
        b->hp -= dmg; if (b->hp<0) b->hp=0;
        scpy(S->bat.msg, "BROKEN RAID MIRRORS YOUR LAST MOVE!", 96);
        { char *q2=S->bat.msg2; q2=sappend(q2, BUD_ELF[b->name_idx%BUD_NAMES_N]); q2=sappend(q2, " TOOK ");
          q2=sapp_int(q2, (int)(dmg+0.5f)); q2=sappend(q2, " DAMAGE. RUDE."); *q2=0; }
        sfx(4); return;
    }
    if (S->bat.boss==3 && S->bat.strikes%4==0){
        // the Hoarder's trick: it HOARDS the blow — no damage, and it
        // feels better for it. Hit it three times between its hoards.
        S->bat.hhp += 30; if (S->bat.hhp > S->bat.hmax) S->bat.hhp = S->bat.hmax;
        scpy(S->bat.msg, "THE HOARDER HOARDS YOUR HIT. IT FEELS BETTER.", 96);
        scpy(S->bat.msg2, "SOMEWHERE, A CACHE SINGS.", 72);
        sfx(11); return;
    }
    if (S->bat.boss==2 && S->bat.hhp <= S->bat.hmax*0.34f && !S->bat.zcharge){
        S->bat.zcharge = 1;
        scpy(S->bat.msg, "ZERO-DAY ZED IS CHARGING SOMETHING AWFUL...", 96);
        scpy(S->bat.msg2, "BRACE. OR RUN. BOTH ARE VALID.", 72);
        sfx(16); return;
    }
    float mult2 = 1.0f;
    if (S->bat.boss==2){
        if (S->bat.zcharge){ mult2 = 2.5f; S->bat.zcharge = 0; }
        else if (S->bat.hhp <= S->bat.hmax*0.67f) mult2 = 2.0f;
    }
    float dmg = (4.0f + 3.2f*S->bat.lvl) * rng_range(0.85f, 1.15f) * f * mult2;
    b->hp -= dmg; if (b->hp<0) b->hp=0;
    { char *q=S->bat.msg; q=sappend(q, bw_foe_name(S)); q=sappend(q, " USED "); q=sappend(q, mv); q=sappend(q, "!"); *q=0;
      char *q2=S->bat.msg2; q2=sappend(q2, BUD_ELF[b->name_idx%BUD_NAMES_N]); q2=sappend(q2, " TOOK ");
      q2=sapp_int(q2, (int)(dmg+0.5f)); q2=sappend(q2, " DAMAGE."); *q2=0; }
    if (S->bat.boss==2){
        int ph = S->bat.hhp > S->bat.hmax*0.67f? 0 : S->bat.hhp > S->bat.hmax*0.34f? 1 : 2;
        if (ph > S->bat.zph){ S->bat.zph = ph;
            scpy(S->bat.msg2, ph==1? "ZED: FINE. BOTH HANDS, THEN." : "ZED: ZERO DAY. TOMORROW IS CANCELLED.", 72); }
    }
    sfx(4);
}
// execute the highlighted attack (shared by the input handler and tests)
static void bw_battle_pick(int pi){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    int mv = S->bat.attacks[S->bat.sel];
    const char *nm; float cost, mult; bw_attack_info(b, mv, &nm, &cost, &mult);
    if (mv>=6){   // a consumable: it spends the turn (Ctrl-Z spends the battle)
        int it = mv-6;
        if (b->items[it]<=0) return;
        b->items[it]--;
        if (mv==9){ scpy(S->bat.msg, "CTRL-Z! THE LAST TEN SECONDS UNDO THEMSELVES.", 96); scpy(S->bat.msg2, "", 72); S->bat.phase=6; S->bat.t=1.0f; sfx(11); return; }
        if (mv==6){ b->hp += b->maxhp*0.4f; if (b->hp>b->maxhp) b->hp=b->maxhp;
            char *q=S->bat.msg; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " USED PATCH NOTES."); *q=0;
            char *q2=S->bat.msg2; q2=sappend(q2, "BACK TO "); q2=sapp_int(q2, (int)b->hp); q2=sappend(q2, " HP."); *q2=0; }
        else if (mv==7){ b->energy += 50; if (b->energy>BUD_ENERGY_MAX) b->energy=BUD_ENERGY_MAX;
            char *q=S->bat.msg; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " GLUGGED AN ENERGY DRINK."); *q=0;
            scpy(S->bat.msg2, "THE WORLD VIBRATES SLIGHTLY FASTER.", 72); }
        else { S->buff=1;
            char *q=S->bat.msg; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " ATE THE ADRENAL COOKIE."); *q=0;
            scpy(S->bat.msg2, "NEXT HIT LANDS DOUBLE.", 72); }
        S->bat.phase=1; S->bat.t=1.15f; sfx(11);
        return;
    }
    if (mv==5){
        if (rng_f() < 0.75f){ scpy(S->bat.msg, "GOT AWAY CLEAN!", 96); scpy(S->bat.msg2, "", 72); S->bat.phase=6; S->bat.t=1.0f; sfx(11); }
        else { scpy(S->bat.msg, "COULDN'T GET AWAY!", 96); scpy(S->bat.msg2, "", 72); bw_foe_strike(pi); S->bat.phase=2; S->bat.t=1.2f; }
        return;
    }
    if (b->energy < cost){ scpy(S->bat.msg, "NOT ENOUGH ENERGY!", 96); scpy(S->bat.msg2, "SLEEP AT A TENT TO REFILL.", 72); sfx(16); return; }
    b->energy -= cost;
    if (S->buff){ mult *= 2.0f; S->buff=0; }   // the Adrenal Cookie, cashing in
    float dmg = buddy_dmg(b) * 3.2f * mult * rng_range(0.9f, 1.1f);
    S->bat.lastdmg = dmg;
    S->bat.hhp -= dmg; if (S->bat.hhp<0) S->bat.hhp=0;
    b->lunge_t = 0.22f;
    { char *q=S->bat.msg; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " USED "); q=sappend(q, nm); q=sappend(q, "!"); *q=0;
      char *q2=S->bat.msg2; q2=sappend(q2, "IT TOOK "); q2=sapp_int(q2, (int)(dmg+0.5f)); q2=sappend(q2, " DAMAGE."); *q2=0; }
    S->bat.phase=1; S->bat.t=1.15f; sfx(3);
}
// touch parity for the battle menu (Daniel, Nov 4: on mobile there
// was no way to tap an attack — the menu only listened to keys/pads).
// Tap a move in the grid to use it. Geometry mirrors bw_battle_draw.
static void bw_battle_tap(int pi, int vx, int vy, int vw2, int vh2){
    BwSim *S = &BWS[pi];
    if (!S->bat.active || S->bat.phase!=0) return;
    int rows = (S->bat.natt+1)/2, bh = 84 + (rows>3? (rows-3)*15 : 0), rstep = rows>3? 15 : 18;
    int top = vy+vh2-bh+22;                        // first row's text y
    if (IN.tap_y < top-6 || IN.tap_y >= top-6 + rows*rstep) return;
    if (IN.tap_x < vx+10) return;
    int r = (IN.tap_y - (top-6)) / rstep;
    int c = (IN.tap_x - (vx+16)) / (vw2/2-20); if (c<0) c=0; if (c>1) c=1;
    int idx = r*2 + c;
    if (idx<0 || idx>=S->bat.natt) return;
    S->bat.sel = idx; IN.tapped = 0; bw_battle_pick(pi);
}
static void bw_battle_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    if (S->bat.phase==0){
        if (in->up){ S->bat.sel = (S->bat.sel + S->bat.natt - 1) % S->bat.natt; sfx(6); }
        if (in->down){ S->bat.sel = (S->bat.sel + 1) % S->bat.natt; sfx(6); }
        if (in->sel) bw_battle_pick(pi);
        return;
    }
    S->bat.t -= dt;
    if (S->bat.t>0) return;
    if (S->bat.phase==5){ S->bat.phase=0; }
    else if (S->bat.phase==1){
        if (S->bat.hhp<=0){
            int xp = 4 + 2*S->bat.lvl; b->xp += xp;   // calibrated Nov 3 (Daniel: BW paid far too much, too easily)
            { char *q=S->bat.msg; q=sappend(q, bw_foe_name(S)); q=sappend(q, " CRASHED!"); *q=0;
              char *q2=S->bat.msg2; q2=sappend(q2, BUD_ELF[b->name_idx%BUD_NAMES_N]); q2=sappend(q2, " GAINED ");
              q2=sapp_int(q2, xp); q2=sappend(q2, " XP."); *q2=0; }
            S->bat.phase=3; S->bat.t=1.6f; sfx(11);
        } else { bw_foe_strike(pi); S->bat.phase=2; S->bat.t=1.2f; }
    }
    else if (S->bat.phase==2){
        if (b->hp<=0){ scpy(S->bat.msg, "YOUR BUDDY FAINTED!", 96); scpy(S->bat.msg2, "EVERYTHING GOES DARK...", 72); S->bat.phase=4; S->bat.t=1.8f; sfx(16); }
        else S->bat.phase=0;
    }
    else if (S->bat.phase==3){ bw_battle_end(pi, 1); }
    else if (S->bat.phase==4){ bw_battle_end(pi, 0); }
    else if (S->bat.phase==6){ bw_battle_end(pi, 0); }
}
