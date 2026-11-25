// THE SURVIVAL GAME — modes/cart/cart_render.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from cart.c lines 760-1340 in the Nov 2026 fine-grained refactor; code unchanged.

static void crt_draw_driver(int cx, int seat_y, int w, int species, u32 coat){
    int hr = w * 19 / 100; if (hr < 3) hr = 3;
    u32 dark = col_shade(coat, 0.64f);
    int hx = cx, hy = seat_y - hr/2;
    if (species == 3)   // platypus: the flat tail rides behind the seat
        ellipse_fill(hx, seat_y + w*20/100, hr + hr/3, hr/2 + 1, dark);
    rect_fill(cx - w*23/100, seat_y, w*46/100, w*23/100, dark);   // shoulders and back
    rect_fill(cx - w*23/100, seat_y + w*2/100, w*46/100, w*5/100 > 1? w*5/100 : 1, col_shade(dark, 1.30f));   // shoulder sheen (v1.27 polish)
    if (species == 5){   // KITCHEN SINK: basin silhouette, faucet on top
        rect_fill(hx - hr, hy - hr/2, hr*2, hr + hr/2, coat);
        rect_line(hx - hr, hy - hr/2, hr*2, hr + hr/2, dark);
        rect_fill(hx + hr/4, hy - hr - hr/3, hr/5 + 1, hr/2 + hr/3, COL(178,184,190));
        rect_fill(hx - hr/3, hy - hr - hr/3, hr*2/3 + hr/4, hr/5 + 1, COL(178,184,190));
        disc_fill(hx - hr/2, hy - hr/4, hr/4 + 1, COL(178,184,190));
        disc_fill(hx + hr/2, hy - hr/4, hr/4 + 1, COL(178,184,190));
    } else if (species == 4){   // PET ROCK: a crag that drives
        rect_fill(hx - hr, hy - hr*3/4, hr*2, hr + hr*3/4, coat);
        if (w >= 15){
            line_thick(hx - hr/2, hy - hr/3, hx, hy + hr/4, 1, dark);
            line_thick(hx + hr/3, hy - hr/2, hx + hr/2, hy + hr/5, 1, dark);
        }
    } else {
        disc_fill(hx, hy, hr, dark);
        disc_fill(hx, hy, hr - (hr/6 + 1), coat);
        if (species == 0){   // PUPPY: floppy ears, mid-flop
            ellipse_fill(hx - hr - hr/5, hy + hr/6, hr/2, hr*3/4, dark);
            ellipse_fill(hx + hr + hr/5, hy + hr/6, hr/2, hr*3/4, dark);
        } else if (species == 1){   // KITTY: pointy ears, don't care
            tri_fill(hx - hr, hy - hr/3, hx - hr/3, hy - hr/3, hx - hr*3/4, hy - hr - hr/4, coat);
            tri_fill(hx + hr/3, hy - hr/3, hx + hr, hy - hr/3, hx + hr*3/4, hy - hr - hr/4, coat);
        } else if (species == 2){   // MOOSE: the antlers are the tell
            u32 ant = COL(214,186,138);
            int lw = w >= 30? 3 : 1;
            line_thick(hx - hr*3/4, hy - hr*3/4, hx - hr - hr/2, hy - hr - hr/3, lw, ant);
            line_thick(hx - hr - hr/3, hy - hr - hr/5, hx - hr - hr/2, hy - hr*3/4, lw > 1? 2 : 1, ant);
            line_thick(hx + hr*3/4, hy - hr*3/4, hx + hr + hr/2, hy - hr - hr/3, lw, ant);
            line_thick(hx + hr + hr/3, hy - hr - hr/5, hx + hr + hr/2, hy - hr*3/4, lw > 1? 2 : 1, ant);
        }
    }
    if (species != 5 && w >= 12)   // a sunlit crest on the head (v1.27 polish)
        disc_fill(hx - hr/3, hy - hr/3, hr/4 > 1? hr/4 : 1, col_shade(coat, 1.30f));
    if (w >= 15){   // the famous glasses, oversized as is traditional
        int gr = hr * 2 / 5; if (gr < 2) gr = 2;
        circle_out(hx - hr/2, hy, gr, COL(20,18,26));
        circle_out(hx + hr/2, hy, gr, COL(20,18,26));
        line_thick(hx - hr/2 + gr, hy, hx + hr/2 - gr, hy, 1, COL(20,18,26));
    }
}
static void crt_render_view(int sim_pi, int viewer_pi){
    BwSim *S = &BWS[sim_pi]; Buddy *b = &G.bud[viewer_pi];
    int vk = (viewer_pi == sim_pi)? 0 : 1;   // the kart this viewer is driving
    int vx = G.view_x0, vy = G.view_y0, vw2 = G.view_w>0? G.view_w : VW, vh2 = G.view_h>0? G.view_h : VH;
    float cs = (float)vw2 / 640.0f; { float csy = (float)vh2 / 360.0f; if (csy < cs) cs = csy; }
    int ox = vx + (int)((vw2 - 640*cs) / 2), oy = vy + (int)((vh2 - 360*cs) / 2);
#define KTX(lx) (ox + (int)((lx) * cs))
#define KTY(ly) (oy + (int)((ly) * cs))
#define KTS(v) ((int)((v) * cs) < 1 ? 1 : (int)((v) * cs))
    if (S->crt_state == 3){
        // ---- the WAITING ROOM
        static const u32 KCOL2[6] = { COL(230, 60, 50), COL(70, 130, 240), COL(250, 200, 50),
                                      COL(90, 200, 90), COL(200, 90, 220), COL(240, 140, 40) };
        static const char *COLNAME[6] = { "RED", "BLUE", "YELLOW", "GREEN", "PURPLE", "ORANGE" };
        int mycol = (vk == 0)? S->crt_colsel : S->crt_colsel2;
        rect_fill(vx, vy, vw2, vh2, COL(18, 42, 24));
        for (int ty2 = 0; ty2 < 12; ty2++) for (int tx2 = 0; tx2 < 20; tx2++)
            if ((tx2 + ty2) & 1) rect_fill(KTX(tx2*32), KTY(ty2*32), KTS(32) + 1, KTS(32) + 1, COL(22, 52, 28));
        draw_text_center(vx + vw2/2, vy + 26, "SUPER BUDDY CART", vw2 >= 620? 3 : 2, COL(250, 200, 50));
        draw_text_center(vx + vw2/2, vy + 62, "THE WAITING ROOM", 2, COL(255, 255, 255));
        { char buf[80]; char *q = buf;
          q = sappend(q, "GRAND PRIX - "); q = sappend(q, S->crt_cup? "FAR CUP: CACHE TO THE DEPTHS" : "HOME CUP: MEADOWS TO BADLANDS"); *q = 0;
          draw_text_center(vx + vw2/2, vy + 84, buf, 1, COL(250, 200, 50)); }
        // your kart, big and proud, wearing your pick
        { int cx = vx + vw2/2, cy = vy + vh2/2 - 6;
          disc_fill(cx - KTS(48), cy - KTS(35), KTS(17), COL(16, 14, 24));
          disc_fill(cx + KTS(48), cy - KTS(35), KTS(17), COL(16, 14, 24));
          disc_fill(cx - KTS(48), cy + KTS(35), KTS(17), COL(16, 14, 24));
          disc_fill(cx + KTS(48), cy + KTS(35), KTS(17), COL(16, 14, 24));
          rect_fill(cx - KTS(61), cy - KTS(38), KTS(122), KTS(77), KCOL2[mycol]);
          rect_fill(cx - KTS(10), cy - KTS(38), KTS(20), KTS(77), col_shade(KCOL2[mycol], 1.22f));   // racing stripe (v1.27)
          rect_line(cx - KTS(61), cy - KTS(38), KTS(122), KTS(77), COL(255, 255, 255));
          disc_fill(cx, cy + KTS(18), KTS(11), COL(245, 245, 245));   // number roundel (v1.27)
          disc_fill(cx, cy + KTS(18), KTS(7), col_shade(KCOL2[mycol], 0.65f));
          crt_draw_driver(cx, cy - KTS(28), KTS(118), b->species, crt_coat_for(b->species, b->color, viewer_pi));
          tri_fill(cx - KTS(92), cy - KTS(16), cx - KTS(92), cy + KTS(16), cx - KTS(70), cy, COL(255, 255, 255));
          tri_fill(cx + KTS(92), cy - KTS(16), cx + KTS(92), cy + KTS(16), cx + KTS(70), cy, COL(255, 255, 255));
          draw_text_center(cx, cy + KTS(64), COLNAME[mycol], 2, COL(255, 255, 255)); }
        { char b[96]; char *q=b; q=sappend(q, "IN THE RACE: HOLD "); if (!(hint_dev()==DEV_TOUCH && !G.touch_ctl)){ q=sappend(q, lbla(H_FIRE)); q=sappend(q, " "); } q=sappend(q, "TO GO, "); q=sappend(q, lbl_steer()); *q=0;   // v1.30 (audit F8)
          draw_text_center(vx + vw2/2, vy + 98, b, 1, COL(170, 200, 190)); }
        if (vh2 >= 300){ // MEDAL TIMES (feat 3): your medals on this cup's four tracks (full-height views only — the split room is already a squeeze)
            char buf[128]; char *q = buf;
            q = sappend(q, "MEDALS: ");
            for (int ti = 0; ti < 4; ti++){
                int tr = S->crt_cup * 4 + ti;
                if (ti) q = sappend(q, " | ");
                q = sappend(q, CRT_SHORT[tr]); q = sappend(q, " ");
                q = sappend(q, medal_name(b->crt_medal_best[tr]));   // v1.27: the EARNED medal (podium rule), not a derivation from the best time
            }
            *q = 0;
            draw_text_center(vx + vw2/2, vy + 112, buf, 1, COL(200, 214, 200));
        }
        draw_text_center(vx + vw2/2, vy + vh2 - 104, vk == 0? "LEFT / RIGHT - COLOUR      UP / DOWN - CUP" : "LEFT / RIGHT - COLOUR      THE HOST PICKS THE CUP", 1, COL(200, 230, 200));
        if (S->crt_duo >= 0){
            int imready = (vk == 0)? (S->crt_ready & 1) : (S->crt_ready & 2);
            int theyready = (vk == 0)? (S->crt_ready & 2) : (S->crt_ready & 1);
            { char buf[64]; char *q = buf;
              q = sappend(q, "PARTNER IS HERE, IN THE "); q = sappend(q, COLNAME[(vk == 0)? S->crt_colsel2 : S->crt_colsel]); q = sappend(q, " KART"); *q = 0;
              draw_text_center(vx + vw2/2, vy + vh2 - 82, buf, 1, COL(255, 240, 200)); }
            draw_text_center(vx + vw2/2, vy + vh2 - 62, theyready? "PARTNER: READY!" : "PARTNER: STILL PICKING...", 1, theyready? COL(120, 255, 120) : COL(220, 220, 220));
            { char b[64]; char *q=b; q=sappend(q, lbla(H_FIRE)); q=sappend(q, " WHEN READY"); *q=0;
              draw_text_center(vx + vw2/2, vy + vh2 - 42, imready? "YOU: READY!" : b, 1, imready? COL(120, 255, 120) : COL(255, 255, 255)); }   // v1.30 (audit F6)
        } else {
            draw_text_center(vx + vw2/2, vy + vh2 - 82, "NO PARTNER YET", 1, COL(255, 240, 200));
            draw_text_center(vx + vw2/2, vy + vh2 - 62, "THE OTHER PLAYER MUST PICK THE CART TOO", 1, COL(220, 220, 220));
            { char b[80]; char *q=b; q=sappend(q, lbla(H_FIRE)); q=sappend(q, ": START THE GRAND PRIX   -   LEAVE: BACK OUT"); *q=0;
              draw_text_center(vx + vw2/2, vy + vh2 - 42, b, 1, COL(255, 255, 255)); }   // v1.30 (audit F6)
        }
        if (S->note_t > 0){
            rect_fill(vx, vy + vh2 - 22, vw2, 22, COL(10, 12, 10));
            draw_text_center(vx + vw2/2, vy + vh2 - 15, S->note, 1, COL(220, 255, 220));
        }
        return;
    }
    if (S->crt_state == 4 || S->crt_state == 5){
        // ---- the STANDINGS between (and after) the cup's races
        int fin = (S->crt_state == 5);
        rect_fill(vx, vy, vw2, vh2, col_shade(REGION_GB[S->crt_track], 0.35f));
        for (int ty2 = 0; ty2 < 12; ty2++) for (int tx2 = 0; tx2 < 20; tx2++)
            if ((tx2 + ty2) & 1) rect_fill(KTX(tx2*32), KTY(ty2*32), KTS(32) + 1, KTS(32) + 1, col_shade(REGION_GB[S->crt_track], 0.45f));
        draw_text_center(vx + vw2/2, vy + 24, fin? "FINAL STANDINGS" : "STANDINGS", 3, COL(250, 200, 50));
        { char buf[80]; char *q = buf;
          q = sappend(q, S->crt_cup? "FAR CUP" : "HOME CUP");
          if (!fin){ q = sappend(q, " - AFTER RACE "); q = sapp_int(q, S->crt_race + 1); q = sappend(q, " OF 4"); }
          *q = 0; draw_text_center(vx + vw2/2, vy + 62, buf, 1, COL(255, 255, 255)); }
        int ord[6]; crt_order(S, ord);
        for (int r = 0; r < 6; r++){
            int k = ord[r], ry = vy + 92 + r * 30;
            if (k == vk) rect_fill(vx + vw2/2 - 170, ry - 5, 340, 24, COL(52, 46, 22));
            char buf[80]; char *q = buf;
            q = sappend(q, "P"); q = sapp_int(q, r + 1); q = sappend(q, "  ");
            q = sappend(q, BUD_ELF[S->crt_nameidx[k] % BUD_NAMES_N]);
            if (k == vk) q = sappend(q, "  (YOU)");
            else if (S->crt_duo >= 0 && k == 1 - vk) q = sappend(q, "  (PARTNER)");
            q = sappend(q, "  -  "); q = sapp_int(q, S->crt_pts[k]); q = sappend(q, " PTS"); *q = 0;
            draw_text_center(vx + vw2/2, ry, buf, 2, (k == vk)? COL(255, 240, 160) : COL(225, 230, 235));
        }
        if (fin){
            Buddy *vb = &G.bud[viewer_pi];
            char buf[80]; char *q = buf;
            q = sappend(q, "YOU RANKED P"); q = sapp_int(q, S->crt_gprank[vk]); q = sappend(q, " OF 6");
            int best = (S->crt_cup == 0)? vb->crt_cupbest % 8 : vb->crt_cupbest / 8;
            if (best){ q = sappend(q, "   -   CUP BEST P"); q = sapp_int(q, best); }
            *q = 0;
            draw_text_center(vx + vw2/2, vy + 92 + 6 * 30 + 12, buf, 1, COL(255, 240, 200));
            { // MEDAL TIMES (feat 3): your medal on the cup's last track
              char buf2[96]; char *q2 = buf2; int tb = vb->crt_tbest[S->crt_track];
              q2 = sappend(q2, "YOUR "); q2 = sappend(q2, CRT_SHORT[S->crt_track]); q2 = sappend(q2, " BEST ");
              if (tb){ q2 = sapp_time(q2, tb); q2 = sappend(q2, " - "); q2 = sappend(q2, medal_name(vb->crt_medal_best[S->crt_track])); }
              else q2 = sappend(q2, "- NO TIME YET");
              *q2 = 0;
              draw_text_center(vx + vw2/2, vy + 92 + 6 * 30 + 30, buf2, 1, medal_col(vb->crt_medal_best[S->crt_track])); }
            if (S->crt_gprank[vk] == 1) draw_text_center(vx + vw2/2, vy + 92 + 6 * 30 + 48, "CHAMPION OF THE CUP!", 1, COL(250, 200, 50));
        } else {
            char buf[80]; char *q = buf;
            q = sappend(q, "NEXT: "); q = sappend(q, REGION_NAME[S->crt_cup * 4 + S->crt_race + 1]); *q = 0;
            draw_text_center(vx + vw2/2, vy + 92 + 6 * 30 + 12, buf, 1, COL(200, 230, 200));
            { // MEDAL TIMES (feat 3): your medal on the track just raced
              Buddy *vb = &G.bud[viewer_pi];
              char buf2[96]; char *q2 = buf2; int tb = vb->crt_tbest[S->crt_track];
              q2 = sappend(q2, "YOUR "); q2 = sappend(q2, CRT_SHORT[S->crt_track]); q2 = sappend(q2, " BEST ");
              if (tb){ q2 = sapp_time(q2, tb); q2 = sappend(q2, " - "); q2 = sappend(q2, medal_name(vb->crt_medal_best[S->crt_track])); }
              else q2 = sappend(q2, "- NO TIME YET");
              *q2 = 0;
              draw_text_center(vx + vw2/2, vy + 92 + 6 * 30 + 30, buf2, 1, medal_col(vb->crt_medal_best[S->crt_track])); }
        }
        if (S->note_t > 0){
            rect_fill(vx, vy + vh2 - 22, vw2, 22, COL(10, 12, 10));
            draw_text_center(vx + vw2/2, vy + vh2 - 15, S->note, 1, COL(220, 255, 220));
        }
        return;
    }
    // ---- the race, SNES-style: the world stays flat, the camera sits
    // behind your kart, and the ground plane is projected forward —
    // grass bands, rumble strips and all — from your seat.
    static const u32 KCOL[6] = { COL(230, 60, 50), COL(70, 130, 240), COL(250, 200, 50),
                                 COL(90, 200, 90), COL(200, 90, 220), COL(240, 140, 40) };
    { float kx = S->crt_x[vk], ky = S->crt_y[vk], kh = S->crt_h[vk];
      float chx = fcos(kh), chy = fsin(kh);
      float camx = kx - chx * 40.0f, camy = ky - chy * 40.0f;
      float F = (float)vw2 * 0.82f;
      int horizon = vy + (int)(vh2 * 0.40f);
      // project a world point: returns 0 when it is behind the camera
      // (the lambda-free C89 way: a tiny macro pair over locals)
#define CRT_PROJ(wx, wy, outf, outsx, outsy) do { \
        float dx_ = (wx) - camx, dy_ = (wy) - camy; \
        float f_ = dx_ * chx + dy_ * chy; \
        float l_ = dx_ * (-chy) + dy_ * chx; \
        (outf) = f_; \
        (outsx) = vx + vw2/2 + (int)(l_ * F / (f_ < 1.0f? 1.0f : f_)); \
        (outsy) = horizon + (int)(15.0f * F / (f_ < 1.0f? 1.0f : f_)); \
      } while (0)
      // sky: a four-band gradient, a haloed sun, drifting clouds,
      // a far hill line, and the tree line in front of it — all of
      // it sliding as you steer, all of it wearing the REGION
      int rg = S->crt_track;
      // THE LIVING SKY (v1.25 feat 12): the kart sky wears the hour
      // too — bands and clouds through tod_sky, the sun traded for
      // the moon at night; the road and the karts keep daylight.
      u32 skyT = tod_sky(CRT_SKY[rg][0]), skyH = tod_sky(CRT_SKY[rg][1]);
      u32 gndA = col_shade(REGION_GA[rg], 0.55f), gndB = col_shade(REGION_GB[rg], 0.55f);
      u32 aprA = col_shade(REGION_GA[rg], 0.74f), aprB = col_shade(REGION_GB[rg], 0.74f);
      int dark = (rg == 5 || rg == 7) || tod_darkf() > 0.5f;
      { int sh = horizon - vy; if (sh < 4) sh = 4;
        rect_fill(vx, vy, vw2, sh/4 + 1, col_shade(skyT, 0.74f));
        rect_fill(vx, vy + sh/4, vw2, sh/4 + 1, skyT);
        rect_fill(vx, vy + sh/2, vw2, sh/4 + 1, col_shade(skyH, 0.93f));
        rect_fill(vx, vy + 3*sh/4, vw2, sh - 3*sh/4 + 1, skyH); }
      if (dark){   // stars over the dark countries (and every country at night) — v1.27 polish
          int skyh2 = horizon - vy; if (skyh2 < 12) skyh2 = 12;
          for (int st = 0; st < 26; st++){
              u32 hv = hash2(st * 5 + 2, 17, 0x57A2u);
              int sxp = vx + (int)(hv % (u32)(vw2 > 4? vw2 - 2 : 3));
              int syp = vy + 3 + (int)((hv >> 8) % (u32)(skyh2 - 8));
              rect_fill(sxp, syp, ((hv >> 16) & 3u)? 1 : 2, 1, COL(226, 230, 246));
          }
      }
      disc_fill(vx + vw2 - 90, vy + 44, 45, col_mix(skyH, dark? COL(200, 204, 225) : COL(255, 236, 150), 0.30f));   // sun/moon glow (v1.27)
      disc_fill(vx + vw2 - 90, vy + 44, 37, col_mix(skyH, dark? COL(200, 204, 225) : COL(255, 236, 150), 0.55f));
      disc_fill(vx + vw2 - 90, vy + 44, 31, dark? COL(200, 204, 225) : COL(255, 246, 196));
      disc_fill(vx + vw2 - 90, vy + 44, 20, dark? COL(226, 230, 242) : COL(255, 236, 150));
      { int slide = (int)(kh * 260.0f);
        u32 cloudc = tod_sky(dark? COL(126, 122, 158) : COL(248, 251, 255));
        for (int ci = 0; ci < 4; ci++){
            int cw = 54 + (ci * 29) % 40;
            int cxx = (ci * 317 + 60 - slide * 2 - (int)(S->crt_t * 6.0f)) % (vw2 + 240);
            if (cxx < 0) cxx += vw2 + 240;
            int cxp2 = vx + cxx - 120, cyp2 = vy + 14 + (ci * 23) % 30;
            ellipse_fill(cxp2, cyp2, cw/2, cw/6, cloudc);
            ellipse_fill(cxp2 + cw/5, cyp2 - cw/10, cw/3, cw/7, col_shade(cloudc, 1.03f));
            ellipse_fill(cxp2 - cw/4, cyp2 + 2, cw/3, cw/8, col_shade(cloudc, 0.94f));
        }
        { int slide2 = (int)(kh * 130.0f);   // the FAR ridge (v1.27 polish): paler, slower
                                             // parallax than the hill line in front of it, with
                                             // the odd distant stand block sitting on it
          u32 ridgec = col_mix(col_shade(REGION_GA[rg], 0.85f), skyH, 0.45f);
          for (int ri = -1; ri < vw2 / 210 + 2; ri++){
              int rxp = vx + ri * 210 - ((slide2 % 210) + 210) % 210;
              int ridx = ri + slide2 / 210;
              int rh = 36 + (int)(hash2(ridx, 19, 0xB1D6u) % 30);
              tri_fill(rxp, horizon, rxp + 105, horizon - rh, rxp + 210, horizon, ridgec);
              if ((hash2(ridx, 23, 0x51D3u) % 3u) == 0u){
                  int bw3 = 40 + (int)(hash2(ridx, 29, 0x51D3u) % 26);
                  rect_fill(rxp + 82, horizon - 15, bw3, 15, col_shade(ridgec, 0.88f));
                  rect_fill(rxp + 82 - 3, horizon - 18, bw3 + 6, 3, col_shade(ridgec, 0.78f));
              }
          } }
        for (int hi2 = -1; hi2 < vw2 / 130 + 2; hi2++){
            int hxp = vx + hi2 * 130 - ((slide % 130) + 130) % 130;
            int hh = 20 + (int)(hash2(hi2 + slide / 130, 7, 0x411u) % 22);
            tri_fill(hxp, horizon, hxp + 65, horizon - hh, hxp + 130, horizon, col_shade(REGION_GA[rg], 0.8f));
        }
        for (int ti = -1; ti < vw2 / 74 + 2; ti++){
            int txp = vx + ti * 74 - ((slide % 74) + 74) % 74;
            int th = 16 + ((ti * 37 + (slide / 74) * 13) & 15);
            tri_fill(txp, horizon, txp + 22, horizon - th - 10, txp + 44, horizon, col_shade(REGION_GA[rg], 0.6f));
            rect_fill(txp + 19, horizon - 6, 6, 6, COL(90, 64, 40));
        } }
      rect_fill(vx, horizon, vw2, vy + vh2 - horizon, gndB);
      // the road ahead: sample the centreline from your segment forward
      { int startseg = crt_nearest_full(kx, ky);
        enum { NS = 46 };
        int rsx[NS + 1], rsy[NS + 1], lsx[NS + 1], lsy[NS + 1], rrx[NS + 1], rry[NS + 1], lrx[NS + 1], lry[NS + 1];
        int arx[NS + 1], ary[NS + 1], alx[NS + 1], aly[NS + 1], erx[NS + 1], ery[NS + 1], elx[NS + 1], ely[NS + 1];
        float rf[NS + 1];
        for (int j = 0; j <= NS; j++){
            int si = (startseg + j) % CRT_N, sj = (si + 1) % CRT_N;
            float dxn = CRT_WX[sj] - CRT_WX[si], dyn = CRT_WY[sj] - CRT_WY[si];
            float dl = fsqrt(dxn*dxn + dyn*dyn); if (dl < 0.001f) dl = 1.0f;
            float nx = -dyn / dl, ny = dxn / dl;
            float f1, f2;
            CRT_PROJ(CRT_WX[si] + nx * CRT_HW, CRT_WY[si] + ny * CRT_HW, f1, rsx[j], rsy[j]);
            CRT_PROJ(CRT_WX[si] - nx * CRT_HW, CRT_WY[si] - ny * CRT_HW, f2, lsx[j], lsy[j]);
            rf[j] = (f1 + f2) * 0.5f;
            CRT_PROJ(CRT_WX[si] + nx * CRT_RUM, CRT_WY[si] + ny * CRT_RUM, f1, rrx[j], rry[j]);
            CRT_PROJ(CRT_WX[si] - nx * CRT_RUM, CRT_WY[si] - ny * CRT_RUM, f2, lrx[j], lry[j]);
            CRT_PROJ(CRT_WX[si] + nx * CRT_APR, CRT_WY[si] + ny * CRT_APR, f1, arx[j], ary[j]);
            CRT_PROJ(CRT_WX[si] - nx * CRT_APR, CRT_WY[si] - ny * CRT_APR, f2, alx[j], aly[j]);
            CRT_PROJ(CRT_WX[si] + nx * CRT_EDG, CRT_WY[si] + ny * CRT_EDG, f1, erx[j], ery[j]);
            CRT_PROJ(CRT_WX[si] - nx * CRT_EDG, CRT_WY[si] - ny * CRT_EDG, f2, elx[j], ely[j]);
        }
        for (int j = NS - 1; j >= 0; j--){
            if (rf[j + 1] < 4.0f) continue;
            int par = (startseg + j) & 1;
            if (rsy[j] > horizon) rect_fill(vx, rsy[j], vw2, rsy[j + 1] - rsy[j] + 1, par? gndA : gndB);
            if (rf[j] < 4.0f) continue;
            // the manicured apron the track sits in
            u32 ap = par? aprA : aprB;
            tri_fill(arx[j], ary[j], alx[j], aly[j], alx[j + 1], aly[j + 1], ap);
            tri_fill(arx[j], ary[j], alx[j + 1], aly[j + 1], arx[j + 1], ary[j + 1], ap);
            u32 rum = par? COL(226, 226, 226) : COL(205, 50, 45);
            tri_fill(rrx[j], rry[j], lrx[j], lry[j], lrx[j + 1], lry[j + 1], rum);
            tri_fill(rrx[j], rry[j], lrx[j + 1], lry[j + 1], rrx[j + 1], rry[j + 1], rum);
            u32 rd = par? COL(62, 62, 70) : COL(56, 56, 64);
            tri_fill(rsx[j], rsy[j], lsx[j], lsy[j], lsx[j + 1], lsy[j + 1], rd);
            tri_fill(rsx[j], rsy[j], lsx[j + 1], lsy[j + 1], rsx[j + 1], rsy[j + 1], rd);
            // crisp white edge lines just inside the rumble
            tri_fill(rsx[j], rsy[j], erx[j], ery[j], erx[j + 1], ery[j + 1], COL(232, 236, 238));
            tri_fill(rsx[j], rsy[j], erx[j + 1], ery[j + 1], rsx[j + 1], rsy[j + 1], COL(232, 236, 238));
            tri_fill(elx[j], ely[j], lsx[j], lsy[j], lsx[j + 1], lsy[j + 1], COL(232, 236, 238));
            tri_fill(elx[j], ely[j], lsx[j + 1], lsy[j + 1], elx[j + 1], ely[j + 1], COL(232, 236, 238));
            if (((startseg + j) % 3) == 0 && rf[j] < 340.0f){
                // the centre dashes: painted speed, rushing at you
                float cxj = (rsx[j] + lsx[j]) * 0.5f, cyj = (rsy[j] + lsy[j]) * 0.5f;
                float cxn = (rsx[j+1] + lsx[j+1]) * 0.5f, cyn = (rsy[j+1] + lsy[j+1]) * 0.5f;
                float ex = (float)(rsx[j] - lsx[j]), ey = (float)(rsy[j] - lsy[j]);
                float el = fsqrt(ex*ex + ey*ey); if (el < 1.0f) el = 1.0f;
                float exn = (float)(rsx[j+1] - lsx[j+1]), eyn = (float)(rsy[j+1] - lsy[j+1]);
                float eln = fsqrt(exn*exn + eyn*eyn); if (eln < 1.0f) eln = 1.0f;
                float hw = el * 0.030f; if (hw < 1.0f) hw = 1.0f;
                float hwn = eln * 0.030f; if (hwn < 1.0f) hwn = 1.0f;
                float ux = ex / el, uy = ey / el, uxn = exn / eln, uyn = eyn / eln;
                tri_fill((int)(cxj - ux*hw), (int)(cyj - uy*hw), (int)(cxj + ux*hw), (int)(cyj + uy*hw), (int)(cxn + uxn*hwn), (int)(cyn + uyn*hwn), COL(228, 228, 210));
                tri_fill((int)(cxj - ux*hw), (int)(cyj - uy*hw), (int)(cxn + uxn*hwn), (int)(cyn + uyn*hwn), (int)(cxn - uxn*hwn), (int)(cyn - uyn*hwn), COL(228, 228, 210));
            }
            // trackside dressing (v1.27 polish): red-capped barrier
            // posts at the rumble edge, verge tufts and the odd flower
            // at the apron edge — all keyed off the ABSOLUTE segment
            // index via unsigned hash2, so a given spot of verge looks
            // the same every frame and every lap (no flicker), and
            // nothing here touches the road's geometry or its reads.
            if (rf[j] > 14.0f && rf[j] < 320.0f){
                int segi = startseg + j;
                int ph = (int)(11.0f * F / rf[j]); if (ph < 3) ph = 3; if (ph > 24) ph = 24;
                if ((segi % 4) == 0){
                    for (int sd = 0; sd < 2; sd++){
                        int px = sd? rrx[j] : lrx[j], py = sd? rry[j] : lry[j];
                        int pwd = ph/4; if (pwd < 1) pwd = 1;
                        rect_fill(px - pwd/2, py - ph, pwd, ph, COL(238, 240, 242));
                        rect_fill(px - pwd/2, py - ph, pwd, ph/3 + 1, COL(205, 50, 45));
                        rect_fill(px - pwd/2, py - ph/2, pwd, 1, COL(150, 154, 160));
                    }
                }
                for (int sd = 0; sd < 2; sd++){
                    u32 hv = hash2(segi * 2 + sd, 47, 0xCE27u);
                    if ((hv % 5u) >= 2u) continue;
                    int px = sd? arx[j] : alx[j], py = sd? ary[j] : aly[j];
                    int th2 = ph/2; if (th2 < 2) th2 = 2;
                    u32 tc = col_shade(REGION_GA[rg], 0.62f);
                    rect_fill(px, py - th2, 1, th2, tc);
                    if (th2 > 2) rect_fill(px + 2, py - th2 + 2, 1, th2 - 2, col_shade(tc, 1.2f));
                    if (((hv >> 5) % 6u) == 0u){
                        static const u32 FLW[3] = { COL(250, 214, 90), COL(248, 244, 238), COL(240, 150, 180) };
                        disc_fill(px + 1, py - th2 - 1, th2/3 > 1? th2/3 : 1, FLW[(hv >> 9) % 3u]);
                    }
                }
            }
        }
        // the checkered start line, when it is in view
        { float fx, fy, fhx, fhy; crt_point_at(CRT_OFF, 0, &fx, &fy, &fhx, &fhy);
          float ff; int fsx, fsy; CRT_PROJ(fx, fy, ff, fsx, fsy);
          float fdx = fx - camx, fdy = fy - camy;
          float fl = fdx * (-chy) + fdy * chx;
          if (ff > 8.0f && ff < 620.0f && fl < ff * 1.1f + 50.0f && fl > -(ff * 1.1f + 50.0f)){
              float nx = -fhy, ny = fhx;
              // white border strips either side of the chequer band (v1.27 polish)
              for (int b3 = 0; b3 < 2; b3++){
                  float d0 = b3? 16.0f : -2.5f, d1 = b3? 18.5f : 0.0f;
                  int ax, ay, bx, by, cx2, cy2, dx2, dy2; float tf;
                  CRT_PROJ(fx - nx * CRT_HW + fhx * d0, fy - ny * CRT_HW + fhy * d0, tf, ax, ay);
                  CRT_PROJ(fx + nx * CRT_HW + fhx * d0, fy + ny * CRT_HW + fhy * d0, tf, bx, by);
                  CRT_PROJ(fx + nx * CRT_HW + fhx * d1, fy + ny * CRT_HW + fhy * d1, tf, cx2, cy2);
                  CRT_PROJ(fx - nx * CRT_HW + fhx * d1, fy - ny * CRT_HW + fhy * d1, tf, dx2, dy2);
                  tri_fill(ax, ay, bx, by, cx2, cy2, COL(245, 245, 245));
                  tri_fill(ax, ay, cx2, cy2, dx2, dy2, COL(245, 245, 245));
              }
              // the chequer band proper: TWO rows deep, offset like a
              // real flag (was a single row) — paint only; the line's
              // timing still lives in CRT_OFF/CRT_CUM, untouched
              for (int r3 = 0; r3 < 2; r3++)
              for (int c2 = 0; c2 < 8; c2++){
                  float la = (-28.0f + c2 * 7.0f) * CRT_HW / 30.0f, lb = la + 7.0f * CRT_HW / 30.0f;
                  float d0 = r3 * 8.0f, d1 = d0 + 8.0f;
                  int ax, ay, bx, by, cx2, cy2, dx2, dy2; float tf;
                  CRT_PROJ(fx + nx * la + fhx * d0, fy + ny * la + fhy * d0, tf, ax, ay);
                  CRT_PROJ(fx + nx * lb + fhx * d0, fy + ny * lb + fhy * d0, tf, bx, by);
                  CRT_PROJ(fx + nx * lb + fhx * d1, fy + ny * lb + fhy * d1, tf, cx2, cy2);
                  CRT_PROJ(fx + nx * la + fhx * d1, fy + ny * la + fhy * d1, tf, dx2, dy2);
                  u32 cc = ((c2 + r3) & 1)? COL(16, 14, 24) : COL(245, 245, 245);
                  tri_fill(ax, ay, bx, by, cx2, cy2, cc);
                  tri_fill(ax, ay, cx2, cy2, dx2, dy2, cc);
              }
              // the start gantry: two poles and the banner over the line
              { int gx[2], gy[2], gt[2]; float gf[2]; int ok = 1;
                for (int p2 = 0; p2 < 2; p2++){
                    float la = p2? CRT_HW + 14.0f : -(CRT_HW + 14.0f);
                    CRT_PROJ(fx + nx * la, fy + ny * la, gf[p2], gx[p2], gy[p2]);
                    if (gf[p2] < 6.0f) ok = 0;
                    gt[p2] = gy[p2] - (int)(30.0f * F / gf[p2]);
                }
                if (ok){
                    for (int p2 = 0; p2 < 2; p2++) rect_fill(gx[p2] - 2, gt[p2], 4, gy[p2] - gt[p2], COL(52, 54, 64));
                    for (int p2 = 0; p2 < 2; p2++){ int phgt = gy[p2] - gt[p2];   // white bands on the gantry poles (v1.27)
                        rect_fill(gx[p2] - 2, gt[p2] + phgt/3, 4, 2, COL(238, 240, 242));
                        rect_fill(gx[p2] - 2, gt[p2] + phgt*2/3, 4, 2, COL(238, 240, 242)); }
                    for (int p2 = 0; p2 < 2; p2++){   // little flags, waving on the poles
                        int fw = (int)(7.0f * F / gf[p2]); if (fw < 3) fw = 3;
                        int dir = p2? -1 : 1;
                        int wob = (int)(fsin(S->crt_t * 7.0f + p2 * 2.1f) * fw * 0.18f);
                        tri_fill(gx[p2], gt[p2], gx[p2] + dir * fw, gt[p2] + fw/4 + wob, gx[p2], gt[p2] + fw/2, COL(250, 200, 50));
                    }
                    int bh = (int)(9.0f * F / ff); if (bh < 3) bh = 3;
                    tri_fill(gx[0], gt[0], gx[1], gt[1], gx[1], gt[1] + bh, COL(198, 52, 48));
                    tri_fill(gx[0], gt[0], gx[1], gt[1] + bh, gx[0], gt[0] + bh, COL(198, 52, 48));
                    if (gx[1] - gx[0] > 70){   // chequer trim along the banner's hem (v1.27)
                        int n3 = (gx[1] - gx[0]) / (bh > 2? bh : 2); if (n3 > 24) n3 = 24; if (n3 < 2) n3 = 2;
                        for (int i3 = 0; i3 < n3; i3++){
                            int x0t = gx[0] + (gx[1] - gx[0]) * i3 / n3, x1t = gx[0] + (gx[1] - gx[0]) * (i3 + 1) / n3;
                            int y0t = gt[0] + bh - 2 + (gt[1] - gt[0]) * i3 / n3;
                            rect_fill(x0t, y0t, x1t - x0t + 1, 2, (i3 & 1)? COL(16, 14, 24) : COL(245, 245, 245));
                        }
                    }
                    if (gx[1] - gx[0] > 70){   // v1.32: carnival bulbs along the banner's top edge, chasing party colours
                        static const u32 BULB[4] = { COL(255,205,60), COL(255,110,140), COL(120,220,255), COL(150,255,150) };
                        int nb = (gx[1] - gx[0]) / 22; if (nb > 16) nb = 16; if (nb < 3) nb = 3;
                        int br = bh/4; if (br < 1) br = 1; if (br > 3) br = 3;
                        for (int ib = 0; ib <= nb; ib++){
                            int bxp = gx[0] + (gx[1] - gx[0]) * ib / nb;
                            int byp = gt[0] + (gt[1] - gt[0]) * ib / nb - br - 1;
                            disc_fill(bxp, byp, br, BULB[((int)(S->crt_t * 6) + ib) & 3]);
                            pset(bxp - (br > 1? 1 : 0), byp - (br > 1? 1 : 0), COL(255,255,255));
                        }
                    }
                    if (gx[1] - gx[0] > 210) draw_text_center((gx[0] + gx[1]) / 2, (gt[0] + gt[1]) / 2 + bh/2 - 3, REGION_NAME[rg], 1, COL(255, 244, 220));
                } }
              // chequer marker boards flanking the line (v1.27 polish)
              for (int p3 = 0; p3 < 2; p3++){
                  float la = p3? CRT_HW + 7.0f : -(CRT_HW + 7.0f);
                  int bx2, by2; float bf;
                  CRT_PROJ(fx + nx * la, fy + ny * la, bf, bx2, by2);
                  if (bf < 6.0f) continue;
                  int bs = (int)(10.0f * F / bf); if (bs < 3) bs = 3; if (bs > 30) bs = 30;
                  rect_fill(bx2 - 1, by2 - bs, 2, bs, COL(52, 54, 64));
                  rect_fill(bx2 - bs/2, by2 - bs - bs/2, bs/2, bs/2, COL(245, 245, 245));
                  rect_fill(bx2, by2 - bs - bs/2, bs - bs/2, bs/2, COL(16, 14, 24));
                  rect_fill(bx2 - bs/2, by2 - bs, bs/2, bs/2, COL(16, 14, 24));
                  rect_fill(bx2, by2 - bs, bs - bs/2, bs/2, COL(245, 245, 245));
              }
          } }
        // boost pads lying on the road
        for (int pd = 0; pd < CRT_NPAD; pd++){
            float pcx = CRT_PAD[pd][0], pcy = CRT_PAD[pd][1];
            float ff; int psx, psy; CRT_PROJ(pcx, pcy, ff, psx, psy);
            { float pdx = pcx - camx, pdy = pcy - camy;
              float pl = pdx * (-chy) + pdy * chx;
              if (pl > ff * 1.1f + 50.0f || pl < -(ff * 1.1f + 50.0f)) continue; }
            if (ff < 12.0f || ff > 640.0f) continue;
            int si = crt_nearest_full(pcx, pcy), sj = (si + 1) % CRT_N;
            float dxn = CRT_WX[sj] - CRT_WX[si], dyn = CRT_WY[sj] - CRT_WY[si];
            float dl = fsqrt(dxn*dxn + dyn*dyn); if (dl < 0.001f) dl = 1.0f;
            float ux = dxn / dl, uy = dyn / dl, nx = -uy, ny = ux;
            int ax, ay, bx, by, cx2, cy2, dx2, dy2; float tf;
            int gx2, gy2, hx2, hy2, ix2, iy2, jx2, jy2;
            float pw = CRT_HW * 22.0f / 30.0f, pw2 = CRT_HW * 19.0f / 30.0f;
            CRT_PROJ(pcx + nx * pw - ux * 12.0f, pcy + ny * pw - uy * 12.0f, tf, gx2, gy2);
            CRT_PROJ(pcx - nx * pw - ux * 12.0f, pcy - ny * pw - uy * 12.0f, tf, hx2, hy2);
            CRT_PROJ(pcx - nx * pw + ux * 12.0f, pcy - ny * pw + uy * 12.0f, tf, ix2, iy2);
            CRT_PROJ(pcx + nx * pw + ux * 12.0f, pcy + ny * pw + uy * 12.0f, tf, jx2, jy2);
            tri_fill(gx2, gy2, hx2, hy2, ix2, iy2, COL(255, 216, 118));
            tri_fill(gx2, gy2, ix2, iy2, jx2, jy2, COL(255, 216, 118));
            CRT_PROJ(pcx + nx * pw2 - ux * 9.0f, pcy + ny * pw2 - uy * 9.0f, tf, ax, ay);
            CRT_PROJ(pcx - nx * pw2 - ux * 9.0f, pcy - ny * pw2 - uy * 9.0f, tf, bx, by);
            CRT_PROJ(pcx - nx * pw2 + ux * 9.0f, pcy - ny * pw2 + uy * 9.0f, tf, cx2, cy2);
            CRT_PROJ(pcx + nx * pw2 + ux * 9.0f, pcy + ny * pw2 + uy * 9.0f, tf, dx2, dy2);
            { u32 padc = ((int)(S->crt_t * 6.0f + pd * 1.7f) & 1)? COL(240, 130, 40) : COL(252, 176, 64);
              tri_fill(ax, ay, bx, by, cx2, cy2, padc);
              tri_fill(ax, ay, cx2, cy2, dx2, dy2, padc); }
            if (ff < 150.0f) draw_text_center(psx, psy - 4, ">>", 1, COL(255, 255, 255));
        }
      }
    // everything with depth — trackside scenery and rival karts —
    // in one far-to-near pass, so trees occlude and are occluded
    // exactly like the karts they stand beside
    { enum { MAXIT = 6 + CRT_SCN_N + 20 + 8 };
      float itf[MAXIT]; int itk[MAXIT], itx[MAXIT], itsx[MAXIT], itsy[MAXIT], itn = 0;
      for (int sn = 0; sn < CRT_SCN_N; sn++){
          if (CRT_SCX[sn] < -50.0f) continue;
          float f; int sx, sy; CRT_PROJ(CRT_SCX[sn], CRT_SCY[sn], f, sx, sy);
          if (f < 9.0f || f > 880.0f) continue;
          itf[itn] = f; itk[itn] = 1; itx[itn] = sn; itsx[itn] = sx; itsy[itn] = sy; itn++;
      }
      for (int ob = 0; ob < CRT_NOBS; ob++){
          float f; int sx, sy; CRT_PROJ(CRT_OBS[ob][0], CRT_OBS[ob][1], f, sx, sy);
          if (f < 9.0f || f > 880.0f) continue;
          itf[itn] = f; itk[itn] = 4; itx[itn] = ob; itsx[itn] = sx; itsy[itn] = sy; itn++;
      }
      for (int q2 = 0; q2 < 20; q2++){
          if (S->crt_puffl[q2] <= 0) continue;
          float f; int sx, sy; CRT_PROJ(S->crt_puffx[q2], S->crt_puffy[q2], f, sx, sy);
          if (f < 6.0f || f > 700.0f) continue;
          itf[itn] = f; itk[itn] = 3; itx[itn] = q2; itsx[itn] = sx; itsy[itn] = sy; itn++;
      }
      for (int i = 0; i < 6; i++){ if (i == vk) continue;
          float f; int sx, sy; CRT_PROJ(S->crt_x[i], S->crt_y[i], f, sx, sy);
          if (f < 26.0f || f > 760.0f) continue;
          itf[itn] = f; itk[itn] = 0; itx[itn] = i; itsx[itn] = sx; itsy[itn] = sy; itn++;
      }
      for (int a = 1; a < itn; a++){ float tf = itf[a]; int tk = itk[a], tx2 = itx[a], tsx = itsx[a], tsy = itsy[a]; int b2 = a - 1;
          while (b2 >= 0 && itf[b2] < tf){ itf[b2+1] = itf[b2]; itk[b2+1] = itk[b2]; itx[b2+1] = itx[b2]; itsx[b2+1] = itsx[b2]; itsy[b2+1] = itsy[b2]; b2--; }
          itf[b2+1] = tf; itk[b2+1] = tk; itx[b2+1] = tx2; itsx[b2+1] = tsx; itsy[b2+1] = tsy; }
      for (int oi = 0; oi < itn; oi++){
        float f = itf[oi]; int sx = itsx[oi], base = itsy[oi];
        if (itk[oi] == 4){   // v1.23: a tire-stack OBSTACLE on the road
            int tw = (int)(13.0f * F / f); if (tw < 3) continue;
            if (sx < vx - tw || sx > vx + vw2 + tw) continue;
            int thh = tw/3; if (thh < 1) thh = 1;
            rect_fill(sx - tw/2, base - thh, tw, thh, COL(38, 38, 44));
            rect_fill(sx - tw/2, base - thh*2, tw, thh, COL(226, 226, 226));
            rect_fill(sx - tw/2, base - thh*3, tw, thh, COL(255, 190, 40));
            if (tw >= 7) rect_fill(sx - tw/4, base - thh*3 + 1, tw/2, 1, COL(255, 226, 140));   // top tyre sheen (v1.27)
            continue;
        }
        if (itk[oi] == 1){
            int sn = itx[oi], tp = CRT_SCT[sn];
            if (tp == 0){   // a tree (every seventh wears the region's tuft colour)
                int th = (int)(30.0f * F / f); if (th < 3) continue;
                if (sx < vx - th || sx > vx + vw2 + th) continue;
                ellipse_fill(sx, base, th/4, th/10, col_shade(REGION_GB[rg], 0.45f));
                rect_fill(sx - th/16 - 1, base - th*38/100, th/8 + 2, th*38/100, COL(96, 66, 40));
                u32 g1 = (sn % 7 == 3)? REGION_TUFT[rg] : (sn & 1)? col_shade(REGION_GA[rg], 0.62f) : col_shade(REGION_GA[rg], 0.5f);
                disc_fill(sx, base - th*52/100, th*30/100, g1);
                disc_fill(sx, base - th*80/100, th*22/100, col_shade(g1, 1.18f));
                if (th >= 10){   // sunlit crest + grass tufts at the trunk (v1.27 polish)
                    disc_fill(sx - th/6, base - th*66/100, th/7, col_shade(g1, 1.32f));
                    rect_fill(sx - th/5, base - 2, 1, 2, REGION_TUFT[rg]);
                    rect_fill(sx + th/6, base - 2, 1, 2, REGION_TUFT[rg]);
                }
            } else if (tp == 1){   // a tire stack
                int tw = (int)(13.0f * F / f); if (tw < 3) continue;
                if (sx < vx - tw || sx > vx + vw2 + tw) continue;
                int thh = tw/3; if (thh < 1) thh = 1;
                rect_fill(sx - tw/2, base - thh, tw, thh, COL(38, 38, 44));
                rect_fill(sx - tw/2, base - thh*2, tw, thh, COL(226, 226, 226));
                rect_fill(sx - tw/2, base - thh*3, tw, thh, (sn & 1)? COL(205, 50, 45) : COL(38, 38, 44));
                if (tw >= 7) rect_fill(sx - tw/4, base - thh*3 + 1, tw/2, 1, COL(238, 240, 242));   // top tyre sheen (v1.27)
            } else {   // the grandstand, watching the top straight
                // Nov 15 (Daniel's "weird gray wall"): unclamped,
                // the stand's projection at grid distance filled
                // half the screen with flat concrete. Bound it to a
                // building's size, and don't draw it at all when the
                // kart is right alongside it.
                if (f < 42.0f) continue;
                int gw = (int)(175.0f * F / f); if (gw < 12) continue;
                if (gw > 380) gw = 380;
                int gh = (int)(42.0f * F / f); if (gh > 92) gh = 92;
                if (sx < vx - gw/2 || sx > vx + vw2 + gw/2) continue;
                rect_fill(sx - gw/2, base - gh, gw, gh, COL(116, 126, 148));
                for (int r2 = 0; r2 < 4; r2++) for (int c3 = 0; c3 < gw/9; c3++){
                    u32 h = hash2(sn * 31 + c3, r2 * 7 + 1, 0x57A0u);
                    static const u32 CROWD[5] = { COL(240,200,90), COL(230,90,80), COL(120,180,240), COL(240,240,240), COL(120,220,140) };
                    rect_fill(sx - gw/2 + 3 + c3 * 9, base - gh + 5 + r2 * (gh/5), 4, 4, CROWD[h % 5]);
                }
                rect_fill(sx - gw/2 - 4, base - gh - gh/6 - 2, gw + 8, gh/6 + 2, COL(64, 70, 88));
                for (int aw = 0; aw < (gw + 8) / 10; aw++)
                    if (aw & 1) rect_fill(sx - gw/2 - 4 + aw * 10, base - gh - gh/6 - 2, 10, gh/6 + 2, COL(198, 52, 48));
                rect_fill(sx - gw/2 - 4, base - gh - 3, gw + 8, 3, COL(40, 44, 58));   // shadow line under the roof (v1.27)
                if (gw > 60){   // support columns + a fascia stripe in the region colour (v1.27)
                    for (int sp = 0; sp < 3; sp++) rect_fill(sx - gw/3 + sp * (gw/3), base - gh, 3, gh, COL(84, 90, 108));
                    rect_fill(sx - gw/2, base - 4, gw, 2, col_shade(REGION_GA[rg], 0.9f));
                }
                if (gw > 300) draw_text_center(sx, base - gh - gh/6 + gh/12 - 3, "GRAND PRIX", 1, COL(255, 240, 200));
            }
            continue;
        }
        if (itk[oi] == 3){   // an exhaust puff, swelling as it fades
            int q2 = itx[oi];
            float lf = S->crt_puffl[q2]; if (lf < 0) lf = 0;
            int pr = (int)((0.45f - lf) * 30.0f * F / f) + 2;
            if (pr > 26) pr = 26;
            disc_fill(sx, base - pr/2, pr, COL(216, 220, 224));
            disc_fill(sx - pr/3, base - pr, pr/2, COL(240, 243, 245));
            continue;
        }
        int i = itx[oi];
        int kw = (int)(17.0f * F / f); if (kw < 3) kw = 3;
        if (kw > vw2 * 2 / 5) kw = vw2 * 2 / 5;
        int kh2 = (int)(kw * 0.66f); if (kh2 < 2) kh2 = 2;
        if (sx < vx - kw || sx > vx + vw2 + kw) continue;
        ellipse_fill(sx, base, kw/2 + 1, kh2/4 + 1, COL(20, 40, 22));
        if (S->crt_boost[i] > 0)
            tri_fill(sx - kw/4, base, sx + kw/4, base, sx, base + kh2/2 + 2, COL(255, 160, 60));
        rect_fill(sx - kw/2, base - kh2/2, kw/4 + 1, kh2/2, COL(16, 14, 24));
        rect_fill(sx + kw/2 - kw/4, base - kh2/2, kw/4 + 1, kh2/2, COL(16, 14, 24));
        rect_fill(sx - kw/2, base - kh2, kw, kh2 * 3/4, KCOL[S->crt_kcol[i]]);
        if (kw >= 8) rect_fill(sx - kw/10, base - kh2, kw/5, kh2 * 3/4, col_shade(KCOL[S->crt_kcol[i]], 1.22f));   // centre racing stripe (v1.27)
        rect_line(sx - kw/2, base - kh2, kw, kh2 * 3/4, COL(16, 14, 24));
        if (kw >= 8){
            rect_fill(sx - kw/2 + 2, base - kh2 + 1, kw - 4, kh2/5, col_shade(KCOL[S->crt_kcol[i]], 1.30f));
            rect_fill(sx - kw/2 + kw/5, base - kh2/4, kw * 3/5, kh2/4, col_shade(KCOL[S->crt_kcol[i]], 0.60f));
        }
        if (kw >= 12){   // wheel hubs + taillights, so the wheels read as wheels (v1.27)
            rect_fill(sx - kw/2 + kw/16, base - kh2/8 - 1, kw/8, 2, COL(150, 156, 164));
            rect_fill(sx + kw/2 - kw/4 + kw/16, base - kh2/8 - 1, kw/8, 2, COL(150, 156, 164));
            disc_fill(sx - kw/2 + kw/10, base - kh2/8, kw >= 24? 2 : 1, COL(255, 70, 60));
            disc_fill(sx + kw/2 - kw/10, base - kh2/8, kw >= 24? 2 : 1, COL(255, 70, 60));
        }
        if (kw >= 16){   // the number roundel on the kart's back (v1.27)
            disc_fill(sx, base - kh2*2/5, kw/8, COL(245, 245, 245));
            disc_fill(sx, base - kh2*2/5, kw/13, col_shade(KCOL[S->crt_kcol[i]], 0.65f));
        }
        crt_draw_driver(sx, base - kh2 + kh2/8 + ((S->crt_v[i] > 60.0f)? (int)(fsin(S->crt_t * 22.0f + i * 1.93f) * kw * 0.012f) : 0), kw, S->crt_species[i], crt_coat_for(S->crt_species[i], S->crt_dcolidx[i], i));
        if (f < 300.0f){
            int ty2 = base - kh2 - kw*42/100 - 10;
            if (S->crt_duo >= 0 && i == 1 - vk)
                tri_fill(sx - 5, ty2 - 2, sx + 5, ty2 - 2, sx, ty2 + 5, COL(255, 255, 255));
            draw_text_center(sx, ty2, BUD_ELF[S->crt_nameidx[i] % BUD_NAMES_N], 1, COL(255, 255, 255));
        }
      } }
    // your own kart, big at the bottom of the screen, from behind
    { int ow = vw2 / 5; if (ow < 110) ow = 110; if (ow > 240) ow = 240;
      int oh = ow * 5 / 8;
      int bob = (S->crt_v[vk] > 60.0f)? (int)(fsin(S->crt_t * 26.0f) * 1.6f) : 0;
      int cxp = vx + vw2/2, base = vy + vh2 - 4 + bob;
      ellipse_fill(cxp, base, ow/2, oh/6, COL(20, 40, 22));
      if (S->crt_boost[vk] > 0){
          u32 fl = ((int)(S->crt_t * 20.0f) & 1)? COL(255, 160, 60) : COL(255, 210, 90);
          tri_fill(cxp - ow/3, base - 2, cxp - ow/6, base - 2, cxp - ow/4, base + oh/3, fl);
          tri_fill(cxp + ow/6, base - 2, cxp + ow/3, base - 2, cxp + ow/4, base + oh/3, fl);
      }
      rect_fill(cxp - ow/2, base - oh/3, ow/5, oh/3, COL(16, 14, 24));
      rect_fill(cxp + ow/2 - ow/5, base - oh/3, ow/5, oh/3, COL(16, 14, 24));
      rect_fill(cxp - ow/2 + ow/10, base - oh, ow * 4/5, oh * 3/4, KCOL[S->crt_kcol[vk]]);
      rect_fill(cxp - ow/12, base - oh, ow/6, oh * 3/4, col_shade(KCOL[S->crt_kcol[vk]], 1.22f));   // centre racing stripe (v1.27)
      rect_line(cxp - ow/2 + ow/10, base - oh, ow * 4/5, oh * 3/4, COL(16, 14, 24));
      rect_fill(cxp - ow/2 + ow/10 + 3, base - oh + 2, ow * 4/5 - 6, oh/6, col_shade(KCOL[S->crt_kcol[vk]], 1.30f));
      rect_fill(cxp - ow/2 + ow/10 + ow/6, base - oh/4, ow * 4/5 - ow/3, oh/4, col_shade(KCOL[S->crt_kcol[vk]], 0.60f));
      // v1.27 detail: number roundel, wheel hubs, taillights, exhausts, bumper line
      disc_fill(cxp, base - oh/2, ow/10, COL(245, 245, 245));
      disc_fill(cxp, base - oh/2, ow/16, col_shade(KCOL[S->crt_kcol[vk]], 0.65f));
      rect_fill(cxp - ow/2 + ow/20, base - oh/6 - 2, ow/10, 4, COL(150, 156, 164));
      rect_fill(cxp + ow/2 - ow/5 + ow/20, base - oh/6 - 2, ow/10, 4, COL(150, 156, 164));
      disc_fill(cxp - ow/2 + ow/10 + ow/14, base - oh/6, 3, COL(255, 70, 60));
      disc_fill(cxp + ow/2 - ow/10 - ow/14, base - oh/6, 3, COL(255, 70, 60));
      rect_fill(cxp - ow/9, base - oh/9, ow/13, oh/11, COL(96, 102, 110));
      rect_fill(cxp + ow/9 - ow/13, base - oh/9, ow/13, oh/11, COL(96, 102, 110));
      rect_fill(cxp - ow/2 + ow/10, base - oh/4 - 2, ow * 4/5, 2, COL(16, 14, 24));
      crt_draw_driver(cxp, base - oh + oh/8, ow, S->crt_species[vk], crt_coat_for(S->crt_species[vk], S->crt_dcolidx[vk], vk));
    }
    // minimap in the corner: the whole flat circuit, SMK-style.
    // (positioned from the CLIP rect, not the view rect: in split
    // screen the view origin stays at 0 and the clip does the split,
    // so a view-relative corner lands in the other player's half)
    if (clip_x1 - clip_x0 >= 380){
        int cw2 = clip_x1 - clip_x0;
        int mw = cw2 >= 470? 100 : 76, mh = cw2 >= 470? 62 : 48, mx0 = clip_x1 - mw - 8, my0 = clip_y0 + 30;
        rect_fill(mx0, my0, mw, mh, COL(10, 26, 14));
        rect_line(mx0, my0, mw, mh, COL(120, 160, 120));
        for (int i = 0; i < CRT_N; i += 2)
            rect_fill(mx0 + 4 + (int)(CRT_WX[i] * (mw - 8) / CRT_WW), my0 + 4 + (int)(CRT_WY[i] * (mh - 8) / CRT_WH), 2, 2, COL(120, 130, 120));
        for (int i = 0; i < 6; i++){
            u32 dc = (i == vk)? COL(255, 255, 255) : (S->crt_duo >= 0 && i == 1 - vk)? COL(255, 230, 90) : COL(160, 160, 160);
            rect_fill(mx0 + 3 + (int)(S->crt_x[i] * (mw - 8) / CRT_WW), my0 + 3 + (int)(S->crt_y[i] * (mh - 8) / CRT_WH), 3, 3, dc);
        }
    }
#undef CRT_PROJ
    }
    // HUD band
    rect_fill(vx, vy, vw2, 24, COL(10, 12, 16));
    { char buf[96]; char *q = buf;
      int lap = (int)(S->crt_prog[vk] / CRT_LEN) + 1; if (lap < 1) lap = 1; if (lap > 3) lap = 3;
      int tds = (int)(S->crt_t * 10.0f); int sec = (tds / 10) % 60;
      q = sappend(q, "RACE "); q = sapp_int(q, S->crt_race + 1); q = sappend(q, "/4  LAP "); q = sapp_int(q, lap); q = sappend(q, "/3  POS ");
      q = sapp_int(q, S->crt_place[vk]); q = sappend(q, "/6  TIME ");
      q = sapp_int(q, tds / 600); q = sappend(q, ":");
      if (sec < 10) q = sappend(q, "0");
      q = sapp_int(q, sec); q = sappend(q, "."); q = sapp_int(q, tds % 10); *q = 0;
      draw_text(vx + 10, vy + 8, buf, 1, COL(255, 240, 200)); }
    // v1.32: the track name used to sit at the band's centre, where a
    // long name ("CLEAN CODE MEADOWS") ran straight into the RACE /
    // LAP / POS / TIME string on the left — the two texts struck
    // through each other. It now right-aligns in the gap between
    // that string and the BEST P field.
    if (vw2 >= 600) draw_text(vx + vw2 - 86 - text_w(REGION_NAME[S->crt_track], 1), vy + 8, REGION_NAME[S->crt_track], 1, COL(250, 200, 50));
    if (vw2 >= 600 && b->crt_best_pos){ char buf[48]; char *q = buf;
        q = sappend(q, "BEST P"); q = sapp_int(q, b->crt_best_pos); *q = 0;
        draw_text(vx + vw2 - 70, vy + 8, buf, 1, COL(200, 214, 200)); }
    if (S->crt_state == 0){
        int n = (int)(S->crt_count + 0.999f); if (n < 1) n = 1; if (n > 3) n = 3;
        char nb[2] = { (char)('0' + n), 0 };
        draw_text_center(vx + vw2/2, vy + vh2/2 - 66, nb, 8, COL(255, 220, 90));
        draw_text_center(vx + vw2/2, vy + vh2/2 + 22, "GET READY...", 2, COL(255, 255, 255));
        { char b[96]; char *q=b; q=sappend(q, "HOLD "); if (!(hint_dev()==DEV_TOUCH && !G.touch_ctl)){ q=sappend(q, lbla(H_FIRE)); q=sappend(q, " "); } q=sappend(q, "TO GO - "); q=sappend(q, lbl_steer()); *q=0;   // v1.30 (audit F8)
          draw_text_center(vx + vw2/2, vy + vh2/2 + 52, b, 1, COL(200, 230, 200)); }
    } else if (S->crt_state == 1 && S->crt_t < 0.9f){
        draw_text_center(vx + vw2/2, vy + vh2/2 - 40, "GO!", 6, COL(120, 255, 120));
    } else if (S->crt_state == 2){
        { float el = 3.4f - S->crt_fint; if (el < 0) el = 0;   // confetti over the finish
          static const u32 CONF[5] = { COL(250,200,50), COL(230,90,80), COL(120,180,240), COL(120,220,140), COL(240,240,240) };
          for (int ci = 0; ci < 42; ci++){
              u32 h1 = hash2(ci, 5, 0xC0F1u);
              int cxp2 = vx + (int)(h1 % (u32)(vw2 > 4? vw2 : 5));
              int cyp2 = vy + (int)((h1 / 7u + (u32)(el * (46.0f + (float)(h1 % 30u)))) % (u32)(vh2 > 4? vh2 : 5));
              rect_fill(cxp2, cyp2, 4, 3, CONF[h1 % 5]);
          } }
        rect_fill(vx + vw2/2 - 130, vy + vh2/2 - 44, 260, 84, COL(12, 14, 22));
        rect_line(vx + vw2/2 - 130, vy + vh2/2 - 44, 260, 84, COL(250, 200, 50));
        { char buf[64]; char *q = buf;
          q = sappend(q, "FINISH!  P"); q = sapp_int(q, S->crt_hfin[vk]); *q = 0;
          draw_text_center(vx + vw2/2, vy + vh2/2 - 32, buf, 3, COL(255, 240, 200)); }
        { char buf[64]; char *q = buf; int tds = (int)(S->crt_t * 10.0f); int sec = (tds / 10) % 60;
          q = sappend(q, "TIME "); q = sapp_int(q, tds / 600); q = sappend(q, ":");
          if (sec < 10) q = sappend(q, "0");
          q = sapp_int(q, sec); q = sappend(q, "."); q = sapp_int(q, tds % 10); *q = 0;
          draw_text_center(vx + vw2/2, vy + vh2/2 + 8, buf, 1, COL(200, 214, 200)); }
        { // MEDAL TIMES (feat 3): the tier of the finish just run, off
          // the frozen race clock; the record lands mid-panel, so a
          // best set by this race shows as soon as it banks.
          // v1.27: the tier is the EARNED one — time tier capped by
          // the finish place (medal_tier_finish, podium only).
          int tds = (int)(S->crt_t * 10.0f);
          int tier = medal_tier_finish(tds, crt_medal[S->crt_track], S->crt_hfin[vk]);
          char buf[80]; char *q = buf;
          if (G.zen_run){ q = sappend(q, "ZEN RUN - NO MEDALS"); *q = 0;
              draw_text_center(vx + vw2/2, vy + vh2/2 + 24, buf, 1, COL(160, 160, 168)); }
          else if (tier){ q = sappend(q, medal_name(tier)); q = sappend(q, " MEDAL!");
              if (b->crt_tbest[S->crt_track] == tds) q = sappend(q, "  NEW TRACK BEST!");
              *q = 0;
              draw_text_center(vx + vw2/2, vy + vh2/2 + 24, buf, 1, medal_col(tier)); }
          else if (S->crt_hfin[vk] > 3 && medal_tier(tds, crt_medal[S->crt_track]) > 0){
              q = sappend(q, "NO MEDAL - MEDALS ARE FOR THE TOP 3"); *q = 0;
              draw_text_center(vx + vw2/2, vy + vh2/2 + 24, buf, 1, COL(160, 160, 168)); }
          else { q = sappend(q, "NO MEDAL - BRONZE IS "); q = sapp_time(q, crt_medal[S->crt_track][2]); *q = 0;
              draw_text_center(vx + vw2/2, vy + vh2/2 + 24, buf, 1, COL(160, 160, 168)); } }
    }
    if (S->note_t > 0){
        rect_fill(vx, vy + vh2 - 22, vw2, 22, COL(10, 12, 10));
        draw_text_center(vx + vw2/2, vy + vh2 - 15, S->note, 1, COL(220, 255, 220));
    }
#undef KTX
#undef KTY
#undef KTS
}
static void crt_render(int pi){ crt_render_view(pi, pi); }

