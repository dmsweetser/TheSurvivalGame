// THE SURVIVAL GAME — war/snap.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 1249-1483 in the Nov 2026 fine-grained refactor; code unchanged.

// --- battle snapshots (v1.17, Daniel: leaving must SAVE the battle
// and pause it) -------------------------------------------------------
// The canonical text lives in war_snap[pi] (save section): keys wb /
// wu / ws / wg / wt / wf / wh in the "cw" idiom — ',' inside a record,
// ';' between records, one space between keys. Coordinates are
// arena-relative ints EXCEPT the rally (absolute, so 0 stays "unset").
// The save writer emits the text verbatim; the loader rebuilds it;
// restore parses it back into the BwSim. Projectiles, attack
// cooldowns, selection and scorch marks are transient — never saved.
static char *war_put_key(char *p, char pfx, char c){ *p++=pfx; *p++=c; *p++='='; *p=0; return p; }   // v1.29: a snapshot key under the park's prefix (w campaign / k skirmish)
static void war_snap_write(int pi, char *dst, int sk){
    BwSim *S = &BWS[pi];
    char *p = dst;
    char P = sk? 'k' : 'w';
    p = war_put_key(p, P, 'b');
    p = put_int(p, S->war_region); *p++=','; *p=0;
    p = put_int(p, S->war_bits); *p++=','; *p=0;
    p = put_int(p, (int)S->war_t); *p++=','; *p=0;
    p = put_int(p, (int)(S->war_wavet*10.0f + 0.5f)); *p++=','; *p=0;
    p = put_int(p, (int)(S->war_camx - WAR_OX)); *p++=','; *p=0;
    p = put_int(p, (int)(S->war_camy - WAR_OY));
    if (sk){ *p++=','; *p=0; p = put_int(p, S->war_size); *p++=','; *p=0; p = put_int(p, S->war_diff); *p++=','; *p=0; p = put_int(p, S->war_clans); }   // v1.29: the skirmish park carries its setup (size, difficulty) as trailing wb fields — the campaign parse reads six and never sees them. v1.33: the CLAN COUNT rides as a third trailing field; its PRESENCE marks the new format (a pre-v1.33 park's size 3 meant the old 1600x1200 LARGE — see war_snap_sk_restore)
    { int first=1;   // wu: ALIVE units only, slot-stable (dead slots stay dead — refs in utarget must land)
      for (int i=0;i<WAR_MAXU;i++){
          if (S->war_uhp[i]<=0) continue;
          if (first){ *p++=' '; *p=0; p = war_put_key(p, P, 'u'); first=0; } else { *p++=';'; *p=0; }
          p = put_int(p, i); *p++=','; *p=0;
          p = put_int(p, S->war_utype[i]); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_ux[i]-WAR_OX)); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_uy[i]-WAR_OY)); *p++=','; *p=0;
          p = put_int(p, (int)S->war_uhp[i]); *p++=','; *p=0;
          p = put_int(p, S->war_umaxhp[i]); *p++=','; *p=0;
          p = put_int(p, S->war_ustate[i]); *p++=','; *p=0;
          p = put_int(p, S->war_utarget[i]); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_utx[i]-WAR_OX)); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_uty[i]-WAR_OY)); *p++=','; *p=0;
          p = put_int(p, S->war_ucarry[i]); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_uharv[i]*10.0f + 0.5f)); *p++=','; *p=0;
          p = put_int(p, S->war_uclan[i]);   // v1.33: the unit's clan, trailing — old parks lack it and default on read (bugs to clan 1, yours to 0)
      } }
    { int first=1;   // ws: ALIVE structs — queue rides as qlen + the queued types (addendum 2), rally last (addendum 3)
      for (int s=0;s<WAR_MAXS;s++){
          if (S->war_shp[s]<=0) continue;
          if (first){ *p++=' '; *p=0; p = war_put_key(p, P, 's'); first=0; } else { *p++=';'; *p=0; }
          p = put_int(p, s); *p++=','; *p=0;
          p = put_int(p, S->war_stype[s]); *p++=','; *p=0;
          p = put_int(p, S->war_sside[s]); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_sx[s]-WAR_OX)); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_sy[s]-WAR_OY)); *p++=','; *p=0;
          p = put_int(p, (int)S->war_shp[s]); *p++=','; *p=0;
          p = put_int(p, S->war_smaxhp[s]); *p++=','; *p=0;
          p = put_int(p, S->war_qlen[s]); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_qt[s]*10.0f + 0.5f)); *p++=','; *p=0;
          p = put_int(p, (int)(S->war_scd[s]*10.0f + 0.5f));
          for (int k=0;k<S->war_qlen[s];k++){ *p++=','; *p=0; p = put_int(p, S->war_queue[s][k]); }
          *p++=','; *p=0; p = put_int(p, (int)S->war_rallyx[s]);
          *p++=','; *p=0; p = put_int(p, (int)S->war_rallyy[s]); *p++=','; *p=0;
          p = put_int(p, S->war_sclan[s]);   // v1.33: the structure's clan, trailing — old parks default on read (hives to clan 1, yours to 0)
      } }
    *p++=' '; *p=0; p = war_put_key(p, P, 'g');   // bit nodes: every slot, amounts may be 0 — indices matter
    for (int n=0;n<S->war_nn;n++){
        if (n){ *p++=';'; *p=0; }
        p = put_int(p, (int)(S->war_nx[n]-WAR_OX)); *p++=','; *p=0;
        p = put_int(p, (int)(S->war_ny[n]-WAR_OY)); *p++=','; *p=0;
        p = put_int(p, S->war_namt[n]);
    }
    *p++=' '; *p=0; p = war_put_key(p, P, 't');   // terrain: ALL entries — WAR_TREF aims at these indices
    for (int t=0;t<S->war_tn;t++){
        if (t){ *p++=';'; *p=0; }
        p = put_int(p, (int)(S->war_tx[t]-WAR_OX)); *p++=','; *p=0;
        p = put_int(p, (int)(S->war_ty[t]-WAR_OY)); *p++=','; *p=0;
        p = put_int(p, S->war_ttype[t]); *p++=','; *p=0;
        p = put_int(p, S->war_trad[t]); *p++=','; *p=0;
        p = put_int(p, (int)S->war_thp[t]); *p++=','; *p=0;
        p = put_int(p, S->war_tmaxhp[t]);
    }
    *p++=' '; *p=0; p = war_put_key(p, P, 'f');   // fog: explored-or-visible as a bit set over the whole fog array in its own memory order (v1.33: 338 words — any field size round-trips bit-for-bit; pre-v1.33 parks carried 38 words in 40-stride order and the reader still speaks that, see war_snap_restore_text)
    for (int w=0;w<(WAR_FOG_W*WAR_FOG_H+31)/32;w++){
        if (w){ *p++=','; *p=0; }
        u32 word=0;
        for (int b=0;b<32;b++){ int c=w*32+b; if (c<WAR_FOG_W*WAR_FOG_H && S->war_fog[c]) word |= (1u<<b); }
        p = put_int(p, (int)word);
    }
    { int first=1;   // wh (v1.21): each living hive's purse — slot, bits. The economy rides the park.
      for (int s=0;s<WAR_MAXS;s++){
          if (S->war_shp[s]<=0 || S->war_stype[s]!=WAR_S_HIVE) continue;
          if (first){ *p++=' '; *p=0; p = war_put_key(p, P, 'h'); first=0; } else { *p++=';'; *p=0; }
          p = put_int(p, s); *p++=','; *p=0;
          p = put_int(p, S->war_hbits[s]);
      } }
    *p=0;
    if (!sk){ war_snap_on[pi]=1; war_snap_reg[pi]=S->war_region; }
}
static void war_snap_serialize(int pi){ war_snap_write(pi, war_snap[pi], 0); }
static const char *war_snap_for_save(int pi){   // the save writer's door: refresh from a live battle, then hand over the text (0 = nothing parked)
    // v1.18: a live battle in a DIFFERENT region than the parked one
    // must NOT re-serialize over the park — the autosave would
    // silently evict the saved battle from the file (Daniel's
    // playtest: SAVE & QUIT, tap a region, fresh fight, saved
    // fight gone). The parked text is returned as-is instead.
    // v1.19: a live SKIRMISH is never serialized at all — it is
    // ad-hoc by definition; whatever campaign battle is parked
    // (if any) is returned untouched.
    // feat 13: a GUEST's war_state mirrors the owner's battle for
    // rendering, but their sim holds no battle — serializing it
    // would park an empty field over the guest's own saved battle.
    if (BWS[pi].war_state==1 && !BWS[pi].war_guest && !BWS[pi].war_skirmish && !(war_snap_on[pi] && war_snap_reg[pi]!=BWS[pi].war_region)) war_snap_serialize(pi);
    return war_snap_on[pi]? war_snap[pi] : 0;
}
// v1.29: the SKIRMISH park — a second, independent snapshot slot
// (Daniel: "make it so you can save your skirmish"). Same text,
// same writer, its own buffer and save tokens (kb/ku/ks/kg/kt/
// kf/kh), so the campaign park's v1.18/v1.19 scoping is untouched:
// the two parks never read or evict each other. The door mirrors
// the campaign one — a live skirmish serializes at every save
// (the 5s autosave included), so leaving the cabinet or closing
// the game mid-skirmish keeps the battle; a live CAMPAIGN battle
// leaves the skirmish park exactly as it stands.
static void war_snap_sk_serialize(int pi){ war_snap_write(pi, war_snap_sk[pi], 1); war_snap_sk_on[pi]=1; }
static const char *war_snap_sk_for_save(int pi){
    if (BWS[pi].war_state==1 && !BWS[pi].war_guest && BWS[pi].war_skirmish) war_snap_sk_serialize(pi);
    return war_snap_sk_on[pi]? war_snap_sk[pi] : 0;
}
// (war_snap_sk_clear lives in save/saveio.c with the buffers —
// war/hive.c and war/path.c call it and precede this file.)
static const char *war_snap_val(const char *snap, const char *key){
    int kl = slen(key);
    const char *s = snap;
    while (*s){
        if ((s==snap || s[-1]==' ') && s[0]==key[0]){
            int ok=1; for (int i=0;i<kl;i++) if (s[i]!=key[i]){ ok=0; break; }
            if (ok && s[kl]=='=') return s+kl+1;
        }
        while (*s && *s!=' ') s++;
        if (*s==' ') s++;
    }
    return 0;
}
static int war_rd_int(const char **qp){   // one int, then step over exactly one separator (',' / ';' / ' ')
    const char *q = *qp;
    int v = parse_int(q);
    while (*q && *q!=',' && *q!=';' && *q!=' ') q++;
    if (*q==',' || *q==';') q++;
    *qp = q;
    return v;
}
static int war_rec_fields(const char *v){   // v1.33: fields in the record starting at v (','-separated, ';'/' '/end-terminated) — how the reader knows a TRAILING field (a clan) is present without mistaking the next record's first field for it
    int n=1;
    while (*v && *v!=';' && *v!=' '){ if (*v==',') n++; v++; }
    return n;
}
static void war_snap_restore_text(int pi, const char *snap, char pfx){
    BwSim *S = &BWS[pi];
    char kb[3]={pfx,'b',0}, ku[3]={pfx,'u',0}, ks[3]={pfx,'s',0}, kg[3]={pfx,'g',0}, kt[3]={pfx,'t',0}, kf[3]={pfx,'f',0}, kh[3]={pfx,'h',0};
    const char *v;
    war_clear_field(S);   // v1.19: also restores campaign geometry (war_skirmish=0, war_aw/war_ah = WAR_AW/WAR_AH) — the skirmish wrapper re-applies its own geometry after the parse
    if ((v = war_snap_val(snap, kb))){
        S->war_region = war_rd_int(&v);
        S->war_bits = war_rd_int(&v);
        S->war_t = (float)war_rd_int(&v);
        S->war_wavet = war_rd_int(&v) * 0.1f;
        S->war_camx = WAR_OX + (float)war_rd_int(&v);
        S->war_camy = WAR_OY + (float)war_rd_int(&v);
    }
    if (S->war_region<0) S->war_region=0; if (S->war_region>7) S->war_region=7;
    if ((v = war_snap_val(snap, ku))){
        while (*v && *v!=' '){
            int nf = war_rec_fields(v);   // v1.33: 13 fields = the clan rides; 12 = a pre-v1.33 park
            int idx = war_rd_int(&v);
            int type = war_rd_int(&v);
            int x = war_rd_int(&v), y = war_rd_int(&v);
            int hp = war_rd_int(&v), maxhp = war_rd_int(&v);
            int state = war_rd_int(&v), target = war_rd_int(&v);
            int tx = war_rd_int(&v), ty = war_rd_int(&v);
            int carry = war_rd_int(&v), harv = war_rd_int(&v);
            int uclan = nf>12? war_rd_int(&v) : -1;
            if (idx>=0 && idx<WAR_MAXU){
                S->war_utype[idx]=type; S->war_ux[idx]=WAR_OX+(float)x; S->war_uy[idx]=WAR_OY+(float)y;
                S->war_uhp[idx]=(float)hp; S->war_umaxhp[idx]=maxhp;
                S->war_ustate[idx]=state; S->war_utarget[idx]=target;
                S->war_utx[idx]=WAR_OX+(float)tx; S->war_uty[idx]=WAR_OY+(float)ty;
                S->war_ucarry[idx]=carry; S->war_uharv[idx]=harv*0.1f;
                S->war_uclan[idx] = uclan>=0? uclan : (war_u_side(type)? 1 : 0);   // v1.33: an old park's bugs all fly clan 1 — the single swarm they were saved as
                S->war_uatk[idx]=0; S->war_usel[idx]=0;
                S->war_wpn[idx]=0; S->war_wpi[idx]=0; S->war_stallt[idx]=0; S->war_repath[idx]=0;
                S->war_stallx[idx]=S->war_ux[idx]; S->war_stally[idx]=S->war_uy[idx];
            }
        }
    }
    if ((v = war_snap_val(snap, ks))){
        while (*v && *v!=' '){
            int nf = war_rec_fields(v);   // v1.33: 13+qlen fields = the clan rides; 12+qlen = a pre-v1.33 park
            int idx = war_rd_int(&v);
            int type = war_rd_int(&v), side = war_rd_int(&v);
            int x = war_rd_int(&v), y = war_rd_int(&v);
            int hp = war_rd_int(&v), maxhp = war_rd_int(&v);
            int qlen = war_rd_int(&v);
            int qt = war_rd_int(&v), scd = war_rd_int(&v);
            int qn = qlen<0? 0 : qlen>9? 9 : qlen;
            int qt_[9];
            for (int k=0;k<qn;k++) qt_[k] = war_rd_int(&v);
            int rx = war_rd_int(&v), ry = war_rd_int(&v);
            int sclan = nf>12+qn? war_rd_int(&v) : -1;
            if (idx>=0 && idx<WAR_MAXS){
                S->war_stype[idx]=type; S->war_sside[idx]=side;
                S->war_sx[idx]=WAR_OX+(float)x; S->war_sy[idx]=WAR_OY+(float)y;
                S->war_shp[idx]=(float)hp; S->war_smaxhp[idx]=maxhp;
                S->war_qlen[idx]=qn;
                for (int k=0;k<qn;k++) S->war_queue[idx][k]=qt_[k];
                S->war_qt[idx]=qt*0.1f; S->war_scd[idx]=scd*0.1f;
                S->war_rallyx[idx]=(float)rx; S->war_rallyy[idx]=(float)ry;
                S->war_sclan[idx] = sclan>=0? sclan : (type==WAR_S_HIVE? 1 : 0);   // v1.33: an old park's hives fly clan 1; everything else of the enemy's was a hive anyway
            }
        }
    }
    // v1.21: hive purses. A v1.20 park carries no wh key — every
    // living hive then opens at the campaign starting purse (150),
    // exactly as if the battle had just been built; a v1.21 park
    // restores the purse it saved.
    for (int s=0;s<WAR_MAXS;s++)
        if (S->war_shp[s]>0 && S->war_stype[s]==WAR_S_HIVE) S->war_hbits[s]=150;
    if ((v = war_snap_val(snap, kh))){
        while (*v && *v!=' '){
            int idx = war_rd_int(&v);
            int purse = war_rd_int(&v);
            if (idx>=0 && idx<WAR_MAXS && S->war_shp[idx]>0 && S->war_stype[idx]==WAR_S_HIVE) S->war_hbits[idx]=purse;
        }
    }
    if ((v = war_snap_val(snap, kg))){
        int n=0;
        while (*v && *v!=' ' && n<16){   // v1.19: an 11-node field must restore whole (was capped at 10)
            S->war_nx[n]=WAR_OX+(float)war_rd_int(&v);
            S->war_ny[n]=WAR_OY+(float)war_rd_int(&v);
            S->war_namt[n]=war_rd_int(&v);
            n++;
        }
        S->war_nn=n;
    }
    if ((v = war_snap_val(snap, kt))){
        int t=0;
        while (*v && *v!=' ' && t<WAR_MAXT){
            S->war_tx[t]=WAR_OX+(float)war_rd_int(&v);
            S->war_ty[t]=WAR_OY+(float)war_rd_int(&v);
            S->war_ttype[t]=war_rd_int(&v);
            S->war_trad[t]=war_rd_int(&v);
            S->war_thp[t]=(float)war_rd_int(&v);
            S->war_tmaxhp[t]=war_rd_int(&v);
            S->war_tdeadt[t]=0;
            t++;
        }
        S->war_tn=t;
    }
    for (int c=0;c<WAR_FOG_W*WAR_FOG_H;c++) S->war_fog[c]=0;
    if ((v = war_snap_val(snap, kf))){
        // v1.33: read the words first and count them — 38 words is a
        // pre-v1.33 park (1200 bits in 40-stride order over the old
        // 40x30 grid); anything else is the new format, the array's
        // own memory order, copied straight across
        u32 words[(WAR_FOG_W*WAR_FOG_H+31)/32];
        int nw=0;
        while (*v && *v!=' ' && nw<(WAR_FOG_W*WAR_FOG_H+31)/32) words[nw++] = (u32)war_rd_int(&v);
        if (nw==38){
            for (int c=0;c<1200;c++) if ((words[c>>5]>>(c&31))&1u) S->war_fog[(c/40)*WAR_FOG_W + (c%40)]=1;
        } else {
            for (int w=0;w<nw;w++) for (int b=0;b<32;b++){ int c=w*32+b; if (c<WAR_FOG_W*WAR_FOG_H && ((words[w]>>b)&1u)) S->war_fog[c]=1; }
        }
    }
    S->war_state=1; S->war_pause=0; S->war_bcat=-1;
    S->war_grace=0.35f; S->war_ordt=0; S->war_msgt=0; S->war_endt=0;
    S->war_ltap_t=-10.0f; S->war_ltap_unit=-1; S->war_drag_id=-1;
    war_clamp_cam(S);
    war_fog_update(S);   // stored cells come back explored; this re-marks the visible ones
}
static void war_snap_restore(int pi){
    war_snap_restore_text(pi, war_snap[pi], 'w');
    war_note(&BWS[pi], "BATTLE RESTORED. WELCOME BACK, COMMANDER.");
}
// v1.29: the skirmish restore — the same parse under the k prefix,
// then the skirmish identity and geometry the campaign clear
// reset. Size/difficulty ride the park's kb record as trailing
// fields (defaults STANDARD / NORMAL for a park without them).
static void war_snap_sk_restore(int pi){
    BwSim *S = &BWS[pi];
    const char *snap = war_snap_sk[pi];
    int size = 2, diff = 2, clans = 0;
    const char *v = war_snap_val(snap, "kb");
    if (v){ for (int i=0;i<6;i++) (void)war_rd_int(&v); if (*v && *v!=' '){ size = war_rd_int(&v); if (*v && *v!=' '){ diff = war_rd_int(&v); if (*v && *v!=' ') clans = war_rd_int(&v); } } }
    if (size<1 || size>5) size = 2;
    if (diff<1 || diff>3) diff = 2;
    war_snap_restore_text(pi, snap, 'k');
    S->war_skirmish = 1; S->war_size = size; S->war_diff = diff;
    // v1.33: geometry from the ladder — EXCEPT a pre-v1.33 park
    // (no clans field) whose size 3 was the old LARGE: 1600x1200
    // with 4 lines. Restoring it at the ladder's 2400x1800 would
    // stretch its saved field over ground it never had; it comes
    // back at exactly the size it was saved at. (Sizes 1/2 agree
    // between the eras, so only size 3 needs the distinction.)
    if (clans>0 || size!=3){
        S->war_aw = (float)WAR_SIZE_AW[size]; S->war_ah = (float)WAR_SIZE_AH[size];
        S->war_lines = WAR_SIZE_LINES[size];
    } else S->war_lines = 4;
    S->war_gw = (int)(S->war_aw/40.0f); S->war_gh = (int)(S->war_ah/40.0f);
    if (clans>=1 && clans<=9){ S->war_clans = clans; S->war_clanpick = clans; }
    else {   // a legacy park: one swarm, but its award counts the hives it actually fields
        int hv = war_hives_alive(S);
        S->war_clans = hv>=1? hv : 1;
        S->war_clanpick = S->war_clans<=9? S->war_clans : 9;
    }
    war_clamp_cam(S);
    war_note(S, "SKIRMISH RESTORED. THE BUGS REMEMBER NOTHING.");
}
