// THE SURVIVAL GAME — sim/projectiles.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 817-882 in the Nov 2026 fine-grained refactor; code unchanged.

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
