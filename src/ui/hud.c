// THE SURVIVAL GAME — ui/hud.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 3542-4038 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// HUD, leaderboard, store UI
// ---------------------------------------------------------------------------
static void draw_bar(int x, int y, int w, int h, float frac, u32 col){
    rect_fill(x-1, y-1, w+2, h+2, OUTLINE);
    rect_fill(x, y, w, h, COL(46,40,58));
    if (frac>0){ int fw = (int)(w*fclamp(frac,0,1));
        rect_fill(x, y, fw, h, col);
        if (h>=4) blend_rect(x, y, fw, h/2, COL(255,255,255), 42);   // gloss band (glow-up)
    }
}
// pretty HUD panel: soft shadow, slate body, pale top light, gold corners
static void ui_panel(int x, int y, int w, int h){
    rect_fill(x+3, y+4, w, h, COL(10,9,18));
    rect_fill(x, y, w, h, COL(24,22,38));
    rect_line(x, y, w, h, COL(104,98,138));
    rect_fill(x+1, y+1, w-2, 1, COL(92,86,122));
    u32 g = COL(255,205,40);
    rect_fill(x, y, 9, 2, g);         rect_fill(x, y, 2, 9, g);
    rect_fill(x+w-9, y, 9, 2, g);     rect_fill(x+w-2, y, 2, 9, g);
    rect_fill(x, y+h-2, 9, 2, g);     rect_fill(x, y+h-9, 2, 9, g);
    rect_fill(x+w-9, y+h-2, 9, 2, g); rect_fill(x+w-2, y+h-9, 2, 9, g);
}
// soft vignette for world views (Nov 5 glow-up): the edges of the
// view fall off into a gentle shade so the world feels lit instead of
// flat. Drawn AFTER the world and its entities but BEFORE any HUD or
// panel, so text and bars stay at full brightness — legibility first.
static void ui_vignette(int x0, int y0, int w, int h){
    const int D = 15;
    for (int i=0;i<D;i++){
        int a = (D-i)*3;
        blend_rect(x0, y0+i, w, 1, COL(6,6,14), a);
        blend_rect(x0, y0+h-1-i, w, 1, COL(6,6,14), a);
        blend_rect(x0+i, y0, 1, h, COL(6,6,14), a);
        blend_rect(x0+w-1-i, y0, 1, h, COL(6,6,14), a);
    }
}
// chunky touch button base: drop shadow, cartoon outline, shaded face
static void tbtn_base(int cx, int cy, int r, u32 face){
    disc_fill(cx+2, cy+3, r+2, COL(10,9,18));
    circle_out(cx, cy, r, face);
    circle_shade(cx, cy, r-3, face);
}
// little item glyphs (store rows + weapon chips), drawn in a ~18px box
static void draw_item_icon(int sel, int cx, int cy){
    u32 ink = COL(40,36,50);
    switch (sel){
    case 0:  // mechanical keyboard
        rect_fill(cx-9, cy-5, 18, 11, COL(240,238,228)); rect_line(cx-9, cy-5, 18, 11, ink);
        for (int i=0;i<3;i++){ disc_fill(cx-4+i*4, cy-1, 1, ink); disc_fill(cx-4+i*4, cy+3, 1, ink); }
        break;
    case 1:  // mouse tosser
        ellipse_fill(cx, cy+1, 7, 9, ink); ellipse_fill(cx, cy+1, 6, 8, COL(240,238,228));
        rect_fill(cx, cy-6, 1, 6, ink);
        break;
    case 2:  // rubber duck cannon
        ellipse_fill(cx+1, cy+3, 9, 5, COL(246,204,64));
        disc_fill(cx-4, cy-3, 4, COL(246,204,64));
        tri_fill(cx-10, cy-4, cx-5, cy-5, cx-5, cy-1, COL(232,128,44));
        disc_fill(cx-5, cy-4, 1, ink);
        break;
    case 3:  // PM launcher: a little rocket
        tri_fill(cx, cy-10, cx-3, cy-4, cx+3, cy-4, COL(198,60,50));
        rect_fill(cx-3, cy-4, 7, 10, COL(226,230,238)); rect_line(cx-3, cy-4, 7, 10, ink);
        tri_fill(cx-3, cy+6, cx-7, cy+10, cx-3, cy+10, COL(198,60,50));
        tri_fill(cx+4, cy+6, cx+4, cy+10, cx+7, cy+10, COL(198,60,50));
        disc_fill(cx, cy+9, 2, COL(255,170,60));
        break;
    case 4:  // the debugger: bug in crosshairs
        for (int s=-1;s<=1;s+=2){ rect_fill(cx+s*9-1, cy-1, 3, 2, COL(216,70,60)); rect_fill(cx-1, cy+s*9-1, 2, 3, COL(216,70,60)); }
        disc_fill(cx, cy, 6, ink); disc_fill(cx, cy, 5, COL(98,192,92));
        disc_fill(cx-2, cy-1, 1, ink); disc_fill(cx+2, cy-1, 1, ink);
        break;
    case 5:  // overclock: lightning bolt
        tri_fill(cx+3, cy-9, cx-4, cy+2, cx+0, cy+2, COL(255,205,40));
        tri_fill(cx+0, cy+2, cx-3, cy+9, cx+5, cy-2, COL(255,205,40));
        break;
    case 6:  // caffeine drip: coffee cup
        rect_fill(cx-6, cy-2, 11, 9, COL(240,238,228)); rect_line(cx-6, cy-2, 11, 9, ink);
        rect_fill(cx-6, cy-2, 11, 3, COL(120,72,40));
        rect_line(cx+5, cy+0, 4, 5, ink);
        break;
    case 7:  // firewall shield
        rect_fill(cx-6, cy-8, 13, 10, COL(110,190,255));
        tri_fill(cx-6, cy+2, cx+7, cy+2, cx, cy+10, COL(110,190,255));
        rect_fill(cx-1, cy-5, 3, 9, COL(226,240,255));
        break;
    case 8:  // hoodie armor: a heart
        disc_fill(cx-3, cy-3, 4, COL(226,80,90)); disc_fill(cx+4, cy-3, 4, COL(226,80,90));
        tri_fill(cx-7, cy-2, cx+8, cy-2, cx, cy+9, COL(226,80,90));
        break;
    case 9:  // espresso legs: speed chevrons
        tri_fill(cx-7, cy-5, cx-7, cy+5, cx-1, cy, COL(110,220,235));
        tri_fill(cx+1, cy-5, cx+1, cy+5, cx+7, cy, COL(110,220,235));
        break;
    case 10: // rollback token: circular arrow
        for (int a=0;a<10;a++){ float t = -0.6f + a*0.58f;
            disc_fill(cx+(int)(fcos(t)*6.5f), cy+(int)(fsin(t)*6.5f), 2, COL(110,220,140)); }
        tri_fill(cx+5, cy-9, cx+10, cy-4, cx+2, cy-3, COL(110,220,140));
        break;
    case 11: // hotfix: medical cross
        disc_fill(cx, cy, 9, COL(240,238,228));
        rect_fill(cx-2, cy-6, 5, 12, COL(216,70,60)); rect_fill(cx-6, cy-2, 12, 5, COL(216,70,60));
        break;
    default: // THE BACKFIRE BEAN: a green fellow, goggles on, ready
        ellipse_fill(cx, cy+2, 7, 8, COL(126,196,86));
        ellipse_fill(cx-2, cy-1, 2, 3, COL(168,226,120));
        rect_fill(cx-7, cy-5, 14, 3, COL(90,70,50));
        disc_fill(cx-3, cy-3, 2, COL(220,240,255)); disc_fill(cx+3, cy-3, 2, COL(220,240,255));
        pset(cx-3, cy-3, ink); pset(cx+3, cy-3, ink);
        disc_fill(cx-6, cy+9, 2, COL(190,225,150)); disc_fill(cx+6, cy+10, 2, COL(168,216,120));
        break;
    }
}
// weapon chips: a vertical strip under the status card, one chip per
// owned weapon (only shown once you own 2+). Tappable on touch.
static int owned_count(Player *me){ int n=0; for (int w=0;w<NWEAP;w++) if (me->owned[w]) n++; return n; }
static int chip_rect(Player *me, int w, int *x, int *y){
    int idx=0, found=0;
    for (int i=0;i<NWEAP;i++) if (me->owned[i]){ if (i==w){ *x=8; *y=94+idx*32; found=1; } idx++; }
    return (idx>=2 && found)? 1 : 0;
}
static int chip_at(Player *me, int tx, int ty){
    for (int w=0;w<NWEAP;w++){ int x,y; if (chip_rect(me,w,&x,&y) && tx>=x && tx<x+52 && ty>=y && ty<y+26) return w; }
    return -1;
}
static void render_leaderboard(int x, int y){
    // CONSTANT LEADERBOARD: heaviest hat wins. Always on screen.
    int idx[MAXP], n=0;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected) idx[n++]=i;
    for (int i=0;i<n;i++) for (int j=i+1;j<n;j++)
        if (G.p[idx[j]].carried > G.p[idx[i]].carried){ int t=idx[i]; idx[i]=idx[j]; idx[j]=t; }
    int wpx=150, hpx=26+n*11+14;
    ui_panel(x, y, wpx, hpx);
    draw_text(x+8, y+5, "HEAVIEST HATS", 1, COL(255,205,40));
    for (int i=0;i<n;i++){
        Player *p=&G.p[idx[i]];
        char b[32]; char *q=b;
        q=sapp_int(q, i+1); q=sappend(q, ". ");
        { int ni=0; while (p->name[ni] && ni<8){ *q++=p->name[ni++]; } *q=0; }  // truncated: the KG column must never be overrun
        draw_text(x+8, y+16+i*11, b, 1, i==0? COL(255,240,180):COL(210,210,220));
        fmt_int(b, "", (int)p->carried, "KG");
        draw_text(x+wpx-8-text_w(b,1), y+16+i*11, b, 1, COL(255,240,180));
        if (!p->alive) draw_text(x+wpx-30, y+16+i*11, "DOWN", 1, COL(255,110,110));
    }
    char b[40]; char *q=b;
    q=sappend(q, "BEST: "); q=sappend(q, G.rec_name); q=sappend(q, " ");
    q=sapp_int(q, (int)G.rec_hat); q=sappend(q, "KG");
    draw_text_fit(x+8, y+16+n*11+2, wpx-16, b, 1, COL(150,200,255));
}
// the way-back arrow: orbits (cx, cy) — normally the player's screen
// position — pointing home to the Epicenter, distance underneath.
// Hidden once you're close. Drawn per view in split-screen co-op so
// BOTH players get their own.
static void draw_home_arrow(Player *p, int cx, int cy){
    if (p->in_mall) return;              // inside the mall you can see the door
    float dx = -p->x, dy = -p->y; float d = fsqrt(dx*dx + dy*dy);
    if (d <= 190.0f) return;
    float ca = dx/d, sa = dy/d;
    int ax = cx + (int)(ca*46.0f), ay = cy + (int)(sa*46.0f);
    int tx = ax + (int)(ca*13), ty = ay + (int)(sa*13);
    int bx = ax - (int)(ca*7), by = ay - (int)(sa*7);
    int px = (int)(-sa*9), py = (int)(ca*9);
    tri_fill(tx, ty, bx+px, by+py, bx-px, by-py, COL(30,26,44));
    int t2x = ax + (int)(ca*10), t2y = ay + (int)(sa*10);
    int b2x = ax - (int)(ca*5), b2y = ay - (int)(sa*5);
    int qx = (int)(-sa*6), qy = (int)(ca*6);
    tri_fill(t2x, t2y, b2x+qx, b2y+qy, b2x-qx, b2y-qy, COL(255,214,64));
    char b[24]; char *q=b; q=sapp_int(q, (int)(d/50)); q=sappend(q, "M");
    draw_text_center(ax, ay+17, b, 1, COL(255,240,170));
}
static void render_hud(void){
    Player *me=&G.p[G.my_id];
    // top-left status card (wide enough for the longest hat tier name)
    ui_panel(8, 8, 224, 78);
    draw_bar(14, 14, 120, 8, (float)me->hp/me->maxhp, COL(120,230,90));
    { char b[24]; fmt_int(b, "HP ", me->hp, ""); draw_text(140, 14, b, 1, COL(255,255,255)); }
    if (me->shield_max>0) draw_bar(14, 26, 120, 6, me->shield_max? (float)me->shield/me->shield_max:0, COL(110,190,255));
    { char b[40]; char *q=b; q=sappend(q, WEAPON_NAME[me->weapon]);
      if (me->owned[me->weapon]>1){ q=sappend(q, " X"); q=sapp_int(q, me->owned[me->weapon]); }
      draw_text_fit(14, 38, 204, b, 1, COL(255,220,120)); }
    { char b[24]; char *q=b;
      q=sappend(q, "HAT: "); q=sapp_int(q, (int)me->carried); q=sappend(q, "KG");
      draw_text(14, 50, b, 1, COL(255,240,180)); }
    draw_text_fit(14, 61, 134, hat_tier_name(me->stack_n, me->carried), 1, COL(190,180,150));
    if (me->rollback>0){   // held revive tokens: know you've got one
        char b[20]; char *q=b; q=sappend(q, "TOKEN X"); q=sapp_int(q, me->rollback); *q=0;
        draw_text(226-text_w(b,1), 61, b, 1, COL(120,255,160));
    }
    if (me->beans>0){   // Backfire Beans held: the ride home (tap it, or key 6)
        char b[20]; char *q=b; q=sappend(q, "BEAN X"); q=sapp_int(q, me->beans); *q=0;
        draw_text(226-text_w(b,1), 70, b, 1, COL(150,230,120));
    }
    draw_bar(14, 72, 120, 4, me->carried/200.0f>1?1:me->carried/200.0f, COL(255,190,80));
    // weapon chips under the card: tap one to switch (touch), or use the
    // number keys. Gold frame = what you're holding; XN = stacked copies.
    for (int w=0;w<NWEAP;w++){ int cx, cy;
        if (!chip_rect(me, w, &cx, &cy)) continue;
        int cur = (me->weapon==w);
        rect_fill(cx+2, cy+3, 52, 26, COL(10,9,18));
        rect_fill(cx, cy, 52, 26, cur? COL(66,56,28) : COL(24,22,38));
        rect_line(cx, cy, 52, 26, cur? COL(255,205,40) : COL(104,98,138));
        draw_item_icon(w, cx+13, cy+13);
        if (me->owned[w]>1){ char b[8]; char *q=b; q=sappend(q, "X"); q=sapp_int(q, me->owned[w]);
            draw_text(cx+25, cy+9, b, 1, COL(255,240,200)); }
        else { char b[8]; char *q=b; q=sapp_int(q, w+1);
            draw_text(cx+25, cy+9, b, 1, COL(165,160,185)); }
    }
    // top-center: zone + distance, centered in the free band between the
    // status card (ends x=232) and the leaderboard (starts x=482) so long
    // zone names can never run into either panel
    float d = fsqrt(me->x*me->x + me->y*me->y);
    ui_panel(254, 5, 214, 46);   // v1.22: the zone pill wears the same panel chrome as the status card and the leaderboard
    if (me->in_mall && in_srcroom[me-G.p]){
        draw_text_center(361, 7, "THE SOURCE ROOM", 1, COL(255,255,255));
        draw_text_center(361, 19, "ROOT ACCESS", 1, COL(170,255,200));
        draw_text_center(361, 31, "SAFE ZONE - NO BUGS", 1, COL(150,230,150));
    } else if (me->in_mall && me->in_arcade){
        draw_text_center(361, 7, "THE ARCADE", 1, COL(255,255,255));
        draw_text_center(361, 19, "SEVEN FINE GAMES", 1, COL(200,210,230));
        draw_text_center(361, 31, "SAFE ZONE - NO BUGS", 1, COL(150,230,150));
    } else if (me->in_mall){
        draw_text_center(361, 7, "THE BUG MALL", 1, COL(255,255,255));
        draw_text_center(361, 19, "TEN FINE STOPS", 1, COL(200,210,230));
        draw_text_center(361, 31, "SAFE ZONE - NO BUGS", 1, COL(150,230,150));
    } else {
    draw_text_center_fit(361, 7, 200, zone_name(d), 1, COL(255,255,255));
    { char b[32]; char *q=b; q=sapp_int(q, (int)(d/50)); q=sappend(q, "M FROM EPICENTER");
      draw_text_center(361, 19, b, 1, COL(200,210,230)); }
    { compute_threat();
      char b[40]; char *q=b; q=sappend(q, "THREAT X"); q=sapp_int(q, (int)G.threat);
      *q++='.'; *q++=(char)('0'+((int)(G.threat*10)%10)); *q=0;
      if (G.zen_run) q=sappend(q, " - ZEN MODE");
      else if (G.diff_run) { q=sappend(q, " - "); q=sappend(q, DIFF_NAME[G.diff_run&3]); }
      draw_text_center_fit(361, 31, 200, b, 1, COL(255,150,90)); }
    }
    render_leaderboard(VW-158, 8);
    // bottom prompts (key names follow the player's bindings)
    { char pb[64]; const char *pr = interact_prompt(me, pb, 0);
      if (pr) draw_text_center(VW/2, VH-34, pr, 1, COL(255,240,150));
      else if (me->stack_n>=3 && !me->in_mall)
        { char b[64]; char *q=b; q=sappend(q, lbla(H_WALL)); q=sappend(q, " - THROW DOWN 3 CARCASSES AS A WALL");
          draw_text_center(VW/2, VH-34, b, 1, COL(220,220,200)); } }
    // (no bottom control line any more — controls live in the pause
    // menu's CONTROLS panel now; Daniel, Nov 6: the line overlapped)
    // run stats live under the player panel now, not at the bottom
    // of the screen (Daniel, Nov 6: the bottom stays for prompts)
    { char b[40]; char *q=b; int t=(int)G.time;
      q=sappend(q, "TIME "); q=sapp_int(q, t/60); q=sappend(q, ":");
      if (t%60<10) q=sappend(q, "0"); q=sapp_int(q, t%60);
      q=sappend(q, " - SQUASHED: "); q=sapp_int(q, me->kills);
      draw_text(14, 92, b, 1, COL(190,195,210)); }
    // way back: a gold arrow orbiting the player, pointing at the
    // Epicenter, with the distance under it (hidden once you're close)
    draw_home_arrow(me, VW/2, VH/2);
    // a partner is down: a red arrow points at the body, however far
    // it is — getting to them is the whole game now. The viewer is
    // whoever is still standing (in co-op both players watch the
    // same merged screen, centered on the rescuer).
    { Player *viewer = me->alive? me : 0;
      if (!viewer) for (int j=0;j<MAXP;j++) if (G.p[j].connected && G.p[j].alive){ viewer=&G.p[j]; break; }
      if (viewer) for (int j=0;j<MAXP;j++){ Player *dp=&G.p[j];
        if (dp==viewer || !dp->connected || dp->alive) continue;
        float dx = dp->x-viewer->x, dy = dp->y-viewer->y; float d = fsqrt(dx*dx+dy*dy);
        if (d > 60.0f){
            float ca = dx/d, sa = dy/d;
            int cx2 = VW/2, cy2 = VH/2;
            // a wider orbit than the gold home arrow (46), so the two
            // never sit on top of each other when a partner went down
            // near the Epicenter — exactly where rescues point home
            int ax = cx2 + (int)(ca*66.0f), ay = cy2 + (int)(sa*66.0f);
            int tx = ax + (int)(ca*13), ty = ay + (int)(sa*13);
            int bx = ax - (int)(ca*7), by = ay - (int)(sa*7);
            int px = (int)(-sa*9), py = (int)(ca*9);
            tri_fill(tx, ty, bx+px, by+py, bx-px, by-py, COL(30,26,44));
            int t2x = ax + (int)(ca*10), t2y = ay + (int)(sa*10);
            int b2x = ax - (int)(ca*5), b2y = ay - (int)(sa*5);
            int qx = (int)(-sa*6), qy = (int)(ca*6);
            tri_fill(t2x, t2y, b2x+qx, b2y+qy, b2x-qx, b2y-qy, COL(255,70,60));
            char b[40]; char *s=b; s=sappend(s, dp->name); s=sappend(s, " DOWN - ");
            s=sapp_int(s, (int)(d/50)); s=sappend(s, "M"); *s=0;
            draw_text_center(ax, ay+17, b, 1, COL(255,150,140));
        }
        break;
      } }
    // WALL button (both touch schemes): throws down 3 carcasses as a wall.
    // It pulses once you're carrying enough to build one.
    if (mobile_ui() && !pad_mode()){
        int pulse = (me->stack_n>=3) && (fsin(G.anim_t*4.0f)>0.0f);
        tbtn_base(TP_WALL_CX, TP_WALL_CY, 24, pulse? COL(158,128,54) : COL(62,56,86));
        u32 br = COL(232,186,108);   // little brick wall glyph
        rect_fill(TP_WALL_CX-10, TP_WALL_CY-11, 9, 4, br); rect_fill(TP_WALL_CX+1, TP_WALL_CY-11, 9, 4, br);
        rect_fill(TP_WALL_CX-10, TP_WALL_CY-6, 6, 4, br);  rect_fill(TP_WALL_CX-3, TP_WALL_CY-6, 6, 4, br);
        rect_fill(TP_WALL_CX+4, TP_WALL_CY-6, 6, 4, br);
        draw_text(TP_WALL_CX-text_w("WALL",1)/2, TP_WALL_CY+4, "WALL", 1, COL(255,244,200));
    }
    // drag scheme still needs a weapon cycler (stick mode draws its own below)
    if (G.touch_ctl==0 && mobile_ui()){
        tbtn_base(TP_WPN_CX, TP_WPN_CY, 18, COL(66,60,96));
        draw_text(TP_WPN_CX-text_w("WPN",1)/2, TP_WPN_CY-4, "WPN", 1, COL(225,230,245));
    }
    // stick controls (touch scheme 1): floating joystick + fixed buttons
    if (G.touch_ctl==1 && mobile_ui()){
        tbtn_base(TP_FIRE_CX, TP_FIRE_CY, 34, (IN.tp_held&32)? COL(224,88,62):COL(158,54,46));
        { u32 lc = COL(255,240,230); int gy = TP_FIRE_CY-6;   // crosshair glyph
          rect_fill(TP_FIRE_CX-1, gy-10, 3, 6, lc); rect_fill(TP_FIRE_CX-1, gy+5, 3, 6, lc);
          rect_fill(TP_FIRE_CX-10, gy-1, 6, 3, lc); rect_fill(TP_FIRE_CX+5, gy-1, 6, 3, lc);
          disc_fill(TP_FIRE_CX, gy, 2, lc); }
        draw_text(TP_FIRE_CX-text_w("FIRE",1)/2, TP_FIRE_CY+14, "FIRE", 1, COL(255,240,230));
        tbtn_base(TP_ACT_CX, TP_ACT_CY, 22, COL(56,92,62));
        { u32 lc = COL(230,255,235); int gy = TP_ACT_CY-3;    // up-arrow glyph
          tri_fill(TP_ACT_CX, gy-8, TP_ACT_CX-6, gy+1, TP_ACT_CX+6, gy+1, lc);
          rect_fill(TP_ACT_CX-2, gy+1, 5, 5, lc); }
        draw_text(TP_ACT_CX-text_w("USE",1)/2, TP_ACT_CY+9, "USE", 1, COL(230,255,235));
        tbtn_base(TP_WPN_CX, TP_WPN_CY, 18, COL(66,60,96));
        draw_text(TP_WPN_CX-text_w("WPN",1)/2, TP_WPN_CY-4, "WPN", 1, COL(225,230,245));
        if (IN.joy_on){
            circle_out(IN.joy_ax, IN.joy_ay, 42, COL(34,31,52));
            circle_shade(IN.joy_ax, IN.joy_ay, 39, COL(40,36,58));
            disc_fill(IN.joy_ax, IN.joy_ay, 25, COL(30,27,46));
            circle_out(IN.joy_kx, IN.joy_ky, 17, COL(198,204,220));
            circle_shade(IN.joy_kx, IN.joy_ky, 14, COL(198,204,220));
            disc_fill(IN.joy_kx-5, IN.joy_ky-5, 4, COL(244,246,250));
        } else {
            // faint home base so players know where the stick lives
            disc_fill(TP_DPAD_CX, TP_DPAD_CY, 42, COL(24,22,38));
            circle_out(TP_DPAD_CX, TP_DPAD_CY, 15, COL(52,48,72));
        }
    }
    // damage vignette
    if (me->hurt_t>0){
        rect_fill(0,0,VW,3,COL(230,60,60)); rect_fill(0,VH-3,VW,3,COL(230,60,60));
        rect_fill(0,0,3,VH,COL(230,60,60)); rect_fill(VW-3,0,3,VH,COL(230,60,60));
    }
}
// store layout shared by render + input — rows are tall and the text is
// double-size so phone fingers can hit them
#define STORE_X 70
#define STORE_Y 4
#define STORE_W 500
#define STORE_H 352
#define STORE_ROW0 54
#define STORE_ROWH 18
#define STORE_UPGOFF 18
#define STORE_ROWS 13
static int store_item_price(int sel, Player *p, int *owned_out){
    // sel: 0..4 weapons, 5..12 upgrades; returns price or -1 (owned/maxed)
    *owned_out=0;
    if (sel<5){
        if (p->owned[sel]>=MAXCOPIES){ *owned_out=1; return -1; }
        // stacking escalates: every copy costs more than the one before
        // (copy N costs N x the base price; the free starter keyboard
        // escalates from a 10 KG unit so its stack isn't free either)
        int unit = weap_price[sel]? weap_price[sel] : 10;
        return unit * (p->owned[sel]+1);
    }
    int u=sel-5;
    if (u==5){ return upg_price_tab[5][0]; }                  // rollback: always buyable
    if (u==7){ return upg_price_tab[7][0]; }                  // THE BACKFIRE BEAN: always buyable, never cheap
    if (u==6){                                            // hotfix: heal, unless full
        if (p->hp>=p->maxhp){ *owned_out=1; return -1; }
        return upg_price_tab[6][0];
    }
    if (p->upg[u]>=UPG_MAXLV[u]){ *owned_out=1; return -1; }
    return upg_price(u, p->upg[u]);
}
static const char *store_item_name(int sel){
    return sel<5? WEAPON_NAME[sel] : UPG_NAME[sel-5];
}
static const char *store_item_blurb(int sel, Player *p){
    if (sel<5){
        static char bb[96];
        if (p->owned[sel]>=MAXCOPIES){ char *q=bb; q=sappend(q, "FULLY STACKED - MAXIMUM FAN!"); *q=0; return bb; }
        if (p->owned[sel]>0){
            int ow2; int nx = store_item_price(sel, p, &ow2);
            char *q=bb; q=sappend(q, "STACKED X"); q=sapp_int(q, p->owned[sel]);
            q=sappend(q, sel==4? " - BEAMS RING OUT - NEXT COPY " : " - SHOTS FAN OUT - NEXT COPY ");
            q=sapp_int(q, nx); q=sappend(q, " KG");
            *q=0; return bb;
        }
        return WEAPON_BLURB[sel];
    }
    switch (sel-5){
        case 0: return "All weapons hit 25% harder per level.";
        case 1: return "All weapons fire 18% faster per level.";
        case 2: return "A firewall bubble eats damage. Regenerates.";
        case 3: return "+25 max HP per level. Comfy AND protective.";
        case 4: return "Heavy hats slow you less per level. A bit quicker bare, too.";
        case 5: return "Death? Rolled back. Revive at half HP.";
        case 7: { static char bb7[96]; char *q=bb7;   // v1.30 (audit F13): the pad's bean is the Y button — name the viewer's own input
                  q=sappend(q, "Squeeze him: PBBBT - straight home to the Epicenter. ");
                  q=sappend(q, hint_dev()==DEV_PAD? "(Y BUTTON)" : (hint_dev()==DEV_TOUCH? "(TAP THE BEAN)" : "(KEY 6)"));
                  *q=0; return bb7; }
        default: return "A hotfix, applied directly to you. +50 HP.";
    }
}
// the Bug Bounty panel for whoever is shopping (G.store_pi). Solo it
// covers the screen over a dim; in split-screen co-op it squeezes into
// the shopper's half (ui_compact) with the living world — shield and
// all — still visible around it.
static void draw_store_panel(void){
    Player *me=&G.p[G.store_pi];
    int sel = (G.p2_slot>=0 && G.store_pi==G.p2_slot)? G.store_sel2 : G.store_sel;
    int y=STORE_Y, h=STORE_H;
    int w = G.ui_compact? 308 : STORE_W;
    int x = G.ui_compact? G.view_x0 + (G.view_w - w)/2 : STORE_X;
    if (!G.ui_compact) rect_fill(0, 0, VW, VH, COL(16,14,28));   // dim (solid, drawn as overlay panels)
    else rect_fill(G.view_x0, G.view_y0, G.view_w, G.view_h>0? G.view_h : VH, COL(16,14,28));   // split: dim the shopper's whole half, so no world (or bug) peeks around the panel
    rect_fill(x, y+1, w+8, h+8, COL(8,6,14));            // drop shadow
    rect_fill(x-4, y-4, w+8, h+8, OUTLINE);
    rect_fill(x, y, w, h, COL(250,244,225));
    rect_line(x+3, y+3, w-6, h-6, COL(198,164,84));      // gold inner trim
    rect_fill(x, y, w, 32, COL(74,36,34));               // title band
    rect_fill(x, y+31, w, 2, COL(198,164,84));
    draw_text_center(x+w/2, y+3, "THE COUNTER", 2, COL(255,226,150));
    { char b[56]; char *q=b;
      q=sappend(q, "ON YOUR HEAD: "); q=sapp_int(q, (int)me->carried); q=sappend(q, " KG");
      if (!G.ui_compact){ q=sappend(q, "  ("); q=sappend(q, hat_tier_name(me->stack_n, me->carried)); q=sappend(q, ")"); }
      draw_text_center(x+w/2, y+20, b, 1, COL(255,240,205)); }
    rect_fill(x+14, STORE_ROW0-11, 6, 6, COL(190,60,40));
    if (G.ui_compact){
        draw_text(x+24, STORE_ROW0-13, "WEAPONS - STACK FOREVER", 1, COL(160,60,40));
    } else {
        draw_text(x+24, STORE_ROW0-16, "WEAPONS - STACK FOREVER", 2, COL(160,60,40));
        rect_fill(x+308, STORE_ROW0-9, w-322, 2, COL(216,200,160));
    }
    if (!G.ui_compact)
    { const char *sh = "SHIELD: UP";   // you're in here, so it's up — sits on the title band
      draw_text(x+w-14-text_w(sh,1), y+8, sh, 1, COL(150,220,255)); }
    rect_fill(x+14, STORE_ROW0+5*STORE_ROWH+5, 6, 6, COL(190,60,40));
    if (G.ui_compact){
        draw_text(x+24, STORE_ROW0+5*STORE_ROWH+3, "UPGRADES - STACK FOREVER", 1, COL(160,60,40));
    } else {
        draw_text(x+24, STORE_ROW0+5*STORE_ROWH, "UPGRADES - STACK FOREVER", 2, COL(160,60,40));
        rect_fill(x+320, STORE_ROW0+5*STORE_ROWH+7, w-334, 2, COL(216,200,160));
    }
    for (int i=0;i<STORE_ROWS;i++){
        int ry = STORE_ROW0 + (i<5? i*STORE_ROWH : i*STORE_ROWH+STORE_UPGOFF);
        int owned; int price = store_item_price(i, me, &owned);
        if (i%2==0) rect_fill(x+8, ry-2, w-16, STORE_ROWH, COL(243,236,210));   // zebra
        rect_line(x+8, ry-2, w-16, STORE_ROWH, COL(228,219,192));               // row card edge
        if (i==sel){
            rect_fill(x+8, ry-2, w-16, STORE_ROWH, COL(255,224,138));
            rect_fill(x+8, ry-2, 3, STORE_ROWH, COL(190,60,40));                // selection tab
            rect_line(x+8, ry-2, w-16, STORE_ROWH, COL(198,164,84));
        }
        rect_fill(x+30, ry+1, 16, 16, i==sel? COL(255,240,196) : COL(255,252,240));
        rect_line(x+30, ry+1, 16, 16, COL(198,164,84));
        draw_item_icon(i, x+38, ry+9);
        if (G.ui_compact) draw_text(x+56, ry+5, store_item_name(i), 1, COL(40,36,50));
        else draw_text(x+56, ry+2, store_item_name(i), 2, COL(40,36,50));
        // stack count as plain text — X1, X2, X3... — not pips.
        // Rollback shows tokens held.
        { int cnt = -1;
          if (i<5) cnt = me->owned[i];
          else if (i-5==5) cnt = me->rollback;
          else if (i==12) cnt = me->beans;
          else if (i-5!=6) cnt = me->upg[i-5];
          if (cnt>0){ char b[16]; char *q=b; *q++='X'; q=sapp_int(q, cnt); *q=0;
              // the count lives in its own column LEFT of the icon
              // (Daniel, Nov 5): one fixed place the eye learns,
              // instead of at the end of names of random length
              draw_text(x+28-text_w(b,1), ry+5, b, 1, COL(150,95,30)); } }
        char b[16];
        if (owned) scpy(b, i==11? "FULL HP" : "MAXED", 16);
        else fmt_int(b, "", price, " KG");
        u32 pc = owned? COL(120,150,110) : ((int)me->carried>=price? COL(150,95,30) : COL(186,128,116));
        if (G.ui_compact) draw_text(x+w-14-text_w(b,1), ry+5, b, 1, pc);
        else draw_text(x+w-14-text_w(b,2), ry+2, b, 2, pc);
    }
    // selected blurb + status
    int by = y+h-40;
    int bh = G.ui_compact? 24 : 20;
    rect_fill(x+8, by-4, w-16, bh, COL(240,232,205));
    rect_line(x+8, by-4, w-16, bh, COL(198,164,84));
    rect_fill(x+8, by-4, 3, bh, COL(190,60,40));
    if (G.ui_compact){
        // narrow panel: wrap the blurb onto two lines at a word break
        char bb[96]; scpy(bb, store_item_blurb(sel, me), 96);
        int cut=-1; for (int i=0; bb[i] && i<34; i++) if (bb[i]==' ') cut=i;
        if (cut>0 && bb[cut+1]){ bb[cut]=0;
            draw_text(x+14, by+1, bb, 1, COL(80,66,44));
            draw_text(x+14, by+12, bb+cut+1, 1, COL(80,66,44));
        } else draw_text(x+14, by+2, bb, 1, COL(80,66,44));
    }
    else draw_text(x+14, by+2, store_item_blurb(sel, me), 1, COL(80,66,44));
    if (pay_active) draw_text_center(x+w/2, y+h-13, "COUNTING CARCASSES...", 1, COL(190,60,40));
    else if (G.store_msg_t>0) draw_text_center(x+w/2, y+h-13, G.store_msg, 1, COL(60,140,60));
    else if (G.ui_compact){ if (hint_dev()==DEV_TOUCH) draw_text_center(x+w/2, y+h-13, "TAP A ROW - TAP AGAIN TO BUY", 1, COL(88,76,54));
        else { char b[80]; char *q=b; q=sappend(q, "SHIELD UP - "); q=sappend(q, lbla(H_FIRE)); q=sappend(q, " BUYS - "); q=sappend(q, lbla(H_BACK)); q=sappend(q, " LEAVES");
               draw_text_center(x+w/2, y+h-13, b, 1, COL(88,76,54)); } }
    else if (hint_dev()==DEV_TOUCH) draw_text_center(x+w/2, y+h-13, "TAP A ROW - TAP AGAIN TO BUY - TAP OUTSIDE TO LEAVE", 1, COL(88,76,54));
    else { char b[96]; char *q=b; q=sappend(q, lbla(H_MOVE)); q=sappend(q, " CHOOSE - "); q=sappend(q, lbla(H_CONFIRM));
           q=sappend(q, " BUYS - "); q=sappend(q, lbla(H_INT)); q=sappend(q, " LEAVES");
           draw_text_center(x+w/2, y+h-13, b, 1, COL(88,76,54)); }
}
static void render_store(void){
    G.store_pi = G.my_id; G.ui_compact = 0;
    g_dev_ctx = dev_of(G.store_pi);
    render_world();
    draw_active_panel();
}


