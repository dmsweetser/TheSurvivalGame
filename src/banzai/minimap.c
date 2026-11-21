// THE SURVIVAL GAME — banzai/minimap.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 3261-3409 in the Nov 2026 fine-grained refactor; code unchanged.

// ---- minimap (Nov 3, Daniel: "desperately need a minimap in the
// story mode." A quest compass arrow shipped alongside and was cut the
// same day at his word — "too much of a gimme"; minimap only.)
// The current story objective, if any: main quests first, the hermit
// errand last; after the farm falls, Patchina hands out the funeral.
static int bw_quest_target(Buddy *b, float *tx, float *ty){
    if (b->bw_mode!=1 || !b->story_started) return 0;
    if (bw_qstate(b,0)==1){ *tx=BW_NPC_XY[0][0]; *ty=BW_NPC_XY[0][1]; return 1; }
    if (bw_qstate(b,1)==1){ *tx=BW_SITE_XY[1][0]; *ty=BW_SITE_XY[1][1]; return 1; }
    if (bw_qstate(b,1)==2 && bw_qstate(b,3)==0){ *tx=BW_NPC_XY[0][0]; *ty=BW_NPC_XY[0][1]; return 1; }
    if (bw_qstate_x(b,5)==1){ *tx=BW_SITE_XY[4][0]; *ty=BW_SITE_XY[4][1]; return 1; }
    if (bw_qstate(b,3)==1){ *tx=BW_SITE_XY[3][0]; *ty=BW_SITE_XY[3][1]; return 1; }
    if (bw_qstate_x(b,4)==1){
        int best=-1; float bd=1e18f;
        for (int i=0;i<3;i++){ if ((b->story_bits>>(24+i))&1) continue;
            float dx=BW_TOOL_XY[i][0]-b->x, dy=BW_TOOL_XY[i][1]-b->y, d=dx*dx+dy*dy;
            if (d<bd){ bd=d; best=i; } }
        if (best>=0){ *tx=BW_TOOL_XY[best][0]; *ty=BW_TOOL_XY[best][1]; return 1; }
        *tx=BW_NPC_XY[1][0]; *ty=BW_NPC_XY[1][1]; return 1;   // all found: back to Mop
    }
    if (bw_qstate2(b,7)==1){ *tx=BW_NPC_XY[2][0]; *ty=BW_NPC_XY[2][1]; return 1; }   // the parcel goes to Glitch
    if (bw_qstate2(b,6)==1){
        if (bw_lakekills(b)>=2){ *tx=BW_NPC_XY[2][0]; *ty=BW_NPC_XY[2][1]; }
        else { *tx=BW_SITE_XY[2][0]; *ty=BW_SITE_XY[2][1]; }
        return 1;
    }
    if (bw_qstate2(b,8)==1){ bw_bitling_pos(tx, ty); return 1; }
    if (bw_qstate(b,2)==1){ *tx=BW_NPC_XY[2][0]; *ty=BW_NPC_XY[2][1]; return 1; }
    if (bw_qstate2(b,9)==1){
        int best=-1; float bd=1e18f;
        for (int i=0;i<6;i++){ if ((b->story_bits2>>(11+i))&1) continue;
            float dx=BW_HSIGN_XY[i][0]-b->x, dy=BW_HSIGN_XY[i][1]-b->y, d=dx*dx+dy*dy;
            if (d<bd){ bd=d; best=i; } }
        if (best>=0){ *tx=BW_HSIGN_XY[best][0]; *ty=BW_HSIGN_XY[best][1]; return 1; }
    }
    // THE REBUILDING (feat 14): the chapter's errands point too —
    // after every main-story arm, since they can only be active
    // once the whole list above is done
    if (bw_qstate3(b,10)==1){ *tx=BW_SITE_XY[2][0]; *ty=BW_SITE_XY[2][1]; return 1; }   // the cider waits on the lake ice
    if (bw_qstate3(b,11)==1){
        BwSim *S = &BWS[b - G.bud];
        if (S->ch2_esc_on && bw_duster_dist(b) > 120){ *tx=S->ch2_esc_x; *ty=S->ch2_esc_y; return 1; }   // fetch Duster first
        *tx=BW_SITE_XY[1][0]; *ty=BW_SITE_XY[1][1]; return 1;   // then the farm
    }
    if (bw_qstate3(b,12)==1){ *tx=BW_SITE_XY[1][0]; *ty=BW_SITE_XY[1][1]; return 1; }   // the racks are rattling
    if (bw_qstate3(b,13)==1){ *tx=BW_NPC_XY[0][0]; *ty=BW_NPC_XY[0][1]; return 1; }   // home for Patch Day
    return 0;
}
static void bw_draw_minimap(int pi){
    Buddy *b=&G.bud[pi];
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW;
    const int MW=126, MH=98;
    int mx=vx+vw2-8-MW, my=vy+8;
    rect_fill(mx, my, MW, MH, COL(12,16,20)); rect_line(mx, my, MW, MH, COL(198,164,84));
    int px0=mx+7, py0=my+7, pw=MW-14, ph=MH-14;
    // The map rides with you (Nov 4, Daniel: "you can go off the map
    // lol"): a 3900x2580 window CENTRED on the buddy over an ocean
    // that runs to the world's hard edge. You are always the dot in
    // the middle; an objective beyond the window hugs its edge.
    const float WINW=3900.0f, WINH=2580.0f;
    float mox = b->x - WINW/2, moy = b->y - WINH/2;
    #define BW_MMX(wx) (px0 + (int)(((wx)-mox)*(float)pw/WINW))
    #define BW_MMY(wy) (py0 + (int)(((wy)-moy)*(float)ph/WINH))
    rect_fill(px0, py0, pw, ph, COL(32,80,136));   // ocean, everywhere the land is not
    { int lx0=BW_MMX(BW_WX0), ly0=BW_MMY(BW_WY0), lx1=BW_MMX(BW_WX1), ly1=BW_MMY(BW_WY1);
      int cx0 = lx0<px0? px0 : lx0, cy0 = ly0<py0? py0 : ly0;
      int cx1 = lx1>px0+pw? px0+pw : lx1, cy1 = ly1>py0+ph? py0+ph : ly1;
      if (cx1>cx0 && cy1>cy0) rect_fill(cx0, cy0, cx1-cx0, cy1-cy0, COL(52,96,44)); }
    if (b->bw_mode==1){
        static const u32 BSC[5] = { COL(232,196,120), COL(186,192,198), COL(170,220,245), COL(255,110,90), COL(170,180,120) };
        for (int i=0;i<5;i++){
            float wx=BW_SITE_XY[i][0], wy=BW_SITE_XY[i][1];
            if (wx<b->x-WINW/2 || wx>b->x+WINW/2 || wy<b->y-WINH/2 || wy>b->y+WINH/2) continue;
            disc_fill(BW_MMX(wx), BW_MMY(wy), 3, BSC[i]);
        }
    } else {
        if (0>=b->x-WINW/2 && 0<=b->x+WINW/2 && 0>=b->y-WINH/2 && 0<=b->y+WINH/2)
            disc_fill(BW_MMX(0), BW_MMY(0), 2, COL(240,248,240));   // the gate home
    }
    float tx, ty;
    if (bw_quest_target(b, &tx, &ty)){
        int sx=BW_MMX(tx), sy=BW_MMY(ty), r = fsin(G.anim_t*4.0f)>0? 5 : 4;
        if (sx<px0+5) sx=px0+5; if (sx>px0+pw-5) sx=px0+pw-5;
        if (sy<py0+5) sy=py0+5; if (sy>py0+ph-5) sy=py0+ph-5;
        tri_fill(sx-r, sy, sx, sy-r, sx+r, sy, COL(255,214,64));
        tri_fill(sx-r, sy, sx, sy+r, sx+r, sy, COL(255,214,64));
    }
    { int sx=BW_MMX(b->x), sy=BW_MMY(b->y); disc_fill(sx, sy, 3, OUTLINE); disc_fill(sx, sy, 2, COL(255,244,210)); }
    #undef BW_MMX
    #undef BW_MMY
}
// Story text lives in a dialogue box now (Nov 4, Daniel: "the text just
// shows up across the top, underneath the map even — have it be in a
// dialogue box with better contrast at the bottom"). Every note — NPC
// lines, quest updates, drops — used to be one bare line of pale text
// floating over the meadow at y+66. Now: an opaque dark box with a gold
// rim along the BOTTOM of the view, text word-wrapped to two lines,
// drawn after the minimap so nothing ever slides underneath it. The
// box sits exactly where the context-hint pill lives; the note wins
// while it is up. Battle text keeps its own box in the battle render.
static void bw_draw_note(int pi){
    BwSim *S = &BWS[pi];
    if (S->note_t<=0 || !S->note[0]) return;
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    int bw2 = vw2-16; if (bw2<120) bw2=vw2;
    int bx = vx + (vw2-bw2)/2, by = vy + vh2 - 48, bh = 44;
    rect_fill(bx, by, bw2, bh, COL(12,14,20)); rect_line(bx, by, bw2, bh, COL(198,164,84));
    int maxw = bw2 - 20;
    // word-wrap into at most two lines by measured width
    char l1[84], l2[84]; l1[0]=0; l2[0]=0; int line=1;
    const char *p = S->note;
    while (*p){
        while (*p==' ') p++;
        if (!*p) break;
        const char *w = p; while (*p && *p!=' ') p++;
        int wl = (int)(p-w);
        char word[40]; int wc = wl>38? 38 : wl; for (int i=0;i<wc;i++) word[i]=w[i]; word[wc]=0;
        for (int attempt=0; attempt<2; attempt++){
            char *cur = line==1? l1 : l2;
            char trial[88]; trial[0]=0;
            char *tp = sappend(trial, cur);
            if (*cur) tp = sappend(tp, " ");
            tp = sappend(tp, word); (void)tp;
            if (text_w(trial, 1) <= maxw || !*cur){ scpy(cur, trial, 84); break; }
            if (line==1){ line=2; continue; }
            // both lines full: the note is longer than the box (cannot
            // happen for the 72-char note buffer at full width)
        }
    }
    if (l2[0]){ draw_text(bx+10, by+8, l1, 1, COL(255,250,235)); draw_text(bx+10, by+24, l2, 1, COL(255,250,235)); }
    else draw_text(bx+10, by+16, l1, 1, COL(255,250,235));
}
// THE REBUILDING (feat 14): the cider run's countdown — the one
// piece of chapter HUD. A gold-rimmed pill at the top centre of
// the view (the minimap owns the corner, the note owns the
// bottom), drawn while a run is live and never otherwise.
static void bw_draw_ch2_hud(int pi){
    BwSim *S = &BWS[pi];
    Buddy *b=&G.bud[pi];
    if (!S->ch2_race_on || b->bw_mode!=1) return;
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW;
    int t = (int)(S->ch2_race_t + 0.999f); if (t<0) t=0;
    char buf[24]; char *q=buf; q=sappend(q, "CIDER RUN "); q=sapp_int(q, t/60); q=sappend(q, ":"); if (t%60<10) q=sappend(q, "0"); q=sapp_int(q, t%60); *q=0;
    int hw2 = text_w(buf, 2);
    rect_fill(vx + vw2/2 - hw2/2 - 8, vy + 10, hw2+16, 24, COL(12,14,20));
    rect_line(vx + vw2/2 - hw2/2 - 8, vy + 10, hw2+16, 24, COL(198,164,84));
    draw_text_center(vx + vw2/2, vy + 16, buf, 2, COL(255,214,64));
}

