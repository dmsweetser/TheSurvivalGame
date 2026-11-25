// THE SURVIVAL GAME — war/terrain.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 327-450 in the Nov 2026 fine-grained refactor; code unchanged.

// --- terrain ---------------------------------------------------------
// v1.18 (Daniel: "novel every time you play"): the scatter is rolled
// from the battle's own seed (bseed), with the region mixed in for
// flavour — the same region never grows the same woods twice. Hive
// clearances below read the ACTUAL hive structs, which
// war_start_battle has already placed from the same seed.
static void war_gen_terrain(BwSim *S, u32 bseed){
    int region = S->war_region;
    S->war_tn = 0;
    u32 seed = bseed ^ (0x7E44u + (u32)region * 977u);
    // v1.27 (Daniel: "needs more randomization"): the scatter runs
    // wider — 44..80 features (was 52..68), radius 9..16 (was
    // 10..14), and the kind mix itself is rolled per battle from
    // three profiles (balanced / woods-heavy / ruins-heavy) instead
    // of the fixed 50/25/25 split.
    int want = 44 + (int)(hash2(region, 7, seed) % 37u);   // 44..80 features
    int mix = (int)(hash2(region, 9, seed) % 3u);          // the per-battle kind-mix profile
    for (int t=0; t<900 && S->war_tn<want; t++){
        float x = WAR_OX + 44.0f + hash_f(t, 1, seed) * (S->war_aw - 88.0f);
        float y = WAR_OY + 44.0f + hash_f(t, 2, seed) * (S->war_ah - 88.0f);
        float dx, dy;
        dx = x-(WAR_OX+280); dy = y-(WAR_OY+S->war_ah*0.5f); if (dx*dx+dy*dy < 104.0f*104.0f) continue;   // the Core's clearing (the Core sits at mid-height on any field)
        int bad = 0;
        for (int s=0;s<WAR_MAXS && !bad;s++){
            if (S->war_shp[s]<=0 || S->war_stype[s]!=WAR_S_HIVE) continue;
            dx = x-S->war_sx[s]; dy = y-S->war_sy[s]; if (dx*dx+dy*dy < 84.0f*84.0f) bad = 1;
        }
        for (int n=0;n<S->war_nn && !bad;n++){
            dx = x-S->war_nx[n]; dy = y-S->war_ny[n]; if (dx*dx+dy*dy < 54.0f*54.0f) bad = 1;
        }
        for (int j=0;j<S->war_tn && !bad;j++){
            dx = x-S->war_tx[j]; dy = y-S->war_ty[j]; if (dx*dx+dy*dy < 34.0f*34.0f) bad = 1;   // v1.27: 34 (was 30) — radii run to 16 now, so neighbours need the extra room
        }
        if (bad) continue;
        float r = hash_f(t, 3, seed);
        int kind;
        if (mix==1)      kind = r < 0.65f? 0 : r < 0.80f? 1 : 2 + (int)(hash2(t, 4, seed) % 3u);   // woods-heavy: 65% tree / 15% rock / 20% ruins
        else if (mix==2) kind = r < 0.30f? 0 : r < 0.50f? 1 : 2 + (int)(hash2(t, 4, seed) % 3u);   // ruins-heavy: 30% tree / 20% rock / 50% ruins
        else             kind = r < 0.50f? 0 : r < 0.75f? 1 : 2 + (int)(hash2(t, 4, seed) % 3u);   // balanced: the old 50/25/25
        int i = S->war_tn++;
        S->war_tx[i]=x; S->war_ty[i]=y; S->war_ttype[i]=kind;
        S->war_trad[i] = 9 + (int)(hash2(t, 5, seed) % 8u);   // v1.27: 9..16 (was 10..14)
        S->war_tmaxhp[i] = WAR_THP[kind]; S->war_thp[i] = (float)WAR_THP[kind]; S->war_tdeadt[i] = 0;
    }
    // v1.18: BARRIER LINES (terrain kind 5) — runs of
    // indestructible slabs with a gap rolled into every line, so
    // (v1.27: ~30% of the runs are horizontal, axes swapped)
    // each battle has walls to think around AND a path through
    // every one of them. Lines never span the full arena height,
    // so a Core->hive route always exists even ignoring the gaps.
    // Segments ride the same arrays as debris: collision
    // (war_separate), the A* blocked grid and the snapshot all key
    // off thp>0, and war_hurt_terrain refuses kind 5 — they can
    // never be removed, only routed around.
    { int debris_n = S->war_tn;   // the 30u spacing test below runs against DEBRIS only — a line's own segments stand 24u apart by design
      int lines = S->war_lines>0? S->war_lines : (region>=2? 3 : 2);   // v1.19: a skirmish overrides the line count
      for (int k=0;k<lines;k++){
          // v1.27: ~30% of lines run HORIZONTAL — the vertical
          // line's rolls with the axes swapped (same hash slots:
          // 21 anchor, 22 length, 23 start, 24 gap), so a
          // horizontal line is a vertical one laid on its side,
          // with the same gap and clearance machinery throughout.
          if (hash_f(k, 25, bseed) < 0.30f){
              float yk = S->war_ah*0.30f + (float)k*(S->war_ah*0.175f) + hash_f(k, 21, bseed)*(S->war_ah*0.075f);
              float xlen = S->war_aw*0.35f + hash_f(k, 22, bseed)*(S->war_aw*0.2667f);
              float x0 = S->war_aw*0.05f + hash_f(k, 23, bseed)*((S->war_aw-S->war_aw*0.1f)-xlen);
              float gx = x0 + S->war_aw*0.075f + hash_f(k, 24, bseed)*(xlen-S->war_aw*0.15f);
              for (float x=x0; x<=x0+xlen; x+=24.0f){
                  if (S->war_tn >= WAR_MAXT-2) break;
                  float gdx = x-gx; if (gdx<0) gdx=-gdx;
                  if (gdx < 55.0f) continue;   // the gap: a ~110u path through the line
                  float wx = WAR_OX + x, wy = WAR_OY + yk;
                  float dx, dy;
                  dx = wx-(WAR_OX+280); dy = wy-(WAR_OY+S->war_ah*0.5f); if (dx*dx+dy*dy < 130.0f*130.0f) continue;   // the Core's clearing, as with vertical lines
                  int bad = 0;
                  for (int s=0;s<WAR_MAXS && !bad;s++){
                      if (S->war_shp[s]<=0 || S->war_stype[s]!=WAR_S_HIVE) continue;
                      dx = wx-S->war_sx[s]; dy = wy-S->war_sy[s]; if (dx*dx+dy*dy < 115.0f*115.0f) bad = 1;
                  }
                  for (int n=0;n<S->war_nn && !bad;n++){
                      dx = wx-S->war_nx[n]; dy = wy-S->war_ny[n]; if (dx*dx+dy*dy < 75.0f*75.0f) bad = 1;
                  }
                  for (int j=0;j<debris_n && !bad;j++){
                      dx = wx-S->war_tx[j]; dy = wy-S->war_ty[j]; if (dx*dx+dy*dy < 30.0f*30.0f) bad = 1;
                  }
                  if (bad) continue;
                  int i = S->war_tn++;
                  S->war_tx[i]=wx; S->war_ty[i]=wy; S->war_ttype[i]=5;
                  S->war_trad[i]=15;
                  S->war_tmaxhp[i]=WAR_THP[5]; S->war_thp[i]=(float)WAR_THP[5]; S->war_tdeadt[i]=0;
              }
              continue;
          }
          // v1.19: geometry is proportional to the live field —
          // at the campaign's 1600x1200 these are the same numbers
          // as before (x 560 + k*230 + roll*120, ylen 420..740,
          // y0 base 60, gap centre y0+90 + roll*(ylen-180)).
          float xk = S->war_aw*0.35f + (float)k*(S->war_aw*0.14375f) + hash_f(k, 21, bseed)*(S->war_aw*0.075f);
          float ylen = S->war_ah*0.35f + hash_f(k, 22, bseed)*(S->war_ah*0.2667f);
          float y0 = S->war_ah*0.05f + hash_f(k, 23, bseed)*((S->war_ah-S->war_ah*0.1f)-ylen);
          float gy = y0 + S->war_ah*0.075f + hash_f(k, 24, bseed)*(ylen-S->war_ah*0.15f);
          for (float y=y0; y<=y0+ylen; y+=24.0f){
              if (S->war_tn >= WAR_MAXT-2) break;
              float gdy = y-gy; if (gdy<0) gdy=-gdy;
              if (gdy < 55.0f) continue;   // the gap: a ~110u path through the line
              float wx = WAR_OX + xk, wy = WAR_OY + y;
              float dx, dy;
              dx = wx-(WAR_OX+280); dy = wy-(WAR_OY+S->war_ah*0.5f); if (dx*dx+dy*dy < 130.0f*130.0f) continue;   // the Core's clearing, wider than debris' — a wall must never pen the base in
              int bad = 0;
              for (int s=0;s<WAR_MAXS && !bad;s++){
                  if (S->war_shp[s]<=0 || S->war_stype[s]!=WAR_S_HIVE) continue;
                  dx = wx-S->war_sx[s]; dy = wy-S->war_sy[s]; if (dx*dx+dy*dy < 115.0f*115.0f) bad = 1;
              }
              for (int n=0;n<S->war_nn && !bad;n++){
                  dx = wx-S->war_nx[n]; dy = wy-S->war_ny[n]; if (dx*dx+dy*dy < 75.0f*75.0f) bad = 1;
              }
              for (int j=0;j<debris_n && !bad;j++){
                  dx = wx-S->war_tx[j]; dy = wy-S->war_ty[j]; if (dx*dx+dy*dy < 30.0f*30.0f) bad = 1;
              }
              if (bad) continue;
              int i = S->war_tn++;
              S->war_tx[i]=wx; S->war_ty[i]=wy; S->war_ttype[i]=5;
              S->war_trad[i]=15;
              S->war_tmaxhp[i]=WAR_THP[5]; S->war_thp[i]=(float)WAR_THP[5]; S->war_tdeadt[i]=0;
          }
      } }
}
static int war_spawn_unit(BwSim *S, int type, float x, float y){
    for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]<=0){
        S->war_utype[i]=type; S->war_ux[i]=x; S->war_uy[i]=y;
        S->war_umaxhp[i]=war_u_hp(type, S->war_region); S->war_uhp[i]=(float)S->war_umaxhp[i];
        S->war_ustate[i]=0; S->war_utarget[i]=0; S->war_ucarry[i]=0; S->war_usel[i]=0;
        S->war_uatk[i]=0; S->war_uharv[i]=0; S->war_utx[i]=x; S->war_uty[i]=y;
        S->war_urepair[i]=0; S->war_urepacc[i]=0; S->war_urepscan[i]=0;
        S->war_uclan[i]=0; S->war_uaggro[i]=0;   // v1.33: a fresh unit has no clan and no grudge (bug spawns stamp their clan on after)
        S->war_wpn[i]=0; S->war_wpi[i]=0; S->war_stallt[i]=0; S->war_repath[i]=0;
        S->war_stallx[i]=x; S->war_stally[i]=y; S->war_ptgx[i]=x; S->war_ptgy[i]=y;
        return i;
    }
    return -1;
}
static int war_spawn_struct(BwSim *S, int type, int side, float x, float y){
    for (int i=0;i<WAR_MAXS;i++) if (S->war_shp[i]<=0){
        int hp = type==WAR_S_HIVE? 220 + 60*S->war_region : WAR_SHP[type];
        S->war_stype[i]=type; S->war_sside[i]=side; S->war_sx[i]=x; S->war_sy[i]=y;
        S->war_smaxhp[i]=hp; S->war_shp[i]=(float)hp; S->war_scd[i]=0; S->war_qt[i]=0; S->war_qlen[i]=0; S->war_rallyx[i]=0; S->war_rallyy[i]=0; S->war_hbits[i]=0;
        S->war_sclan[i]=0; S->war_smineracc[i]=0;   // v1.33: clan stamped on by the spawner (hives in war_build_battle); the trickle starts empty
        return i;
    }
    return -1;
}
// the wave composition rule, shared by free spawns (the starting
// garrison) and bought ones (v1.21 waves): from region 2, every
// third bug off the line is a spitter
static int war_bug_type_next(BwSim *S){
    if (S->war_region>=2 && (war_count_units(S, 1) % 3)==2) return WAR_U_SPIT;
    return WAR_U_CRAWL;
}
static int war_spawn_bug_t(BwSim *S, int hive, int wave, int type){   // returns the unit slot, -1 when the swarm is capped or the field is full
    if (war_count_clan(S, war_s_team(S, hive)) >= 40) return -1;   // v1.33: the 40-bug cap is per CLAN now — nine clans field nine swarms (the old side-wide cap would have starved a free-for-all)
    float a = (float)(war_count_units(S, 1) % 8) * 0.8f + (wave? 2.0f : 0.0f);
    int i = war_spawn_unit(S, type, S->war_sx[hive] + fcos(a)*(WAR_SRAD[WAR_S_HIVE]+16),
                                   S->war_sy[hive] + fsin(a)*(WAR_SRAD[WAR_S_HIVE]+16));
    if (i>=0){
        S->war_uclan[i] = war_s_team(S, hive);   // v1.33: a bug is born into its hive's clan
        S->war_ustate[i] = wave? 1 : 0;
        S->war_utx[i] = S->war_sx[hive]; S->war_uty[i] = S->war_sy[hive];   // a guard's home
    }
    return i;
}
static void war_spawn_bug(BwSim *S, int hive, int wave){
    war_spawn_bug_t(S, hive, wave, war_bug_type_next(S));
}
