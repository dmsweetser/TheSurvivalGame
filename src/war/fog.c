// THE SURVIVAL GAME — war/fog.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 297-326 in the Nov 2026 fine-grained refactor; code unchanged.

// --- fog of war ------------------------------------------------------
// Cells are 40u over the 1600x1200 arena. Explored is sticky: a cell
// that has ever been visible stays explored for the whole battle.
static int war_fog_at(BwSim *S, float wx, float wy){
    int cx = (int)((wx - WAR_OX)/40.0f), cy = (int)((wy - WAR_OY)/40.0f);
    if (cx<0 || cy<0 || cx>=S->war_gw || cy>=S->war_gh) return 2;   // off-arena (or off a SMALL skirmish field): don't hide the frame
    return S->war_fog[cy*WAR_FOG_W + cx];
}
static void war_fog_mark(BwSim *S, float wx, float wy, float r){
    int cx0 = (int)((wx - r - WAR_OX)/40.0f), cx1 = (int)((wx + r - WAR_OX)/40.0f);
    int cy0 = (int)((wy - r - WAR_OY)/40.0f), cy1 = (int)((wy + r - WAR_OY)/40.0f);
    if (cx0<0) cx0=0; if (cy0<0) cy0=0;
    if (cx1>=S->war_gw) cx1=S->war_gw-1; if (cy1>=S->war_gh) cy1=S->war_gh-1;
    for (int cy=cy0; cy<=cy1; cy++) for (int cx=cx0; cx<=cx1; cx++){
        float px = WAR_OX + cx*40.0f + 20.0f, py = WAR_OY + cy*40.0f + 20.0f;
        float dx = px-wx, dy = py-wy;
        if (dx*dx + dy*dy <= r*r) S->war_fog[cy*WAR_FOG_W + cx] = 2;
    }
}
static void war_fog_update(BwSim *S){
    for (int i=0;i<WAR_FOG_W*WAR_FOG_H;i++) if (S->war_fog[i]==2) S->war_fog[i]=1;   // visible lapses into explored, never back to black
    for (int i=0;i<WAR_MAXU;i++){
        if (S->war_uhp[i]<=0 || war_u_side(S->war_utype[i])!=0) continue;
        war_fog_mark(S, S->war_ux[i], S->war_uy[i], war_u_sight(S->war_utype[i]));
    }
    for (int s=0;s<WAR_MAXS;s++){
        if (S->war_shp[s]<=0 || S->war_sside[s]!=0) continue;
        war_fog_mark(S, S->war_sx[s], S->war_sy[s], S->war_stype[s]==WAR_S_HQ? 200.0f : 170.0f);
    }
}
