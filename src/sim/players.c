// THE SURVIVAL GAME — sim/players.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 658-816 in the Nov 2026 fine-grained refactor; code unchanged.

static void update_players(float dt){
    for (int i=0;i<MAXP;i++){
        Player *p=&G.p[i];
        if (!p->connected) continue;
        if (p->fire_cd>0 && p->weapon!=4) { /* handled in fire */ }
        if (p->wall_cd>0) p->wall_cd-=dt;
        if (p->shed_cd>0) p->shed_cd-=dt;
        if (p->wof>0) p->wof-=dt;
        p->idle_t += dt;                       // movement zeroes this (idle anim)
        if (p->hurt_t>0) p->hurt_t-=dt;
        if (p->revive_t>0) p->revive_t-=dt;
        if (beam_vis[i]>0) beam_vis[i]-=dt;
        update_beam(i, dt);
        if (!p->alive){ p->dead_t+=dt; continue; }
        if (p->shield_regen_t>0) p->shield_regen_t-=dt;
        else if (p->shield < p->shield_max){
            // 14 points a second, accumulated fractionally: the old
            // line added (int)(14*dt+0.5) a frame, which truncates to
            // 0 at 30-60 fps — the shield silently never recharged.
            shield_frac[i] += 14.0f*dt;
            int whole = (int)shield_frac[i];
            if (whole>0){ p->shield += whole; shield_frac[i] -= (float)whole;
                          if (p->shield>p->shield_max){ p->shield=p->shield_max; shield_frac[i]=0; } }
        }
        // THE BACKFIRE BEAN ride: sputter (0–0.45s), the BLAST teleports
        // you home (0.45s), then you drop out of the sky onto the plaza
        // (to 1.05s) and stand there dizzy (to 1.6s). A passenger does
        // not move or fire — the bean is driving.
        if (p->bean_t>0){
            float eb = 1.6f - p->bean_t;          // elapsed at frame start
            p->bean_t -= dt; if (p->bean_t<0) p->bean_t=0;
            float e = 1.6f - p->bean_t;           // elapsed now
            if (eb<0.45f && e>=0.45f){
                p->x=0; p->y=70;
                Buddy *bb=&G.bud[i];
                if (bb->active && !bb->in_bw){ bb->x=p->x+26; bb->y=p->y+12; }
                splat(p->x, p->y+6, COL(168,216,120), 22, 130);
                splat(p->x, p->y+6, COL(240,248,230), 12, 90);
                spawn_floater(p->x, p->y-34, "PBBBT!", COL(150,220,120));
                sfx(22);
                if (i==G.my_id) do_save();
            } else if (e<0.45f){
                if ((int)(e*14)!=(int)(eb*14))
                    spawn_particle(p->x+rng_range(-6,6), p->y+8, COL(168,216,120), 14, 0.4f, 3);
            } else if (e<1.05f){
                if ((int)(e*20)!=(int)(eb*20))
                    spawn_particle(p->x+rng_range(-5,5), p->y-14, COL(240,248,230), 12, 0.35f, 2);
            } else if (eb<1.05f){
                splat(p->x, p->y+8, COL(168,216,120), 16, 110);
                splat(p->x, p->y+8, COL(240,248,230), 10, 80);
                sfx(5);
            }
            continue;
        }
        // THE BEAN JETPACK (epicenter launches): steer freely, fly
        // over everything at jet speed; firing or one landed hit
        // ends it where you are (see bean_jet_end). No hat burden,
        // no prop collision — you're above all of it.
        if (p->jet_fall>0){
            p->jet_fall -= dt;
            if ((int)(G.time*20)!=(int)((G.time-dt)*20))
                spawn_particle(p->x+rng_range(-5,5), p->y-8, COL(240,248,230), 10, 0.3f, 2);
            continue;
        }
        if (p->bean_jet>0){
            if (p->cur_keys & K_FIRE){ bean_jet_end(i, 1); continue; }
            float jx=0, jy=0;
            if (p->cur_keys & K_UP) jy-=1;
            if (p->cur_keys & K_DOWN) jy+=1;
            if (p->cur_keys & K_LEFT) jx-=1;
            if (p->cur_keys & K_RIGHT) jx+=1;
            float jl = fsqrt(jx*jx+jy*jy);
            if (jl>0){
                jx/=jl; jy/=jl;
                p->aim = fatan2(jy, jx);
                p->x += jx*335.0f*dt; p->y += jy*335.0f*dt;
                p->walk_ph += dt*6; p->idle_t = 0;
            }
            p->on_wall=-1;
            if ((int)(G.time*24)!=(int)((G.time-dt)*24))
                spawn_particle(p->x+rng_range(-4,4), p->y+10, COL(168,216,120), 12, 0.35f, 3);
            continue;
        }
        // movement
        float mx=0, my=0;
        if (p->cur_keys & K_UP) my-=1;
        if (p->cur_keys & K_DOWN) my+=1;
        if (p->cur_keys & K_LEFT) mx-=1;
        if (p->cur_keys & K_RIGHT) mx+=1;
        float ml = fsqrt(mx*mx+my*my);
        if (ml>0){
            mx/=ml; my/=ml;
            // Cyberdogs rules: you face (and therefore fire) the way you move
            p->aim = fatan2(my, mx);
            float sp = player_speed(p);
            float nx = p->x + mx*sp*dt, ny = p->y + my*sp*dt;
            // The mall interior stands on world chunks the junkyard
            // generator also fills — its solid crates and fax machines
            // were colliding in the mall even though draw_mall never
            // draws them (Daniel's "invisible impediments", Nov 3).
            // Inside, only mall_collide governs.
            if (p->in_mall){ p->x = nx; p->y = ny; }
            else {
                if (!prop_collide(nx, p->y, 11)) p->x = nx;
                if (!prop_collide(p->x, ny, 11)) p->y = ny;
            }
            wall_step(p);
            if (in_srcroom[i]) srcroom_collide(p);
            else if (p->in_arcade) arcade_collide(p);
            else if (p->in_mall) mall_collide(p);
            p->walk_ph += dt*sp*0.055f;
            p->idle_t = 0;
            if ((p->whale_on & 8) && !p->in_mall){   // Whales Only: sparkle trail
                whale_spark_t[i] -= dt*sp;
                if (whale_spark_t[i]<=0){ whale_spark_t[i]=13;
                    spawn_particle(p->x + (float)rng_range(-8,8), p->y + (float)rng_range(0,9),
                                   COL(255,230,140), 26, 0.7f, 2); }
            }
            if (p->in_mall && !p->in_arcade && !in_srcroom[i] && speed_curve(p) <= 40.0f){
                // WALK OF FAME (Daniel, Nov 14, 2026): crawling through
                // the mall under a big hat is a parade, not a slog —
                // confetti rains and the marquee flashes while you creep.
                // Gate tightened later the same day at Daniel's word:
                // ONLY at the minimum speed — the burden curve pinned
                // to the 40 px/s floor (a ~150KG+ hat at level 0). A
                // merely slow walk (curve between 40 and the old 64
                // line) earns no parade. Sprinting while pinned still
                // counts: the curve is what is pinned, not the stride.
                p->wof = 0.3f;
                wof_spark_t[i] -= dt*sp;
                if (wof_spark_t[i]<=0){ wof_spark_t[i]=2.2f;   // MORE confetti (Daniel, Nov 14): bursts ~3x as often,
                    for (int k=0;k<6;k++)                        // 3x the pieces, wider and taller
                        spawn_particle(p->x + (float)rng_range(-32,32), p->y - (float)rng_range(4,66),
                                       WOF_COL[(int)(rng_f()*5.99f)], 40, 1.3f, 2 + (int)(rng_f()*2.99f)); }
            }
        } else if (i!=G.my_id && G.net_mode==NET_HOST){
            p->aim = p->net_aim;      // remote player standing still: keep
                                      // the facing they last sent us
        }
        // WHALES ONLY: PARADE DUCK — it follows. It does nothing else.
        if ((p->whale_on & 256) && p->connected && p->alive){
            float ddx = p->x - duck_x[i], ddy = p->y - duck_y[i];
            if (!duck_on[i] || ddx*ddx + ddy*ddy > 300.0f*300.0f){
                duck_x[i] = p->x - fcos(p->aim)*30; duck_y[i] = p->y - fsin(p->aim)*30; duck_on[i] = 1;
            }
            float tx = p->x - fcos(p->aim)*30, ty = p->y - fsin(p->aim)*30;
            float dx = tx - duck_x[i], dy = ty - duck_y[i], dd = fsqrt(dx*dx + dy*dy);
            if (dd > 4){
                float dsp = 165.0f*dt; if (dsp > dd) dsp = dd;
                duck_x[i] += dx/dd*dsp; duck_y[i] += dy/dd*dsp;
                duck_ph[i] += dt*11; if (dx < -1) duck_face[i] = -1; else if (dx > 1) duck_face[i] = 1;
            }
        } else duck_on[i] = 0;
        if (!p->in_mall && p->on_wall<0) collide_hut(&p->x, &p->y, 11);
        if (G.state==GS_PLAY) player_fire(p, i, dt);
        else if (p->fire_cd>0) p->fire_cd-=dt;
        if (p->want_wall){ p->want_wall=0; if (G.state==GS_PLAY) place_wall(i); }
    }
}

