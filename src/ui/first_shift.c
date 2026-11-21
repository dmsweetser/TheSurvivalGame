// THE SURVIVAL GAME — ui/first_shift.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 4039-4188 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// FIRST SHIFT (v1.25) — the HUD prompt, drawn once per frame from
// the frame tail (mall.c) so solo and split-screen co-op see the
// identical shared bar. Text and primitive shapes only: no sprite
// assumptions, so Eddie Mode is safe. The chain core is in save.c.
// ---------------------------------------------------------------------------
// where the current place-step wants the player to go, routed
// through doors when the target sits in another room of the
// complex. Returns 0 for steps with no place.
static int fs_target(Player *me, float *tx, float *ty, const char **label){
    float dx, dy; const char *lb;
    if (G.fs_step==3){ dx=-315; dy=MALL_CY-148; lb="THE COUNTER"; }
    else if (G.fs_step==4){ dx=105; dy=MALL_CY-148; lb="BUG BANK"; }
    else if (G.fs_step==5){ dx=0; dy=ARC_CY-108; lb="THE ARCADE"; }
    else return 0;
    if (G.fs_step==5){
        if (me->in_arcade){ *tx=dx; *ty=dy; }
        else if (me->in_mall){ *tx=350; *ty=MALL_CY+26; }   // the Arcade door
        else { *tx=0; *ty=-10; }                            // the mall hut door
    } else {
        if (me->in_arcade){ *tx=-330; *ty=ARC_CY+24; }      // the Arcade exit mat
        else if (me->in_mall){ *tx=dx; *ty=dy; }
        else { *tx=0; *ty=-10; }                            // the mall hut door
    }
    *label=lb;
    return 1;
}
static void fs_draw_prompt(void){
    if (G.state!=GS_PLAY) return;
    int mi = G.my_id>=0? G.my_id : 0;
    Player *me=&G.p[mi];
    int flashing = fs_flash_t>0;
    if (G.fs_state!=1 && !flashing) return;
    // inside a cabinet the mode owns the screen; the finale flash
    // is the one thing that still shows there
    if (G.bud[mi].in_bw && !flashing) return;
    if (G.fs_state==1 && !flashing){
        float tx, ty; const char *label=0;
        if (fs_target(me, &tx, &ty, &label)){
            float dx=tx-me->x, dy=ty-me->y; float d=fsqrt(dx*dx+dy*dy);
            if (d>150.0f){
                // an orbiting arrow in the home-arrow idiom, at its
                // own radius (home rides 46, a downed partner 66)
                float ca=dx/d, sa=dy/d;
                int ax=VW/2+(int)(ca*62.0f), ay=VH/2+(int)(sa*62.0f);
                int txp=ax+(int)(ca*13), typ=ay+(int)(sa*13);
                int bx=ax-(int)(ca*7), by=ay-(int)(sa*7);
                int px=(int)(-sa*9), py=(int)(ca*9);
                tri_fill(txp, typ, bx+px, by+py, bx-px, by-py, COL(30,26,44));
                int t2x=ax+(int)(ca*10), t2y=ay+(int)(sa*10);
                int b2x=ax-(int)(ca*5), b2y=ay-(int)(sa*5);
                int qx=(int)(-sa*6), qy=(int)(ca*6);
                tri_fill(t2x, t2y, b2x+qx, b2y+qy, b2x-qx, b2y-qy, COL(120,230,255));
                { char b[48]; char *q=b; q=sappend(q, label); q=sappend(q, " - ");
                  q=sapp_int(q, (int)(d/50)); q=sappend(q, "M"); *q=0;
                  draw_text_center(ax, ay+17, b, 1, COL(180,240,255)); }
            } else if (G.p2_slot<0){
                // close: a small bouncing diamond on the spot itself
                int sx=SX(tx), sy=SY(ty);
                if (sx>16 && sx<VW-16 && sy>30 && sy<VH-16){
                    int bob=(int)(fsin(G.anim_t*3.0f)*3.0f);
                    tri_fill(sx, sy-8+bob, sx-6, sy+bob, sx+6, sy+bob, COL(120,230,255));
                    tri_fill(sx, sy+8+bob, sx-6, sy+bob, sx+6, sy+bob, COL(70,160,200));
                    draw_text_center(sx, sy-24+bob, label, 1, COL(180,240,255));
                }
            }
        }
    }
    { char b[96]; char *q=b;
      if (flashing){
          if (fs_flash_fin) q=sappend(q, "FIRST SHIFT COMPLETE!  +");
          else q=sappend(q, "STEP DONE!  +");
          q=sapp_int(q, fs_flash_amt); q=sappend(q, " XP"); *q=0;
      } else {
          q=sappend(q, "FIRST SHIFT "); q=sapp_int(q, G.fs_step+1); q=sappend(q, "/6:  ");
          q=sappend(q, FS_TEXT[G.fs_step]); *q=0;
      }
      int w = text_w(b,1)+28; if (w>560) w=560;
      int x = VW/2-w/2, y = VH-62;
      ui_panel(x, y, w, 24);
      draw_text_center(VW/2, y+8, b, 1, flashing? COL(150,255,190) : COL(255,240,180)); }
}
// WEEKLY CHALLENGE (v1.25 feature 6): the completion / new-week
// banner — the First Shift flash's twin, drawn just above its bar
// (y = VH-90 vs VH-62) so a step and a week can celebrate in the
// same frame without colliding. No prompt bar of its own: the
// challenge has no single target to point at.
static void wk_draw_flash(void){
    if (G.state!=GS_PLAY || wk_flash_t<=0 || !wk_flash_kind) return;
    const char *b = wk_flash_kind==1? "WEEKLY CHALLENGE COMPLETE! +50 XP"
                                    : "A NEW WEEK! THE BOARD HAS A FRESH CHALLENGE.";
    int w = text_w(b,1)+28; if (w>560) w=560;
    int x = VW/2-w/2, y = VH-90;
    ui_panel(x, y, w, 24);
    draw_text_center(VW/2, y+8, b, 1, wk_flash_kind==1? COL(150,255,190) : COL(255,240,180));
}
// NAMED ELITES (v1.25 feature 8): the hunt's HUD — an amber tracker
// in the First Shift marker idiom (orbiting arrow at its own radius,
// 84: home rides 46, First Shift 62, a downed partner 66) plus the
// claim banner, stacked above the Weekly flash (y = VH-118). The
// tracker scans G.bugs itself, so a LAN guest — whose bugs arrive by
// snapshot, elite byte and all — sees the same arrow. Drawn once per
// frame from the frame tail (mall.c), like its twins.
static void bn_draw_hud(void){
    if (G.state!=GS_PLAY) return;
    int mi = G.my_id>=0? G.my_id : 0;
    Player *me=&G.p[mi];
    // the claim banner (host/local only — the flash is only set there)
    if (bn_flash_t>0 && bn_flash_name>=0){
        char b[80]; char *q=b;
        q=sappend(q, "BOUNTY CLAIMED! "); q=sappend(q, ELITE_NAME[bn_flash_name%ELITE_N]);
        q=sappend(q, " IS DOWN. +"); q=sapp_int(q, BN_XP); q=sappend(q, " XP"); *q=0;
        int w = text_w(b,1)+28; if (w>600) w=600;
        int x = VW/2-w/2, y = VH-118;
        ui_panel(x, y, w, 24);
        draw_text_center(VW/2, y+8, b, 1, COL(255,214,120));
    }
    // the tracker: only in the overworld, only while an elite lives
    if (me->in_mall || me->in_arcade || G.bud[mi].in_bw) return;
    int li = bn_any_live();
    if (li<0) return;
    Bug *t=&G.bugs[li];
    const char *nm = ELITE_NAME[(t->elite-1)%ELITE_N];
    float dx=t->x-me->x, dy=t->y-me->y; float d=fsqrt(dx*dx+dy*dy);
    if (d>150.0f){
        float ca=dx/d, sa=dy/d;
        int ax=VW/2+(int)(ca*84.0f), ay=VH/2+(int)(sa*84.0f);
        int txp=ax+(int)(ca*13), typ=ay+(int)(sa*13);
        int bx=ax-(int)(ca*7), by=ay-(int)(sa*7);
        int px=(int)(-sa*9), py=(int)(ca*9);
        tri_fill(txp, typ, bx+px, by+py, bx-px, by-py, COL(30,26,44));
        int t2x=ax+(int)(ca*10), t2y=ay+(int)(sa*10);
        int b2x=ax-(int)(ca*5), b2y=ay-(int)(sa*5);
        int qx=(int)(-sa*6), qy=(int)(ca*6);
        tri_fill(t2x, t2y, b2x+qx, b2y+qy, b2x-qx, b2y-qy, COL(255,170,60));
        { char b[64]; char *q=b; q=sappend(q, nm); q=sappend(q, " - ");
          q=sapp_int(q, (int)(d/50)); q=sappend(q, "M"); *q=0;
          draw_text_center(ax, ay+17, b, 1, COL(255,205,110)); }
    } else if (G.p2_slot<0){
        int sx=SX(t->x), sy=SY(t->y);
        if (sx>16 && sx<VW-16 && sy>30 && sy<VH-16){
            int bob=(int)(fsin(G.anim_t*3.0f)*3.0f);
            tri_fill(sx, sy-8+bob, sx-6, sy+bob, sx+6, sy+bob, COL(255,170,60));
            tri_fill(sx, sy+8+bob, sx-6, sy+bob, sx+6, sy+bob, COL(200,120,40));
            // no name here: the bug's own nameplate (drawn with the
            // sprite) already names it — printing it twice at this
            // range just smudges the plate
        }
    }
}
