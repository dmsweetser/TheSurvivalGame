// THE SURVIVAL GAME — render/players_draw.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 2204-2421 in the Nov 2026 fine-grained refactor; code unchanged.

static void draw_player_sprite(Player *p){
    { int pi = (int)(p - G.p);   // WHALES ONLY: PARADE DUCK, drawn at its own spot
      if (pi>=0 && pi<MAXP && duck_on[pi] && p->alive){
        int dx2 = SX(duck_x[pi]), dy2 = SY(duck_y[pi]);
        if (dx2>-30 && dx2<VW+30 && dy2>-30 && dy2<VH+30){
            float dhop = fsin(duck_ph[pi]); if (dhop<0) dhop = -dhop;
            int hop = (int)(dhop*3.0f), fc = duck_face[pi]? duck_face[pi] : 1;
            shadow_ellipse(dx2, dy2+6, 8, 2);
            ellipse_fill(dx2, dy2-hop, 9, 6, COL(250,215,80));
            tri_fill(dx2-fc*7, dy2-2-hop, dx2-fc*12, dy2-5-hop, dx2-fc*7, dy2+1-hop, COL(240,200,70));
            disc_fill(dx2+fc*7, dy2-5-hop, 4, COL(250,220,95));
            tri_fill(dx2+fc*10, dy2-1-hop, dx2+fc*15, dy2+1-hop, dx2+fc*10, dy2+3-hop, COL(240,130,40));
            pset(dx2+fc*8, dy2-6-hop, COL(30,26,44));
        }
      } }
    int x=SX(p->x), y=SY(p->y);
    if (x<-90||x>VW+90||y<-190||y>VH+40) return;
    if ((p->whale_on & 2048) && p->alive){   // WHALES ONLY: AURA OF IMPORTANCE
        int pr = 17 + (int)(fsin(G.anim_t*3.0f + (float)(p->color&3))*2);
        ellipse_fill(x, y+11, pr, 5, COL(240,196,80));
        ellipse_fill(x, y+11, pr-5, 3, COL(255,226,130));
    }
    // bean-borne: dropping out of the sky onto the plaza — the body
    // rides high and settles; the shadow stays on the ground
    int bean_lift=0;
    if (p->bean_t>0){ float e=1.6f-p->bean_t;
        if (e>=0.45f && e<1.05f){ float k=(e-0.45f)/0.6f; bean_lift=(int)((1.0f-k)*(1.0f-k)*90.0f); } }
    if (p->bean_jet>0) bean_lift = 13 + (int)(fsin(G.anim_t*7.0f)*2.0f);   // the jetpack hover
    else if (p->jet_fall>0) bean_lift = (int)(p->jet_fall*30.0f);          // tumbling back down
    shadow_ellipse(x, y+11, 13, 4);
    if (bean_lift>0) y-=bean_lift;
    if (p->on_wall>=0) y-=8;          // up on top of an interlocked wall run
    if ((p->whale_on & 4) && p->alive){   // WHALES ONLY: the ROYAL CAPE, worn behind everything
        int bx2 = x - (int)(fcos(p->aim)*9), by2 = y + 2 - (int)(fsin(p->aim)*5);
        int sway = (int)(fsin(G.anim_t*2.4f + (float)(p->color&3))*3);
        tri_fill(x-7, y-10, x+7, y-10, bx2-11+sway, by2+22, COL(150,30,44));
        tri_fill(x+7, y-10, bx2+11+sway, by2+22, bx2-11+sway, by2+22, COL(150,30,44));
        tri_fill(x-7, y-10, bx2-11+sway, by2+22, bx2, by2+16, COL(120,22,36));
        disc_fill(x, y-10, 2, COL(255,214,90));   // the gold clasp
    }
    int ibob=0, iblink=0;             // idle breathing/blink (set in the body draw)
    if (!p->alive){
        draw_carcass(x, y+4, 9, 0, 0.4f);                          // knocked out
        draw_text_center(x, y-18, "DOWN", 1, COL(255,120,120));
        return;
    }
    if (p->hurt_t>0 && ((int)(G.anim_t*20))%2) { /* flash: skip body this blink */ }
    else {
        u32 hood = HOODIE[p->color&3];
        u32 hair = HAIR_COL[p->color&3];
        // the face leans toward the facing direction
        int fx = (int)(fcos(p->aim)*3), fy = (int)(fsin(p->aim)*2);
        // idle life: standing still, the DEV breathes (a slow bob that
        // carries the head and the whole hat with it), blinks every few
        // seconds, and after a while starts glancing around for bugs
        if (p->idle_t>0.25f){
            ibob = (int)(fsin(G.anim_t*2.1f + (float)(p->color&3)*1.7f)*1.2f);
            float ph = G.anim_t + (float)(p->color&3)*1.31f; ph -= (int)(ph/3.4f)*3.4f;
            iblink = (ph < 0.11f);
            if (p->idle_t>1.5f) fx += (int)(fsin(G.anim_t*0.55f + (float)(p->color&3)*2.3f)*2.0f);
        }
        // legs and animated feet: alternating stride along the facing
        // direction, and each shoe lifts as it swings forward
        int lp = (int)(fsin(p->walk_ph)*4);
        float sax = fcos(p->aim), say = fsin(p->aim);
        float swl = fsin(p->walk_ph), swr = fsin(p->walk_ph + PI);
        float cl = fcos(p->walk_ph), cr = fcos(p->walk_ph + PI);
        int lfx = x-5 + (int)(sax*swl*4), lfy = y+11 + (int)(say*swl*3) - (cl>0? (int)(cl*2):0);
        int rfx = x+4 + (int)(sax*swr*4), rfy = y+11 + (int)(say*swr*3) - (cr>0? (int)(cr*2):0);
        line_thick(x-5, y+3, lfx, lfy-1, 5, COL(64,64,104));   // jeans down to each shoe
        line_thick(x+4, y+3, rfx, rfy-1, 5, COL(64,64,104));
        ellipse_fill(lfx, lfy, 5, 3, COL(42,36,46));           // shoes
        ellipse_fill(rfx, rfy, 5, 3, COL(42,36,46));
        line_thick(lfx-4, lfy+3, lfx+4, lfy+3, 1, COL(205,200,210));  // soles
        line_thick(rfx-4, rfy+3, rfx+4, rfy+3, 1, COL(205,200,210));
        // torso: shaded hoodie, pocket, zipper
        ellipse_fill(x, y, 13, 14, OUTLINE);
        ellipse_shade(x, y, 11, 12, hood);
        rect_fill(x-6, y+5, 12, 4, col_shade(hood, 0.8f));
        line_thick(x, y-10, x, y+4, 1, col_shade(hood, 0.68f));
        // hoodie drawstrings with little aglets
        line_thick(x-3, y-8, x-3, y-3, 1, COL(235,235,240));
        line_thick(x+3, y-8, x+3, y-3, 1, COL(235,235,240));
        circle_fill(x-3, y-2, 1, COL(235,235,240));
        circle_fill(x+3, y-2, 1, COL(235,235,240));
        // free arm, swinging against the stride
        line_thick(x-9, y-3, x-12, y+3+(lp>0?-1:1), 3, col_shade(hood, 0.9f));
        circle_fill(x-12, y+4+(lp>0?-1:1), 2, SKIN);
        // ---- the person: neck, ears, shaded head, hair, and the glasses
        // (drawn before the aim arm so the weapon stays on top)
        rect_fill(x-3, y-11+ibob, 6, 4, col_shade(SKIN, 0.94f));
        circle_fill(x-8, y-14+ibob, 2, SKIN);
        circle_fill(x+8, y-14+ibob, 2, SKIN);
        circle_fill(x, y-15+ibob, 9, OUTLINE);
        circle_shade(x, y-15+ibob, 8, SKIN);
        gloss(x, y-15+ibob, 8);
        // hair: a cap swept across the top, thinning at the temples
        ellipse_fill(x-fx/2, y-20+ibob, 8, 4, hair);
        rect_fill(x-8-fx/2, y-20+ibob, 3, 5, hair);
        rect_fill(x+5-fx/2, y-20+ibob, 3, 4, hair);
        // COMICALLY BIG NERD GLASSES: the lenses are wider than the head.
        // Chunky triple frames, pale lenses with a glint, eyes behind them.
        // The whole face rides a few px toward the facing direction.
        int gw2 = (p->whale_on & 2)? 19 : 13, gh2 = (p->whale_on & 2)? 16 : 12;   // WHALES ONLY: ENORMOUS-ER
        int gy = y-22+fy+ibob - (gh2-12)/2;
        int gl = x-16+fx - (gw2-13), gr = x+3+fx;       // lens box origins, pulled apart so the two lenses read as separate glasses
        line_thick(gl+1, gy+4, x-9, y-14+ibob, 2, OUTLINE);  // temple arms
        line_thick(gr+gw2-1, gy+4, x+9, y-14+ibob, 2, OUTLINE);
        blend_rect(gl, gy, gw2, gh2, COL(200,230,255), 60); // lens sheen
        blend_rect(gr, gy, gw2, gh2, COL(200,230,255), 60);
        for (int s=0;s<3;s++){                          // triple-drawn frames
            rect_line(gl-s, gy-s, gw2+2*s, gh2+2*s, OUTLINE);
            rect_line(gr-s, gy-s, gw2+2*s, gh2+2*s, OUTLINE);
        }
        rect_fill(gl+gw2, gy+3, gr-(gl+gw2), 2, OUTLINE); // slim bridge, clear space between the lenses
        // diagonal glints on each lens
        pset_blend(gl+3, gy+8, COL(255,255,255), 170); pset_blend(gl+4, gy+7, COL(255,255,255), 170);
        pset_blend(gl+5, gy+6, COL(255,255,255), 170); pset_blend(gl+6, gy+5, COL(255,255,255), 140);
        pset_blend(gr+3, gy+8, COL(255,255,255), 170); pset_blend(gr+4, gy+7, COL(255,255,255), 170);
        pset_blend(gr+5, gy+6, COL(255,255,255), 170); pset_blend(gr+6, gy+5, COL(255,255,255), 140);
        if (iblink){                                    // closed for a blink
            line_thick(x-9+fx, gy+6, x-5+fx, gy+6, 1, OUTLINE);
            line_thick(x+5+fx, gy+6, x+9+fx, gy+6, 1, OUTLINE);
        } else {
            circle_fill(x-7+fx, gy+6, 2, OUTLINE);          // eyes, magnified by the lenses
            circle_fill(x+7+fx, gy+6, 2, OUTLINE);
            pset(x-8+fx, gy+5, COL(255,255,255));
            pset(x+6+fx, gy+5, COL(255,255,255));
        }
        line_thick(x-11+fx, gy-3, x-5+fx, gy-3, 1, hair);  // brows peek over the top
        line_thick(x+5+fx, gy-3, x+11+fx, gy-3, 1, hair);
        line_thick(x-2+fx, y-8+fy+ibob, x, y-7+fy+ibob, 1, COL(196,128,104));  // smile below the frames
        line_thick(x, y-7+fy+ibob, x+2+fx, y-8+fy+ibob, 1, COL(196,128,104));
        // aim arm: hoodie sleeve to the elbow, skin to the grip
        int hx = x + (int)(fcos(p->aim)*13), hy = y-4 + (int)(fsin(p->aim)*10);
        int ex = (x+hx)/2, ey = (y-3+hy)/2;
        line_thick(x, y-3, ex, ey, 4, col_shade(hood, 0.92f));
        line_thick(ex, ey, hx, hy, 3, SKIN);
        int w = p->weapon;
        if (w==0){ rect_fill(hx-6, hy-3, 13, 7, OUTLINE); rect_fill(hx-5, hy-2, 11, 5, COL(200,200,215));
                   for (int k=0;k<3;k++) rect_fill(hx-3+k*4, hy-1, 2, 2, COL(90,90,120)); }
        else if (w==1){ ellipse_out(hx, hy, 5, 4, COL(170,170,190)); circle_fill(hx+1, hy-1, 1, COL(240,80,80)); }
        else if (w==2){ circle_out(hx, hy, 5, COL(255,215,60)); circle_fill(hx+4, hy-2, 2, COL(255,150,40)); }
        else if (w==3){ rect_fill(hx-4, hy-5, 10, 8, COL(120,80,40)); rect_fill(hx-4, hy-5, 10, 3, COL(90,60,30));
                        draw_text_plain(hx-3, hy-4, "PM", 1, COL(255,230,150)); }
        else { rect_fill(hx-5, hy-2, 11, 5, COL(60,60,80)); circle_fill(hx+6, hy, 2, COL(255,60,60)); }
        if (w==4 && beam_vis[p-G.p]>0){
            int nb = beam_n[p-G.p]; if (nb<1) nb=1;
            float bf = beam_frac[p-G.p]; if (bf<=0) bf=1;
            int bl = (int)(beam_len[p-G.p] * bf); if (bl<2) bl=2;
            for (int k=0;k<nb;k++){
                float ba = beam_ang[p-G.p][k];
                int bx = x + (int)(fcos(ba)*bl), by = y-4 + (int)(fsin(ba)*bl);
                line_thick(x, y-4, bx, by, 3, COL(120,200,255));
                line_thick(x, y-4, bx, by, 1, COL(230,250,255));
                circle_fill(bx, by, 4, COL(160,220,255));
            }
        }
        // shield bubble: a ring of firewall sparks orbiting the player
        if (p->shield>0){
            for (int a=0;a<14;a++){
                float t = G.anim_t*0.9f + a*(TAU/14);
                int px2 = x+(int)(fcos(t)*19), py2 = y-6+(int)(fsin(t)*22);
                pset(px2, py2, COL(170,220,255)); pset(px2+1, py2, COL(120,190,255));
            }
        }
        // rollback grace: a green time-ring while you're untouchable
        if (p->revive_t>0){
            for (int a=0;a<14;a++){
                float t = -G.anim_t*2.4f + a*(TAU/14);
                int px2 = x+(int)(fcos(t)*20), py2 = y-6+(int)(fsin(t)*23);
                disc_fill(px2, py2, 2, COL(120,255,160));
            }
        }
    }
    draw_hat(p, x, y + ibob);
    // name tag
    draw_text_center(x, y+18, p->name, 1, COL(255,255,255));
    if (p->whale_on & 64)   // WHALES ONLY: the title, floating in gold
        draw_text_center(x, y+29, "~ WHALE ~", 1, COL(255,205,40));
    if (p->whale_on & 32){  // WHALES ONLY: DRAMATIC RAIN — on this player, only this player
        int fall = (int)(G.anim_t*95);
        for (int i2=0;i2<14;i2++){
            int rx = x - 17 + (int)(((unsigned)(i2*2654435761u)>>13) % 35u);
            int ry = y - 36 + (i2*29 + fall) % 48;
            line_thick(rx, ry, rx+2, ry+7, 1, COL(150,200,255));
        }
    }
    // hp pips over head when hurt
    if (p->hp < p->maxhp){
        rect_fill(x-11, y-26, 22, 3, COL(40,30,40));
        rect_fill(x-11, y-26, 22*p->hp/p->maxhp, 3, COL(120,230,90));
    }
    // THE BACKFIRE BEAN, riding the shoulder through his own ride:
    // goggles down, cheeks puffed, entirely pleased with himself.
    if (p->bean_t>0 && p->alive){
        float e = 1.6f - p->bean_t;
        int jx = (e<0.45f && ((int)(G.anim_t*40))%2)? 1:0;   // the sputter shake
        int bx2 = x+11+jx, by2 = y-8+jx;
        ellipse_fill(bx2, by2+6, 6, 3, COL(240,248,230));        // his little cloud seat
        ellipse_fill(bx2, by2, 6, 7, COL(126,196,86));            // bean body
        ellipse_fill(bx2-2, by2-2, 2, 3, COL(168,226,120));      // the shine
        rect_fill(bx2-6, by2-6, 12, 3, COL(90,70,50));           // goggle strap
        disc_fill(bx2-3, by2-4, 2, COL(220,240,255));            // goggles
        disc_fill(bx2+3, by2-4, 2, COL(220,240,255));
        pset(bx2-3, by2-4, COL(30,26,44)); pset(bx2+3, by2-4, COL(30,26,44));
        if (e>=0.45f && e<1.05f){   // mid-flight: the exhaust does the talking
            disc_fill(bx2, by2+11, 4, COL(220,240,200));
            disc_fill(bx2-3, by2+15, 3, COL(190,225,150));
            disc_fill(bx2+3, by2+18, 2, COL(168,216,120));
        }
        if (e>=1.05f){   // touchdown: stars, his and yours alike
            int s1 = (int)(fsin(G.anim_t*5.0f)*8), s2 = (int)(fsin(G.anim_t*5.0f+2.0f)*8);
            rect_fill(x+s1-1, y-27, 3, 1, COL(255,220,90)); rect_fill(x+s1, y-28, 1, 3, COL(255,220,90));
            rect_fill(x-s2-1, y-23, 3, 1, COL(255,220,90)); rect_fill(x-s2, y-24, 1, 3, COL(255,220,90));
        }
    }
}
