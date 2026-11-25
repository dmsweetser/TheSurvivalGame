// THE SURVIVAL GAME — menus/help.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 2096-2249 in the Nov 2026 fine-grained refactor; code unchanged.

static void render_help(void){
    rect_fill(0,0,VW,VH,COL(24,22,38));
    ui_panel(10, 8, VW-20, VH-16);   // v1.24: the shared panel chrome behind the pages
    draw_text_center(VW/2, 18, help_page==2? "MORE" : help_page==1? "THE MALL" : "HOW TO PLAY", 3, COL(255,214,64));
    if (help_page==1){
        // THE MALL (v1.25 MALL ENLARGEMENT): the old page 2 carried
        // 44 lines at 9 px from y=44 — its last line landed at
        // y~431, off the 360-tall panel, and BIG HATS + WALLS were
        // unreadable. The help is three pages now; this page ends
        // at y~305, clear of the footer.
        const char *lines2[] = {
            "DIFFICULTY: EASY IS CLASSIC; THE REST MAKE BUGS MEANER.",
            "",
            "THE OLD BUG BOUNTY HUT IS NOW THE BUG MALL - PRESS",
            "INTERACT AT THE HUT TO STEP INSIDE. TEN STOPS,",
            "ONE BUILDING: THE COUNTER SELLS EVERYTHING IT",
            "ALWAYS DID.",
            "",
            "WHALES ONLY SELLS PURE VANITY FOR REAL MONEY: GOLD",
            "CARCASSES, A CAPE, RAIN THAT FALLS ONLY ON YOU. NO",
            "GAMEPLAY EFFECT WHATSOEVER - JUST GLORY. $0.67 EACH.",
            "",
            "THE BUG BANK KEEPS CARCASSES SAFE - THEY WEIGH NOTHING,",
            "NEVER COUNT TO YOUR HEAVIEST HAT, MOVE IN FRACTIONS.",
            "",
            "THE TROPHY CASE PAST THE BANZAI GATE GATHERS EVERY",
            "PLAQUE, MEDAL AND RECORD YOU EARN. LOOK, DON'T TOUCH.",
            "",
            "THE CHALLENGE BOARD AT THE WEST END POSTS ONE WEEKLY",
            "CHALLENGE - THE SAME FOR EVERYBODY. FINISH IT FOR 50 XP.",
            "",
            "THE BOUNTY BOARD ON THE WEST WALL POSTS ONE NAME AT A",
            "TIME: A NAMED ELITE, THREE TIMES TOO BIG, THAT CRAWLS",
            "OUT WHEN IT PLEASES - MORE OFTEN FAR FROM HOME. THE",
            "BOARD SAYS WHERE IT WAS LAST SEEN, AND AN AMBER ARROW",
            "POINTS THE WAY WHILE IT IS LOOSE. BRING IT DOWN AND",
            "THE KILLER BANKS ITS WEIGHT AGAIN, WITH 25 XP FOR",
            "EVERY BUDDY. IF IT SLIPS AWAY, THE CONTRACT STANDS.",
            "",
            "BUG BOWL: THE MALL'S OWN LANES. ONE CARCASS A BALL,",
            "STRIKES, SPARES, TENTH FRAME - AND A HOUSE LEAGUE.",
        };
        for (int i=0;i<(int)(sizeof(lines2)/sizeof(lines2[0]));i++)
            draw_text_center(VW/2, 44+i*9, lines2[i], 1, COL(225,230,240));
        if (pad_mode()) draw_text_center(VW/2, VH-24, "B - PAGE 3: MORE", 1, COL(160,230,255));
        else if (mobile_ui()) draw_text_center(VW/2, VH-24, "TAP - PAGE 3: MORE", 1, COL(160,230,255));
        else { char b[48]; char *q=b; q=sappend(q, key_name(G.bind[ACT_INTERACT])); q=sappend(q, " - PAGE 3: MORE");
          draw_text_center(VW/2, VH-24, b, 1, COL(160,230,255)); }
        return;
    }
    if (help_page==2){
        const char *lines3[] = {
            "BANZAI BUDDIES: A PUPPY, KITTY, MOOSE, PLATYPUS, PET",
            "ROCK OR KITCHEN SINK FIGHTS AT YOUR SIDE, EARNING XP",
            "FROM KILLS AND ASSISTS TO SPEND ON TRAINING + MOVES.",
            "YOU PICK ITS NAME AND ITS COAT - NEON RAINBOW",
            "INCLUDED.",
            "AND THE SHOP REVIVES THEM - TRAINED ONES COST MORE.",
            "",
            "THE PHOTO BOOTH STANDS ALONE ON THE MALL'S WEST",
            "SIDE: YOU, YOUR BUDDY AND THE HAT, POSED AGAINST",
            "SIX BACKDROPS. SNAPS ARE FREE, THE PORTRAIT HANGS",
            "AT THE BOOTH, AND A COPY IS YOURS TO KEEP.",
            "",
            "BIG HATS ARE FRAGILE: ONCE YOU CARRY 50 KG OR MORE,",
            "EVERY BITE SHAKES A CARCASS LOOSE. DON'T GET GREEDY.",
            "",
            "WALLS LINK UP: RUNS OF 5+ SHARE ONE BIG HEALTH POOL",
            "AND YOU CAN CLIMB UP AND WALK ALONG THE TOP. BUT A",
            "WALL MAY NEVER CLOSE INTO A LOOP - THE LOOP MUST",
            "STAY OPEN.",
        };
        for (int i=0;i<(int)(sizeof(lines3)/sizeof(lines3[0]));i++)
            draw_text_center(VW/2, 44+i*9, lines3[i], 1, COL(225,230,240));
        if (pad_mode()) draw_text_center(VW/2, VH-24, "B - BACK TO MENU", 1, COL(160,230,255));
        else if (mobile_ui()) draw_text_center(VW/2, VH-24, "TAP - BACK TO MENU", 1, COL(160,230,255));
        else { char b[48]; char *q=b; q=sappend(q, key_name(G.bind[ACT_INTERACT])); q=sappend(q, " - BACK TO MENU");
          draw_text_center(VW/2, VH-24, b, 1, COL(160,230,255)); }
        return;
    }
    char mv[40], l0[96], l1[96], l2[96], l3[96], l4[96], l5[96], l6[96];
    scpy(l5, mobile_ui()? "CO-OP: A DESKTOP APP HOSTS - PHONES JOIN IN A BROWSER."
                        : "CO-OP: THE APP HOSTS - FRIENDS JOIN FROM A BROWSER.", 96);
    // v1.31 (audit P2/F17): the device block is six lines now — the
    // Backfire Bean is explained on EVERY variant (it never was), and
    // both touch variants explain weapon switching (WPN button and
    // the weapon chips). The page starts 4px higher to hold the
    // extra line clear of the footer.
    if (pad_mode()){
        scpy(l0, "STICK OR D-PAD - MOVE. YOU FACE THE WAY YOU MOVE,", 96);
        scpy(l1, "AND YOU FIRE THE WAY YOU FACE. A - FIRE + SELECT.", 96);
        scpy(l2, "LB / RB - WEAPONS - X WALL - LT SPRINT - START PAUSE", 96);
        scpy(l3, "B - ENTER AND LEAVE THE STORE. A PICKS, B BACKS OUT.", 96);
        scpy(l4, "TWO PADS PLUGGED IN? FLIP ON 2P CO-OP IN THE MENU.", 96);
        scpy(l6, "Y - BACKFIRE BEAN: SQUEEZE HIM, PBBBT - HOME IN A FLASH.", 96);
    } else if (mobile_ui()){
        scpy(l0, "PUSH YOUR LEFT THUMB TO STEER WITH THE", 96);
        scpy(l1, "STICK. YOU FIRE THE WAY YOU FACE.", 96);
        scpy(l2, "HOLD FIRE TO SHOOT. WPN - OR A TAP ON A WEAPON", 96);
        scpy(l3, "CHIP - SWITCHES WEAPONS. WALL BUILDS A WALL.", 96);
        scpy(l4, "BY THE STORE, TAP USE TO GO IN - OR TAP IT.", 96);
        scpy(l6, "TAP THE BEAN READOUT - PBBBT, STRAIGHT HOME.", 96);
    } else {
    move_key_str(mv);
    { char *q=l0; q=sappend(q, mv); q=sappend(q, " / ARROWS - MOVE. YOU FACE THE WAY YOU MOVE,"); }
    { char *q=l1; q=sappend(q, "AND YOU FIRE THE WAY YOU FACE. "); q=sappend(q, key_name(G.bind[ACT_FIRE])); q=sappend(q, " - FIRE + SELECT."); }
    { char *q=l2; q=sappend(q, key_name(G.bind[ACT_W1])); q=sappend(q, "-"); q=sappend(q, key_name(G.bind[ACT_W5]));
      q=sappend(q, " WEAPONS - "); q=sappend(q, key_name(G.bind[ACT_WALL])); q=sappend(q, " WALL - ");
      q=sappend(q, key_name(G.bind[ACT_SPRINT])); q=sappend(q, " SPRINT - "); q=sappend(q, key_name(G.bind[ACT_PAUSE])); q=sappend(q, " PAUSE (1P)"); }
    { char *q=l3; q=sappend(q, key_name(G.bind[ACT_INTERACT])); q=sappend(q, " - ENTER AND LEAVE THE STORE. REBIND KEYS IN SETTINGS."); }
    scpy(l4, "ON MOBILE: STICK TO MOVE, FIRE TO SHOOT. TAP SELECTS + BUYS.", 96);
    scpy(l6, "6 - BACKFIRE BEAN: SQUEEZE HIM, PBBBT - HOME IN A FLASH.", 96);
    }
    const char *lines[] = {
        l0, l1, l2, l3, l4, l6,
        "",
        "SQUASH BUGS: THEIR CARCASSES LAND ON YOUR HEAD.",
        "YOUR HAT IS YOUR WALLET - AND IT SLOWS YOU DOWN.",
        "HAUL IT TO THE BUG MALL AT THE EPICENTER AND PAY BY",
        "WEIGHT AT THE COUNTER FOR WEAPONS, SHIELDS + MORE.",
        "HURT? A HOTFIX PATCHES YOU UP: +50 HP, 20 KG A POP.",
        "",
        "WHILE ANYONE IS IN THE MALL, A SHIELD SEALS THE",
        "EPICENTER - BUGS CAN'T CROSS THE RING. OTHERWISE IT'S",
        "NOT SAFE: BUGS WANDER RIGHT IN. NEAR HOME THEY ARE",
        "FEW, TINY AND SLOW: POCKET CHANGE.",
        "BUGS SCALE WITH DISTANCE: TO FIND THE BIG ONES YOU",
        "HAVE TO GO FAR OUT - WHERE THEY ARE BIGGER, MEANER,",
        "FAR MORE NUMEROUS, AND EVEN THE LITTLE ONES ARE FAST.",
        "",
        "WALLS OF CARCASS BLOCK BUGS UNTIL THEY CHEW THROUGH.",
        "YOUR SHOTS FLY OVER WALLS - BUGS HAVE TO CHEW THROUGH.",
        "IF EVERY PLAYER FALLS, THE RUN IS OVER. THE WORLD SAVES",
        "AS YOU GO. HEAVIEST HAT TOPS THE LEADERBOARD.",
        "",
        l5,
        "ZEN MODE (MENU): EVERY BUG IS GIGANTIC AND SLOW.",
        "A CALMER GAME - AND ZEN RUNS SET NO RECORDS.",
    };
    for (int i=0;i<(int)(sizeof(lines)/sizeof(lines[0]));i++)
        draw_text_center(VW/2, 44+i*10, lines[i], 1, COL(225,230,240));
    if (pad_mode()) draw_text_center(VW/2, VH-24, "B - PAGE 2: THE MALL", 1, COL(160,230,255));
    else if (mobile_ui()) draw_text_center(VW/2, VH-24, "TAP - PAGE 2: THE MALL", 1, COL(160,230,255));
    else { char b[48]; char *q=b; q=sappend(q, key_name(G.bind[ACT_INTERACT])); q=sappend(q, " - PAGE 2: THE MALL");
      draw_text_center(VW/2, VH-24, b, 1, COL(160,230,255)); }
}
// ---------------------------------------------------------------------------
// Settings: COMFORT rows (v1.25 feature 2) over the key bindings.
// Bindings live in G.bind, are derived into semantic input centrally
// (derive_input), and persist in the save file — the comfort four
// (music volume, SFX volume, mute, screen shake) persist in the
// same global blob beside them. Rows: 0..3 comfort, 4..17 bindings,
// 18 RESET DEFAULTS (which resets the comfort four too). One
// geometry helper serves the renderer and the tap handler so the
// two cannot drift apart.
// ---------------------------------------------------------------------------
