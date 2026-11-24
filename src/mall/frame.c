// THE SURVIVAL GAME — mall/frame.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 2708-3399 in the Nov 2026 fine-grained refactor; code unchanged.

static void draw_weapon_icons(Player *p, int x, int y){
    for (int w=0; w<NWEAP; w++){
        if (!p->owned[w]) continue;
        int sx = x + w*26;
        rect_fill(sx, y, 22, 22, COL(16,14,28));
        rect_line(sx, y, 22, 22, p->weapon==w? COL(255,205,40) : COL(104,98,138));
        draw_item_icon(w, sx+11, y+11);
        if (p->owned[w]>1){ char b[8]; char *q=b; *q++='X'; q=sapp_int(q, p->owned[w]); *q=0;
            draw_text(sx+23-text_w(b,1), y+13, b, 1, COL(255,240,200)); }
    }
}
static void render_hud_compact(Player *p, int vx, int vy, int vw2, int vh2){
    // the split view's status card: what render_hud shows, in a card
    // that hugs the view's top-left corner — plus one context line,
    // what THIS player needs right now: safety in the mall, the threat
    // level out in the field.
    ui_panel(vx+6, vy+6, 236, 96);
    draw_text_fit(vx+12, vy+9, 218, p->name, 1, COL(255,255,255));
    draw_bar(vx+12, vy+20, 130, 7, p->maxhp? (float)p->hp/p->maxhp : 0, COL(120,230,90));
    { char b[24]; fmt_int(b, "HP ", p->hp, ""); draw_text(vx+148, vy+19, b, 1, COL(255,255,255)); }
    if (p->shield_max>0) draw_bar(vx+12, vy+29, 130, 4, p->shield_max? (float)p->shield/p->shield_max:0, COL(110,190,255));
    { char b[40]; char *q=b; q=sappend(q, WEAPON_NAME[p->weapon]);
      if (p->owned[p->weapon]>1){ q=sappend(q, " X"); q=sapp_int(q, p->owned[p->weapon]); }
      *q=0;
      draw_text_fit(vx+12, vy+36, 218, b, 1, COL(255,220,120)); }
    { char b[48]; char *q=b; q=sappend(q, "HAT "); q=sapp_int(q, (int)p->carried); q=sappend(q, " KG - ");
      q=sappend(q, hat_tier_name(p->stack_n, p->carried)); *q=0;
      draw_text_fit(vx+12, vy+47, 218, b, 1, COL(255,240,180)); }
    draw_weapon_icons(p, vx+12, vy+62);
    if (in_srcroom[p-G.p]) draw_text(vx+12, vy+82, "IN THE SOURCE ROOM - SAFE", 1, COL(150,255,190));
    else if (p->in_arcade) draw_text(vx+12, vy+82, "IN THE ARCADE - SAFE", 1, COL(150,230,255));
    else if (p->in_mall) draw_text(vx+12, vy+82, "IN THE MALL - SAFE", 1, COL(150,230,150));
    else { char b[32]; char *q=b; q=sappend(q, "THREAT X"); q=sapp_int(q, (int)G.threat);
           *q++='.'; *q++=(char)('0'+((int)(G.threat*10)%10)); *q=0;
           draw_text(vx+12, vy+82, b, 1, COL(255,150,90)); }
    if (!p->alive) draw_text_center(vx+vw2/2, vy+vh2/2-8, "DOWN", 2, COL(255,90,80));
    else { char pb[64]; const char *pr = interact_prompt(p, pb, 0);   // v1.25: each half names its own player's control (the view pass set the device context)
           if (pr) draw_text_center_fit(vx+vw2/2, vy+vh2-22, vw2-16, pr, 1, COL(255,240,180)); }
}
static void render_hud_mini(Player *p, int vx, int vy, int vw2){
    // a name-and-health strip for a view that is still peeling open:
    // the split is gradual now, and a sliver of screen can't hold the
    // full card yet
    int w = vw2-12; if (w>200) w=200; if (w<40) return;
    ui_panel(vx+6, vy+6, w, 28);
    draw_text_fit(vx+11, vy+9, w-10, p->name, 1, COL(255,255,255));
    draw_bar(vx+11, vy+21, w-22>10? w-22:10, 4, p->maxhp? (float)p->hp/p->maxhp : 0, COL(120,230,90));
}
static void render_p2_chip(void){
    Player *p=&G.p[G.p2_slot];
    ui_panel(VW-208, VH-74, 200, 66);
    draw_text_fit(VW-202, VH-68, 126, p->name, 1, COL(255,255,255));
    if (!p->alive) draw_text(VW-70, VH-68, "DOWN", 1, COL(255,90,80));
    draw_bar(VW-202, VH-57, 104, 6, p->maxhp? (float)p->hp/p->maxhp : 0, COL(120,230,90));
    { char b[24]; char *q=b; q=sappend(q, "HAT "); q=sapp_int(q, (int)p->carried); q=sappend(q, "KG"); *q=0;
      draw_text(VW-92, VH-58, b, 1, COL(255,240,180)); }
    draw_weapon_icons(p, VW-202, VH-46);
}
static void coop_render(float dt){
    Player *me=&G.p[G.my_id], *p2=&G.p[G.p2_slot];
    Player *tp[2]; tp[0]=me; tp[1]=p2;
    // one smoothed camera per player; a downed player's view follows
    // their partner so they can watch the rescue (or the end)
    for (int v=0; v<2; v++){
        Player *t = tp[v]->alive? tp[v] : tp[v^1];
        Buddy *tb = &G.bud[v? G.p2_slot : G.my_id];   // a BW view follows the buddy, not the DEV
        float txv = tb->in_bw? tb->x : t->x, tyv = tb->in_bw? tb->y : t->y;
        if (vzoom[v]<0.01f){ vcamx[v]=txv; vcamy[v]=tyv; vzoom[v]=1.0f; }
        float f = dt*6>1? 1 : dt*6;
        vcamx[v] += (txv - vcamx[v])*f;
        vcamy[v] += (tyv - vcamy[v])*f;
        vzoom[v] += ((tb->in_bw? 1.0f : zoom_target_of(t)) - vzoom[v])*0.045f;
    }
    // --- should the screen be split, and which way? -------------------
    // A panel open, or the players in different zones (one in the mall),
    // forces a full side-by-side split. Otherwise the distance decides:
    // they share a view while they fit in one, and the split's goal is
    // proportional to how far past fitting they have strayed.
    int forced = (lstore[0] || lstore[1] || (me->in_mall != p2->in_mall) || G.bud[G.my_id].in_bw || G.bud[G.p2_slot].in_bw);
    float z = vzoom[0]<vzoom[1]? vzoom[0] : vzoom[1];
    float dx = me->x - p2->x; if (dx<0) dx=-dx;
    float dy = me->y - p2->y; if (dy<0) dy=-dy;
    float fitx = (VW-110)/z, fity = (VH-110)/z;
    if (forced) { vsplit=1; vfit_t=0; }
    else if (dx <= fitx && dy <= fity){ vfit_t += dt; if (vfit_t>0.6f) vsplit=0; }
    else { vfit_t=0; vsplit=1; }
    // a death ends the split on the spot (Daniel): no easing-out —
    // one shared view of the rescue, right now
    if (!me->alive || !p2->alive){ vsplit=0; vsplit_f=0; }
    float goal = 0;
    if (vsplit){
        if (forced){ vorient=1; }
        else {
            int want_vert = dx >= dy;
            if (want_vert != vorient){
                // hysteresis: the other axis must clearly dominate
                // before the seam turns, so it never flip-flops
                if ((want_vert && dx > dy*1.35f) || (!want_vert && dy > dx*1.35f))
                    vorient = want_vert;
            }
        }
        goal = 1;   // always the full, even split — see the note above
        if (vsplit_f < 0.05f)   // a split is being born: remember the sides
            vp1first = vorient? (me->x <= p2->x) : (me->y <= p2->y);
    }
    // ease the amount toward its goal — quicker to open than to close
    if (vsplit_f < goal){ vsplit_f += dt*2.4f; if (vsplit_f>goal) vsplit_f=goal; }
    else if (vsplit_f > goal){ vsplit_f -= dt*1.6f; if (vsplit_f<goal) vsplit_f=goal; }

    if (vsplit_f <= 0.02f && !vsplit){
        // close together: one shared view centered between them
        clip_x0=0; clip_x1=VW; clip_y0=0; clip_y1=VH;
        G.view_x0=0; G.view_w=VW; G.view_y0=0; G.view_h=VH;
        G.camx=(vcamx[0]+vcamx[1])*0.5f; G.camy=(vcamy[0]+vcamy[1])*0.5f;
        G.zoom = vzoom[0]<vzoom[1]? vzoom[0] : vzoom[1];
        G.view_pi=G.my_id; G.cam_lock=1;
        g_dev_ctx = dev_of(G.my_id);
        render_world();
        G.cam_lock=0;
        render_hud();
        render_p2_chip();
        return;
    }
    // apart (or one is shopping): a view each. The seam sweeps in from
    // the edge the second view grows out of, and settles at the middle
    // only when the split is fully open.
    int rx[2], ry[2], rw[2], rh[2], seam;
    if (vorient){
        seam = vp1first? VW - (int)((VW/2)*vsplit_f) : (int)((VW/2)*vsplit_f);
        if (vp1first){ rx[0]=0; rw[0]=seam; rx[1]=seam; rw[1]=VW-seam; }
        else { rx[1]=0; rw[1]=seam; rx[0]=seam; rw[0]=VW-seam; }
        ry[0]=ry[1]=0; rh[0]=rh[1]=VH;
    } else {
        seam = vp1first? VH - (int)((VH/2)*vsplit_f) : (int)((VH/2)*vsplit_f);
        if (vp1first){ ry[0]=0; rh[0]=seam; ry[1]=seam; rh[1]=VH-seam; }
        else { ry[1]=0; rh[1]=seam; ry[0]=seam; rh[0]=VH-seam; }
        rx[0]=rx[1]=0; rw[0]=rw[1]=VW;
    }
    for (int v=0; v<2; v++){
        if (rw[v]<4 || rh[v]<4) continue;   // a hairline view draws nothing yet
        clip_x0=rx[v]; clip_x1=rx[v]+rw[v]; clip_y0=ry[v]; clip_y1=ry[v]+rh[v];
        G.view_x0=rx[v]; G.view_w=rw[v]; G.view_y0=ry[v]; G.view_h=rh[v];
        G.camx=vcamx[v]; G.camy=vcamy[v]; G.zoom=vzoom[v];
        G.view_pi = v? G.p2_slot : G.my_id; G.cam_lock=1;
        g_dev_ctx = dev_of(G.view_pi);   // v1.25: each split view labels its own player's device
        int bwv = G.bud[G.view_pi].in_bw;
        if (bwv) bw_render(G.view_pi); else render_world();
        G.cam_lock=0;
        if (bwv){ /* Banzai World draws its own card and hints */ }
        else if (lstore[v] && rw[v]>=316){
            G.store_pi = v? G.p2_slot : G.my_id;
            g_dev_ctx = dev_of(G.store_pi);
            G.store_station = lstation[v];
            G.ui_compact=1;
            draw_active_panel();
            G.ui_compact=0;
        } else if (rw[v]>=280 && rh[v]>=120) render_hud_compact(tp[v], rx[v], ry[v], rw[v], rh[v]);
        else render_hud_mini(tp[v], rx[v], ry[v], rw[v]);
        if (bwgate[G.view_pi]) bw_gate_draw(G.view_pi, rx[v], ry[v], rw[v], rh[v]);
        if (arcgate[G.view_pi]) arc_gate_draw(G.view_pi, rx[v], ry[v], rw[v], rh[v]);
        // every split view gets its own way-back arrow (hidden near home)
        if (!bwv){ Player *t = tp[v]->alive? tp[v] : tp[v^1];
          draw_home_arrow(t, rx[v] + rw[v]/2, ry[v] + rh[v]/2); }
    }
    clip_x0=0; clip_x1=VW; clip_y0=0; clip_y1=VH;
    G.view_x0=0; G.view_w=VW; G.view_y0=0; G.view_h=VH;
    // the seam (v1.24 chrome): a dark groove with a gold heart-line,
    // and a P1 / P2 chip in each player's hoodie colour at the end of
    // the seam their view sits on, so the halves are labelled
    { int p1v = vp1first? 0 : 1;   // which view index is P1's
      u32 c1 = HOODIE[G.p[G.my_id].color&3], c2 = HOODIE[G.p[G.p2_slot].color&3];
      if (vorient){
          rect_fill(seam-2, 0, 5, VH, COL(8,7,14));
          rect_fill(seam, 0, 1, VH, COL(198,164,84));
          int lx = p1v==0? seam-32 : seam+7, rx2 = p1v==0? seam+7 : seam-32;
          rect_fill(lx, 8, 25, 13, c1); rect_line(lx, 8, 25, 13, OUTLINE);
          draw_text_center(lx+12, 11, "P1", 1, COL(16,14,24));
          rect_fill(rx2, 8, 25, 13, c2); rect_line(rx2, 8, 25, 13, OUTLINE);
          draw_text_center(rx2+12, 11, "P2", 1, COL(16,14,24));
      } else {
          rect_fill(0, seam-2, VW, 5, COL(8,7,14));
          rect_fill(0, seam, VW, 1, COL(198,164,84));
          int ty1 = p1v==0? seam-21 : seam+8, ty2 = p1v==0? seam+8 : seam-21;
          rect_fill(8, ty1, 25, 13, c1); rect_line(8, ty1, 25, 13, OUTLINE);
          draw_text_center(20, ty1+3, "P1", 1, COL(16,14,24));
          rect_fill(8, ty2, 25, 13, c2); rect_line(8, ty2, 25, 13, OUTLINE);
          draw_text_center(20, ty2+3, "P2", 1, COL(16,14,24));
      } }
}
static void coop_frame(float dt){
    Player *me=&G.p[G.my_id];
    Player *p2=&G.p[G.p2_slot];
    // who was in Banzai World when this frame BEGAN: a Start/Esc that
    // leaves BW mid-frame must not also read as a pause press below
    int p1bw0 = G.bud[G.my_id].in_bw, p2bw0 = G.bud[G.p2_slot].in_bw;
    if (G.net_mode==NET_HOST)   // LAN guests still take their net input
        for (int i=1;i<MAXP;i++) if (G.p[i].connected && i!=G.p2_slot) G.p[i].cur_keys = G.p[i].net_keys;
    int p2up=0, p2down=0; p2_store_nav(&p2up, &p2down, dt);
    // P1 at the gate panel: it owns P1's menu input while open
    if (bwgate[G.my_id]){
        me->cur_keys=0;
        if (IN.menu_up){ bwgate_sel[G.my_id] = (bwgate_sel[G.my_id]+2)%3; sfx(6); }
        if (IN.menu_down){ bwgate_sel[G.my_id] = (bwgate_sel[G.my_id]+1)%3; sfx(6); }
        if (IN.tapped){ int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2); bw_gate_tap(G.my_id, vx, vy, vw2, vh2); }
        else if (IN.select_pressed || IN.enter_pressed) bw_gate_pick(G.my_id);
        else if (IN.esc_pressed || IN.pause_pressed){ bwgate[G.my_id]=0; sfx(6); }
    }
    // P1 at a cabinet panel: same deal (three rows at the unlocked CASTLE panel — feat 7)
    if (arcgate[G.my_id]){
        me->cur_keys=0;
        if (IN.menu_up){ arcgate_sel[G.my_id] = (arcgate_sel[G.my_id]+arc_gate_rows(G.my_id)-1)%arc_gate_rows(G.my_id); sfx(6); }
        if (IN.menu_down){ arcgate_sel[G.my_id] = (arcgate_sel[G.my_id]+1)%arc_gate_rows(G.my_id); sfx(6); }
        if (IN.tapped){ int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2); arc_gate_tap(G.my_id, vx, vy, vw2, vh2); }
        else if (IN.select_pressed || IN.enter_pressed) arc_gate_pick(G.my_id);
        else if (IN.esc_pressed || IN.pause_pressed){ arcgate[G.my_id]=0; sfx(6); }
    }
    // P1 in Banzai World: P1's input drives the buddy in its own world
    else if (G.bud[G.my_id].in_bw){
        me->cur_keys=0;
        if (IN.tapped){ int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2); bw_battle_tap(G.my_id, vx, vy, vw2, vh2); }
        BwIn bin; bw_fill_p1(&bin, dt);
        bw_update(G.my_id, dt, &bin);
    }
    // P1's store (keyboard / pad 1)
    else if (bwgate[G.my_id]){ me->cur_keys=0; }
    else if (lstore[0]){
        me->cur_keys=0;
        station_input(G.my_id, lstation[0], IN.menu_up, IN.menu_down, IN.select_pressed);
        if (IN.esc_pressed || IN.interact_pressed || G.tap_close_store){
            // the till is counting YOUR carcasses: the panel stays
            // open until it finishes (Daniel, Nov 6)
            if (pay_active && pay_player==G.my_id){ store_note("THE TILL IS COUNTING - ONE MOMENT"); sfx(16); G.tap_close_store=0; }
            else if (lstation[0]==9 && srcterm_blocks_close(G.my_id)){ G.tap_close_store=0; }   // v1.26: the terminal's inner views own Esc/Interact
            else { lstore[0]=0; me->in_store=0; me->station=-1; G.tap_close_store=0; bud_stage[G.my_id]=0; booth_stage[G.my_id]=0; srcterm_on_close(G.my_id); }
        }
    } else if (IN.interact_pressed && me->alive && downed_near(me)){
        try_revive(G.my_id);
    } else if ((IN.interact_pressed || IN.enter_pressed) && me->alive){
        if (!me->in_mall && near_store(me)){ mall_enter(G.my_id); }
        else if (me->in_mall){
            int st=station_at(me);
            if (st>=0){ lstore[0]=1; lstation[0]=st; G.store_sel=0; me->station=st; me->in_store=1; me->cur_keys=0; sfx(19); }
            else if (src_door_at(me)){ if (src_gate_open(G.my_id)) srcroom_enter(G.my_id); else src_door_sealed(G.my_id); }
            else if (srcroom_exit_at(me)){ srcroom_exit(G.my_id); }
            else if (arcade_door_at(me)){ arcade_enter(G.my_id); }
            else if (bw_door_at(me)){ bw_gate_try(G.my_id); }
            else if (arcade_exit_at(me)){ arcade_exit(G.my_id); }
            else if (cabinet_at(me)>=0){ arc_cab[G.my_id]=cabinet_at(me); arc_gate_try(G.my_id); }
            else if (on_exit_mat(me)){ mall_exit(G.my_id); }
        }
    }
    // P2 at the gate panel
    if (bwgate[G.p2_slot]){
        p2->cur_keys=0;
        if (p2up){ bwgate_sel[G.p2_slot] = (bwgate_sel[G.p2_slot]+2)%3; sfx(6); }
        if (p2down){ bwgate_sel[G.p2_slot] = (bwgate_sel[G.p2_slot]+1)%3; sfx(6); }
        if (act2_edge(ACT_FIRE)) bw_gate_pick(G.p2_slot);
        else if (act2_edge(ACT_INTERACT) || act2_edge(ACT_ESC)){ bwgate[G.p2_slot]=0; sfx(6); }
    }
    // P2 at a cabinet panel
    if (arcgate[G.p2_slot]){
        p2->cur_keys=0;
        if (p2up){ arcgate_sel[G.p2_slot] = (arcgate_sel[G.p2_slot]+arc_gate_rows(G.p2_slot)-1)%arc_gate_rows(G.p2_slot); sfx(6); }
        if (p2down){ arcgate_sel[G.p2_slot] = (arcgate_sel[G.p2_slot]+1)%arc_gate_rows(G.p2_slot); sfx(6); }
        if (act2_edge(ACT_FIRE)) arc_gate_pick(G.p2_slot);
        else if (act2_edge(ACT_INTERACT) || act2_edge(ACT_ESC)){ arcgate[G.p2_slot]=0; sfx(6); }
    }
    // P2 in Banzai World: pad 2 drives P2's buddy in its own half
    else if (G.bud[G.p2_slot].in_bw){
        resolve_p2_input();
        BwIn bin; bin.keys = p2->cur_keys;
        bin.interact = act2_edge(ACT_INTERACT);
        bin.exit_ = act2_edge(ACT_ESC);
        bin.pause = act2_edge(ACT_PAUSE);
        bin.sel = act2_edge(ACT_FIRE);
        bin.up = p2up; bin.down = p2down;
        bin.wall = act2_edge(ACT_WALL);
        bin.sel_held = ((IN.act2_held>>ACT_FIRE)&1) || ((p2->cur_keys & K_UP) && G.bud[G.p2_slot].bw_mode==3);   // the K_UP fold is the platformer's alone: steering never fires (see bw_fill_p1)
        bw_update(G.p2_slot, dt, &bin);
        p2->cur_keys=0;   // P2's DEV stands at the gate meanwhile
    }
    // P2's store (pad 2)
    else if (bwgate[G.p2_slot]){ p2->cur_keys=0; }
    else if (lstore[1]){
        p2->cur_keys=0;
        station_input(G.p2_slot, lstation[1], p2up, p2down, act2_edge(ACT_FIRE));
        if (act2_edge(ACT_INTERACT) || act2_edge(ACT_ESC)){
            // v1.30 (audit H9): the same srcterm guard P1 has — from an
            // inner terminal view, P2's close steps back a view instead
            // of dropping the whole panel.
            if (lstation[1]==9 && srcterm_blocks_close(G.p2_slot)){ /* the terminal's inner views own the close */ }
            else if (!(pay_active && pay_player==G.p2_slot)){ lstore[1]=0; p2->in_store=0; p2->station=-1; bud_stage[G.p2_slot]=0; booth_stage[G.p2_slot]=0; srcterm_on_close(G.p2_slot); }
        }
    } else if (act2_edge(ACT_INTERACT) && p2->alive && downed_near(p2)){
        try_revive(G.p2_slot);
    } else if (act2_edge(ACT_INTERACT) && p2->alive &&
               ((!p2->in_mall && near_store(p2)) || (p2->in_mall && (station_at(p2)>=0 || bw_door_at(p2) || on_exit_mat(p2) || arcade_door_at(p2) || arcade_exit_at(p2) || cabinet_at(p2)>=0 || src_door_at(p2) || srcroom_exit_at(p2))))){
        if (!p2->in_mall){ mall_enter(G.p2_slot); }
        else {
            int st=station_at(p2);
            if (st>=0){ lstore[1]=1; lstation[1]=st; G.store_sel2=0; p2->station=st; p2->in_store=1; p2->cur_keys=0; sfx(19); }
            else if (src_door_at(p2)){ if (src_gate_open(G.p2_slot)) srcroom_enter(G.p2_slot); else src_door_sealed(G.p2_slot); }
            else if (srcroom_exit_at(p2)){ srcroom_exit(G.p2_slot); }
            else if (arcade_door_at(p2)){ arcade_enter(G.p2_slot); }
            else if (bw_door_at(p2)){ bw_gate_try(G.p2_slot); }
            else if (arcade_exit_at(p2)){ arcade_exit(G.p2_slot); }
            else if (cabinet_at(p2)>=0){ arc_cab[G.p2_slot]=cabinet_at(p2); arc_gate_try(G.p2_slot); }
            else mall_exit(G.p2_slot);
        }
    } else resolve_p2_input();
    // pause: either player's Start — but never out from under a shopper
    if (!lstore[0] && !lstore[1] && G.state==GS_PLAY &&
        (G.net_mode==NET_OFF || (G.net_mode==NET_HOST && !any_remote_connected())) &&
        ((IN.pause_pressed && !p1bw0) || (act2_edge(ACT_PAUSE) && !p2bw0))){
        G.state=GS_PAUSE; me->cur_keys=0; p2->cur_keys=0; G.pause_sel=0; G.pause_ctl=0;
        render_pause();
        return;
    }
    update_sim(dt);
    if (G.shake>0){ G.shake -= dt*6; if (G.shake<0) G.shake=0; }
    if (G.state==GS_OVER) render_over();
    else coop_render(dt);
    if (G.net_mode==NET_HOST){
        net_send_t -= dt;
        if (net_send_t<=0){ net_send_t=1.0f/12; host_snapshot(); }
        wall_send_t -= dt;
        if (wall_send_t<=0){ wall_send_t=2.5f; host_send_walls(-1); }
        if (G.state==GS_OVER){
            int o=0; wr_u8(&o,'G'); plat_net_send(pkt, o, -1);
        }
    }
}
static void game_init(void){
    smemset(&G, 0, sizeof(G));
    scpy(G.my_name, "DEV", 13);
    scpy(G.rec_name, "NOBODY", 13);
    G.my_id=0;
    G.p2_slot=-1; G.view_w=VW; G.view_y0=0; G.view_h=VH;
    bind_defaults(G.bind);
    // COMFORT defaults (v1.25 feature 2): full volume, unmuted,
    // shake on — a save that carries the lines overrides these in
    // the load below; a save from before the feature keeps them.
    G.vol_music=10; G.vol_sfx=10; G.mute_all=0; G.shake_on=1;
    livedata_boot();   // LIVE TUNING (v1.26 Item 3): defaults are the static
                       // initializers; persisted overrides reapply here,
                       // after every table exists. Never fails the boot.
    load_records_and_check_save();
    if (G.slot<1 || G.slot>3) G.slot=1;
    probe_slots();
    G.state=GS_MENU;
}
// Turn the platform's physical-key report (+ gamepad actions) into the
// semantic input fields, through the player's bindings. Arrow keys are
// permanent alternates for movement and the only keyboard menu keys —
// that keeps typing in text fields from steering menus.
static void derive_input(float dt){
    // one-frame taps (the touch X button) must edge exactly once and
    // never latch into the held mask: a latched bit only edges the
    // first time, which made the X button work exactly once per load
    u64 pulse = IN.pulse; IN.pulse = 0;
    g_raw_edges = IN.edges_in | pulse | (IN.raw & ~prev_raw);
    IN.edges_in = 0;
    // last-device stamps (see pad_mode / dev_local): P1's pad actions —
    // held or edged — count as gamepad use; any physical key OR the
    // mouse counts as keyboard-class use (a desktop player is a
    // keyboard+mouse player); touch entry points stamp touch_last in
    // platform.c. P2's pad (act2) deliberately does NOT stamp here:
    // P2's device is structurally the gamepad (see dev_of), and its
    // presses must not flip P1's labels on a split screen. (Until
    // v1.25 the key stamp also fired on the sticky touch_seen flag,
    // so one touch made the keyboard stamp "now" forever and the pad
    // could never win again on a touch-capable machine.)
    IN.dev_t += dt;
    if (IN.act_held || IN.act_edges) IN.pad_last = IN.dev_t;
    { float m=0; for (int i=0;i<4;i++){ float a=IN.pad_ax[i]; if (a<0) a=-a; if (a>m) m=a; }
      if (m>0.18f) IN.pad_last = IN.dev_t; }   // a stick pushed past the cursor deadzone is pad use too, even below the digital threshold
    if (IN.raw || g_raw_edges || IN.mdown || IN.mclicked) IN.key_last = IN.dev_t;
    int k=0;
    if (raw_dn(G.bind[ACT_UP])    || raw_dn(PK_UP)    || (IN.act_held>>ACT_UP&1))    k|=K_UP;
    if (raw_dn(G.bind[ACT_DOWN])  || raw_dn(PK_DOWN)  || (IN.act_held>>ACT_DOWN&1))  k|=K_DOWN;
    if (raw_dn(G.bind[ACT_LEFT])  || raw_dn(PK_LEFT)  || (IN.act_held>>ACT_LEFT&1))  k|=K_LEFT;
    if (raw_dn(G.bind[ACT_RIGHT]) || raw_dn(PK_RIGHT) || (IN.act_held>>ACT_RIGHT&1)) k|=K_RIGHT;
    if (raw_dn(G.bind[ACT_FIRE])   || (IN.act_held>>ACT_FIRE&1))   k|=K_FIRE;
    if (raw_dn(G.bind[ACT_SPRINT]) || (IN.act_held>>ACT_SPRINT&1)) k|=K_SPRINT;
    IN.keys = k;
    G.key_edges = k & ~prev_sem; prev_sem = k;
    IN.enter_pressed    = raw_edge(PK_ENTER) || (int)((IN.act_edges>>ACT_ENTER)&1);
    IN.select_pressed   = ((G.key_edges & K_FIRE)!=0) || raw_edge(G.bind[ACT_FIRE])
                          || (int)((IN.act_edges>>ACT_FIRE)&1) || IN.enter_pressed;
    IN.esc_pressed      = raw_edge(PK_ESC)   || (int)((IN.act_edges>>ACT_ESC)&1);
    IN.interact_pressed = raw_edge(G.bind[ACT_INTERACT]) || (int)((IN.act_edges>>ACT_INTERACT)&1);
    IN.wall_pressed     = raw_edge(G.bind[ACT_WALL])     || (int)((IN.act_edges>>ACT_WALL)&1);
    IN.pause_pressed    = raw_edge(G.bind[ACT_PAUSE])    || (int)((IN.act_edges>>ACT_PAUSE)&1);
    IN.backspace        = raw_edge(PK_BACKSPACE);
    IN.weapon_key = 0;
    for (int w=0;w<5;w++)
        if (raw_edge(G.bind[ACT_W1+w]) || ((IN.act_edges>>(ACT_W1+w))&1)) IN.weapon_key=w+1;
    IN.bean_key = raw_edge(26+6) || ((IN.act_edges>>ACT_BEAN)&1);   // the 6 key (digits are PK 26..35) or the pad's Y button
    IN.weapon_cycle = ((IN.act_edges>>ACT_WNEXT)&1)? 1 : ((IN.act_edges>>ACT_WPREV)&1)? -1 : 0;
    // menu navigation: arrow edges, synthesized auto-repeat while held
    // (identical repeat timing in every build), gamepad edges.
    // The initial delay is 0.50s — the standard key-repeat delay, and
    // it must stay comfortably above a deliberate tap's PERCEIVED
    // hold: the web shell's keyup latch adds a 120ms tail, so a
    // 250ms tap's nominal perceived hold was 370ms — only ~10ms
    // under the old 0.38s delay, and the latch's free-running
    // release timer slipped far past it under load (probe-proven in
    // the v1.25 E2E pass: the repeat fired up to 200ms after the
    // physical keyup, stepping menus twice). The shell's latch tail
    // is now frame-gated (overshoot at most one frame past the
    // 120ms tail), and 0.50s keeps a wide margin on every backend;
    // the 0.09s repeat cadence after the first repeat is unchanged.
    IN.menu_up=0; IN.menu_down=0;
    if (raw_edge(PK_UP) || ((IN.act_edges>>ACT_UP)&1)){ IN.menu_up=1; menu_rep_dir=-1; menu_rep_t=0.50f; }
    else if (raw_edge(PK_DOWN) || ((IN.act_edges>>ACT_DOWN)&1)){ IN.menu_down=1; menu_rep_dir=1; menu_rep_t=0.50f; }
    else if (menu_rep_dir!=0 && (raw_dn(PK_UP) || raw_dn(PK_DOWN))){
        menu_rep_t -= dt;
        if (menu_rep_t<=0){
            if (menu_rep_dir<0 && raw_dn(PK_UP)) IN.menu_up=1;
            if (menu_rep_dir>0 && raw_dn(PK_DOWN)) IN.menu_down=1;
            menu_rep_t=0.09f;
        }
    } else menu_rep_dir=0;
    prev_raw = IN.raw;
    IN.pad_edges = IN.act_edges;   // a snapshot for late readers (WAR!'s pad cursor) — act_edges itself is consumed below
    IN.pad2_edges = IN.act2_edges;   // the same snapshot for pad 2 (v1.25 feat 13)
    IN.act_edges = 0;   // consumed
}

// ---------------------------------------------------------------------------
// Game-over portrait: the hat you died in (Daniel). Death crashes the
// stack to zero, so while the local player lives we keep a running
// snapshot of the hat — the game-over screen wears it, and so does the
// share card. (The statics live up by render_over; this is the keeper.)
// ---------------------------------------------------------------------------
static void over_refresh(void){
    if (G.state!=GS_PLAY) return;
    Player *me=&G.p[G.my_id>=0? G.my_id : 0];
    if (!me->connected || !me->alive) return;
    over_stack_n = me->stack_n<MAXSTACK? me->stack_n : MAXSTACK;
    for (int i=0;i<over_stack_n;i++) over_stack[i]=me->stack[i];
    over_carried = me->carried;
    over_have = 1;
}

static double last_now=0; static int booted=0;
void frame(void){
    double now = plat_now();
    float dt = booted? (float)(now-last_now) : 0.016f;
    last_now = now;
    if (dt>0.05f) dt=0.05f; if (dt<0) dt=0;
    if (!booted){ booted=1; game_init(); }
    if (want_autostart){
        want_autostart = 0;
        if (G.state==GS_MENU && G.net_mode==NET_OFF) start_run(NET_CLIENT);
    }
    G.anim_t += dt;
    if (G.store_msg_t>0) G.store_msg_t--;
    derive_input(dt);
    g_dev_ctx = -1;   // v1.25: hint labels default to the local player's device each frame
    G.view_pi = G.my_id>=0? G.my_id : 0;   // split passes override per view
    touch_update(dt);
    // v1.30 (audit D2): on the DRAG scheme a tap IS the interact button in
    // the main game too — Banzai World always folded it (banzai/input.c),
    // the meadow/mall frame never did, so a DRAG player could walk into
    // the mall but open no station panel and never trigger the exit mat:
    // trapped, with SAVE & QUIT the only way out. The weapon chips and
    // the bean (consumed inside touch_update) keep first claim on their
    // taps; every other DRAG tap in play interacts (doors, station
    // discs, the exit mat). Mouse clicks ride the same channel, so a
    // desktop click now interacts under DRAG too. The fold stays out of
    // open panels (co-op P1's lstore and a client's store_open_local run
    // in GS_PLAY, where their own tap handling owns the tap and an
    // interact edge would read as "close"). It also stays out of Banzai
    // World and the gate panels: there a tap has its own meanings (the
    // WAR! map's SKIRMISH button, battle taps, gate rows) and BW's own
    // fill folds taps where they belong — an interact edge leaking in
    // confirmed the selected region instead of opening the skirmish
    // panel (skirmishpathtest caught the first version of this fold).
    if (G.touch_ctl==0 && G.state==GS_PLAY && IN.tapped && !lstore[0] &&
        !(G.net_mode==NET_CLIENT && store_open_local) &&
        !G.bud[G.my_id].in_bw && !arcgate[G.my_id] && !bwgate[G.my_id])
        IN.interact_pressed=1;
    update_audio(dt);
    whale_poll(dt);

    if (G.net_mode==NET_HOST) net_host_poll();
    if (G.net_mode==NET_CLIENT) net_client_poll();
    over_refresh();
    if (G.state!=GS_MENU) G.menu_pad=0;   // title-menu layout latch resets off-menu

    switch (G.state){
    case GS_MENU: menu_input(); render_menu(); break;
    case GS_SETTINGS: settings_input(); render_settings(); break;
    case GS_HELP:
        if (IN.esc_pressed) G.state=GS_MENU;
        else if (IN.interact_pressed || IN.select_pressed || IN.tapped){
            help_page++;   // three pages now (v1.25 MALL ENLARGEMENT): HOW TO PLAY -> THE MALL -> MORE -> menu
            if (help_page>2){ help_page=0; G.state=GS_MENU; }
        }
        render_help(); break;
    case GS_PAUSE:
        if (any_remote_connected()){ G.state=GS_PLAY; G.pause_ctl=0; }   // a guest arrived: action resumes
        else if (G.pause_ctl){
            if (IN.esc_pressed || IN.pause_pressed || IN.select_pressed || IN.interact_pressed || IN.tapped || act2_edge(ACT_PAUSE) || act2_edge(ACT_ESC)) G.pause_ctl=0;
        }
        else if (IN.esc_pressed || IN.pause_pressed || act2_edge(ACT_PAUSE) || act2_edge(ACT_ESC)) G.state=GS_PLAY;
        else {
            int nrows = G.fs_state==1? 4 : 3;   // v1.25: matches render_pause's rows
            if (IN.menu_up) G.pause_sel = (G.pause_sel+nrows-1)%nrows;
            if (IN.menu_down) G.pause_sel = (G.pause_sel+1)%nrows;
            if (IN.tapped){
                for (int i=0;i<nrows;i++){ int y=176+i*26;
                    if (IN.tap_y>=y-6 && IN.tap_y<=y+17){
                        if (i==2 && G.pause_sel!=2) G.pause_sel=2;    // first tap arms quit
                        else if (i==0) G.state=GS_PLAY;
                        else if (i==1) G.pause_ctl=1;
                        else if (i==3){ fs_skip(); G.state=GS_PLAY; }
                        else pause_save_quit();
                        break; } }
            }
            else if (IN.select_pressed || IN.interact_pressed){
                if (G.pause_sel==0) G.state=GS_PLAY;
                else if (G.pause_sel==1) G.pause_ctl=1;
                else if (G.pause_sel==3){ fs_skip(); G.state=GS_PLAY; }
                else pause_save_quit();
            }
        }
        render_pause();
        break;
    case GS_SLOTS: slots_input(); render_slots(); break;
    case GS_PLAY: case GS_STORE: {
        if (G.net_mode==NET_CLIENT){
            if (!client_joined){
                client_joined=1;
                int o=0; wr_u8(&o,'J');
                for (int k2=0;k2<12;k2++) wr_u8(&o, (u8)G.my_name[k2]);
                wr_u8(&o, (u8)G.whale_own); wr_u8(&o, (u8)G.whale_on);
                wr_u8(&o, (u8)(G.whale_own>>8)); wr_u8(&o, (u8)(G.whale_on>>8));
                plat_net_send(pkt, o, 0);
            }
            if (G.my_id<0){
                // connecting screen
                rect_fill(0,0,VW,VH,COL(24,22,38));
                draw_demo_scene();
                rect_fill(0, 140, VW, 60, COL(20,18,36));
                draw_text_center(VW/2, 158, "CONNECTING TO HOST...", 2, COL(255,214,64));
                draw_text_center(VW/2, 184, hint_dev()==DEV_PAD? "BACK - CANCEL" : (hint_dev()==DEV_TOUCH? "TAP - CANCEL" : "ESC - CANCEL"), 1, COL(190,200,220));
                if (IN.esc_pressed || IN.tapped){ plat_net_stop(); G.net_mode=NET_OFF; G.state=GS_MENU; }   // v1.30 (audit H3): the touch text says TAP - CANCEL
                break;
            }
            // client: send inputs, render the host's world
            Player *me=&G.p[G.my_id];
            me->is_local=1;
            resolve_local_input();
            if (store_open_local){
                station_input(G.my_id, client_station, IN.menu_up, IN.menu_down, IN.select_pressed);
                if (client_station==4) bowl_update(G.my_id, dt);   // guests bowl on their own screen
                G.store_station = client_station;
                render_store();
                if (IN.esc_pressed || (IN.interact_pressed && !(client_station==1 && whale_code_mode[G.my_id])) || G.tap_close_store){ store_open_local=0; G.tap_close_store=0; whale_code_mode[G.my_id]=0; client_tell_store(0); }
            } else {
                if ((IN.interact_pressed || IN.enter_pressed || (IN.tapped && !me->in_mall && tap_on_store())) && me->alive){
                    if (!me->in_mall && near_store(me)){ int o=0; wr_u8(&o,'E'); plat_net_send(pkt, o, 0); sfx(19); }
                    else if (me->in_mall){
                        int st = station_at(me);
                        if (st>=0){ store_open_local=1; client_station=st; G.store_sel=0; client_tell_store(1); sfx(19); }
                        else if (on_exit_mat(me)){ int o=0; wr_u8(&o,'E'); plat_net_send(pkt, o, 0); sfx(19); }
                    }
                }
                // particles/floaters still animate locally
                update_fx(dt);
                G.camx += (me->x - G.camx)*(dt*6>1?1:dt*6);
                G.camy += (me->y - G.camy)*(dt*6>1?1:dt*6);
                render_world(); render_hud();
            }
            net_send_t -= dt;
            if (net_send_t<=0){ net_send_t=1.0f/30; net_client_send_input(); }
            break;
        }
        // single player or host: full simulation
        {
            Player *me=&G.p[G.my_id];
            resolve_local_input();
            // local co-op: with the option on and two controllers in,
            // a press on pad 2 drops P2 into the run beside P1
            if (G.p2_slot<0 && G.coop_pref && G.net_mode!=NET_CLIENT && G.state==GS_PLAY && IN.pad_count>=2 && IN.act2_edges)
                coop_spawn_p2(me->x-30, me->y);
            if (G.p2_slot>=0 && G.net_mode!=NET_CLIENT){ coop_frame(dt); break; }
            // remote players take their input from the net
            if (G.net_mode==NET_HOST)
                for (int i=1;i<MAXP;i++) if (G.p[i].connected){
                    G.p[i].cur_keys = G.p[i].net_keys;
                }
            // pause whenever you're effectively solo (the sim simply
            // stops advancing): offline, or hosting with no guests yet
            if (G.state==GS_PLAY && !bwgate[G.my_id] && !arcgate[G.my_id] && !G.bud[G.my_id].in_bw && (G.net_mode==NET_OFF ||
                (G.net_mode==NET_HOST && !any_remote_connected())) &&
                (IN.esc_pressed || IN.pause_pressed)){
                G.state=GS_PAUSE; me->cur_keys=0; G.pause_sel=0; G.pause_ctl=0; render_pause(); break;
            }
            int gate_was = bwgate[G.my_id] || arcgate[G.my_id];
            if (G.state==GS_PLAY && bwgate[G.my_id]){
                me->cur_keys=0;
                if (IN.menu_up){ bwgate_sel[G.my_id] = (bwgate_sel[G.my_id]+2)%3; sfx(6); }
                if (IN.menu_down){ bwgate_sel[G.my_id] = (bwgate_sel[G.my_id]+1)%3; sfx(6); }
                if (IN.tapped) bw_gate_tap(G.my_id, 0, 0, VW, VH);
                else if (IN.select_pressed || IN.enter_pressed) bw_gate_pick(G.my_id);
                else if (IN.esc_pressed || IN.pause_pressed){ bwgate[G.my_id]=0; sfx(6); }
            }
            if (G.state==GS_PLAY && arcgate[G.my_id]){
                me->cur_keys=0;
                if (IN.menu_up){ arcgate_sel[G.my_id] = (arcgate_sel[G.my_id]+arc_gate_rows(G.my_id)-1)%arc_gate_rows(G.my_id); sfx(6); }
                if (IN.menu_down){ arcgate_sel[G.my_id] = (arcgate_sel[G.my_id]+1)%arc_gate_rows(G.my_id); sfx(6); }
                if (IN.tapped) arc_gate_tap(G.my_id, 0, 0, VW, VH);
                else if (IN.select_pressed || IN.enter_pressed) arc_gate_pick(G.my_id);
                else if (IN.esc_pressed || IN.pause_pressed){ arcgate[G.my_id]=0; sfx(6); }
            }
            if (G.state==GS_PLAY && G.bud[G.my_id].in_bw){
                // my buddy is off in Banzai World: my input drives it,
                // and my DEV just stands at the gate in the mall
                me->cur_keys=0;
                if (IN.tapped) bw_battle_tap(G.my_id, 0, 0, VW, VH);
                BwIn bin; bw_fill_p1(&bin, dt);
                bw_update(G.my_id, dt, &bin);
            }
            if (G.state==GS_PLAY && !G.bud[G.my_id].in_bw && !gate_was){
                if ((IN.interact_pressed || IN.enter_pressed || (IN.tapped && !me->in_mall && tap_on_store())) && me->alive){
                    if (IN.interact_pressed && downed_near(me)){ try_revive(G.my_id); }
                    else if (!me->in_mall && near_store(me)){ mall_enter(G.my_id); me->cur_keys=0; }
                    else if (me->in_mall){
                        int st = station_at(me);
                        if (st>=0){ G.state=GS_STORE; G.store_station=st; G.store_sel=0; me->station=st; me->in_store=1; me->cur_keys=0; sfx(19); }
                        else if (src_door_at(me)){ if (src_gate_open(G.my_id)) srcroom_enter(G.my_id); else src_door_sealed(G.my_id); me->cur_keys=0; }
                        else if (srcroom_exit_at(me)){ srcroom_exit(G.my_id); me->cur_keys=0; }
                        else if (arcade_door_at(me)){ arcade_enter(G.my_id); me->cur_keys=0; }
                        else if (bw_door_at(me)){ bw_gate_try(G.my_id); me->cur_keys=0; }
                        else if (arcade_exit_at(me)){ arcade_exit(G.my_id); me->cur_keys=0; }
                        else if (cabinet_at(me)>=0){ arc_cab[G.my_id]=cabinet_at(me); arc_gate_try(G.my_id); me->cur_keys=0; }
                        else if (on_exit_mat(me)){ mall_exit(G.my_id); me->cur_keys=0; }
                    }
                }
            } else if (G.state==GS_STORE) {
                // This was a bare else once: with the gate panel open
                // (or the buddy away in Banzai World) the frame fell
                // into station_input against the STALE G.store_station
                // — picking STORY MODE also withdrew the bank, and
                // taps landed on a phantom panel (Daniel, Nov 4:
                // "the menus got mixed up")
                me->cur_keys=0;
                station_input(G.my_id, G.store_station, IN.menu_up, IN.menu_down, IN.select_pressed);
                // While an unlock code is being typed, the Interact key
                // must not close the panel — codes are full of E's, and
                // the E that types the letter was also slamming the panel
                // shut mid-code (this is why a hand-typed code could
                // never be finished). Esc still backs out.
                if (IN.esc_pressed || (IN.interact_pressed && !(G.store_station==1 && whale_code_mode[G.my_id])) || G.tap_close_store){
                    if (pay_active && pay_player==G.my_id){ store_note("THE TILL IS COUNTING - ONE MOMENT"); sfx(16); G.tap_close_store=0; }
                    else if (G.store_station==9 && srcterm_blocks_close(G.my_id)){ G.tap_close_store=0; }   // v1.26: the terminal's inner views own Esc/Interact
                    else { G.state=GS_PLAY; me->in_store=0; me->station=-1; G.tap_close_store=0; bud_stage[G.my_id]=0; booth_stage[G.my_id]=0; whale_code_mode[G.my_id]=0; srcterm_on_close(G.my_id); }
                }
            }
            update_sim(dt);
            G.camx += (me->x - G.camx)*(dt*6>1?1:dt*6);
            G.camy += (me->y - G.camy)*(dt*6>1?1:dt*6);
            if (G.shake>0){ G.shake -= dt*6; if (G.shake<0) G.shake=0; }
            if (G.state==GS_STORE) render_store();
            else if (G.state==GS_OVER) render_over();
            else if (G.bud[G.my_id].in_bw) bw_render(G.my_id);
            else { render_world(); render_hud(); if (bwgate[G.my_id]) bw_gate_draw(G.my_id, 0, 0, VW, VH); if (arcgate[G.my_id]) arc_gate_draw(G.my_id, 0, 0, VW, VH); }
            if (G.net_mode==NET_HOST){
                net_send_t -= dt;
                if (net_send_t<=0){ net_send_t=1.0f/12; host_snapshot(); }
                wall_send_t -= dt;
                if (wall_send_t<=0){ wall_send_t=2.5f; host_send_walls(-1); }
                if (G.state==GS_OVER){ // tell the clients the run ended
                    int o=0; wr_u8(&o,'G'); plat_net_send(pkt, o, -1);
                }
            }
        }
        break; }
    case GS_OVER:
        G.gameover_t += dt;
        render_over();
        if ((IN.esc_pressed || IN.interact_pressed || IN.select_pressed || IN.tapped) && G.gameover_t>0.6f){
            if (G.net_mode!=NET_OFF){ plat_net_stop(); G.net_mode=NET_OFF; }
            for (int i=0;i<MAXP;i++) G.p[i].connected=0;
            G.p2_slot=-1; coop_reset_views();
            G.state=GS_MENU;
        }
        break;
    }
    // BUG BOWL announcements decay in real time for every connected
    // player, whether or not their lanes panel is open (v1.25 E2E
    // fix): bowl_update only ticks while a player stands at the
    // lanes, so a message set just before the panel closed used to
    // freeze mid-countdown and greet the next visit as a stale
    // "FRAME 1" over the lanes. The announcement's moment is 1.4-1.8s
    // of game time, wherever the player goes meanwhile.
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.bowl[i].msg_t>0) G.bowl[i].msg_t-=dt;
    fs_update(dt);   // v1.25: the First Shift chain polls here so solo, co-op and host all advance it exactly once per frame
    wk_update(dt);   // v1.25 feat 6: the weekly poll rides the same tail — rollover check, then completion
    bn_update(dt);   // v1.25 feat 8: the bounty poll rides the same tail — cooldown, claim settle, live scan
    // v1.24: MODE-TRANSITION FADE — a purely cosmetic veil at the
    // render tail. When the SCENE changes (menu <-> run, entering or
    // leaving the mall / arcade, a Banzai World mode switch), a dark
    // veil lifts over ~0.22s so the cut feels like a transition. It
    // only READS state and draws one blended rect: no input, no sim,
    // no saves are touched, so it cannot desync anything. Pause maps
    // onto the play scene so pausing never triggers it.
    { static int fade_sig = -1; static float fade_t = 0;
      int mi = G.my_id>=0? G.my_id : 0;
      Player *fme = &G.p[mi];
      int fst = G.state==GS_PAUSE? GS_PLAY : G.state;
      int fsig = fst*256 + (fme->in_mall? 64:0) + (fme->in_arcade? 128:0) + (in_srcroom[mi]? 32:0)
               + (G.bud[mi].in_bw? (G.bud[mi].bw_mode&15)+16 : 0);
      if (fade_sig>=0 && fsig!=fade_sig) fade_t = 0.22f;
      fade_sig = fsig;
      if (fade_t>0){ fade_t -= dt; if (fade_t<0) fade_t=0;
          int fa = (int)(165.0f*fade_t/0.22f);
          if (fa>0) blend_rect(0, 0, VW, VH, COL(6,5,12), fa); } }
    fs_draw_prompt();   // v1.25: the First Shift prompt bar + marker, over the veil, under nothing else
    wk_draw_flash();   // v1.25 feat 6: the weekly banner, stacked just above the First Shift bar
    bn_draw_hud();   // v1.25 feat 8: the bounty tracker + claim banner, stacked above the weekly flash
    // clear one-frame input edges
    IN.act2_edges=0;   // pad-2 edges live exactly one frame, like P1's
    IN.mclicked=0; IN.rclicked=0; IN.wall_pressed=0; IN.interact_pressed=0;
    IN.esc_pressed=0; IN.enter_pressed=0; IN.typed_n=0; IN.backspace=0; IN.weapon_key=0; IN.bean_key=0;
    IN.menu_up=0; IN.menu_down=0; IN.select_pressed=0; IN.pause_pressed=0; IN.tapped=0;
    G.tap_close_store=0;
}

