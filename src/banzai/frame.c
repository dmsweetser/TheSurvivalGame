// THE SURVIVAL GAME — banzai/frame.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 1186-1421 in the Nov 2026 fine-grained refactor; code unchanged.

// --- the frame ---------------------------------------------------------------
static void bw_roam_refresh(Buddy *b){
    BwSim *S = &BWS[(int)(b - G.bud)];
    if (b->bw_mode==2 || b->bw_mode==6 || b->bw_mode==7) return;
    u32 seed=(u32)b->bw_seed;
    int cx0=bw_cellof(b->x), cy0=bw_cellof(b->y);
    int want_cell[BW_ROAMN]; int nw=0;
    for (int cx=cx0-3; cx<=cx0+3 && nw<BW_ROAMN; cx++)
        for (int cy=cy0-3; cy<=cy0+3 && nw<BW_ROAMN; cy++){
            float ox, oy; if (bw_cell_feat(cx, cy, seed, &ox, &oy, b->bw_mode==1)!=2) continue;
            int key=bw_cell_key(cx, cy);
            if (bw_defeated_has(S, key)) continue;
            float wx=cx*BW_CELL+ox, wy=cy*BW_CELL+oy;
            if (bw_outside_world(wx, wy)) continue;   // no hackers at sea
            float dx=wx-b->x, dy=wy-b->y;
            if (dx*dx+dy*dy > 640*640) continue;
            want_cell[nw++]=key;
        }
    // keep the roamers we already have, drop the rest, spawn the new
    for (int i=0;i<BW_ROAMN;i++){
        if (!S->roam[i].cell && S->roam[i].x==0 && S->roam[i].y==0) continue;
        int keep=0; for (int j=0;j<nw;j++) if (want_cell[j]==S->roam[i].cell) keep=1;
        if (!keep){ S->roam[i].cell=0; S->roam[i].x=0; S->roam[i].y=0; }
    }
    for (int j=0;j<nw;j++){
        int have=0; for (int i=0;i<BW_ROAMN;i++) if (S->roam[i].cell==want_cell[j]) have=1;
        if (have) continue;
        for (int i=0;i<BW_ROAMN;i++) if (S->roam[i].cell==0 && S->roam[i].x==0 && S->roam[i].y==0){
            // recover the cell coords from the key is not possible; recompute by scan
            int cx0b=bw_cellof(b->x), cy0b=bw_cellof(b->y), done=0;
            for (int cx=cx0b-3; cx<=cx0b+3 && !done; cx++)
                for (int cy=cy0b-3; cy<=cy0b+3 && !done; cy++){
                    if (bw_cell_key(cx, cy)!=want_cell[j]) continue;
                    float ox, oy; bw_cell_feat(cx, cy, seed, &ox, &oy, b->bw_mode==1);
                    S->roam[i].cell=want_cell[j];
                    S->roam[i].x=cx*BW_CELL+ox; S->roam[i].y=cy*BW_CELL+oy;
                    S->roam[i].wt=0; S->roam[i].dir=0; S->roam[i].alert=0; S->roam[i].lvl=bw_level_at(b, S->roam[i].cell);
                    done=1;
                }
            break;
        }
    }
}
static void bw_press_term(int pi, int t){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    int ord[3]; bw_term_order(b, ord);
    int prog = bw_puz_prog(b);
    if (t == ord[prog]){
        prog++; bw_puz_prog_set(b, prog); sfx(19);
        if (prog>=3){
            b->story_bits |= (1 << 15);
            scpy(S->note, "OLD LOCKS LET GO SOMEWHERE. THE INNER AISLE IS OPEN.", 72); S->note_t=4.0f;
        } else {
            char *q=S->note; q=sappend(q, "THE "); q=sappend(q, BW_TERM_NAME[t]); q=sappend(q, " TERMINAL CHIRPS. "); q=sapp_int(q, prog); q=sappend(q, "/3"); *q=0;
            S->note_t=2.6f;
        }
    } else {
        bw_puz_prog_set(b, 0); sfx(16);
        scpy(S->note, "THE TERMINALS BUZZ AND FORGET EVERYTHING. START OVER.", 72); S->note_t=3.2f;
    }
}
static void bw_read_sign(int pi){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    int ord[3]; bw_term_order(b, ord);
    char *q=S->note; q=sappend(q, "SCRATCHED DEEP: ");
    for (int i=0;i<3;i++){ if (i) q=sappend(q, ". "); q=sappend(q, BW_TERM_NAME[ord[i]]); }
    q=sappend(q, ". - R."); *q=0;
    S->note_t=4.5f; sfx(6);
}
static void bw_pick_tool(int pi, int t){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    b->story_bits |= (1<<(24+t)); sfx(11);
    char *q=S->note; q=sappend(q, "FOUND MOP'S "); q=sappend(q, BW_TOOL_NAME[t]); q=sappend(q, "! (");
    q=sapp_int(q, bw_tools_found(b)); q=sappend(q, "/3)"); *q=0;
    S->note_t=3.0f;
}
static void bw_read_hsign(int pi, int idx){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    int first = !((b->story_bits2>>(11+idx))&1);
    if (first) b->story_bits2 |= (1<<(11+idx));
    sfx(6);
    if (first && bw_qstate2(b,9)==1 && bw_signs_read(b)>=3){
        bw_qset2(b,9,2); b->xp += 20;
        scpy(S->note, "THREE SIGNS READ. THE MEADOW APPROVES. +20 XP.", 72); S->note_t=4.0f;
        return;
    }
    char *q=S->note; q=sappend(q, BW_HSIGN_TXT[idx]);
    if (bw_qstate2(b,9)==1){ q=sappend(q, " ("); q=sapp_int(q, bw_signs_read(b)); q=sappend(q, "/3)"); }
    *q=0; S->note_t=4.2f;
}
static void bw_meet_bitling(int pi){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    sfx(11);
    if (bw_qstate2(b,8)==1){
        bw_qset2(b,8,2); b->xp += 30;
        b->items[0] += 1; if (b->items[0]>9) b->items[0]=9;
        scpy(S->note, "BITLING CHIRPS AND TROTS HOME. +30 XP. PATCHINA WILL BE GLAD.", 72); S->note_t=4.5f;
    } else {
        scpy(S->note, "BITLING: BEEP. (IT SEEMS LOST, AND HAPPY ABOUT IT.)", 72); S->note_t=3.4f;
    }
}
// Duster's distance from the buddy (feat 14) — defined here, below
// BwSim; -1 while the escort is not placed.
static float bw_duster_dist(Buddy *b){
    BwSim *S = &BWS[b - G.bud];
    if (!S->ch2_esc_on) return -1;
    float dx=b->x-S->ch2_esc_x, dy=b->y-S->ch2_esc_y;
    return fsqrt(dx*dx + dy*dy);
}
static void bw_talk(int pi, int npc){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    sfx(6);
    S->grace_t = 4.0f;   // a conversation is a breather (Nov 4): with hunters about, nobody should get jumped mid-dialogue or while opening the quest log right after
    if (npc==0){   // Elder Patchina
        // THE REBUILDING (v1.25 feat 14): the post-game chapter is
        // Patchina's new list, and her door is shut until the story
        // is complete — every arm here is gated on it, and the
        // shipped chain below is word-for-word untouched. The chain
        // auto-advances: the escort's finish arms the rematch, the
        // rematch's win arms the festival report.
        if (bw_story_complete(b) && bw_qstate3(b,10)==0){
            bw_qset3(b,10,1); S->ch2_race_t = 120; S->ch2_race_on = 1;   // she is the start line
            scpy(S->note, "PATCHINA: THE CIDER IS ON THE LAKE ICE, AND THE ICE IS MELTING. RACE!", 72); S->note_t=5.0f;
        } else if (bw_story_complete(b) && bw_qstate3(b,10)==1){
            S->ch2_race_t = 120; S->ch2_race_on = 1;   // the run restarts at her door, at a full clock
            scpy(S->note, "PATCHINA: AGAIN, DEAR. THE ICE IS PATIENT. THE CIDER IS NOT. RUN!", 72); S->note_t=4.5f;
        } else if (bw_story_complete(b) && bw_qstate3(b,11)==0){
            bw_qset3(b,11,1); S->ch2_esc_on = 1; S->ch2_esc_x = CH2_DUSTER_X; S->ch2_esc_y = CH2_DUSTER_Y;
            S->ch2_wp_on = 0; S->ch2_path_t = 0; S->ch2_stall = 0; S->ch2_base = 1e9f;
            scpy(S->note, "PATCHINA: WALK DUSTER TO THE SERVER FARM, DEAR. SMALL LEGS, BIG JOB.", 72); S->note_t=4.5f;
        } else if (bw_story_complete(b) && bw_qstate3(b,11)==1){
            scpy(S->note, "PATCHINA: DUSTER FOLLOWS YOUR HEELS. THE FARM, DEAR. EAST-NORTH-EAST.", 72); S->note_t=4.0f;
        } else if (bw_story_complete(b) && bw_qstate3(b,12)==1){
            scpy(S->note, "PATCHINA: IT CAME BACK WRONG, DEAR. FINISH IT PROPERLY.", 72); S->note_t=4.0f;
        } else if (bw_story_complete(b) && bw_qstate3(b,13)==1){
            bw_qset3(b,13,2); b->xp += 60;
            b->items[2] += 2; if (b->items[2]>9) b->items[2]=9;
            S->ch2_fest_t = 24;
            scpy(S->note, "PATCH DAY! THE VILLAGE DANCES IN THE LANTERN LIGHT. +60 XP.", 72); S->note_t=5.0f;
        } else if (bw_chapter_done(b)){
            S->ch2_fest_t = 10;   // the day is re-celebrated, never re-paid
            scpy(S->note, "PATCHINA: EVERY DAY IS A LITTLE PATCH DAY NOW, DEAR.", 72); S->note_t=4.0f;
        }
        else if (bw_qstate(b,0)==1){
            bw_qset(b,0,2); bw_qset(b,1,1); bw_qset(b,2,1); bw_qset2(b,9,1);
            b->items[0] += 2; if (b->items[0]>9) b->items[0]=9;
            scpy(S->note, "PATCHINA: BONK 3 HACKERS AT THE SERVER FARM, DEAR.", 72); S->note_t=4.5f;
        } else if (bw_qstate(b,1)==2 && bw_qstate(b,3)==0){
            bw_qset(b,3,1); bw_qset_x(b,5,1);
            scpy(S->note, "PATCHINA: ZED'S GATE NEEDS ITS CRANK. IT SANK IN THE WESTERN RUINS.", 72); S->note_t=4.5f;
        } else if (bw_qstate(b,1)==2 && bw_qstate_x(b,5)==1){
            scpy(S->note, "PATCHINA: WEST, DEAR. THE RUINS. MIND THE HOARDER.", 72); S->note_t=4.0f;
        } else if (bw_qstate(b,1)==2 && bw_qstate2(b,8)==0){
            bw_qset2(b,8,1);
            scpy(S->note, "PATCHINA: A STRAY PROCESS WANDERS SOUTH OF US. FETCH IT HOME.", 72); S->note_t=4.5f;
        } else if (bw_qstate(b,1)==2){
            scpy(S->note, "PATCHINA: THROUGH THE GATE, DEAR. GIVE HIM A LOVELY FUNERAL.", 72); S->note_t=4.0f;
        } else {
            scpy(S->note, "PATCHINA: THE FARM, DEAR. EAST-NORTH-EAST. MIND THE RACKS.", 72); S->note_t=4.0f;
        }
    } else if (npc==1){   // Mop
        // THE REBUILDING (feat 14): two gated flavour arms — the
        // escort in progress, and the festival he mopped. Twice.
        if (bw_qstate3(b,11)==1){
            scpy(S->note, "MOP: DUSTER MEANS WELL. IT ALSO MEANS SLOWLY.", 72); S->note_t=3.6f;
        } else if (bw_chapter_done(b)){
            scpy(S->note, "MOP: I MOPPED THE FESTIVAL. TWICE.", 72); S->note_t=3.6f;
        }
        else if (bw_qstate_x(b,4)==1 && bw_tools_found(b)>=3){
            bw_qset_x(b,4,2); b->xp += 40;
            b->items[2] += 2; if (b->items[2]>9) b->items[2]=9;
            scpy(S->note, "MOP: MY TOOLS! COOKIES FOR YOU. +40 XP.", 72); S->note_t=4.0f;
        } else if (bw_qstate_x(b,4)==2 && bw_qstate2(b,7)==0){
            bw_qset2(b,7,1); b->story_bits2 |= (1<<10);   // parcel carried
            scpy(S->note, "MOP: TAKE THIS PARCEL TO GLITCH AT THE LAKE. MIND IT.", 72); S->note_t=4.2f;
        } else if (bw_qstate(b,0)==2 && bw_qstate_x(b,4)==0){
            bw_qset_x(b,4,1);
            scpy(S->note, "MOP: LOST MY WRENCH, HAMMER AND BRUSH OUT THERE. FIND THEM?", 72); S->note_t=4.2f;
        } else if (bw_qstate_x(b,4)==1){
            char *q=S->note; q=sappend(q, "MOP: "); q=sapp_int(q, bw_tools_found(b)); q=sappend(q, "/3 TOOLS HOME. THE MEADOW HIDES THEM WELL."); *q=0;
            S->note_t=3.6f;
        } else if (bw_qstate_x(b,4)==2){
            scpy(S->note, "MOP: THE MEADOW FEARS MY MOP AGAIN.", 72); S->note_t=3.6f;
        } else {
            scpy(S->note, "MOP: I PATCH MY SOCKS WITH OLD CODE. SLIGHTLY CURSED.", 72); S->note_t=3.6f;
        }
    } else {   // Glitch, the lake hermit
        // THE REBUILDING (feat 14): the race's far end, and the
        // hermit's review of the festival he did not attend.
        if (bw_qstate3(b,10)==2 && bw_qstate3(b,11)==0){
            scpy(S->note, "GLITCH: FAST FEET. PATCHINA HAS MORE LIST, I EXPECT.", 72); S->note_t=4.0f;
        } else if (bw_chapter_done(b)){
            scpy(S->note, "GLITCH: I DANCED AT PATCH DAY. THE ICE REMEMBERS.", 72); S->note_t=3.6f;
        }
        else if (bw_qstate(b,2)==1){
            bw_qset(b,2,2);
            b->items[1] += 3; if (b->items[1]>9) b->items[1]=9;
            scpy(S->note, "GLITCH: COMPANY! TAKE THESE. +3 ENERGY DRINKS.", 72); S->note_t=4.0f;
        } else if (bw_qstate2(b,7)==1 && ((b->story_bits2>>10)&1)){
            bw_qset2(b,7,2); b->story_bits2 &= ~(1<<10);
            b->xp += 40; b->items[2] += 1; if (b->items[2]>9) b->items[2]=9;
            scpy(S->note, "GLITCH: MOP'S PARCEL! UNBROKEN. +40 XP. THE COOKIE IS YOURS.", 72); S->note_t=4.5f;
        } else if (bw_qstate2(b,6)==1 && bw_lakekills(b)>=2){
            bw_qset2(b,6,2); b->xp += 50;
            b->items[1] += 2; if (b->items[1]>9) b->items[1]=9;
            scpy(S->note, "GLITCH: QUIET ICE AGAIN. +50 XP. DRINKS FOR THE ROAD.", 72); S->note_t=4.5f;
        } else if (bw_qstate(b,2)==2 && bw_qstate2(b,6)==0){
            bw_qset2(b,6,1);
            scpy(S->note, "GLITCH: THE LAKE IS CRAWLING. BONK 2 ON THE ICE FOR ME.", 72); S->note_t=4.5f;
        } else if (bw_qstate_x(b,5)==1 && !bw_has_crank(b)){
            scpy(S->note, "GLITCH: THE CACHE SINGS UNDER THE WESTERN STONES. IT SITS ON IT.", 72); S->note_t=4.0f;
        } else if (bw_has_crank(b) && !((b->story_bits>>17)&1)){
            scpy(S->note, "GLITCH: YOU CRANKED IT. ZED NEXT. GIVE HIM MY WORST.", 72); S->note_t=4.0f;
        } else {
            scpy(S->note, "GLITCH: THE LAKE KEEPS WHAT THE COMPILER THREW AWAY.", 72); S->note_t=3.6f;
        }
    }
}
static void bw_use_item(int pi, int it){
    BwSim *S = &BWS[pi]; Buddy *b=&G.bud[pi];
    if (b->items[it]<=0){ scpy(S->note, "NONE LEFT. HACKERS DROP THEM, CAMPS STOCK THEM.", 72); S->note_t=2.4f; sfx(16); return; }
    if (it==0){
        if (b->hp>=b->maxhp){ scpy(S->note, "ALREADY FINE. SAVE THE PATCH.", 72); S->note_t=2.2f; return; }
        b->items[0]--; b->hp += b->maxhp*0.4f; if (b->hp>b->maxhp) b->hp=b->maxhp;
        scpy(S->note, "PATCH NOTES APPLIED. STINGS LESS.", 72); S->note_t=2.4f; sfx(11);
    } else if (it==1){
        if (b->energy>=BUD_ENERGY_MAX){ scpy(S->note, "ALREADY BUZZING.", 72); S->note_t=2.2f; return; }
        b->items[1]--; b->energy += 50; if (b->energy>BUD_ENERGY_MAX) b->energy=BUD_ENERGY_MAX;
        scpy(S->note, "ENERGY DRINK DOWN. BUZZING.", 72); S->note_t=2.4f; sfx(11);
    } else if (it==2){
        b->items[2]--; S->buff=1;
        scpy(S->note, "COOKIE EATEN. THE NEXT HIT LANDS DOUBLE.", 72); S->note_t=2.6f; sfx(11);
    } else {
        scpy(S->note, "CTRL-Z ONLY UNDOES BATTLES. NICE TRY, THOUGH.", 72); S->note_t=2.4f; sfx(6);
    }
}
