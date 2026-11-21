// THE SURVIVAL GAME — buddies/items.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 1043-1127 in the Nov 2026 fine-grained refactor; code unchanged.

static void grant_item(int pi, int kind, int id){
    Player *p=&G.p[pi];
    if (kind==3){ p->hatstand=1; sfx(11); }                       // Psychic Hat Stand
    else if (kind==4){ buddy_adopt(pi, id); sfx(11); }            // adopt a Banzai Buddy
    else if (kind==5){ Buddy *b=&G.bud[pi]; b->species=id;        // re-species the buddy
        buddy_refresh(pi); b->hp=b->maxhp; b->fainted=0;
        b->x=p->x+26; b->y=p->y+12; sfx(11); }
    else if (kind==6){ Buddy *b=&G.bud[pi];                       // REVIVE a fainted buddy
        b->fainted=0; buddy_refresh(pi); b->hp=b->maxhp;
        b->x=p->x+26; b->y=p->y+12;
        splat(b->x, b->y, COL(150,255,170), 14, 110);
        spawn_floater(b->x, b->y-20, "REVIVED!", COL(150,255,170));
        sfx(11); }
    else if (kind==0){ if (p->owned[id]<MAXCOPIES) p->owned[id]++; p->weapon=id; sfx(11); }
    else {
        if (id==5){ p->rollback++; }
        else if (id==7){ p->beans++; }   // one Backfire Bean, holstered and ready to rip
        else if (id==6){ p->hp += 50; if (p->hp > p->maxhp) p->hp = p->maxhp; }
        else p->upg[id]++;
        p->dmg_mul  = 1.0f + 0.25f*p->upg[0];
        p->rate_mul = 1.0f + 0.18f*p->upg[1];
        p->shield_max = shield_for(p->upg[2]);
        p->maxhp = 100 + 25*p->upg[3];
        if (p->hp > p->maxhp) p->hp = p->maxhp;
        sfx(11);
    }
    if (pi==G.my_id){
        scpy(G.store_msg, kind==3? "THE STAND IS YOURS!"
                                  : kind==4? "BUDDY ADOPTED!"
                                  : kind==6? "BUDDY REVIVED!"
                                  : kind==5? "NEW BUDDY - SAME TRAINING!"
                                  : kind==0? "WEAPON INSTALLED!"
                                  : (id==7? "ONE BACKFIRE BEAN, READY TO RIP" : id==6? "PATCHED UP! +50 HP" : "UPGRADE INSTALLED!"), 64);
        G.store_msg_t = 150;
    }
    push_ev(3, p->x, p->y);
}
static void start_payment(int pi, int kind, int id, int price){
    pay_active=1; pay_player=pi; pay_kind=kind; pay_id=id;
    pay_left=(float)price; pay_total=(float)price; pay_t=0;
}
static void update_payment(float dt){
    if (!pay_active) return;
    Player *p=&G.p[pay_player];
    pay_t -= dt;
    if (pay_t<=0){
        pay_t = 0.05f;
        // in the mall the carcasses fly to the counter, not the plaza
        float fdx = p->in_mall? p->x : 0, fdy = p->in_mall? p->y-70 : -30;
        // BATCH COUNTING (Daniel, Nov 10): one carcass per tick made a
        // big hat take forever to pay with — at scale the till counts
        // in handfuls. The batch grows with how many carcasses the
        // remaining bill will take, so a small price still counts out
        // one by one (the charm of the till) while a 400-carcass hat
        // settles in about a second. The exact-change slice still
        // finishes every payment, so the totals never drift.
        int need = 0; float acc = 0;
        for (int i=p->stack_n-1; i>=0 && acc<pay_left; i--){ acc += p->stack[i]; need++; }
        int batch = need<=6? 1 : need<=24? 3 : need<=80? 6 : need<=200? 12 : 20;
        int fx_left = 2;   // only the first couple of each handful fly visibly
        for (int k=0;k<batch && pay_left>0 && p->stack_n>0;k++){
            if (p->stack[p->stack_n-1] >= pay_left){
                // the till slices the top carcass: exact change, the rest
                // stays on the hat (bugs are weighed, not coinage)
                float w = pay_left;
                p->stack[p->stack_n-1] -= w;
                if (p->stack[p->stack_n-1] <= 0.01f) p->stack_n--;
                p->carried -= w; if (p->carried<0) p->carried=0;
                if (fx_left>0){ spawn_fx(1, p->x, p->y-26, fdx, fdy, w, pay_player); fx_left--; }
                pay_left = 0;
            } else {
                float w = hat_pop(p);
                if (w>0 && fx_left>0){ spawn_fx(1, p->x, p->y-26, fdx, fdy, w, pay_player); fx_left--; }
                pay_left -= w;
            }
        }
        sfx(10);
        if (pay_left<=0 || p->stack_n<=0){
            pay_active=0;
            G.total_earned += pay_total;
            grant_item(pay_player, pay_kind, pay_id);
        }
    }
}

