// THE SURVIVAL GAME — war/orders.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 1807-2001 in the Nov 2026 fine-grained refactor; code unchanged.

static void war_select_unit(BwSim *S, int i, int all_of_type){
    for (int j=0;j<WAR_MAXU;j++) S->war_usel[j]=0;
    S->war_selstruct = -1; S->war_bcat = -1;   // a selection change closes any open category
    if (all_of_type){
        int t = S->war_utype[i];
        for (int j=0;j<WAR_MAXU;j++){
            if (S->war_uhp[j]<=0 || S->war_utype[j]!=t) continue;
            float dx=S->war_ux[j]-S->war_camx, dy=S->war_uy[j]-S->war_camy;
            if (dx>-360 && dx<360 && dy>-220 && dy<220) S->war_usel[j]=1;   // on screen, with a margin
        }
        if (!S->war_usel[i]) S->war_usel[i]=1;
    } else S->war_usel[i]=1;
    sfx(6);
}
// the third tap of a streak: EVERY combat unit you own — melee,
// ranged and both vehicles, map-wide. Harvesters stay out of it.
static void war_select_all_combat(BwSim *S){
    for (int j=0;j<WAR_MAXU;j++)
        S->war_usel[j] = (S->war_uhp[j]>0 && war_u_side(S->war_utype[j])==0 && war_u_combat(S->war_utype[j]))? 1 : 0;
    S->war_selstruct = -1; S->war_bcat = -1;
    sfx(6);
}
static void war_order_move(BwSim *S, float wx, float wy){
    int ord = 0;
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || !S->war_usel[i]) continue;
        float ox = (float)(ord%4 - 1.5f) * 16.0f, oy = (float)((ord/4)%3 - 1) * 16.0f;
        ord++;
        S->war_ustate[i]=1; S->war_utarget[i]=0;
        S->war_utx[i]=wx+ox; S->war_uty[i]=wy+oy;
        S->war_urepair[i]=0;   // a fresh order supersedes repair work (v1.19 addendum)
        S->war_wpn[i]=0;   // a fresh order drops any old path (v1.17)
    }
}
static void war_order_attack(BwSim *S, int eref){
    float tx, ty;
    if (!war_ref_alive(S, eref, &tx, &ty)) return;
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || !S->war_usel[i]) continue;
        if (S->war_utype[i]==WAR_U_HARV || S->war_utype[i]==WAR_U_PATCH){ S->war_ustate[i]=1; S->war_utarget[i]=0; S->war_utx[i]=tx; S->war_uty[i]=ty; }   // the weaponless just walk there — the barrier-tap precedent: an enemy/debris tap is a move order to a patcher, never an attack
        else { S->war_ustate[i]=2; S->war_utarget[i]=eref; }
        S->war_urepair[i]=0;   // a fresh order supersedes repair work (v1.19 addendum)
        S->war_wpn[i]=0;   // a fresh order drops any old path (v1.17)
    }
}
// the heart of every tap: a point on the battlefield (world coords)
static void war_tap_world(int pi, float wx, float wy){
    BwSim *S = war_bs(pi);   // feat 13: orders land on the shared army, whoever gives them
    if (S->war_place>=0){ war_try_place(pi, wx, wy); return; }
    int bu=-1; float bd=20.0f*20.0f;
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || war_u_side(S->war_utype[i])!=0) continue;
        float dx=wx-S->war_ux[i], dy=wy-S->war_uy[i], d2=dx*dx+dy*dy;
        if (d2<bd){ bd=d2; bu=i; }
    }
    if (bu>=0){
        // the tap streak (Daniel's ladder): one tap selects the unit,
        // a second inside the window selects its whole type on screen,
        // a third escalates to every combat unit you own — and further
        // taps inside the same streak hold that full selection.
        int streak = (S->war_ltap_unit==bu && S->war_t - S->war_ltap_t < 0.35f)? S->war_ltap_n + 1 : 1;
        S->war_ltap_unit=bu; S->war_ltap_t=S->war_t; S->war_ltap_n=streak;
        if (streak>=3) war_select_all_combat(S);
        else war_select_unit(S, bu, streak==2);
        return;
    }
    S->war_ltap_unit=-1; S->war_ltap_n=0;   // any other tap ends the streak
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0 || S->war_sside[s]!=0) continue;
        float dx=wx-S->war_sx[s], dy=wy-S->war_sy[s], rr=(float)WAR_SRAD[S->war_stype[s]]+8.0f;
        if (dx*dx+dy*dy < rr*rr){
            // v1.19 addendum: patchers in the selection + a DAMAGED
            // friendly structure = a REPAIR order, not a selection
            // change. Other selected units keep their orders,
            // exactly as a friendly-structure tap always left them.
            // A full-health structure falls through to the selection
            // behaviour below — there is nothing to fix.
            if (S->war_shp[s] < (float)S->war_smaxhp[s]){
                int anyp = 0;
                for (int i=0;i<WAR_MAXU;i++)
                    if (S->war_uhp[i]>0 && S->war_usel[i] && S->war_utype[i]==WAR_U_PATCH){
                        S->war_urepair[i]=s+1; S->war_ustate[i]=0; S->war_utarget[i]=0; S->war_wpn[i]=0; anyp = 1;
                    }
                if (anyp){ S->war_ordx=wx; S->war_ordy=wy; S->war_ordt=0.5f; sfx(6); return; }
            }
            if (s == S->war_selstruct){
                // v1.19 addendum (Daniel): tapping the selected
                // building itself toggles it OFF — the same cleanup
                // as every other deselect path (the pad retires its
                // bar highlight on the selection change by itself)
                S->war_selstruct=-1; S->war_bcat=-1; sfx(6); return;
            }
            for (int i=0;i<WAR_MAXU;i++) S->war_usel[i]=0;
            S->war_selstruct=s; S->war_bcat=-1; sfx(6); return;   // a selection change closes any open category (a lone category reopens itself in the bar)
        }
    }
    int eref=0; float ed=1e18f, edist=1e18f;
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || war_u_side(S->war_utype[i])!=1) continue;
        float dx=wx-S->war_ux[i], dy=wy-S->war_uy[i], d2=dx*dx+dy*dy;
        if (d2<18.0f*18.0f && d2<ed){ ed=d2; edist=fsqrt(d2); eref=i+1; }
    }
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0 || S->war_sside[s]!=1) continue;
        float dx=wx-S->war_sx[s], dy=wy-S->war_sy[s], rr=(float)WAR_SRAD[S->war_stype[s]]+8.0f;
        if (dx*dx+dy*dy < rr*rr){ eref=-(s+1); edist=fsqrt(dx*dx+dy*dy); break; }
    }
    // debris (Daniel, Nov 16): a tap on a terrain feature is an attack
    // order on it — your combat units can clear the field — unless an
    // enemy unit or structure sits even closer to the tap point.
    // Harvesters in the selection just walk there, as for any tap.
    { int fref=0; float fdist=1e18f;
      for (int t=0;t<S->war_tn;t++){
          if (S->war_thp[t]<=0) continue;
          if (S->war_ttype[t]==5) continue;   // v1.18: barriers are NOT attack targets — a tap on one falls through to a plain move order below
          float dx=wx-S->war_tx[t], dy=wy-S->war_ty[t], rr=(float)S->war_trad[t]+6.0f, d2=dx*dx+dy*dy;
          if (d2 < rr*rr){ float d=fsqrt(d2); if (d<fdist){ fdist=d; fref=WAR_TREF(t); } }
      }
      if (fref && (!eref || fdist < edist)){
          if (war_sel_count(S)>0){
              war_order_attack(S, fref);
              S->war_ordx=wx; S->war_ordy=wy; S->war_ordt=0.5f; sfx(6);
          }
          return;
      } }
    if (eref){
        if (war_sel_count(S)>0){
            war_order_attack(S, eref);
            S->war_ordx=wx; S->war_ordy=wy; S->war_ordt=0.5f; sfx(6);
        }
        return;
    }
    if (war_sel_count(S)>0){
        war_order_move(S, wx, wy);
        S->war_ordx=wx; S->war_ordy=wy; S->war_ordt=0.5f; sfx(6);
    } else if (S->war_selstruct>=0){
        int ss = S->war_selstruct;
        int st = war_struct_alive(S, ss)? S->war_stype[ss] : -1;
        if (st==WAR_S_HQ || st==WAR_S_PATCH || st==WAR_S_ASM){
            // rally point (addendum 3): a field tap with a PRODUCER
            // selected plants its flag instead of deselecting — the
            // building stays selected, fresh units walk to the flag
            S->war_rallyx[ss]=wx; S->war_rallyy[ss]=wy;
            S->war_ordx=wx; S->war_ordy=wy; S->war_ordt=0.5f;   // the order-marker flash, reused
            sfx(6);
        } else { S->war_selstruct=-1; S->war_bcat=-1; sfx(6); }   // a non-producer just deselects, as always
    }
}
static void war_handle_tap(int pi, int sx, int sy){
    BwSim *S = war_bs(pi);   // feat 13: the tap acts on the shared battle, in the TAPPER's view
    if (S->war_pause){ war_pause_tap(pi, sx, sy); return; }   // v1.17: the pause menu is modal — the tap never reaches the field
    int vx, vy, vw2, vh2; pi_view_rect(pi, &vx, &vy, &vw2, &vh2);
    if (S->war_bcat>=0 && sy >= vy+vh2-WAR_BAR_H-WAR_SUB_H && sy < vy+vh2-WAR_BAR_H){
        // the options strip (addendum 1): it holds the row war_bar_list
        // describes. WAR_BAR_H is untouched — the world view never
        // resizes; the strip simply owns these taps while it is drawn.
        if (sx>=vx && sx<vx+vw2){
            int rel = sx - vx - 6;
            int slot = rel/90;
            if (rel>=0 && slot<5 && (rel%90)<84) war_bar_activate(pi, slot);
        }
        return;
    }
    if (sy >= vy+vh2-WAR_BAR_H){   // the command bar owns the bottom strip
        if (sx>=vx && sx<vx+vw2){
            int rel = sx - vx - 6;
            int slot = rel/90;
            if (rel>=0 && slot<5 && (rel%90)<84){
                if (S->war_bcat>=0){
                    // a category is open above: the bar row itself IS
                    // the categories — tapping one switches the strip
                    int ca[5], cc[5]; const char *cn[5];
                    int cn2 = war_bar_cats(S, ca, cc, cn);
                    if (slot<cn2){ S->war_bcat = ca[slot]-20; S->war_barhl=-1; sfx(6); }
                } else war_bar_activate(pi, slot);
            }
        }
        return;
    }
    float wx = S->war_camx + (float)(sx - vx - vw2/2);
    float wy = S->war_camy + WAR_BAR_H*0.5f + (float)(sy - vy - vh2/2);
    war_tap_world(pi, wx, wy);
}
// Grab-the-world panning (reworked Nov 16, Daniel: the old pan
// "didn't expect what was actually happening"). The old code anchored
// on wherever the finger happened to be when the touch crossed the
// 14px "moved" threshold, so the world trailed the finger by that
// much on every drag — and on the STICK scheme it never ran at all,
// leaving the field stick to shove the camera at a fixed 430 u/s
// through BwIn keys. Now the anchor is the DOWN point (tp x0/y0) and
// the camera as it stood when the finger landed: cam = anchor -
// (finger - down), so the world point grabbed stays under the finger
// from the first pixel, on touch and mouse alike (same channel).
// Returns 1 while a pan drag owns the field (the caller then skips
// the key/stick camera pan, so the two never fight).
