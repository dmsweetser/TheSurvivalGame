// THE SURVIVAL GAME — sim/bugs.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 883-1049 in the Nov 2026 fine-grained refactor; code unchanged.

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
        // the buddy quarry counts only while the buddy is IN THE
        // FIELD: when its owner steps into the Mall (or the buddy
        // is in Banzai World) the bug falls back to hunting players
        // — and freezes with the rest while everybody shops (the
        // shield doctrine above). Without this, a buddy-tagged bug
        // — above all the named elite (feat 8) — measured its
        // "left behind" distance to the buddy's Mall position and
        // was culled the moment the party went shopping.
        if (b->btarget>=0 && G.bud[b->btarget].active && G.bud[b->btarget].hp>0 && !G.bud[b->btarget].fainted
            && !G.bud[b->btarget].in_bw && !G.p[b->btarget].in_mall){
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

