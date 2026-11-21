// THE SURVIVAL GAME — combat/combat.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 1436-1568 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Combat
// ---------------------------------------------------------------------------
static void fire_proj(int owner, int kind, float x, float y, float ang, float dmg, int agent){
    for (int i=0;i<MAXPROJ;i++) if (!G.projs[i].alive){
        Proj *q=&G.projs[i];
        q->alive=1; q->kind=kind; q->owner=owner; q->x=x; q->y=y; q->dmg=dmg; q->agent=agent;
        q->bounces=0; q->target=-1; q->spin=rng_f()*TAU;
        if (kind==0){ float s=430; q->vx=fcos(ang)*s; q->vy=fsin(ang)*s; q->life=0.85f; }
        if (kind==1){ float s=330; q->vx=fcos(ang)*s; q->vy=fsin(ang)*s; q->life=1.15f; }
        if (kind==2){ float s=360; q->vx=fcos(ang)*s; q->vy=fsin(ang)*s; q->life=1.5f; }
        if (kind==3){ float s=265; q->vx=fcos(ang)*s; q->vy=fsin(ang)*s; q->life=1.6f; }
        return;
    }
}
static void hurt_bug(int bi, float dmg, int owner, float kx, float ky);
static void hurt_buddy(int owner, float dmg);
static void explode_pm(float x, float y, float dmg, int owner, int agent){
    splat(x, y, COL(255,200,80), 16, 130);
    for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
        float dx=G.bugs[i].x-x, dy=G.bugs[i].y-y;
        if (dx*dx+dy*dy < 95*95){
            float d=fsqrt(dx*dx+dy*dy)+0.01f;
            hurt_bug(i, dmg, owner, dx/d*180, dy/d*180);
        }
    }
    spawn_floater(x, y-14, "SYNERGY!", COL(255,220,90));
    sfx(7);
}
static void kill_bug(int bi, int owner){
    Bug *b=&G.bugs[bi];
    b->alive=0;
    G.total_kills++;
    // NAMED ELITE (v1.25 feat 8): the bounty's bug is down. Record
    // the claim for bn_update (save.c) to settle on the frame tail —
    // the carcass below still flies to the killer's hat exactly like
    // any kill; the bonus (weight again in the killer's bank, crew
    // XP) is the board's business, not combat's.
    if (b->elite){
        G.bn_claim_pending = 1;
        G.bn_claim_pi = owner;
        G.bn_claim_w = b->w;
        G.bn_claim_name = b->elite - 1;
        b->elite = 0;
    }
    // assist XP: a buddy that had bitten this bug learns from its fall
    if (b->btarget>=0 && b->btarget!=owner && G.bud[b->btarget].active && !G.bud[b->btarget].fainted)
        G.bud[b->btarget].xp += 2 + (int)(b->w*0.3f);
    if (owner>=0 && owner<MAXP){
        G.p[owner].kills++;
        // the carcass flies to the killer's head and joins the stack
        CarcassFx *f = spawn_fx(0, b->x, b->y, G.p[owner].x, G.p[owner].y-24, b->w, owner);
        if (!f) hat_push(&G.p[owner], b->w);           // fx full: instant
    }
    splat(b->x, b->y, BUG_COL[b->kind], 10, 110);
    // two-tone burst (glow-up): pale belly bits drift longer, and a
    // few white sparks snap out fast — a squash should FEEL juicy
    for (int i=0;i<5;i++) spawn_particle(b->x, b->y, BUG_BELLY[b->kind], 70, 0.8f, 2);
    for (int i=0;i<3;i++) spawn_particle(b->x, b->y, COL(255,252,240), 170, 0.3f, 1);
    if (owner>=0 && owner<MAXP && (G.p[owner].whale_on & 1024)){   // WHALES ONLY: CONFETTI SQUASH
        static const u32 PARTY[6] = { COL(255,90,120), COL(255,200,60), COL(90,220,120),
                                      COL(90,180,255), COL(200,120,255), COL(255,140,60) };
        for (int i=0;i<14;i++) spawn_particle(b->x, b->y, PARTY[i%6], 95, 1.15f, 2);
    }
    push_ev(1, b->x, b->y);
    sfx(5);
}
static void hurt_bug(int bi, float dmg, int owner, float kx, float ky){
    Bug *b=&G.bugs[bi];
    if (!b->alive) return;
    b->hp -= dmg; b->hit_t = 0.12f;
    b->vx += kx*0.4f; b->vy += ky*0.4f;
    if (b->hp<=0) kill_bug(bi, owner);
}
// a bean jetpack ends the moment its rider takes a hit or fires
// (Daniel, Nov 10): the bean dumps you where you are, mid-field.
static void bean_jet_end(int pi, int shot){
    Player *p=&G.p[pi];
    if (p->bean_jet<=0) return;
    p->bean_jet=0; p->jet_fall=0.32f;
    splat(p->x, p->y+6, COL(168,216,120), 12, 100);
    spawn_floater(p->x, p->y-30, shot? "BACK TO EARTH!" : "KNOCKED FROM THE SKY!", COL(255,220,150));
    sfx(5);
}
static void hurt_player(int pi, float dmg){
    Player *p=&G.p[pi];
    if (!p->alive || G.state!=GS_PLAY) return;
    if (p->revive_t>0) return;               // just rolled back: untouchable
    if (p->bean_jet>0) bean_jet_end(pi, 0);   // one hit ends the jet
    if (p->shield>0){
        float absorbed = dmg < (float)p->shield ? dmg : (float)p->shield;
        p->shield -= (int)absorbed; dmg -= absorbed;
        p->shield_regen_t = 3.0f;
        if (absorbed>0) sfx(8);
    }
    if (dmg>0){
        p->hp -= (int)dmg; p->hurt_t = 0.25f; sfx(9);
        splat(p->x, p->y, COL(230,60,60), 5, 80);
        // ENDGAME PRESSURE (Daniel's pick #1): a heavy hat is a
        // liability. Once you're carrying real weight, every bite that
        // gets through knocks the top carcass clean off your head.
        // What's on your skull is at risk; what's in the Bug Bank is not.
        if (p->carried>=50.0f && p->stack_n>0 && p->shed_cd<=0){
            float w = hat_pop(p);
            if (w>0){
                p->shed_cd = 0.7f;
                splat(p->x, p->y-24, BUG_COL[(int)w%4], 7, 120);
                floater_num(p->x, p->y-40, "-", (int)w, "KG", COL(255,120,110));
            }
        }
    }
    if (p->hp<=0){
        if (p->rollback>0){
            p->rollback--; p->hp = p->maxhp/2; p->shield = p->shield_max/2;
            p->revive_t = 3.0f;              // 3s of grace to get clear
            // and shove the swarm that killed you off your face
            for (int i=0;i<MAXBUGS;i++) if (G.bugs[i].alive){
                float dx=G.bugs[i].x-p->x, dy=G.bugs[i].y-p->y, d2=dx*dx+dy*dy;
                if (d2 < 140*140 && d2>0.01f){ float d=fsqrt(d2); G.bugs[i].vx += dx/d*340; G.bugs[i].vy += dy/d*340; }
            }
            spawn_floater(p->x, p->y-34, "ROLLBACK!", COL(120,255,160));
            splat(p->x, p->y, COL(120,255,160), 22, 160);
            sfx(12);
            return;
        }
        p->alive=0; p->dead_t=0;
        // the hat comes crashing down
        for (int i=0;i<p->stack_n && i<26;i++) spawn_particle(p->x, p->y-20, BUG_COL[i%4], 150, 0.9f, 3);
        p->stack_n=0; p->carried=0;
        sfx(13);
    }
}

