// THE SURVIVAL GAME — war/path.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 614-1248 in the Nov 2026 fine-grained refactor; code unchanged.

// --- pathfinding (v1.17, Daniel: units must conceive of going
// AROUND a wall) ------------------------------------------------------
// war_move_to used to steer in a straight line and let war_separate
// push the unit out of obstacle circles; with the target behind a
// firewall line or a building the two forces reach a stable
// equilibrium and the unit pins forever. Now: grid A* over the fog
// grid's geometry (40x30 cells of 40u), requested ONLY when a unit
// stalls (0.6s of trying to step with <3u of net movement), so idle
// behaviour and cost are unchanged. The grid and the LOS shortcut are
// SIDE-AWARE (addendum 1): a gate is an obstacle only to side 1 —
// your harvesters route through your gates; bugs route around them
// or chew through. All scratch is fixed static arrays; every loop is
// bounded (expansion cap, ring cap, sample cap) — no wasm hangs.
static unsigned char war_pf_block[WAR_FOG_W*WAR_FOG_H];
static int war_pf_gw = WAR_FOG_W, war_pf_gh = WAR_FOG_H;   // v1.19: the live field's grid dims, set by war_pf_find — a SMALL skirmish field's grid is a subgrid of these same arrays (cells outside it are filled blocked)
static int war_pf_g[WAR_FOG_W*WAR_FOG_H];
static int war_pf_from[WAR_FOG_W*WAR_FOG_H];
static unsigned char war_pf_closed[WAR_FOG_W*WAR_FOG_H];
static int war_pf_path[WAR_FOG_W*WAR_FOG_H];
static float war_pf_tpx[128], war_pf_tpy[128];
static int war_pf_blocked_at(BwSim *S, int side, float wx, float wy){
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0) continue;
        if (side==0 && S->war_stype[s]==WAR_S_GATE) continue;   // your gates are doors — to you
        float rr = (float)WAR_SRAD[S->war_stype[s]] + 13.0f;
        float dx=wx-S->war_sx[s], dy=wy-S->war_sy[s];
        if (dx*dx+dy*dy < rr*rr) return 1;
    }
    for (int t=0;t<S->war_tn;t++){
        if (S->war_thp[t]<=0) continue;
        float rr = (float)S->war_trad[t] + 13.0f;
        float dx=wx-S->war_tx[t], dy=wy-S->war_ty[t];
        if (dx*dx+dy*dy < rr*rr) return 1;
    }
    return 0;
}
static int war_pf_seg_blocked(BwSim *S, int side, float x0, float y0, float x1, float y1){
    float dx=x1-x0, dy=y1-y0, d=fsqrt(dx*dx+dy*dy);
    int steps = (int)(d/10.0f) + 1; if (steps>200) steps=200;
    for (int k=0;k<=steps;k++){
        float t = (float)k/(float)steps;
        if (war_pf_blocked_at(S, side, x0+dx*t, y0+dy*t)) return 1;
    }
    return 0;
}
static int war_pf_nearest_free(int cx, int cy, float wx, float wy){
    // the free cell whose CENTRE is closest to (wx,wy), searching
    // rings outward from (cx,cy); the first ring with any free cell
    // wins. Distance matters, not just ring order: a unit pressing a
    // wall stands in a blocked cell, and the first free cell in ring
    // order can be the one BEHIND the wall — its centre then becomes
    // the path's first waypoint, unreachable, and the unit jams
    // against the wall forever (found by wartest check 5: a crawler
    // frozen at the wall face for 45s). The nearest free centre is
    // on the unit's own side of the obstacle, where it can be
    // steered to, and the grid path leads on from there.
    for (int r=0;r<=4;r++){
        int best=-1; float bd=1e18f;
        for (int dy=-r; dy<=r; dy++) for (int dx=-r; dx<=r; dx++){
            if (r>0 && dx>-r && dx<r && dy>-r && dy<r) continue;   // the ring only, never the filled square
            int nx=cx+dx, ny=cy+dy;
            if (nx<0 || ny<0 || nx>=war_pf_gw || ny>=war_pf_gh) continue;
            if (war_pf_block[ny*WAR_FOG_W+nx]) continue;
            float ccx = WAR_OX + nx*40.0f + 20.0f, ccy = WAR_OY + ny*40.0f + 20.0f;
            float ddx = ccx-wx, ddy = ccy-wy, d2 = ddx*ddx + ddy*ddy;
            if (d2<bd){ bd=d2; best = ny*WAR_FOG_W+nx; }
        }
        if (best>=0) return best;
    }
    return -1;
}
static int war_pf_find(BwSim *S, int i, float tx, float ty){
    int side = war_u_side(S->war_utype[i]);
    war_pf_gw = S->war_gw; war_pf_gh = S->war_gh;
    for (int cy=0; cy<WAR_FOG_H; cy++) for (int cx=0; cx<WAR_FOG_W; cx++)
        war_pf_block[cy*WAR_FOG_W+cx] = (cx>=war_pf_gw || cy>=war_pf_gh)? 1 : (unsigned char)war_pf_blocked_at(S, side, WAR_OX + cx*40.0f + 20.0f, WAR_OY + cy*40.0f + 20.0f);
    int start = war_pf_nearest_free((int)((S->war_ux[i]-WAR_OX)/40.0f), (int)((S->war_uy[i]-WAR_OY)/40.0f), S->war_ux[i], S->war_uy[i]);
    int goal = war_pf_nearest_free((int)((tx-WAR_OX)/40.0f), (int)((ty-WAR_OY)/40.0f), tx, ty);
    if (start<0 || goal<0) return 0;
    int gx = goal%WAR_FOG_W, gy = goal/WAR_FOG_W;
    for (int c=0;c<WAR_FOG_W*WAR_FOG_H;c++){ war_pf_g[c]=-1; war_pf_from[c]=-1; war_pf_closed[c]=0; }
    war_pf_g[start]=0;
    int found = (start==goal), expansions = 0;
    while (!found){
        int best=-1, bf=0;
        for (int c=0;c<WAR_FOG_W*WAR_FOG_H;c++){
            if (war_pf_g[c]<0 || war_pf_closed[c]) continue;
            int cx=c%WAR_FOG_W, cy=c/WAR_FOG_W;
            int adx = cx>gx? cx-gx : gx-cx, ady = cy>gy? cy-gy : gy-cy;
            int f = war_pf_g[c] + 10*(adx>ady? adx:ady) + 4*(adx<ady? adx:ady);   // octile heuristic
            if (best<0 || f<bf){ best=c; bf=f; }
        }
        if (best<0) break;                    // the frontier ran dry: unreachable
        if (++expansions > 4000) break;       // bounded, always
        war_pf_closed[best]=1;
        if (best==goal){ found=1; break; }
        int bx=best%WAR_FOG_W, by=best/WAR_FOG_W;
        for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++){
            if (!dx && !dy) continue;
            int nx=bx+dx, ny=by+dy;
            if (nx<0 || ny<0 || nx>=war_pf_gw || ny>=war_pf_gh) continue;
            int nc = ny*WAR_FOG_W+nx;
            if (war_pf_block[nc] || war_pf_closed[nc]) continue;
            if (dx && dy && (war_pf_block[by*WAR_FOG_W+nx] || war_pf_block[ny*WAR_FOG_W+bx])) continue;   // no corner cutting
            int ng = war_pf_g[best] + ((dx && dy)? 14 : 10);
            if (war_pf_g[nc]<0 || ng<war_pf_g[nc]){ war_pf_g[nc]=ng; war_pf_from[nc]=best; }
        }
    }
    if (!found) return 0;
    // the came-from chain, goal-first; every step strictly descends
    // the g-score, so the walk is bounded by the chain itself
    int pn=0, c=goal;
    while (c>=0 && pn<WAR_FOG_W*WAR_FOG_H){ war_pf_path[pn++]=c; if (c==start) break; c=war_pf_from[c]; }
    if (pn<1 || war_pf_path[pn-1]!=start) return 0;
    // turning points only: walk in travel order, keep a cell when the
    // direction of travel changes (plus the first and last cells)
    int tn=0, pdx=0, pdy=0;
    for (int k=pn-1; k>=0; k--){
        int cx=war_pf_path[k]%WAR_FOG_W, cy=war_pf_path[k]/WAR_FOG_W;
        int dx=0, dy=0;
        if (k>0){ dx = war_pf_path[k-1]%WAR_FOG_W - cx; dy = war_pf_path[k-1]/WAR_FOG_W - cy; }
        if ((k==pn-1 || k==0 || dx!=pdx || dy!=pdy) && tn<128){
            war_pf_tpx[tn]=WAR_OX+cx*40.0f+20.0f; war_pf_tpy[tn]=WAR_OY+cy*40.0f+20.0f; tn++;
        }
        pdx=dx; pdy=dy;
    }
    // LOS shortcut: from wherever we stand, jump to the farthest
    // turning point with a clear segment (attempts capped)
    int wn=0, idx=0; float ax=S->war_ux[i], ay=S->war_uy[i];
    while (idx<tn && wn<31){
        int jump=idx, tries=0;
        for (int k=tn-1; k>idx && tries<24; k--, tries++)
            if (!war_pf_seg_blocked(S, side, ax, ay, war_pf_tpx[k], war_pf_tpy[k])){ jump=k; break; }
        S->war_wpx[i][wn]=war_pf_tpx[jump]; S->war_wpy[i][wn]=war_pf_tpy[jump]; wn++;
        ax=war_pf_tpx[jump]; ay=war_pf_tpy[jump]; idx=jump+1;
    }
    if (wn<1) return 0;
    S->war_wpn[i]=(unsigned char)wn; S->war_wpi[i]=0;
    S->war_ptgx[i]=tx; S->war_ptgy[i]=ty;
    return 1;
}
// one stall window: 1 when a full 0.6s passed with <3u of net
// movement — the unit is pressing against the world, not travelling
static int war_pf_stall_tick(BwSim *S, int i, float dt){
    S->war_stallt[i] += dt;
    if (S->war_stallt[i] < 0.6f) return 0;
    float mdx=S->war_ux[i]-S->war_stallx[i], mdy=S->war_uy[i]-S->war_stally[i];
    int stuck = (mdx*mdx + mdy*mdy < 9.0f);
    S->war_stallt[i]=0; S->war_stallx[i]=S->war_ux[i]; S->war_stally[i]=S->war_uy[i];
    return stuck;
}
static float war_move_to(BwSim *S, int i, float tx, float ty, float stopd, float dt){
    float dx=tx-S->war_ux[i], dy=ty-S->war_uy[i];
    float d=fsqrt(dx*dx+dy*dy);
    if (S->war_repath[i] > 0) S->war_repath[i] -= dt;
    if (S->war_wpn[i]>0){
        float pdx=tx-S->war_ptgx[i], pdy=ty-S->war_ptgy[i];
        if (pdx*pdx+pdy*pdy > 24.0f*24.0f) S->war_wpn[i]=0;   // the target JUMPED since last frame (a new order, a dead target replaced): the path is stale intent
        else { S->war_ptgx[i]=tx; S->war_ptgy[i]=ty; }        // a MOVING target (a unit under pursuit) re-anchors every frame: its step-by-step motion is the same intent, not staleness — a cumulative-drift rule here killed every pursuit path within a second (wartest check 5: the bait's ordinary charge-and-return oscillation, ±30u, dropped the bug's path mid-lunge three times in a row)
    }
    if (S->war_wpn[i]>0){
        if (war_pf_stall_tick(S, i, dt)) S->war_wpn[i]=0;   // the path itself jammed (a new wall, a crowd): drop it and re-think below
        else {
            while (S->war_wpi[i] < S->war_wpn[i]){
                float wx=S->war_wpx[i][S->war_wpi[i]], wy=S->war_wpy[i][S->war_wpi[i]];
                float wdx=wx-S->war_ux[i], wdy=wy-S->war_uy[i];
                if (wdx*wdx+wdy*wdy <= 100.0f){ S->war_wpi[i]++; continue; }
                war_move_step(S, i, wx, wy, 6.0f, dt);
                return d;
            }
            S->war_wpn[i]=0;   // waypoints exhausted: the last leg is a straight line at the real target, caller's stopd intact
        }
    }
    if (d > stopd + 1.0f){
        if (war_pf_stall_tick(S, i, dt) && S->war_repath[i] <= 0.0f)
            S->war_repath[i] = war_pf_find(S, i, tx, ty)? 1.5f : 2.0f;   // no route (a walled-in Core): keep pressing, ask again later
    } else { S->war_stallt[i]=0; S->war_stallx[i]=S->war_ux[i]; S->war_stally[i]=S->war_uy[i]; }   // standing to fire never accumulates stall
    return war_move_step(S, i, tx, ty, stopd, dt);
}
static void war_harv_update(int pi, int i, float dt){
    BwSim *S = &BWS[pi];
    int st = S->war_ustate[i];
    float x = S->war_ux[i], y = S->war_uy[i];
    if (st==1){   // a manual move order: go there, then think again
        if (war_move_to(S, i, S->war_utx[i], S->war_uty[i], 4.0f, dt) <= 6.0f) S->war_ustate[i]=0;
        return;
    }
    if (st==3){   // parked on a node, scooping bits
        S->war_uharv[i] -= dt;
        if (S->war_uharv[i] <= 0){
            int n = S->war_utarget[i]-1;   // harvesters store the node index+1 in utarget
            if (n>=0 && n<S->war_nn && S->war_namt[n]>0){
                int take = WAR_CARRY - S->war_ucarry[i];
                if (take > S->war_namt[n]) take = S->war_namt[n];
                S->war_namt[n] -= take; S->war_ucarry[i] += take;
                if (take>0) war_fx_spawn(S, x, y-6, 1);   // cosmetic: the scoop sparkle
            }
            S->war_ustate[i] = 0;
        }
        return;
    }
    if (S->war_ucarry[i] >= WAR_CARRY || st==4){   // full (or already homeward): dump it at the Core
        int hq = war_find_hq(S);
        if (hq<0 || !war_struct_alive(S, hq)){ S->war_ustate[i]=0; return; }
        float d = war_move_to(S, i, S->war_sx[hq], S->war_sy[hq], WAR_SRAD[WAR_S_HQ]+8.0f, dt);
        if (d <= WAR_SRAD[WAR_S_HQ]+16.0f){
            if (S->war_ucarry[i]>0){ S->war_bits += S->war_ucarry[i]; S->war_ucarry[i]=0; sfx(6); war_fx_spawn(S, x, y-6, 1); }   // cosmetic: the deposit sparkle
            S->war_ustate[i]=0;
        } else S->war_ustate[i]=4;
        return;
    }
    // find the nearest node with bits left and go scoop
    { int bn=-1; float bd=1e18f;
      for (int n=0;n<S->war_nn;n++){
          if (S->war_namt[n]<=0) continue;
          float dx=S->war_nx[n]-x, dy=S->war_ny[n]-y, d2=dx*dx+dy*dy;
          if (d2<bd){ bd=d2; bn=n; }
      }
      if (bn<0){   // the field is dry: hand over what's carried, then wait by the Core
          if (S->war_ucarry[i]>0){ S->war_ustate[i]=4; return; }
          int hq = war_find_hq(S);
          if (hq>=0) war_move_to(S, i, S->war_sx[hq]+48, S->war_sy[hq]+34, 6.0f, dt);
          return;
      }
      if (war_move_to(S, i, S->war_nx[bn], S->war_ny[bn], 14.0f, dt) <= 20.0f){
          S->war_ustate[i]=3; S->war_uharv[i]=2.0f; S->war_utarget[i]=bn+1;
      } }
}
// v1.21: the BUG HARVESTER — the hive's half of the economy, the
// player's harvester loop mirrored exactly: same shared nodes, same
// 2s scoop, same WAR_CARRY load, hauled home and deposited — into
// its own hive's purse instead of yours. It has no weapon and never
// will: its states are the harvester's own (0 think / 3 scooping /
// 4 homeward), and what it carries can be raided out of it only by
// killing it, exactly like yours.
static void war_bharv_update(int pi, int i, float dt){
    BwSim *S = &BWS[pi];
    int st = S->war_ustate[i];
    float x = S->war_ux[i], y = S->war_uy[i];
    if (st==3){   // parked on a node, scooping bits
        S->war_uharv[i] -= dt;
        if (S->war_uharv[i] <= 0){
            int n = S->war_utarget[i]-1;   // the node index+1 rides in utarget, as with yours
            if (n>=0 && n<S->war_nn && S->war_namt[n]>0){
                int take = WAR_CARRY - S->war_ucarry[i];
                if (take > S->war_namt[n]) take = S->war_namt[n];
                S->war_namt[n] -= take; S->war_ucarry[i] += take;
                if (take>0) war_fx_spawn(S, x, y-6, 1);   // cosmetic: the scoop sparkle
            }
            S->war_ustate[i] = 0;
        }
        return;
    }
    if (S->war_ucarry[i] >= WAR_CARRY || st==4){   // full (or already homeward): haul it to the home hive
        int h = war_home_hive(S, x, y);
        if (h<0){ S->war_ustate[i]=0; return; }   // no hive left to pay — the battle is already decided
        float d = war_move_to(S, i, S->war_sx[h], S->war_sy[h], WAR_SRAD[WAR_S_HIVE]+8.0f, dt);
        if (d <= WAR_SRAD[WAR_S_HIVE]+16.0f){
            if (S->war_ucarry[i]>0){ S->war_hbits[h] += S->war_ucarry[i]; S->war_ucarry[i]=0; war_fx_spawn(S, x, y-6, 1); }   // cosmetic: the deposit sparkle (no sfx — the hive works in silence)
            S->war_ustate[i]=0;
        } else S->war_ustate[i]=4;
        return;
    }
    // find the nearest node with bits left and go scoop — the SAME
    // pool the player draws from: a node the hive drains is gone
    { int bn=-1; float bd=1e18f;
      for (int n=0;n<S->war_nn;n++){
          if (S->war_namt[n]<=0) continue;
          float dx=S->war_nx[n]-x, dy=S->war_ny[n]-y, d2=dx*dx+dy*dy;
          if (d2<bd){ bd=d2; bn=n; }
      }
      if (bn<0){   // the field is dry: hand over what's carried, then wait by the hive
          if (S->war_ucarry[i]>0){ S->war_ustate[i]=4; return; }
          int h = war_home_hive(S, x, y);
          if (h>=0) war_move_to(S, i, S->war_sx[h]+52, S->war_sy[h]+38, 6.0f, dt);
          return;
      }
      if (war_move_to(S, i, S->war_nx[bn], S->war_ny[bn], 14.0f, dt) <= 20.0f){
          S->war_ustate[i]=3; S->war_uharv[i]=2.0f; S->war_utarget[i]=bn+1;
      } }
}
// v1.19 addendum: the PATCHER — repair infantry. It has no
// weapon and never will: its whole update is walk-to-work and
// channel. A player order owns it while it runs (a move order
// walks, a repair order fixes the named structure); with no
// order in progress it AUTOSEEKS — about twice a second it looks
// for the nearest damaged friendly structure anywhere on the
// field and goes to fix it (Daniel: "I want it to autoseek").
// Channel: +12 HP/s, billed at 1 bit per 4 HP — fractional bits
// accumulate in war_urepacc and only whole bits leave the purse.
// It stops when the structure is full, when the purse runs dry
// mid-bill (the OUT OF BITS note fires once per emptying), when
// another order lands, or when it dies.
static void war_patch_update(int pi, int i, float dt){
    BwSim *S = &BWS[pi];
    (void)pi;   // the channel bills the shared purse and heals through S directly — no per-player call-outs
    if (S->war_bits > 0) S->war_repnoted = 0;   // the purse recovered: the next emptying may call out again
    if (S->war_ustate[i]==1){   // a plain move order owns the unit until it arrives; autoseek resumes after
        if (war_move_to(S, i, S->war_utx[i], S->war_uty[i], 4.0f, dt) <= 6.0f) S->war_ustate[i]=0;
        return;
    }
    int s = S->war_urepair[i]-1;
    if (s>=0 && (!war_struct_alive(S, s) || S->war_sside[s]!=0 || S->war_shp[s] >= (float)S->war_smaxhp[s])){
        S->war_urepair[i]=0; s=-1;   // the work is gone or done — fall back to autoseek
    }
    if (s<0){
        S->war_urepscan[i] -= dt;
        if (S->war_urepscan[i] <= 0){
            S->war_urepscan[i] = 0.5f;
            if (S->war_bits > 0){   // an empty purse means wait, not wander (the note already fired at the halt)
                float bd = 1e18f; int bs = -1;
                for (int t=0;t<WAR_MAXS;t++){
                    if (S->war_shp[t]<=0 || S->war_sside[t]!=0) continue;
                    if (S->war_shp[t] >= (float)S->war_smaxhp[t]) continue;
                    float dx=S->war_sx[t]-S->war_ux[i], dy=S->war_sy[t]-S->war_uy[i], d2=dx*dx+dy*dy;
                    if (d2<bd){ bd=d2; bs=t; }
                }
                if (bs>=0){ S->war_urepair[i]=bs+1; s=bs; }
            }
        }
        if (s<0) return;
    }
    float rr = (float)WAR_SRAD[S->war_stype[s]] + 14.0f;
    if (war_move_to(S, i, S->war_sx[s], S->war_sy[s], rr, dt) > rr + 2.0f) return;   // still walking to the work
    float room = (float)S->war_smaxhp[s] - S->war_shp[s];
    float heal = 12.0f*dt; if (heal > room) heal = room;
    S->war_urepacc[i] += heal*0.25f;
    int pay = (int)S->war_urepacc[i];
    if (pay > S->war_bits) pay = S->war_bits;
    if (pay>0){ S->war_bits -= pay; S->war_urepacc[i] -= (float)pay; }
    if (S->war_bits<=0 && S->war_urepacc[i] >= 1.0f){
        // a whole bit is owed and the purse is empty: halt here
        S->war_urepair[i]=0; S->war_urepacc[i]=0;
        if (!S->war_repnoted){ S->war_repnoted=1; war_note(S, "OUT OF BITS - REPAIRS HALTED."); }
        return;
    }
    S->war_shp[s] += heal;
    if (S->war_shp[s] >= (float)S->war_smaxhp[s]){ S->war_shp[s] = (float)S->war_smaxhp[s]; S->war_urepair[i]=0; }
}
static void war_unit_update(int pi, int i, float dt){
    BwSim *S = &BWS[pi];
    int type = S->war_utype[i], side = war_u_side(type);
    if (S->war_uatk[i] > 0) S->war_uatk[i] -= dt;
    if (type == WAR_U_HARV){ war_harv_update(pi, i, dt); return; }
    if (type == WAR_U_BHARV){ war_bharv_update(pi, i, dt); return; }
    if (type == WAR_U_PATCH){ war_patch_update(pi, i, dt); return; }
    float x = S->war_ux[i], y = S->war_uy[i];
    int state = S->war_ustate[i];
    if (state==2){   // an attack order whose target died is just an order again
        float tx, ty;
        if (!war_ref_alive(S, S->war_utarget[i], &tx, &ty)){ S->war_ustate[i]=0; S->war_utarget[i]=0; state=0; }
    }
    int tref = 0;
    if (state==2) tref = S->war_utarget[i];
    else if (side==0){   // a buddy on guard: bugs first, a hive only if it is right there
        float aggro = state==1? 80.0f : 150.0f;
        tref = war_nearest_enemy(S, 0, x, y, aggro, 1, 0);
        if (!tref && state==0) tref = war_nearest_enemy(S, 0, x, y, 70.0f, 0, 1);
    } else if (state==1){   // a wave bug: whatever it meets, then the base
        tref = war_nearest_enemy(S, 1, x, y, 170.0f, 1, 0);
        if (!tref) tref = war_nearest_enemy(S, 1, x, y, 210.0f, 0, 1);
    } else {   // a hive guard: anything of yours that wanders close
        tref = war_nearest_enemy(S, 1, x, y, 180.0f, 1, 1);
    }
    if (tref){
        float tx, ty;
        war_ref_alive(S, tref, &tx, &ty);
        float dx=tx-x, dy=ty-y, dist=fsqrt(dx*dx+dy*dy);
        if (side==0 && state==0 && dist > 260.0f) tref = 0;   // guards don't chase across the map
        else {
            float rng = war_u_range(type) + war_ref_radius(S, tref);
            float d = war_move_to(S, i, tx, ty, rng*0.92f, dt);
            if (d <= rng + 2.0f && S->war_uatk[i] <= 0){
                S->war_uatk[i] = war_u_rate(type);
                int dmg = war_u_dmg(type, S->war_region);
                int ft = war_ref_feat(tref);
                // the siege gun is built to crack STRUCTURES — hives
                // above all. Debris is not a structure: trees, rocks
                // and rubble take the plain rate, never the x2.
                if (type==WAR_U_SIEGE && tref<0 && ft<0) dmg *= 2;
                if (type==WAR_U_MELEE || type==WAR_U_CRAWL){
                    if (tref>0) war_hurt_unit(pi, tref-1, dmg);
                    else if (ft>=0) war_hurt_terrain(pi, ft, dmg, side);
                    else war_hurt_struct(pi, -tref-1, dmg);
                } else war_fire(S, side, x, y, tref, dmg);
            }
            return;
        }
    }
    if (state==1 && side==0){   // a plain move order
        if (war_move_to(S, i, S->war_utx[i], S->war_uty[i], 4.0f, dt) <= 6.0f) S->war_ustate[i]=0;
        return;
    }
    if (side==1 && state==1){   // wave bug, nothing in the way: march on the Core
        int hq = war_find_hq(S);
        if (hq>=0 && war_struct_alive(S, hq)) war_move_to(S, i, S->war_sx[hq], S->war_sy[hq], WAR_SRAD[WAR_S_HQ]+10.0f, dt);
        return;
    }
    if (side==1 && state==0){   // guard bug: drift back home if it strayed
        float dx=S->war_utx[i]-x, dy=S->war_uty[i]-y;
        if (dx*dx+dy*dy > 60.0f*60.0f) war_move_to(S, i, S->war_utx[i], S->war_uty[i], 42.0f, dt);
        return;
    }
}
static void war_struct_update(int pi, int s, float dt){
    BwSim *S = &BWS[pi];
    if (S->war_scd[s] > 0) S->war_scd[s] -= dt;
    int type = S->war_stype[s];
    if (type==WAR_S_ZAP){
        if (S->war_scd[s] <= 0){
            int tref = war_nearest_enemy(S, 0, S->war_sx[s], S->war_sy[s], 118.0f, 1, 0);
            if (tref){ war_fire(S, 0, S->war_sx[s], S->war_sy[s]-10, tref, 8); S->war_scd[s]=0.55f; }
        }
    } else if (type==WAR_S_HIVE){
        // v1.21: the hive spends NOTHING between waves. The old
        // free defender drip is gone — guards are bought at wave
        // time out of the purse, from what's left after the wave
        // buy, like everything else the hive fields. (The starting
        // garrison still arrives free at battle build.)
    }
    if (S->war_qlen[s] > 0){   // the training QUEUE (addendum 2): the front item cooks on war_qt, the rest wait their turn
        S->war_qt[s] -= dt;
        if (S->war_qt[s] <= 0){
            if (war_count_units(S, 0) < 24){
                int ut = S->war_queue[s][0];
                float a = (float)((s*47) % 6) * 1.1f;
                int ni = war_spawn_unit(S, ut, S->war_sx[s] + fcos(a)*(WAR_SRAD[type]+16),
                                              S->war_sy[s] + fsin(a)*(WAR_SRAD[type]+16));
                if (ni>=0 && S->war_rallyx[s]!=0){   // rally (addendum 3): fresh units walk to the flag — harvesters too; on arrival they think for themselves
                    S->war_ustate[ni]=1; S->war_utarget[ni]=0;
                    S->war_utx[ni]=S->war_rallyx[s]; S->war_uty[ni]=S->war_rallyy[s];
                    S->war_wpn[ni]=0;
                }
                for (int k=1;k<S->war_qlen[s];k++) S->war_queue[s][k-1]=S->war_queue[s][k];
                S->war_qlen[s]--;
                if (S->war_qlen[s]>0) S->war_qt[s]=WAR_TRAIN_TIME[S->war_queue[s][0]];
                if (pi==G.my_id) sfx(19);
            } else S->war_qt[s] = 0.5f;   // army full: the front item waits at the door, and the queue behind it waits too
        }
    }
}
static void war_separate(BwSim *S){
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0) continue;
        for (int j=i+1;j<WAR_MAXU;j++){
            if (S->war_uhp[j]<=0) continue;
            float dx=S->war_ux[j]-S->war_ux[i], dy=S->war_uy[j]-S->war_uy[i];
            float d2=dx*dx+dy*dy;
            if (d2 < 15.0f*15.0f && d2 > 0.01f){
                float d=fsqrt(d2), push=(15.0f-d)*0.5f, px=dx/d*push, py=dy/d*push;
                S->war_ux[i]-=px; S->war_uy[i]-=py; S->war_ux[j]+=px; S->war_uy[j]+=py;
            } else if (d2 <= 0.01f) S->war_ux[j] += 1.5f;
        }
        for (int s=0;s<WAR_MAXS;s++){
            if (S->war_shp[s]<=0) continue;
            if (S->war_stype[s]==WAR_S_GATE && war_u_side(S->war_utype[i])==0) continue;   // gates (addendum 1): YOUR units walk straight through; bugs still hit a wall
            float rad = (float)WAR_SRAD[S->war_stype[s]] + 9.0f;
            float dx=S->war_ux[i]-S->war_sx[s], dy=S->war_uy[i]-S->war_sy[s];
            float d2=dx*dx+dy*dy;
            if (d2 < rad*rad){
                float d=fsqrt(d2);
                if (d<0.5f) S->war_ux[i] = S->war_sx[s]+rad;
                else { S->war_ux[i] = S->war_sx[s]+dx/d*rad; S->war_uy[i] = S->war_sy[s]+dy/d*rad; }
            }
        }
        for (int t=0;t<S->war_tn;t++){   // terrain blocks feet and wheels, never shots — and felled debris blocks nothing
            if (S->war_thp[t]<=0) continue;
            float rad = (float)S->war_trad[t] + 9.0f;
            float dx=S->war_ux[i]-S->war_tx[t], dy=S->war_uy[i]-S->war_ty[t];
            float d2=dx*dx+dy*dy;
            if (d2 < rad*rad){
                float d=fsqrt(d2);
                if (d<0.5f) S->war_ux[i] = S->war_tx[t]+rad;
                else { S->war_ux[i] = S->war_tx[t]+dx/d*rad; S->war_uy[i] = S->war_ty[t]+dy/d*rad; }
            }
        }
        if (S->war_ux[i] < WAR_OX+12) S->war_ux[i] = WAR_OX+12;
        if (S->war_ux[i] > WAR_OX+S->war_aw-12) S->war_ux[i] = WAR_OX+S->war_aw-12;
        if (S->war_uy[i] < WAR_OY+12) S->war_uy[i] = WAR_OY+12;
        if (S->war_uy[i] > WAR_OY+S->war_ah-12) S->war_uy[i] = WAR_OY+S->war_ah-12;
    }
}
static void war_proj_update(int pi, float dt){
    BwSim *S = &BWS[pi];
    for (int i=0;i<WAR_MAXP;i++){
        if (S->war_pt[i]<=0) continue;
        S->war_pt[i] -= dt;
        float tx, ty;
        if (!war_ref_alive(S, S->war_ptar[i], &tx, &ty)){ S->war_pt[i]=0; continue; }
        float dx=tx-S->war_px[i], dy=ty-S->war_py[i], d=fsqrt(dx*dx+dy*dy);
        float sp = S->war_pside[i]==0? 320.0f : 250.0f;
        if (d <= sp*dt + 7.0f){
            int ft = war_ref_feat(S->war_ptar[i]);
            if (S->war_ptar[i]>0) war_hurt_unit(pi, S->war_ptar[i]-1, S->war_pdmg[i]);
            else if (ft>=0) war_hurt_terrain(pi, ft, S->war_pdmg[i], S->war_pside[i]);
            else war_hurt_struct(pi, -S->war_ptar[i]-1, S->war_pdmg[i]);
            S->war_pt[i]=0;
        } else { S->war_px[i] += dx/d*sp*dt; S->war_py[i] += dy/d*sp*dt; }
    }
}
static float war_wave_dt(int region){ float v = 30.0f - 2.0f*region; return v<16.0f? 16.0f : v; }
// v1.19: eleven nodes, 8,200 bits on a campaign field (was nine /
// 4,800) — (560,600) and (1180,600) join the midfield: the centre
// is contested ground now, not just a road between two halves.
// The skirmish sizes draw from this same table (SMALL takes a
// scaled subset; LARGE adds two corner nodes of its own).
static const float WAR_ND[11][2] = { {430,430},{430,770},{800,300},{800,900},{800,600},{1180,350},{1180,850},{620,120},{1010,600},{560,600},{1180,600} };
static const int WAR_NA[11] = { 550, 550, 700, 700, 850, 850, 850, 700, 950, 600, 900 };
static void war_place_nodes(BwSim *S, const float (*nd)[2], const int *na, int nn, float sx, float sy){
    S->war_nn = nn;
    for (int n=0;n<nn;n++){ S->war_nx[n]=WAR_OX+nd[n][0]*sx; S->war_ny[n]=WAR_OY+nd[n][1]*sy; S->war_namt[n]=na[n]; }
}
// v1.27: NODE JITTER (Daniel: "needs more randomization") —
// war_place_nodes lays the fixed table out before the battle
// exists; once the Core and the hives stand (war_build_battle),
// every node rolls off its table spot by a bseed-derived offset
// (±90 x, ±70 y, clamped 60 inside the arena) and its amount by
// ±20%. A candidate spot must clear the Core by 130, every hive
// by 110 and every other node by 120 — 24 bounded tries, then the
// node keeps its table spot (its amount is still rolled). The
// battle snapshot serializes the JITTERED nodes (the wg key) and
// restore never re-runs this, so a parked battle returns with
// exactly the field it left.
static void war_jitter_nodes(BwSim *S){
    float corex = WAR_OX+280.0f, corey = WAR_OY+S->war_ah*0.5f;
    for (int n=0;n<S->war_nn;n++){
        int amt = (int)((float)S->war_namt[n] * (0.8f + hash_f(n, 103, S->war_bseed)*0.4f) + 0.5f);
        S->war_namt[n] = amt<1? 1 : amt;
        float bx = S->war_nx[n], by = S->war_ny[n];
        for (int t=0;t<24;t++){
            float x = bx + (hash_f(n*31+t, 101, S->war_bseed)*2.0f - 1.0f) * 90.0f;
            float y = by + (hash_f(n*31+t, 102, S->war_bseed)*2.0f - 1.0f) * 70.0f;
            if (x < WAR_OX+60.0f) x = WAR_OX+60.0f;
            if (x > WAR_OX+S->war_aw-60.0f) x = WAR_OX+S->war_aw-60.0f;
            if (y < WAR_OY+60.0f) y = WAR_OY+60.0f;
            if (y > WAR_OY+S->war_ah-60.0f) y = WAR_OY+S->war_ah-60.0f;
            float dx = x-corex, dy = y-corey;
            if (dx*dx+dy*dy < 130.0f*130.0f) continue;   // the Core's surroundings stay clear
            int bad = 0;
            for (int s=0;s<WAR_MAXS && !bad;s++){
                if (S->war_shp[s]<=0 || S->war_stype[s]!=WAR_S_HIVE) continue;
                dx = x-S->war_sx[s]; dy = y-S->war_sy[s]; if (dx*dx+dy*dy < 110.0f*110.0f) bad = 1;
            }
            for (int m=0;m<S->war_nn && !bad;m++){
                if (m==n) continue;
                dx = x-S->war_nx[m]; dy = y-S->war_ny[m]; if (dx*dx+dy*dy < 120.0f*120.0f) bad = 1;
            }
            if (bad) continue;
            S->war_nx[n]=x; S->war_ny[n]=y;
            break;
        }
    }
}
// the shared battle build: seed, purse, Core, starting units,
// hives (banded), terrain, fog, camera, banner. The caller has
// already cleared the field, set the field geometry (war_aw /
// war_ah / gw / gh / lines / skirmish) and placed the bit nodes —
// terrain generation tests against the nodes and hives, so the
// nodes must be down before this runs.
static void war_build_battle(int pi, int region, int bits, int nh){
    BwSim *S = &BWS[pi];
    // v1.18 (Daniel: "novel every time you play"): this battle's
    // seed mixes the region, the run clock and a per-process
    // battle counter — RESTARTs included — through the unsigned
    // hash, so no two starts ever lay out the same field. The seed
    // itself is never saved: hives are structs and the terrain
    // rides the battle snapshot, so a parked battle restores its
    // own rolled field exactly.
    { static u32 war_battle_ctr = 0;
      war_battle_ctr += 1u;
      u32 tbits = (u32)(int)(G.time * 4.0f);
      S->war_bseed = hash2((i32)(tbits + (u32)region * 977u), (i32)(war_battle_ctr * 131u + (u32)region), 0x5EEDu ^ (war_battle_ctr * 2654435761u)); }
    if (S->war_seedforce) S->war_bseed = (u32)S->war_seedforce;   // test hook (dbg_warseed): a pinned seed lays out the same field every battle — wartest only; zero in shipped play, so the roll above is untouched there
    S->war_state=1; S->war_region=region; S->war_bits=bits; S->war_t=0;
    // v1.19 addendum (Daniel): the FIRST wave is paced — at least
    // 45s of breathing room before the hive sends anything, so a
    // fresh commander can get the economy standing. Only the
    // first: the wave-spawn reset keeps the normal interval, and a
    // restored parked battle keeps its saved war_wavet.
    { float fw = war_wave_dt(region)*1.5f; S->war_wavet = fw<45.0f? 45.0f : fw; }
    S->war_grace=0.35f; S->war_ordt=0; S->war_msgt=0;
    S->war_ltap_t=-10.0f; S->war_ltap_unit=-1; S->war_drag_id=-1;
    float corey = S->war_ah*0.5f;   // the Core anchors the west edge at mid-height on any field
    war_spawn_struct(S, WAR_S_HQ, 0, WAR_OX+280, WAR_OY+corey);       // struct 0 is always the Core
    war_spawn_unit(S, WAR_U_HARV, WAR_OX+336, WAR_OY+corey+52.0f);
    war_spawn_unit(S, WAR_U_MELEE, WAR_OX+336, WAR_OY+corey-52.0f);
    // v1.18: the hives land along the FAR side, banded-random — the
    // field height is cut into nh equal bands and each hive rolls
    // its spot inside its own band (x deep in the east, y anywhere
    // in the band), so the swarm is always spread out but never
    // where it stood last time. No more "just go to the far right".
    // (At the campaign's 1600x1200: x 1240..1460, margin 130 —
    // v1.27 widened the x roll from 150 to 220.)
    { float band_h = S->war_ah / (float)nh;
      float hmargin = S->war_ah*(130.0f/1200.0f);
      for (int h=0;h<nh;h++){
          float hx = (S->war_aw-360.0f) + hash_f(h, 11, S->war_bseed) * 220.0f;
          float hy = (float)h * band_h + hmargin + hash_f(h, 12, S->war_bseed) * (band_h - 2.0f*hmargin);
          int hi = war_spawn_struct(S, WAR_S_HIVE, 1, WAR_OX+hx, WAR_OY+hy);
          // v1.27: garrison jitter — 2-3 guards by hash (was a
          // flat 2); the region>=2 bonus bug still stacks on top
          int guards = 2 + (hash_f(h, 41, S->war_bseed) < 0.5f? 1 : 0);
          for (int g=0;g<guards;g++) war_spawn_bug(S, hi, 0);
          if (region>=2) war_spawn_bug(S, hi, 0);
          if (hi>=0){   // v1.21: every hive opens its purse and fields its workforce — two bug harvesters, free at the start like the garrison
              S->war_hbits[hi] = war_hive_start_bits(S);
              war_spawn_bharv(S, hi); war_spawn_bharv(S, hi);
          }
      } }
    war_jitter_nodes(S);   // v1.27: nodes roll off the table BEFORE terrain generation reads them for clearances
    war_gen_terrain(S, S->war_bseed);
    for (int i=0;i<WAR_FOG_W*WAR_FOG_H;i++) S->war_fog[i]=0;   // a fresh battle starts in the dark
    war_fog_update(S);
    S->war_camx = WAR_OX+280; S->war_camy = WAR_OY+corey;
    war_clamp_cam(S);
    if (S->war_skirmish){
        static const char *SKN[4] = { "", "SMALL", "STANDARD", "LARGE" };
        static const char *SKD[4] = { "", "EASY", "NORMAL", "HARD" };
        char buf[72]; char *q=buf; q=sappend(q, "SKIRMISH - "); q=sappend(q, SKN[S->war_size]); q=sappend(q, " FIELD, "); q=sappend(q, SKD[S->war_diff]); q=sappend(q, " ENEMY. KILL THE HIVE"); if (nh>1) q=sappend(q, "S"); q=sappend(q, "."); *q=0;
        war_note(S, buf);
    } else {
        char buf[72]; char *q=buf; q=sappend(q, "CONQUER "); q=sappend(q, REGION_NAME[region]);
          q=sappend(q, "! HARVEST BITS. KILL THE HIVE"); if (nh>1) q=sappend(q, "S"); q=sappend(q, "."); *q=0;
          war_note(S, buf);
    }
    sfx(19);
}
static void war_start_battle(int pi, int region){
    BwSim *S = &BWS[pi];
    if (region<0) region=0; if (region>7) region=7;
    war_clear_field(S);   // resets to campaign geometry, war_skirmish=0
    war_place_nodes(S, WAR_ND, WAR_NA, 11, 1.0f, 1.0f);
    int nh = 1 + (region>=3? 1:0) + (region>=6? 1:0);
    war_build_battle(pi, region, 300, nh);   // v1.19: 300 starting bits (was 250)
    war_link_pull(pi);   // feat 13: a launch pulls the other local commander in
}
// v1.19: a SKIRMISH — an ad-hoc battle on the player's picked
// size and difficulty. It never touches war_prog, is never
// parked, and never disturbs a parked campaign battle (nothing
// here serializes or clears a snapshot). The difficulty doubles
// as the battle's region equivalent: every region-keyed strength
// (unit/hive HP, wave strength and pace, hive count banding)
// reads it exactly as it would a campaign region.
static void war_start_skirmish(int pi){
    BwSim *S = &BWS[pi];
    int size = (S->war_size>=1 && S->war_size<=3)? S->war_size : 2;
    int diff = (S->war_diff>=1 && S->war_diff<=3)? S->war_diff : 2;
    S->war_size = size; S->war_diff = diff;
    war_clear_field(S);
    S->war_skirmish = 1;
    if (size==1){ S->war_aw=1200.0f; S->war_ah=900.0f; S->war_lines=2; }
    else if (size==3){ S->war_lines=4; }
    else { S->war_lines=3; }
    S->war_gw = (int)(S->war_aw/40.0f); S->war_gh = (int)(S->war_ah/40.0f);
    int region = diff==1? 1 : diff==3? 6 : 3;   // EASY / NORMAL / HARD as region equivalents
    int bits = size==1? 250 : size==3? 400 : 300;
    if (size==1){
        // a subset of the standard table, scaled onto the small
        // field: both west nodes, the centre, both east nodes and
        // the east-midfield node — 4,250 bits
        static const int IDX[6] = { 0, 1, 4, 5, 6, 10 };
        float nd[6][2]; int na[6];
        for (int k=0;k<6;k++){ nd[k][0]=WAR_ND[IDX[k]][0]; nd[k][1]=WAR_ND[IDX[k]][1]; na[k]=WAR_NA[IDX[k]]; }
        war_place_nodes(S, (const float (*)[2])nd, na, 6, S->war_aw/1600.0f, S->war_ah/1200.0f);
    } else if (size==3){
        // the standard eleven plus two corner nodes — 9,500 bits,
        // a dense field
        float nd[13][2]; int na[13];
        for (int k=0;k<11;k++){ nd[k][0]=WAR_ND[k][0]; nd[k][1]=WAR_ND[k][1]; na[k]=WAR_NA[k]; }
        nd[11][0]=300.0f; nd[11][1]=150.0f; na[11]=500;
        nd[12][0]=1450.0f; nd[12][1]=900.0f; na[12]=800;
        war_place_nodes(S, (const float (*)[2])nd, na, 13, 1.0f, 1.0f);
    } else {
        war_place_nodes(S, WAR_ND, WAR_NA, 11, 1.0f, 1.0f);
    }
    int nh = diff;              // EASY 1 / NORMAL 2 / HARD 3 hives
    if (size==1 && nh>2) nh=2;  // a SMALL field caps at 2
    war_build_battle(pi, region, bits, nh);
    war_link_pull(pi);   // feat 13: a launch pulls the other local commander in
}
static float war_axis(float a);   // defined with the pad code below; the pause menu's pad handler needs it early
