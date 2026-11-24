// THE SURVIVAL GAME — menus/menu_render.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 1918-2095 in the Nov 2026 fine-grained refactor; code unchanged.

static void render_menu(void){
    draw_demo_scene();
    // dead bugs drift up past the scene like ash — the evidence rises
    for (int i=0;i<12;i++){
        unsigned h = (unsigned)i*2654435761u;
        float rise = G.anim_t*(12.0f + (float)((h>>16)%11)) + (float)(h%340);
        rise -= (int)(rise/400.0f)*400.0f;
        int cx = (int)((h>>8)%620) + 10 + (int)(fsin(G.anim_t*1.2f + (float)i*1.7f)*9.0f);
        draw_carcass(cx, 372 - (int)rise, 5 + (int)((h>>20)%3), (int)((h>>24)%4), G.anim_t*0.8f + (float)i);
    }
    // dim the top for the title (band shortened Nov 14, 2026 at
    // Daniel's word: the tall header was crowding the rows below it
    // into the records strip — the whole menu stack now starts higher)
    rect_fill(0, 0, VW, 96, COL(20,18,36));
    // twinkling stars in the title band
    for (int i=0;i<42;i++){
        unsigned h = (unsigned)i*2246822519u + 97u;
        int sx = (int)((h>>7)%636) + 2, sy = (int)((h>>15)%84) + 4;
        int ph = (int)(G.anim_t*2.0f + (float)((h>>24)%5));
        u32 c = (ph%5==0)? COL(255,250,220) : (ph%5==1)? COL(190,205,255) : COL(90,100,140);
        rect_fill(sx, sy, 2, 2, c);
    }
    rect_fill(0, 94, VW, 2, COL(255,205,40));
    glow_ellipse(VW/2, 28, 200, 24, COL(255,214,120), 26);   // a warm halo behind the title (glow-up)
    // the title, letter by letter on a slow wave, gold shimmering
    // (the font advances 8px per char times scale — 32px at scale 4)
    { const char *T = "THE SURVIVAL GAME";
      int x0 = VW/2 - (17*32)/2;
      for (int i=0;i<17;i++){
          if (T[i]==' ') continue;
          char ch[2]; ch[0]=T[i]; ch[1]=0;
          float ph2 = G.anim_t*3.0f - (float)i*0.55f;
          int y = 8 + (int)(fsin(ph2)*3.0f);
          float t2 = (fsin(ph2)+1.0f)*0.5f;
          draw_text(x0+i*32+2, y+3, ch, 4, COL(12, 10, 26));   // v1.22: a drop shadow grounds the logo
          draw_text(x0+i*32, y, ch, 4, COL(255, 214 + (int)(34.0f*t2), 64 + (int)(120.0f*t2)));
      } }
    { float tp = (fsin(G.anim_t*2.0f)+1.0f)*0.5f;
      draw_text_center(VW/2, 50, "SQUASH BUGS. WEAR THE EVIDENCE.", 1, COL(160 + (int)(70.0f*tp), 230 + (int)(20.0f*tp), 255)); }
    // menu items (selection also covers the two text fields below them:
    // rows n and n+1 are NAME and HOST IP — keyboard only, no mouse)
    int n=menu_count();
    if (G.menu_sel>=n+MENU_FIELDS) G.menu_sel=0;
    for (int i=0;i<n;i++){
        int y=108+i*22;
        const char *label=menu_item(i);
        if (i==G.menu_sel){
            rect_fill(0, y-4, VW, 18, COL(255,205,40));   // full-width bar: the whole row is the option
            draw_text_center(VW/2, y, label, 2, COL(30,26,44));
        } else draw_text_center(VW/2, y, label, 2, COL(235,238,245));
    }
    // name field (+ ip field on native builds)
    int fy=108+n*22+8;
    int caret = ((int)(G.anim_t*2))%2==0;
    int nx = MENU_SIMPLE? VW/2-82 : VW/2-150;
    draw_text(nx, fy+3, "NAME:", 1, G.menu_sel==n? COL(255,205,40):COL(220,225,235));
    rect_fill(nx+42, fy, 130, 13, G.menu_sel==n? COL(70,80,110):COL(44,46,66));
    rect_line(nx+42, fy, 130, 13, G.menu_sel==n? COL(255,205,40):COL(140,150,180));
    { char b[16]; char *q=b; q=sappend(q, G.my_name); if (G.menu_sel==n && caret) q=sappend(q, "_");
      draw_text(nx+46, fy+3, b, 1, COL(255,255,255)); }
    if (G.whale_on & 8192){   // WHALES ONLY: NAME IN LIGHTS — marquee bulbs chase around the name box
        static const u32 BULB[4] = { COL(255,90,90), COL(255,205,60), COL(120,230,255), COL(255,240,210) };
        int bx0 = nx+42, by0 = fy, bx1 = nx+42+130, by1 = fy+13, bi = 0;
        for (int bx=bx0; bx<=bx1; bx+=11){ disc_fill(bx, by0-3, 2, BULB[(bi + (int)(G.anim_t*6))&3]); bi++; disc_fill(bx, by1+3, 2, BULB[(bi + (int)(G.anim_t*6))&3]); bi++; }
        for (int by=by0+6; by<by1; by+=11){ disc_fill(bx0-4, by, 2, BULB[(bi + (int)(G.anim_t*6))&3]); bi++; disc_fill(bx1+4, by, 2, BULB[(bi + (int)(G.anim_t*6))&3]); bi++; }
    }
    if (!MENU_SIMPLE){
    draw_text(VW/2+34, fy+3, "HOST IP:", 1, G.menu_sel==n+1? COL(255,205,40):COL(220,225,235));
    rect_fill(VW/2+92, fy, 130, 13, G.menu_sel==n+1? COL(70,80,110):COL(44,46,66));
    rect_line(VW/2+92, fy, 130, 13, G.menu_sel==n+1? COL(255,205,40):COL(140,150,180));
    { char b[52]; char *q=b; q=sappend(q, G.join_ip[0]? G.join_ip : "192.168.1.X");
      if (G.menu_sel==n+1 && caret) q=sappend(q, "_");
      draw_text(VW/2+96, fy+3, b, 1, COL(255,255,255)); }
    }
    // hosting builds: tell the player how friends join — they just open
    // this machine's address in a browser. No install, no mode to pick.
    { const char *ja = plat_join_addr();
      if (ja && ja[0]){
          char b[96]; char *q=b;
          q=sappend(q, "http://"); q=sappend(q, ja);
          draw_text_center(VW/2, fy+24, "PLAY WITH FRIENDS - THEY JUST OPEN THIS IN A BROWSER:", 1, COL(150,160,185));
          draw_text_center(VW/2, fy+38, b, 2, COL(255,214,64));
      } }
    // records strip
    { char b[128]; char *q=b;
      q=sappend(q, "ALL-TIME HEAVIEST HAT: "); q=sappend(q, G.rec_name); q=sappend(q, " - ");
      q=sapp_int(q, (int)G.rec_hat); q=sappend(q, " KG  |  SQUASHED: ");
      q=sapp_int(q, G.rec_kills); q=sappend(q, "  |  BEST BUG BOWL: "); q=sapp_int(q, G.rec_bowl);
      draw_text_center(VW/2, VH-30, b, 1, COL(170,190,220)); }
    // (no standing controls line under the menu any more, Daniel,
    // Nov 6: the bottom of the screen stays clean — the pause menu's
    // CONTROLS panel is the reference, and menus work the same
    // everywhere: move, select, tap.)
    if (G.store_msg_t>0) draw_text_center(VW/2, 68, G.store_msg, 1, COL(255,130,130));
    (void)menu_items;
}
static void render_slots(void){
    draw_demo_scene();
    rect_fill(0, 0, VW, 96, COL(20,18,36));
    rect_fill(0, 94, VW, 2, COL(255,205,40));
    draw_text_center(VW/2+2, 16, "THE SURVIVAL GAME", 2, COL(12, 10, 26));   // v1.22: shadow, matching the title
    draw_text_center(VW/2, 14, "THE SURVIVAL GAME", 2, COL(255,214,64));
    draw_text_center(VW/2, 44, G.slots_mode==0? "LOAD A SAVE" : "NEW GAME - PICK A SLOT", 2, COL(235,238,245));
    for (int i=0;i<3;i++){
        int s=i+1, y=116+i*52, x=100, w=440;
        int sel = (G.slots_sel==i);
        ui_panel(x, y, w, 44);   // v1.22: slot cards wear the shared panel chrome
        if (sel){ blend_rect(x, y, w, 44, COL(255,205,40), 36);
                  rect_line(x, y, w, 44, COL(255,205,40)); rect_line(x+1, y+1, w-2, 44-2, COL(255,205,40)); }
        { char b[40]; char *q=b; q=sappend(q, "SLOT "); q=sapp_int(q, s);
          if (G.slot_has[s] && pk_name[s][0]){ q=sappend(q, " - "); q=sappend(q, pk_name[s]); }
          *q=0;
          draw_text(x+16, y+7, b, 2, sel? COL(255,214,64) : COL(220,225,235)); }
        if (G.slots_confirm==s && G.slots_mode==1){
            draw_text(x+16, y+28, "START FRESH HERE? THE OLD SAVE GOES AWAY.", 1, COL(255,120,120));
        } else if (!G.slot_has[s]){
            draw_text(x+16, y+28, "EMPTY", 1, COL(130,138,160));
        } else {
            char b[96]; char *q=b;
            q=sappend(q, pk_name[s]);
            if (pk_bname[s][0]){ q=sappend(q, " + "); q=sappend(q, pk_bname[s]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[pk_bspecies[s]%BUDN]); }
            if (pk_hat[s]>0){ q=sappend(q, " - HAT "); q=sapp_int(q, pk_hat[s]); q=sappend(q, " KG"); }
            q=sappend(q, " - "); q=sapp_int(q, pk_kills[s]); q=sappend(q, " SQUASHED - ");
            q=sapp_int(q, pk_time[s]/60); q=sappend(q, ":");
            if (pk_time[s]%60<10) q=sappend(q, "0"); q=sapp_int(q, pk_time[s]%60);
            if (pk_zen[s]) q=sappend(q, " - ZEN");
            draw_text(x+16, y+28, b, 1, COL(200,210,228));
        }
    }
    { int y=278;
      if (G.slots_sel==3){ rect_fill(0, y-4, VW, 19, COL(255,205,40)); draw_text_center(VW/2, y, "BACK", 2, COL(30,26,44)); }
      else draw_text_center(VW/2, y, "BACK", 2, COL(235,238,245)); }
    { char fb[80]; char *q=fb;   // v1.30 (audit F14): name the real FIRE bind, not a hard-coded SPACE
      if (hint_dev()==DEV_PAD) q=sappend(q, "STICK/DPAD CHOOSE - A OPENS");
      else if (hint_dev()==DEV_TOUCH) q=sappend(q, "TAP A SLOT TO OPEN IT");
      else { q=sappend(q, "ARROWS CHOOSE - "); q=sappend(q, key_name(G.bind[ACT_FIRE])); q=sappend(q, " OPENS"); }
      *q=0; draw_text_center(VW/2, 306, fb, 1, COL(140,150,170)); }
    if (G.store_msg_t>0) draw_text_center(VW/2, 330, G.store_msg, 1, COL(255,130,130));
}
static void menu_input(void){
    // The selection walks the menu items AND the text fields below them
    // (row n = NAME, row n+1 = HOST IP, native only). On a field row,
    // typed letters go into the field; Space selects (it must not also
    // land in the field as a typed char). E is just a letter there.
    int n=menu_count();
    int rows=n+MENU_FIELDS;
    if (IN.menu_up){ G.menu_sel=(G.menu_sel+rows-1)%rows; }
    if (IN.menu_down){ G.menu_sel=(G.menu_sel+1)%rows; }
    // touch: tap an item to go straight to it, tap a field to select it
    if (IN.tapped){
        int fy=108+n*22+8;
        if (IN.tap_y>=fy-4 && IN.tap_y<=fy+17){
            G.menu_sel = (MENU_FIELDS==2 && IN.tap_x>=376)? n+1 : n;
        } else for (int i=0;i<n;i++){
            if (IN.tap_y>=108+i*22-6 && IN.tap_y<=108+i*22+16){ menu_activate(i); return; }
        }
    }
    if (G.menu_sel<n){
        if (IN.interact_pressed || IN.select_pressed) menu_activate(G.menu_sel);
    } else {
        if (IN.select_pressed){
            if (G.menu_sel==n){ if (MENU_FIELDS==2) G.menu_sel=n+1; }  // name -> ip
            else menu_activate(G.has_save? 3:2);                       // ip -> JOIN LAN GAME
        } else {
            if (G.menu_sel==n) field_append(G.my_name, 13);
            else field_append(G.join_ip, 48);
        }
    }
    G.edit_field = G.menu_sel==n? 1 : (G.menu_sel==n+1? 2 : 0);
}
static void move_key_str(char *b){
    // the four movement key names: concatenated when all are single
    // characters ("WASD"), slash-joined otherwise ("W/UP/S/DOWN" style).
    // key_name() returns a shared scratch buffer, so each name is copied
    // into the output before the next call.
    static const int idx[4] = { ACT_UP, ACT_LEFT, ACT_DOWN, ACT_RIGHT };
    int one=1;
    for (int i=0;i<4;i++) if (key_name(G.bind[idx[i]])[1]) one=0;
    char *q=b;
    for (int i=0;i<4;i++){ if (i && !one) q=sappend(q, "/"); q=sappend(q, key_name(G.bind[idx[i]])); }
    *q=0;
}
