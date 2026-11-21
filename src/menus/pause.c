// THE SURVIVAL GAME — menus/pause.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 2389-2587 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
static void draw_controls_panel(int vx, int vy, int vw2, int vh2, int ctx){
    static const char *CTX_NAME[9] = { "THE MEADOW + THE MALL", "STORY + FREE PLAY", "ENTRANCES", "SUPER BUDDY RUN", "BUDDY CRAFT", "BUG BOWL", "TANK RESCUE", "SUPER BUDDY CART", "WAR!" };
    if (ctx<0 || ctx>8) ctx = 0;
    const char *act[10]; const char *btn[10]; int n=0;
    static char mv[40], spr[8], pau[8], btnstore[10][24];
    if (hint_dev()==DEV_PAD) scpy(mv, "STICK / DPAD", 40);
    else if (hint_dev()==DEV_TOUCH) scpy(mv, G.touch_ctl? "LEFT STICK" : "DRAG", 40);
    else move_key_str(mv);
    scpy(spr, hint_dev()==DEV_PAD? "LT" : key_name(G.bind[ACT_SPRINT]), 8);
    scpy(pau, lbla(H_PAUSE), 8);
    const char *back = hint_dev()==DEV_PAD? "START" : (hint_dev()==DEV_TOUCH? "THE MALL DOOR" : "ESC");
    // ROW copies the button label NOW: key_name() returns a shared
    // static buffer, so storing its pointer across calls would leave
    // every row showing the LAST key looked up (INTERACT read as F).
#define CTL_ROW(a, b) do{ act[n]=(a); scpy(btnstore[n], (b), 24); btn[n]=btnstore[n]; n++; }while(0)
    switch (ctx){
      case 0:
        CTL_ROW("MOVE", mv);
        CTL_ROW("FIRE", lbl_fire());
        CTL_ROW("INTERACT", lbl_int());
        CTL_ROW("THROW A WALL", lbl_wall());
        CTL_ROW("WEAPONS", hint_dev()==DEV_PAD? "LB / RB" : (hint_dev()==DEV_TOUCH? "TAP THE CHIPS" : "1 - 5"));
        CTL_ROW("BACKFIRE BEAN", lbl_bean());
        if (hint_dev()!=DEV_TOUCH) CTL_ROW("SPRINT", spr);
        if (hint_dev()!=DEV_TOUCH) CTL_ROW("PAUSE MENU", pau);
        break;
      case 5:
        CTL_ROW("POSITION", mv);
        CTL_ROW("LOCK AIM, SET POWER, ROLL", lbl_fire());
        CTL_ROW("LEAVE THE LANE", lbl_int());
        break;
      case 1:
        CTL_ROW("MOVE", mv);
        CTL_ROW("CHOOSE / ATTACK", lbl_fire());
        CTL_ROW("TALK / USE", lbl_int());
        CTL_ROW("LOG + PACK", lbl_wall());
        CTL_ROW("LEAVE", back);
        break;
      case 2:
        CTL_ROW("MOVE", mv);
        CTL_ROW("THROW A STICK", lbl_fire());
        CTL_ROW("OPEN / UNLOCK", lbl_int());
        CTL_ROW("USE A PATCH NOTE", lbl_wall());
        CTL_ROW("LEAVE", back);
        break;
      case 3:
        CTL_ROW("RUN", mv);
        CTL_ROW("JUMP", lbl_fire());
        if (!mobile_ui()) CTL_ROW("STOMP (IN THE AIR)", "DOWN");
        CTL_ROW("LEAVE", back);
        break;
      case 6:
        CTL_ROW("DRIVE", mv);
        CTL_ROW("FIRE THE CANNON", lbl_fire());
        CTL_ROW("LEAVE", back);
        break;
      case 7:
        CTL_ROW("STEER", mv);
        CTL_ROW("GO (HOLD IT!)", lbl_fire());
        CTL_ROW("LEAVE", back);
        break;
      case 8:
        CTL_ROW("SELECT / ORDER", lbla(H_POINT));
        CTL_ROW("SELECT ALL OF A TYPE", hint_dev()==DEV_PAD? "A TWICE ON A UNIT" : (hint_dev()==DEV_TOUCH? "DOUBLE-TAP A UNIT" : "DOUBLE-CLICK A UNIT"));
        CTL_ROW("BUILD / TRAIN", hint_dev()==DEV_PAD? "A ON THE BOTTOM BAR" : (hint_dev()==DEV_TOUCH? "TAP THE BOTTOM BAR" : "CLICK THE BOTTOM BAR"));
        CTL_ROW("PAN THE CAMERA", hint_dev()==DEV_PAD? "STICK" : (hint_dev()==DEV_TOUCH? "DRAG" : "DRAG / ARROW KEYS"));
        CTL_ROW("LEAVE", back);
        break;
      default:
        CTL_ROW("MOVE", mv);
        CTL_ROW("SWING", lbl_fire());
        CTL_ROW("GATHER / REST", lbl_int());
        CTL_ROW("CRAFTING MENU", lbl_wall());
        CTL_ROW("IN THE MENU: PICK", mv);
        CTL_ROW("IN THE MENU: CRAFT", lbl_fire());
        CTL_ROW("LEAVE", back);
        break;
    }
#undef CTL_ROW
    int w = vw2<460? vw2-12 : 448; if (w<280) w=vw2-8;
    int h = 58 + n*19 + 24;
    int x = vx+(vw2-w)/2, y = vy+(vh2-h)/2; if (y<vy+2) y=vy+2;
    blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 120);
    rect_fill(x, y, w, h, COL(16,18,28)); rect_line(x, y, w, h, COL(198,164,84));
    draw_text_center(x+w/2, y+8, "CONTROLS", 2, COL(255,214,64));
    draw_text_center(x+w/2, y+27, CTX_NAME[ctx], 1, COL(150,170,190));
    for (int i=0;i<n;i++){
        int ry = y+44+i*19;
        draw_text(x+16, ry, act[i], 1, COL(228,232,238));
        draw_text(x+w-16-text_w(btn[i],1), ry, btn[i], 1, COL(255,220,90));
    }
    draw_text_center(x+w/2, y+h-15, pad_mode()? "B / START - BACK" : (mobile_ui()? "TAP ANYWHERE - BACK" : "ESC / SPACE - BACK"), 1, COL(140,150,170));
}

static void render_pause(void){
    render_world();
    blend_rect(0, 0, VW, VH, COL(10,8,20), 150);
    if (G.pause_ctl){
        Player *me=&G.p[G.my_id];
        draw_controls_panel(0, 0, VW, VH, (me->in_mall && me->station==4)? 5 : 0);
        return;
    }
    draw_text_center(VW/2+3, 104+3, "PAUSED", 4, COL(10,8,20));
    draw_text_center(VW/2, 104, "PAUSED", 4, COL(255,214,64));
    // WEEKLY CHALLENGE (v1.25 feature 6): the week's standing as a
    // plain line between the title and the rows — deliberately NOT
    // a row (First Shift's skip-row contract keeps indices sacred).
    if (G.wk_week!=0 && G.net_mode!=NET_CLIENT){
        char b[64]; char *q=b; q=sappend(q, "WEEKLY: ");
        int t,tg; wk_challenge(G.wk_week, &t, &tg);
        q=sappend(q, WK_NAME[t]);
        if (G.wk_done_week==G.wk_week) q=sappend(q, " - DONE!");
        else { q=sappend(q, " - "); q=sapp_int(q, wk_progress()); q=sappend(q, "/"); q=sapp_int(q, tg); }
        *q=0;
        draw_text_center(VW/2, 150, b, 1, COL(170,200,220));
    }
    // NAMED ELITES (v1.25 feature 8): while the named one is loose,
    // the pause menu says so — a state line under the weekly one,
    // never a row (the skip-row indices stay sacred here too)
    if (G.bn_live && G.net_mode!=NET_CLIENT){
        char bb[80]; char *q=bb;
        q=sappend(q, "BOUNTY: "); q=sappend(q, ELITE_NAME[bn_name_idx(G.bn_serial)]);
        q=sappend(q, " IS LOOSE IN ");
        q=sappend(q, (G.bn_live_reg>=0 && G.bn_live_reg<NREGIONS)? REGION_NAME[G.bn_live_reg] : "?"); *q=0;
        draw_text_center(VW/2, 163, bb, 1, COL(255,205,110));
    }
    { static const char *rows[4] = { "CONTINUE", "CONTROLS", "SAVE + QUIT TO TITLE", "SKIP FIRST SHIFT" };
      int nrows = G.fs_state==1? 4 : 3;   // v1.25: the skip row exists only while the First Shift chain is active
      for (int i=0;i<nrows;i++){
          int y=176+i*26;
          if (i==G.pause_sel){ rect_fill(0, y-4, VW, 19, COL(255,205,40)); draw_text_center(VW/2, y, rows[i], 2, COL(30,26,44)); }
          else draw_text_center(VW/2, y, rows[i], 2, COL(235,238,245));
      } }
    { char b[56]; char *q=b; q=sappend(q, "SAVES TO SLOT "); q=sapp_int(q, G.slot);
      q=sappend(q, " - YOUR HAT IS SAFE HERE.");
      draw_text_center(VW/2, 264, b, 1, COL(150,160,185)); }
    draw_text_center(VW/2, 282, pad_mode()? "STICK/DPAD + A - START ALSO RESUMES" : (mobile_ui()? "TAP A ROW - TAP SAVE + QUIT TWICE" : "ARROWS + SPACE - ESC RESUMES"), 1, COL(140,150,170));
}
static float over_stack[MAXSTACK];
static int over_stack_n=0;
static float over_carried=0;
static int over_have=0;
static void render_over(void){
    render_world();
    ui_panel(120, 60, 400, 220);   // v1.22: shared panel chrome under the red border
    rect_line(120, 60, 400, 220, COL(255,90,90));
    // you, as you fell — wearing the hat you died in (Daniel). Drawn
    // before the text so a tall hat never hides a word.
    if (over_have){
        static Player hero;
        Player *me=&G.p[G.my_id>=0? G.my_id : 0];
        smemset(&hero, 0, sizeof(hero));
        hero.connected=1; hero.alive=1; hero.name[0]=0;
        hero.color=me->color; hero.weapon=0; hero.owned[0]=1;
        hero.hp=hero.maxhp=100; hero.aim=-0.6f; hero.walk_ph=0;
        hero.whale_on=me->whale_on; hero.whale_own=me->whale_own;
        hero.top_worn=me->top_worn;   // HAT TOPPERS (feat 5): the portrait wears the trophy it died in, too
        hero.stack_n=over_stack_n;
        for (int i=0;i<over_stack_n;i++) hero.stack[i]=over_stack[i];
        hero.carried=over_carried;
        // stand him at screen (168, 252) whatever the camera is doing:
        // invert SX/SY at zoom 1 so the portrait is always full size
        float pz=G.zoom; G.zoom=1.0f;
        int vw2 = G.view_w>0? G.view_w : VW, vh2 = G.view_h>0? G.view_h : VH;
        hero.x = G.camx + (float)(168 - G.view_x0 - vw2/2);
        hero.y = G.camy + (float)(252 - G.view_y0 - vh2/2);
        draw_player_sprite(&hero);
        G.zoom=pz;
    }
    draw_text_center(VW/2, 78, "GAME OVER", 3, COL(255,110,110));
    draw_text_center(VW/2, 106, "THE BUGS GOT THE WHOLE TEAM", 1, COL(230,235,245));
    { char b[64]; int t=(int)G.time; char *q=b;
      q=sappend(q, "SURVIVED "); q=sapp_int(q, t/60); q=sappend(q, ":");
      if (t%60<10) q=sappend(q,"0"); q=sapp_int(q, t%60);
      q=sappend(q, " - SQUASHED "); q=sapp_int(q, G.total_kills);
      q=sappend(q, " - CASHED "); q=sapp_int(q, (int)G.total_earned); q=sappend(q, " KG");
      draw_text_fit(238, 128, 272, b, 1, COL(255,240,180)); }
    int y=152;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        char b[64]; char *q=b;
        q=sappend(q, G.p[i].name); q=sappend(q, " - HEAVIEST HAT ");
        q=sapp_int(q, (int)G.p[i].heaviest); q=sappend(q, " KG - ");
        q=sapp_int(q, G.p[i].kills); q=sappend(q, " KILLS");
        draw_text_fit(238, y, 272, b, 1, COL(210,220,235)); y+=14;
    }
    { char b[64]; char *q=b;
      q=sappend(q, "ALL-TIME BEST HAT: "); q=sappend(q, G.rec_name); q=sappend(q, " ");
      q=sapp_int(q, (int)G.rec_hat); q=sappend(q, " KG");
      draw_text_fit(238, y+8, 272, b, 1, COL(150,200,255)); }
    if (hint_dev()==DEV_TOUCH) draw_text_center(VW/2, 258, "TAP - BACK TO MENU", 1, COL(255,240,150));
    else { char b[48]; char *q=b; q=sappend(q, lbla(H_FIRE)); q=sappend(q, " - BACK TO MENU");
      draw_text_center(VW/2, 258, b, 1, COL(255,240,150)); }
#ifdef PLAT_WASM
    // (the share buttons under the canvas speak for themselves —
    // no standing line pointing at them; Daniel, Nov 6)
#endif
}

