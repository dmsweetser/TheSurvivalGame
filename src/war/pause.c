// THE SURVIVAL GAME — war/pause.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 1484-1568 in the Nov 2026 fine-grained refactor; code unchanged.

// --- the pause menu (v1.17) ------------------------------------------
// Esc in a battle no longer abandons it: the battle freezes behind a
// three-choice panel — RESUME, RESTART BATTLE, SAVE & QUIT. One set
// of rects, shared by the renderer and every input path (tap, pad
// cursor, keys 1-3), so they can never disagree about the buttons.
static void war_pause_rects(int vx, int vy, int vw2, int vh2, int *px, int *py, int *pw, int *ph){
    *pw = vw2<420? vw2-24 : 396; *ph = 196;
    *px = vx+(vw2-*pw)/2; *py = vy+(vh2-*ph)/2 - 10;
}
static int war_pause_btn_y(int py, int n){ return py + 56 + n*44; }   // three 36px buttons, 8px apart
static void war_pause_pick(int pi, int n){
    if (BWS[pi].war_guest) pi = BWS[pi].war_guest-1;   // feat 13: a guest pauser's picks act on the battle (the owner's sim)
    BwSim *S = &BWS[pi];
    if (n==0){ S->war_pause=0; S->war_grace=0.3f; sfx(6); }   // RESUME (the grace keeps the closing press from leaking an order)
    else if (S->war_skirmish && n==1){   // v1.19: RESTART a skirmish — re-roll the same size/difficulty; NO campaign snapshot clear (a campaign park is never this fight's to discard). v1.29: the re-roll supersedes any saved skirmish, so the SKIRMISH park is cleared (mirroring the campaign restart's own-park clear below)
        war_snap_sk_clear(pi);
        S->war_pause=0;
        war_start_skirmish(pi);
    }
    else if (S->war_skirmish && n==2){   // v1.29: SAVE & QUIT — the skirmish parks in its OWN slot (the kb..kh text), beside any campaign park, and waits for the entry panel (was QUIT BATTLE in v1.19, when a skirmish could not be saved at all)
        war_snap_sk_serialize(pi);
        do_save();
        S->war_pause=0;
        war_clear_field(S);
        S->war_state=0; S->war_grace=0.3f;
        war_note(S, "SKIRMISH SAVED. IT WILL WAIT FOR YOU.");
        if (S->war_duo>=0){ int g=S->war_duo; war_link_break(pi); BWS[g].war_state=0; BWS[g].war_grace=0.3f; war_note(&BWS[g], "THE COMMANDER SAVED THE SKIRMISH."); }   // feat 13: the guest goes back to their own map too
        sfx(19);
    }
    else if (n==1){   // RESTART BATTLE: this region, fresh, and no snapshot of the old fight survives — unless the park belongs to another region (v1.18 scoping, as in war_win/war_lose)
        if (!war_snap_on[pi] || war_snap_reg[pi]==S->war_region) war_snap_clear(pi);
        S->war_pause=0;
        war_start_battle(pi, S->war_region);
    } else if (n==2){   // SAVE & QUIT: park the battle, persist, back to the map
        war_snap_serialize(pi);
        do_save();
        S->war_pause=0;
        war_clear_field(S);
        S->war_state=0; S->war_grace=0.3f;
        war_note(S, "BATTLE SAVED. IT WILL WAIT FOR YOU.");
        if (S->war_duo>=0){ int g=S->war_duo; war_link_break(pi); BWS[g].war_state=0; BWS[g].war_grace=0.3f; war_note(&BWS[g], "THE COMMANDER SAVED THE BATTLE."); }   // feat 13: the guest goes back to their own map too
        sfx(19);
    }
}
static void war_pause_tap(int pi, int sx, int sy){
    int vx, vy, vw2, vh2; pi_view_rect(pi, &vx, &vy, &vw2, &vh2);
    int px, py, pw, ph; war_pause_rects(vx, vy, vw2, vh2, &px, &py, &pw, &ph);
    for (int n=0;n<3;n++){
        int by = war_pause_btn_y(py, n);
        if (sx>=px+40 && sx<px+pw-40 && sy>=by && sy<by+36){ war_pause_pick(pi, n); return; }
    }
    // a tap outside the panel does nothing: the menu is modal
}
// the pad in the pause menu: the cursor flies the frozen field, A
// presses through war_pause_tap, B resumes — war_pad_update's peel
// logic stays out of it
static void war_pad_pause(int pi, const float *ax, u32 pe, float dt){
    BwSim *S = war_bs(pi);   // feat 13: either commander can drive the pause with their own pad
    int vx, vy, vw2, vh2; pi_view_rect(pi, &vx, &vy, &vw2, &vh2);
    float lx = war_axis(ax[0]), ly = war_axis(ax[1]);
    int stick = (ax[0]>0.18f || ax[0]<-0.18f || ax[1]>0.18f || ax[1]<-0.18f);
    u32 btns = (1u<<ACT_FIRE)|(1u<<ACT_INTERACT);
    if (!S->war_curon){
        if (!war_pad_live(pi) || !(stick || (pe & btns))) return;
        S->war_curon = 1;
        S->war_curx = S->war_camx; S->war_cury = S->war_camy + WAR_BAR_H*0.5f;   // wake at the camera's centre, under the panel
        if (!stick) return;   // a button wake spends the press on waking
        pe = 0;
    }
    if (lx || ly){ S->war_curx += lx * 540.0f * dt; S->war_cury += ly * 540.0f * dt; }
    if (S->war_curx < WAR_OX) S->war_curx = WAR_OX;
    if (S->war_curx > WAR_OX+S->war_aw) S->war_curx = WAR_OX+S->war_aw;
    if (S->war_cury < WAR_OY) S->war_cury = WAR_OY;
    if (S->war_cury > WAR_OY+S->war_ah) S->war_cury = WAR_OY+S->war_ah;
    { float csx = (float)vx + vw2/2 + (S->war_curx - S->war_camx);   // no edge-follow: the camera holds still, the cursor stays on screen
      float csy = (float)vy + vh2/2 + (S->war_cury - S->war_camy - WAR_BAR_H*0.5f);
      if (csx < vx) S->war_curx += (vx - csx);
      if (csx > vx+vw2) S->war_curx -= (csx - (vx+vw2));
      if (csy < vy) S->war_cury += (vy - csy);
      if (csy > vy+vh2) S->war_cury -= (csy - (vy+vh2));
    }
    if (pe & (1u<<ACT_INTERACT)){ war_pause_pick(pi, 0); return; }   // B = RESUME
    if ((pe & (1u<<ACT_FIRE)) && S->war_grace<=0){
        int csx = vx + vw2/2 + (int)(S->war_curx - S->war_camx);
        int csy = vy + vh2/2 + (int)(S->war_cury - S->war_camy - WAR_BAR_H*0.5f);
        war_pause_tap(pi, csx, csy);
    }
}
