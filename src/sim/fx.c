// THE SURVIVAL GAME — sim/fx.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 1050-1080 in the Nov 2026 fine-grained refactor; code unchanged.

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

