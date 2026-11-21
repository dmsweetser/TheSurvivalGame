// THE SURVIVAL GAME — banzai/gates.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 920-1047 in the Nov 2026 fine-grained refactor; code unchanged.

// --- enter / exit ------------------------------------------------------------
static void bw_gate_try(int pi){
    Player *p=&G.p[pi]; Buddy *b=&G.bud[pi];
    if (G.net_mode==NET_CLIENT) return;
    // split-screen co-op is welcome (you play in your own half); a LAN
    // session with guests connected is not — their screens have no BW
    if (G.net_mode==NET_HOST && any_remote_connected()){ spawn_floater(p->x, p->y-24, "BANZAI WORLD IS A SOLO TRIP", COL(255,200,150)); sfx(16); return; }
    if (!b->active){ spawn_floater(p->x, p->y-24, "YOU NEED A BANZAI BUDDY FIRST", COL(255,150,150)); sfx(16); return; }
    if (b->fainted){ spawn_floater(p->x, p->y-24, "YOUR BUDDY FAINTED - REVIVE IT FIRST", COL(255,150,150)); sfx(16); return; }
    bwgate[pi]=1; bwgate_sel[pi]=0; sfx(19);
}
static void bw_enter_mode(int pi, int mode){
    g_dev_ctx = dev_of(pi);   // v1.25: the entry note names the entering player's controls
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    if (G.net_mode==NET_CLIENT) return;
    if (!b->active || b->fainted) return;
    bwgate[pi]=0;
    S->from_arcade = bw_arc_src[pi]; S->arc_cab = bw_arc_cab[pi]; bw_arc_src[pi]=0;
    if (!b->bw_started){
        b->bw_started = 1;
        b->bw_seed = (int)(0x1000 + (rng_u32() % 60000));
        b->bw_x = 0; b->bw_y = 56;
    }
    b->bw_mode = mode;
    if (mode==2){ ent_reset(pi);
        // DEEPER DOORS (feat 11): the cabinet panel's chapter
        // request, consumed here and only here — ent_reset has
        // defaulted the run to Chapter 1, and no ent_rnd call
        // happens between the reset and this line
        S->ent_chap = bw_ent_ch2_req[pi]? 2 : 1; bw_ent_ch2_req[pi]=0; }
    if (mode==1 && !b->story_started){
        b->story_started = 1; bw_qset(b, 0, 1);   // quest 0: meet the elder
    }
    S->defeated_n = 0; S->bat.active = 0; S->sleep_t = 0; S->grace_t = 0; S->note_t = 0;
    // THE REBUILDING (feat 14): the chapter's clocks are per-trip —
    // the cider run parks (Patchina's door restarts it), the
    // festival ends at the gate, and Duster is re-posted at the
    // village for as long as its escort is still owed.
    S->ch2_race_on = 0; S->ch2_race_t = 0; S->ch2_fest_t = 0;
    S->ch2_esc_on = (mode==1 && bw_qstate3(b,11)==1)? 1 : 0;
    if (S->ch2_esc_on){ S->ch2_esc_x = CH2_DUSTER_X; S->ch2_esc_y = CH2_DUSTER_Y; }
    S->ch2_wp_on = 0; S->ch2_path_t = 0; S->ch2_stall = 0; S->ch2_base = 1e9f;
    for (int i=0;i<BW_ROAMN;i++) S->roam[i].alert = 0;   // a fresh entry is a fresh temper: no hunt survives the gate (alerts also freeze during battles, so a stale one could otherwise outlive its mode)
    S->log_open = 0; S->site_cur = -2; S->bw_menu = 0; S->bw_msel = 0;
    if (mode==2){ b->x = 3.5f*BW_TILE; b->y = ENT_DOORY;
        if (S->ent_chap==2){ S->ent_lux = b->x + 36; S->ent_luy = b->y - 30; } }   // DEEPER DOORS (feat 11): Lumen waits at the threshold
    else if (mode==3){ plat_enter(pi); }
    else if (mode==4){ craft_enter(pi); }
    else if (mode==5){ tank_enter(pi); }
    else if (mode==6){ S->cas_shuffle = bw_cas_shuffle_req[pi]; bw_cas_shuffle_req[pi]=0; cas_enter(pi); }
    else if (mode==7){ crt_enter(pi); }
    else if (mode==8){ war_enter(pi); }
    else { b->x = b->bw_x; b->y = b->bw_y; }
    b->in_bw = 1;   // the DEV stays parked at the gate in the mall
    if (mode==2 && S->ent_chap==2){ char *q=S->note; q=sappend(q, "DEEPER DOORS. LUMEN COMES WITH YOU. "); q=sappend(q, lbl_int()); q=sappend(q, " OPENS DOORS + DRAWERS."); *q=0; }
    else if (mode==2){ char *q=S->note; q=sappend(q, "ENTRANCES. A COZY HOTEL. "); q=sappend(q, lbl_int()); q=sappend(q, " OPENS DOORS + DRAWERS. MIND THE ZIP BUG."); *q=0; }
    else if (mode==3){ char *q=S->note; q=sappend(q, "LEVEL "); q=sapp_int(q, S->plat_lvl); q=sappend(q, " - "); q=sappend(q, REGION_NAME[S->plat_region]); q=sappend(q, ". "); q=sappend(q, lbl_fire()); q=sappend(q, " JUMPS. STOMP BUGS!"); *q=0; }
    else if (mode==4){ char *q=S->note; q=sappend(q, "BUDDY CRAFT. "); q=sappend(q, lbl_int()); q=sappend(q, " GATHERS + RESTS. "); q=sappend(q, lbl_fire()); q=sappend(q, " SWINGS. "); q=sappend(q, lbl_wall()); q=sappend(q, " CRAFTS."); *q=0; }
    else if (mode==5){ char *q=S->note; q=sappend(q, "TANK RESCUE. "); q=sappend(q, lbl_fire()); q=sappend(q, " FIRES THE CANNON. FREE CAPTIVES, DROP AT PADS."); *q=0; }
    else if (mode==6 && S->cas_shuffle){ char *q=S->note; q=sappend(q, "CASTLE SHUFFLE! THE TREASURES HAVE MOVED. FIND THE KEY, ESCAPE!"); *q=0; }
    else if (mode==6){ char *q=S->note; q=sappend(q, "CASTLE! RUN INTO THINGS TO TAKE OR FIGHT. FIND THE KEY, ESCAPE!"); *q=0; }
    else if (mode==7){ char *q=S->note; q=sappend(q, "THE WAITING ROOM. PICK YOUR COLOUR!"); *q=0; }
    else if (mode==8){ char *q=S->note; q=sappend(q, "WAR! "); q=sappend(q, hint_dev()==DEV_PAD? "PICK" : lbla(H_POINT)); q=sappend(q, " A REGION TO INVADE IT. CONQUER ALL EIGHT!"); *q=0; }
    else if (mode==1 && bw_chapter_done(b)) scpy(S->note, "PATCH DAY, EVERY DAY. THE VILLAGE REMEMBERS YOU.", 72);
    else if (mode==1 && bw_story_complete(b)) scpy(S->note, "BACK TO THE STORY. PATCHINA HAS A NEW LIST.", 72);
    else if (mode==1 && b->story_bits==(1<<0)) scpy(S->note, "STORY MODE. PATCH VILLAGE LIES NORTH-WEST.", 72);
    else if (mode==1) scpy(S->note, "BACK TO THE STORY. THE MEADOW REMEMBERS.", 72);
    else scpy(S->note, "BANZAI WORLD. WATCH FOR HACKERS.", 72);
    S->note_t = 3.4f;
    sfx(19);
}
// the gate panel: STORY MODE / FREE PLAY / LEAVE, drawn over a view
static void bw_gate_draw(int pi, int vx, int vy, int vw2, int vh2){
    Buddy *b=&G.bud[pi];
    int w = vw2<340? vw2-16 : 320, h = 150;
    int x = vx + (vw2-w)/2, y = vy + (vh2-h)/2;
    if (y < vy+4) y = vy+4;
    rect_fill(x, y, w, h, COL(18,22,20)); rect_line(x, y, w, h, COL(198,164,84));
    draw_text_center(x+w/2, y+10, "BANZAI WORLD", 2, COL(255,214,64));
    { char buf[64]; char *q=buf; q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[b->species%BUDN]); q=sappend(q, " IS ITCHING TO GO"); *q=0;
      draw_text_center(x+w/2, y+32, buf, 1, COL(200,214,200)); }
    // the gate is for the WORLD now (Nov 10): story and free play.
    // Entrances, Super Buddy Run and Buddy Craft moved to the Arcade.
    static const char *DESC[3] = {
        "A SASSY LITTLE ADVENTURE. QUESTS, PLACES, TROUBLE.",
        "THE ENDLESS MEADOW. ROAM, BONK, NAP.",
        "STAY IN THE MALL." };
    for (int r=0;r<3;r++){
        int ry = y + 52 + r*20;
        const char *nm = r==0? (b->story_started? "CONTINUE STORY" : "STORY MODE") : r==1? "FREE PLAY" : "LEAVE";
        if (bwgate_sel[pi]==r){ rect_fill(x+10, ry-3, w-20, 15, COL(70,58,30)); draw_text(x+16, ry, ">", 1, COL(255,220,90)); }
        draw_text(x+30, ry, nm, 1, bwgate_sel[pi]==r? COL(255,244,210) : COL(170,180,170));
    }
    { const char *dsc = DESC[bwgate_sel[pi]%3];
      // THE REBUILDING (feat 14): the gate keeps its shipped three
      // rows, but STORY MODE's description turns for a finished
      // story — the chapter advertises itself here, not on a new row
      if (bwgate_sel[pi]==0 && b->story_started && bw_story_complete(b))
          dsc = bw_chapter_done(b)? "PATCH DAY, EVERY DAY. THE VILLAGE STANDS TALL AGAIN."
                                  : "THE VILLAGE IS REBUILDING. PATCHINA HAS A NEW LIST.";
      draw_text_center(x+w/2, y+h-16, dsc, 1, COL(150,170,190)); }
    if (mobile_ui() && !pad_mode() && pi==G.my_id)
        draw_text_center(x+w/2, y+h-30, "TAP A MODE TO PLAY", 1, COL(120,132,150));
    else { char buf[64]; char *q=buf; q=sappend(q, lbl_fire()); q=sappend(q, " - GO IN    "); q=sappend(q, lbl_exit()); q=sappend(q, " - STAY"); *q=0;
      draw_text_center(x+w/2, y+h-30, buf, 1, COL(120,132,150)); }
}
static void bw_gate_pick(int pi){
    int s2 = bwgate_sel[pi];
    if (s2==0) bw_enter_mode(pi, 1);
    else if (s2==1) bw_enter_mode(pi, 0);
    else { bwgate[pi]=0; sfx(6); }
}
// touch parity for the gate panel (the store panels all take taps;
// this one never did, so on a phone the rows were decoration —
// Daniel, Nov 4). Tap a row to choose it; tap outside to back out.
// Geometry mirrors bw_gate_draw exactly.
static void bw_gate_tap(int pi, int vx, int vy, int vw2, int vh2){
    int w = vw2<340? vw2-16 : 320, h = 150;
    int x = vx + (vw2-w)/2, y = vy + (vh2-h)/2;
    if (y < vy+4) y = vy+4;
    IN.tapped = 0;
    if (IN.tap_x<x || IN.tap_x>=x+w || IN.tap_y<y || IN.tap_y>=y+h){ bwgate[pi]=0; sfx(6); return; }
    for (int r=0;r<3;r++){
        int ry = y + 52 + r*20;
        if (IN.tap_y>=ry-4 && IN.tap_y<=ry+15){ bwgate_sel[pi]=r; bw_gate_pick(pi); return; }
    }
}
