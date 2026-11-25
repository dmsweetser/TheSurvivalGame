// THE SURVIVAL GAME — mall/bowl_draw.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 1130-1379 in the Nov 2026 fine-grained refactor; code unchanged.

static void draw_pin_shape(int x, int y, float tilt){
    int tx = (int)tilt;
    ellipse_fill(x, y+7, 6, 3, COL(198,192,184));
    ellipse_fill(x+tx, y, 5, 8, COL(245,242,238));
    rect_fill(x+tx-4, y-3, 8, 3, COL(200,60,50));
    disc_fill(x+tx, y-6, 3, COL(245,242,238));
    pset(x+tx-1, y-6, COL(30,30,40)); pset(x+tx+1, y-6, COL(30,30,40));   // beady bug eyes
}
// BUG BOWL LEAGUE (feat 10): the lanes' league card — standings and
// tonight's posted scores, drawn in the case/board idiom (boxed
// tables, gold headers, right-aligned numbers). Attract and
// game-over get the full boxes; a game in progress gets one line;
// compact (split-screen) views get the same facts as two lines.
static void draw_league_card(int x0, int vw, int cx, int compact){
    BowlLeague *L=&G.blg[G.store_pi];
    int season = L->season<1? 1 : L->season;
    int nights = L->night;   // nights completed this season
    Bowl *bw=&G.bowl[G.store_pi];
    if (compact){
        int y = 97;
        char b[96]; char *q=b;
        if (G.zen_run){ q=sappend(q, "LEAGUE: ZEN PRACTICE - GAMES DON'T COUNT"); }
        else { q=sappend(q, "LEAGUE S"); q=sapp_int(q, season); q=sappend(q, " NIGHT ");
               q=sapp_int(q, nights+1); q=sappend(q, " OF 6 - YOU "); q=sapp_int(q, L->spins);
               q=sappend(q, " - RIVALS "); q=sapp_int(q, lg_rival_total(season,0,nights));
               q=sappend(q, "/"); q=sapp_int(q, lg_rival_total(season,1,nights));
               q=sappend(q, "/"); q=sapp_int(q, lg_rival_total(season,2,nights)); }
        *q=0; draw_text_center(cx, y, b, 1, COL(200,220,255));
        if (!G.zen_run && !bw->active){
            q=b; q=sappend(q, "TONIGHT: "); q=sapp_int(q, lg_rival_score(season,nights,0));
            q=sappend(q, "/"); q=sapp_int(q, lg_rival_score(season,nights,1));
            q=sappend(q, "/"); q=sapp_int(q, lg_rival_score(season,nights,2));
            q=sappend(q, " - TARGET "); int tg=lg_rival_score(season,nights,0);
            { int t1=lg_rival_score(season,nights,1), t2=lg_rival_score(season,nights,2); if (t1>tg) tg=t1; if (t2>tg) tg=t2; }
            q=sapp_int(q, tg); *q=0;
            draw_text_center(cx, y+9, b, 1, COL(255,230,170));
        }
        return;
    }
    // ---- left box: the standings + the career book
    { int bx=x0+8, bw2=cx-122-bx, by=104, bh=136;
      rect_fill(bx, by, bw2, bh, COL(24,20,40)); rect_line(bx, by, bw2, bh, COL(198,164,84));
      { char b[64]; char *q=b; q=sappend(q, "THE LEAGUE - SEASON "); q=sapp_int(q, season); *q=0;
        draw_text_center(bx+bw2/2, by+6, b, 1, COL(255,230,170)); }
      // standings rows, cumulative pins, sorted (ties: YOU first, then rival order)
      { const char *nm[4]; int pn[4], you[4];
        nm[0]="YOU"; pn[0]=L->spins; you[0]=1;
        for (int r=0;r<3;r++){ nm[r+1]=LG_RIVAL_NAME[r]; pn[r+1]=lg_rival_total(season, r, nights); you[r+1]=0; }
        for (int i=0;i<3;i++) for (int j=i+1;j<4;j++)
            if (pn[j]>pn[i] || (pn[j]==pn[i] && you[j] && !you[i])){
                const char *tn=nm[i]; nm[i]=nm[j]; nm[j]=tn;
                int tp=pn[i]; pn[i]=pn[j]; pn[j]=tp; int ty=you[i]; you[i]=you[j]; you[j]=ty; }
        for (int i=0;i<4;i++){
            int ry=by+20+i*12; char nb[16]; char *q=nb; q=sapp_int(q, pn[i]); *q=0;
            u32 c = you[i]? COL(160,230,255) : i==0? COL(255,230,170) : COL(220,224,235);
            draw_text(bx+8, ry, nm[i], 1, c);
            draw_text(bx+bw2-8-text_w(nb,1), ry, nb, 1, c);
        } }
      { char b[80]; char *q=b; q=sappend(q, "CAREER: "); q=sapp_int(q, L->games);
        q=sappend(q, L->games==1? " GAME" : " GAMES"); *q=0;
        draw_text(bx+8, by+72, b, 1, COL(200,220,255)); }
      { char b[80]; char *q=b; q=sappend(q, "BEST "); q=sapp_int(q, L->best);
        q=sappend(q, " - PINS "); q=sapp_int(q, L->pins); *q=0;
        draw_text(bx+8, by+84, b, 1, COL(200,220,255)); }
      { char b[80]; char *q=b; q=sappend(q, "STRIKES "); q=sapp_int(q, L->strikes); *q=0;
        draw_text(bx+8, by+96, b, 1, COL(200,220,255)); }
      if (L->fin_season>0){
          char b[80]; char *q=b; q=sappend(q, "SEASON "); q=sapp_int(q, L->fin_season); q=sappend(q, ": ");
          if (L->fin_place==1) q=sappend(q, "CHAMPION!");
          else { q=sapp_int(q, L->fin_place); q=sappend(q, " OF 4"); }
          *q=0; draw_text(bx+8, by+110, b, 1, COL(255,230,170));
          if (L->trophies>0){ q=b; q=sappend(q, "LEAGUE TROPHIES: "); q=sapp_int(q, L->trophies); *q=0;
              draw_text(bx+8, by+122, b, 1, COL(255,230,170)); }
      }
    }
    // ---- right box: tonight's posted card (the rivals bowl first)
    { int bx=cx+122, bw2=(x0+vw-8)-bx, by=104, bh=88;
      rect_fill(bx, by, bw2, bh, COL(24,20,40)); rect_line(bx, by, bw2, bh, COL(198,164,84));
      if (G.zen_run){
          draw_text_center(bx+bw2/2, by+6, "ZEN PRACTICE", 1, COL(255,230,170));
          draw_text_center(bx+bw2/2, by+26, "GAMES HERE DON'T COUNT.", 1, COL(220,224,235));
          draw_text_center(bx+bw2/2, by+38, "THE BOOK WAITS FOR", 1, COL(220,224,235));
          draw_text_center(bx+bw2/2, by+50, "A REAL RUN.", 1, COL(220,224,235));
      } else {
          int tn = bw->game_over? nights : nights;   // after a close, season/night already rolled: this is the NEXT card
          { char b[64]; char *q=b;
            q=sappend(q, bw->game_over? "NEXT - NIGHT " : "TONIGHT - NIGHT ");
            q=sapp_int(q, tn+1); q=sappend(q, " OF 6"); *q=0;
            draw_text_center(bx+bw2/2, by+6, b, 1, COL(255,230,170)); }
          { int idx[3]={0,1,2};
            for (int i=0;i<2;i++) for (int j=i+1;j<3;j++)
                if (lg_rival_score(season,tn,idx[j]) > lg_rival_score(season,tn,idx[i])){ int t=idx[i]; idx[i]=idx[j]; idx[j]=t; }
            for (int i=0;i<3;i++){
                int ry=by+20+i*12; char nb[16]; char *q=nb;
                q=sapp_int(q, lg_rival_score(season, tn, idx[i])); *q=0;
                draw_text(bx+8, ry, LG_RIVAL_NAME[idx[i]], 1, COL(220,224,235));
                draw_text(bx+bw2-8-text_w(nb,1), ry, nb, 1, COL(220,224,235));
            }
            { char b[64]; char *q=b; int tg=lg_rival_score(season,tn,idx[0]);
              q=sappend(q, "TARGET: "); q=sapp_int(q, tg); *q=0;
              draw_text_center(bx+bw2/2, by+58, b, 1, COL(255,230,170));
              draw_text_center(bx+bw2/2, by+70, "TIES GO TO YOU", 1, COL(255,230,170)); } }
      }
    }
}
static void draw_bowling(void){
    Bowl *bw=&G.bowl[G.store_pi]; Player *me=&G.p[G.store_pi];
    int x0=G.view_x0, vw=G.view_w, compact=G.ui_compact;
    int cx = x0 + vw/2;
    rect_fill(x0, G.view_y0, vw, G.view_h>0? G.view_h : VH, COL(16,14,28));
    // v1.24 backdrop: hash speckle on the alley wall, so the lane
    // stops floating in a flat void (view-relative: split-safe)
    { int vh9 = G.view_h>0? G.view_h : VH;
      for (int gy=8; gy<vh9-8; gy+=26) for (int gx=10; gx<vw-8; gx+=26){
          int hx = x0+gx + (int)(hash_f(gx, gy, 31)*14), hy = G.view_y0+gy + (int)(hash_f(gx, gy, 47)*12);
          pset_blend(hx, hy, COL(120,110,170), 26 + (int)(hash_f(gx, gy, 59)*30));
      } }
    // header
    rect_fill(x0+4, 22, vw-8, 30, COL(50,70,120)); rect_line(x0+4, 22, vw-8, 30, OUTLINE);
    draw_text_center(cx, 26, "BUG BOWL", 2, COL(255,230,170));
    { char b[64]; char *q=b;
      if (!bw->active) q=sappend(q, "THE MALL LANES - ONE CARCASS A BALL");
      else if (bw->game_over){ q=sappend(q, "FINAL SCORE: "); q=sapp_int(q, bw->final); }
      else { if (bw->frame<9){ q=sappend(q, "FRAME "); q=sapp_int(q, bw->frame+1); }
             else q=sappend(q, "TENTH FRAME");
             q=sappend(q, " - BALL "); q=sapp_int(q, bw->ball+1);
             q=sappend(q, " - CARCASSES: "); q=sapp_int(q, me->stack_n); }
      *q=0; draw_text_center(cx, 42, b, 1, COL(200,220,255)); }
    // scoresheet: ten frame boxes, roll marks on top, running total below
    int sy=58, sh=compact? 36 : 40, fw=(vw-16)/10;
    for (int f=0;f<10;f++){
        int fx = x0+8+f*fw;
        rect_line(fx, sy, fw-2, sh, COL(150,160,190));
        int cells = f<9? 2 : 3, cw2=(fw-2)/cells;
        for (int c=1;c<cells;c++) rect_fill(fx+c*cw2, sy, 1, 13, COL(150,160,190));
        rect_fill(fx, sy+13, fw-2, 1, COL(150,160,190));
        int fri = bowl_frame_ri(bw, f);
        char mb[2]; mb[1]=0;
        if (f<9){
            int strike = (fri<bw->nrolls && bw->rolls[fri]==10);
            if (strike){ mb[0]='X'; draw_text_center(fx+cw2+cw2/2, sy+3, mb, 1, COL(255,230,170)); }
            else {
                if (fri<bw->nrolls){ mb[0]=bw->marks[fri]; draw_text_center(fx+cw2/2, sy+3, mb, 1, COL(255,230,170)); }
                if (fri+1<bw->nrolls){ mb[0]=bw->marks[fri+1]; draw_text_center(fx+cw2+cw2/2, sy+3, mb, 1, COL(255,230,170)); }
            }
        } else for (int c=0;c<3;c++)
            if (fri+c<bw->nrolls){ mb[0]=bw->marks[fri+c]; draw_text_center(fx+c*cw2+cw2/2, sy+3, mb, 1, COL(255,230,170)); }
        int cum = bowl_cum(bw, f);
        if (cum>=0){ char cb[8]; char *q=cb; q=sapp_int(q, cum); *q=0;
                     draw_text_center(fx+(fw-2)/2, sy+sh-14, cb, 1, COL(240,244,255)); }
    }
    // BUG BOWL LEAGUE (feat 10): the league card — full boxes on the
    // attract and game-over screens, condensed lines in compact views
    if (compact || !bw->active || bw->game_over) draw_league_card(x0, vw, cx, compact);
    // the lane
    float KX = compact? 1.15f : 1.5f, KY = 0.49f;
    int foul = VH-54;
    int lane_top = foul - (int)(392*KY);
    rect_fill(cx-(int)(76*KX), lane_top, (int)(152*KX), foul-lane_top+8, COL(96,80,66));
    rect_fill(cx-(int)(60*KX), lane_top, (int)(120*KX), foul-lane_top+8, COL(216,186,130));
    rect_fill(cx-(int)(60*KX), lane_top, (int)(120*KX), (int)(112*KY), COL(202,172,118));   // the deck
    for (int a=0;a<5;a++){ int ax = cx + (int)((a-2)*15*KX), ay = foul - (int)(150*KY) - (a==2? 10:0);
        tri_fill(ax, ay-6, ax-3, ay+4, ax+3, ay+4, COL(196,164,108)); }
    rect_fill(cx-(int)(60*KX), foul, (int)(120*KX), 3, COL(200,60,50));                    // foul line
    // v1.24 lane grounding: gutter shadow where lane meets the
    // surround, warm light pooled on the deck, and a wooden approach
    // under the bowler's feet
    rect_fill(cx-(int)(62*KX), lane_top, 2, foul-lane_top+8, COL(60,48,38));
    rect_fill(cx+(int)(60*KX), lane_top, 2, foul-lane_top+8, COL(60,48,38));
    glow_ellipse(cx, lane_top+14, (int)(86*KX), 30, COL(255,190,90), 30);
    rect_fill(cx-(int)(76*KX), foul+8, (int)(152*KX), VH-foul-28, COL(110,88,62));
    for (int px2=1; px2<6; px2++) rect_fill(cx-(int)(76*KX)+px2*(int)(152*KX)/6, foul+8, 1, VH-foul-28, COL(94,74,52));
    rect_fill(cx-(int)(76*KX), foul+8, (int)(152*KX), 2, COL(140,114,80));
    // pins, far row first
    for (int i=9;i>=0;i--){
        if (bw->standing[i]) draw_pin_shape(cx + (int)(PIN_X[i]*KX), foul - (int)(PIN_Y[i]*KY), 0);
        else if (bw->pft[i]>0) draw_pin_shape(cx + (int)(bw->pfx[i]*KX), foul - (int)(bw->pfy[i]*KY), bw->pfvx[i]*0.045f);
    }
    // the ball: a carcass, rolled. Before the throw it waits in hand.
    // Drawn from BOWL_BALL_R so the picture IS the physics: the sprite's
    // radius on screen is the radius that knocks pins (draw_carcass's
    // body radius is size+3 screen px, hence the -3).
    if (bw->bl_live) draw_carcass(cx + (int)(bw->bx*KX), foul - (int)(bw->by*KY), (int)(BOWL_BALL_R*KX)-3, bw->bl_kind, bw->by*0.06f);
    else if (bw->active && !bw->game_over && bw->phase<=2)
        draw_carcass(cx + (int)(bw->thrower_x*KX) + 10, foul+13, (int)(BOWL_BALL_R*KX)-3, 0, 0.5f);
    // the bowler: a mini dev in the player's own hoodie
    if (bw->active && !bw->game_over){
        int txp = cx + (int)(bw->thrower_x*KX), typ = foul+16;
        ellipse_fill(txp, typ+8, 7, 3, COL(40,36,50));
        rect_fill(txp-5, typ-2, 10, 9, HOODIE[me->color&3]); rect_line(txp-5, typ-2, 10, 9, OUTLINE);
        disc_fill(txp, typ-6, 4, COL(240,200,160));
        if (bw->phase==1 || bw->phase==2){
            int sxp = cx + (int)(bw->thrower_x*KX), syp = foul - (int)(6*KY);
            int ex = sxp + (int)(fsin(bw->angle)*54), ey = syp - (int)(fcos(bw->angle)*54);
            for (int d=1;d<=3;d++){ int px2=sxp+(ex-sxp)*d/3, py2=syp+(ey-syp)*d/3;
                disc_fill(px2, py2, 1, COL(255,240,200)); }
        }
        if (bw->phase==2){
            rect_fill(cx-70, VH-42, 140, 8, COL(60,60,70)); rect_line(cx-70, VH-42, 140, 8, OUTLINE);
            u32 pc = bw->power<50? COL(90,200,90) : bw->power<80? COL(240,200,60) : COL(230,80,60);
            rect_fill(cx-69, VH-41, (int)(bw->power*1.38f), 6, pc);
            draw_text_center(cx, VH-52, "POWER", 1, COL(200,210,230));
        }
    }
    if (bw->msg_t>0) draw_text_center(cx, foul-108, bw->msg, 2, COL(255,214,64));
    // BUG BOWL LEAGUE (feat 10): the night's verdict under the final
    if (bw->game_over){
        BowlLeague *L=&G.blg[G.store_pi];
        if (G.zen_run) draw_text_center(cx, foul-88, "ZEN PRACTICE - NOTHING COUNTED", 1, COL(200,220,255));
        else if (L->last_night>0){
            char b[80]; char *q=b; q=sappend(q, "NIGHT "); q=sapp_int(q, L->last_night);
            q=sappend(q, ": YOU "); q=sapp_int(q, L->last_score); q=sappend(q, " - ");
            q=sappend(q, L->last_place==1? "WON THE NIGHT" : L->last_place==2? "2ND TONIGHT" : L->last_place==3? "3RD TONIGHT" : "4TH TONIGHT");
            *q=0; draw_text_center(cx, foul-88, b, 1, COL(255,230,170));
            if (L->last_night>=LG_NIGHTS && L->fin_season==L->last_season){
                q=b; q=sappend(q, "SEASON "); q=sapp_int(q, L->fin_season); q=sappend(q, ": ");
                if (L->fin_place==1) q=sappend(q, "CHAMPION! +50 XP");
                else { q=sapp_int(q, L->fin_place); q=sappend(q, " OF 4"); }
                *q=0; draw_text_center(cx, foul-76, b, 1, COL(255,230,170));
            }
        }
    }
    if (!bw->active && G.rec_bowl>0){ char b[48]; char *q=b; q=sappend(q, "BEST GAME SO FAR: "); q=sapp_int(q, G.rec_bowl); *q=0;
        draw_text_center(cx, foul+30, b, 1, COL(200,220,255)); }
    if (bw->active && !bw->game_over && me->stack_n<=0 && bw->phase<=2)
        draw_text_center(cx, foul+30, "NO CARCASSES - GO SQUASH SOME BUGS!", 1, COL(255,140,120));
    // BUG BOWL LEAGUE (feat 10): the night in progress, at the foul
    // line's spare slot (the no-carcasses warning above keeps it when
    // the bowler is empty-handed)
    if (bw->active && !bw->game_over && me->stack_n>0){
        BowlLeague *L=&G.blg[G.store_pi];
        char b[80]; char *q=b;
        if (G.zen_run) q=sappend(q, "ZEN PRACTICE - THIS GAME WON'T COUNT");
        else { int season = L->season<1? 1 : L->season;
               int tg=lg_rival_score(season, L->night, 0);
               { int t1=lg_rival_score(season, L->night, 1), t2=lg_rival_score(season, L->night, 2); if (t1>tg) tg=t1; if (t2>tg) tg=t2; }
               q=sappend(q, "LEAGUE NIGHT "); q=sapp_int(q, L->night+1); q=sappend(q, " OF 6 - TARGET "); q=sapp_int(q, tg); }
        *q=0; draw_text_center(cx, foul+30, b, 1, COL(200,220,255));
    }
    // the prompt line — v1.30 (audit F5/D1): composed for the VIEWING
    // player's device (the pass sets g_dev_ctx): it used to print the
    // keyboard FIRE bind for P1 and a hard "A" for P2 whatever either
    // was holding. On touch the throw is a tap and the aim is a drag
    // (the lanes have no button deck), and the shell X is the way out.
    { int tch = (hint_dev()==DEV_TOUCH);
      const char *ak = tch? "TAP" : lbla(H_FIRE);
      char pb[128]; char *q=pb;
      if (!bw->active || bw->game_over){ q=sappend(q, ak); q=sappend(q, " - NEW GAME"); }
      else if (bw->phase==0){ if (tch) q=sappend(q, "HOLD TO AIM - TAP TO THROW");
                              else { q=sappend(q, "LEFT/RIGHT - POSITION - "); q=sappend(q, ak); q=sappend(q, " - AIM"); } }
      else if (bw->phase==1){ q=sappend(q, ak); q=sappend(q, " - LOCK THE ANGLE"); }
      else if (bw->phase==2){ q=sappend(q, ak); q=sappend(q, " - THROW!"); }
      else q=sappend(q, "ROLLING...");
      if (tch) q=sappend(q, "   X - LEAVE");
      *q=0; draw_text_center(cx, VH-12, pb, 1, COL(160,230,255)); }
}

