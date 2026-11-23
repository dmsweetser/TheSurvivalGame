// THE SURVIVAL GAME — save/start.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 791-1315 in the Nov 2026 fine-grained refactor; code unchanged.

// v1.27: defined with the Cart medal record (modes/cart/cart_track.c,
// later in the unity order) — the loader calls it for pre-v1.27 saves.
static void crt_migrate_medals(Buddy *b);

static void start_run(int net_mode){
    G.edit_field = 0;   // a run never owns a menu text field (Nov 9)
    reset_world();
    for (int i=0;i<MAXP;i++){ G.p[i].connected=0; G.p[i].is_local=0; }
    G.net_mode = net_mode; G.my_id = 0;
    G.zen_run = G.zen;
    G.diff_run = G.difficulty;
    G.p2_slot=-1; coop_reset_views();
    player_fresh(&G.p[0], G.my_name, 0, 0, 70);
    G.p[0].connected=1; G.p[0].is_local=1;
    G.p[0].whale_own=G.whale_own; G.p[0].whale_on=G.whale_on;
    if (net_mode!=NET_CLIENT && G.coop_pref && IN.pad_count>=2) coop_spawn_p2(-30, 70);
    // FIRST SHIFT (v1.25): a genuinely fresh run arms the guided
    // chain — except for a LAN client, who doesn't own the run.
    G.fs_state = (net_mode!=NET_CLIENT)? 1 : 0;
    G.fs_step = 0;
    for (int i=0;i<MAXP;i++) G.fs_pool[i]=0;
    G.fs_fp0 = fs_fingerprint();
    fs_flash_t = 0;
    // WEEKLY CHALLENGE (v1.25 feature 6): a fresh slot joins the
    // current week with clean books — the ledger starts empty and
    // the baselines snapshot the fresh run's zeros. A LAN client
    // never tracks a week (the host owns it).
    G.wk_week = (net_mode!=NET_CLIENT)? wk_week_now() : 0;
    G.wk_done_week = 0; G.wk_total = 0; G.wk_prev_week = 0; G.wk_prev_done = 0;
    wk_snapshot();
    // NAMED ELITES (v1.25 feature 8): a fresh slot posts serial 0
    // with a clean ledger. A LAN client never owns a bounty.
    G.bn_serial = 0; G.bn_done = 0; G.bn_cool = 0;
    G.bn_claim_pending = 0; G.bn_live = 0; G.bn_live_reg = 0; G.bn_last_reg = -1;
    bn_flash_t = 0; bn_flash_name = -1;
    // HAT PORTRAIT BOOTH (v1.25 feature 9): a fresh slot has no
    // portrait and the dials sit at their defaults (backdrop 0,
    // hat AS WORN). The shell hand-off starts silent.
    for (int i=0;i<MAXP;i++){ smemset(&portrait[i], 0, sizeof(Portrait)); port_bd[i]=0; port_hat[i]=1; }
    G.port_req_serial = 0;
    G.camx=0; G.camy=40;
    G.state=GS_PLAY;
    if (net_mode!=NET_OFF){
        if (plat_net_start(net_mode, G.join_ip)!=0 && net_mode==NET_CLIENT){
            G.state=GS_MENU; scpy(G.store_msg, "COULD NOT CONNECT", 64);
        }
        if (net_mode==NET_CLIENT){ G.p[0].connected=0; G.p[0].is_local=0; G.my_id=-1; client_joined=0; }
    }
}
static int load_records_and_check_save(void){
    int n = plat_load(savebuf, SAVE_CAP-1);
    if (n<=0) return 0;
    savebuf[n]=0;
    if (slen(savebuf)<4 || savebuf[0]!='T') return 0;
    const char *v;
    if ((v=line_val(savebuf,"rec_name"))) { int i=0; while (v[i] && v[i]!='\n' && i<12){ G.rec_name[i]=v[i]; i++; } G.rec_name[i]=0; }
    if ((v=line_val(savebuf,"rec_hat"))) G.rec_hat=parse_flt(v);
    if ((v=line_val(savebuf,"rec_kills"))) G.rec_kills=parse_int(v);
    if ((v=line_val(savebuf,"rec_bowl"))) G.rec_bowl=parse_int(v);
    if ((v=line_val(savebuf,"rec_time"))) G.rec_time=parse_flt(v);
    if ((v=line_val(savebuf,"my_name"))) { int i=0; while (v[i] && v[i]!='\n' && i<12){ G.my_name[i]=v[i]; i++; } G.my_name[i]=0; }
    if ((v=line_val(savebuf,"bind"))) { const char *q=v;
        for (int i=0;i<14;i++){ int pk=parse_int(q); if (pk>=0 && pk<PK_COUNT) G.bind[i]=pk;
            while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++; else break; } }
    if ((v=line_val(savebuf,"touchctl"))) G.touch_ctl = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"zen"))) G.zen = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"eddie"))) G.eddie = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"whale"))) G.whale_own = parse_int(v) & 32767;
    if ((v=line_val(savebuf,"whaleon"))) G.whale_on = parse_int(v) & 32767;
    if ((v=line_val(savebuf,"coop"))) G.coop_pref = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"diff"))){ G.difficulty = parse_int(v); if (G.difficulty<0 || G.difficulty>3) G.difficulty=0; }
    // COMFORT (v1.25 feature 2): line keys an older save simply does
    // not have — the game_init defaults (10/10/off/on) stand.
    if ((v=line_val(savebuf,"volm"))){ G.vol_music = parse_int(v); if (G.vol_music<0) G.vol_music=0; if (G.vol_music>10) G.vol_music=10; }
    if ((v=line_val(savebuf,"vols"))){ G.vol_sfx = parse_int(v); if (G.vol_sfx<0) G.vol_sfx=0; if (G.vol_sfx>10) G.vol_sfx=10; }
    if ((v=line_val(savebuf,"mute"))) G.mute_all = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"shk"))) G.shake_on = parse_int(v)? 1:0;
    if ((v=line_val(savebuf,"slot"))){ G.slot = parse_int(v); if (G.slot<1 || G.slot>3) G.slot = 1; }
    if ((v=line_val(savebuf,"run")) && parse_int(v)==1) return 1;
    return 0;
}
static void continue_run(int slot){
    int n = plat_load_slot(slot, savebuf, SAVE_CAP-1);
    if (n<=0) return;
    savebuf[n]=0;
    G.slot = slot;
    reset_world();
    for (int i=0;i<MAXP;i++){ G.p[i].connected=0; G.p[i].is_local=0; war_snap_clear(i); }   // parked WAR! battles reload from THIS blob only
    G.net_mode=NET_OFF; G.my_id=0;
    const char *v;
    G.zen_run = G.zen;
    if ((v=line_val(savebuf,"zrun"))) G.zen_run = parse_int(v)? 1:0;
    G.diff_run = 0;   // saves from before difficulty existed were all EASY
    if ((v=line_val(savebuf,"drun"))){ G.diff_run = parse_int(v); if (G.diff_run<0 || G.diff_run>3) G.diff_run=0; }
    if ((v=line_val(savebuf,"time"))) G.time=parse_flt(v);
    if ((v=line_val(savebuf,"tkills"))) G.total_kills=parse_int(v);
    if ((v=line_val(savebuf,"tearned"))) G.total_earned=parse_flt(v);
    // FIRST SHIFT (v1.25): default OFF before parsing — a save from
    // before the chain existed has no fs= line and must never see
    // it. A new-build save resumes its chain mid-step, pool and all
    // (the pools ride the pl-line fp= tokens, parsed below).
    G.fs_state=0; G.fs_step=0; G.fs_fp0=0;
    for (int i=0;i<MAXP;i++) G.fs_pool[i]=0;
    if ((v=line_val(savebuf,"fs"))){ const char *q=v;
        G.fs_state=parse_int(q); while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++;
        G.fs_step=parse_int(q); while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++;
        G.fs_fp0=parse_int(q);
        if (G.fs_state<0 || G.fs_state>3) G.fs_state=0;
        if (G.fs_step<0 || G.fs_step>6) G.fs_step=0;   // 6 = past the last step (chain done)
    }
    // WEEKLY CHALLENGE (v1.25 feature 6): defaults before parsing —
    // a save from before the board existed has no wk= line and keeps
    // the zeros. The rollover at the END of this function (after the
    // pl-lines apply every field the baselines read) then opens the
    // current week for it with baselines at the loaded counters, so
    // lifetime progress never retro-completes a delta challenge.
    G.wk_week=0; G.wk_done_week=0; G.wk_total=0; G.wk_prev_week=0; G.wk_prev_done=0;
    for (int i=0;i<7;i++) G.wk_base[i]=0;
    if ((v=line_val(savebuf,"wk"))){ const char *q=v;
        int *dst[12] = { &G.wk_week, &G.wk_done_week, &G.wk_total, &G.wk_prev_week, &G.wk_prev_done,
                         &G.wk_base[0], &G.wk_base[1], &G.wk_base[2], &G.wk_base[3],
                         &G.wk_base[4], &G.wk_base[5], &G.wk_base[6] };
        for (int i=0;i<12;i++){ *dst[i]=parse_int(q); while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++; else break; }
        if (G.wk_week<0) G.wk_week=0;
        if (G.wk_total<0) G.wk_total=0;
    }
    // NAMED ELITES (v1.25 feature 8): defaults before parsing — a
    // save from before the board existed has no bn= line and posts
    // serial 0 with a clean ledger. Only the contract survives a
    // load (serial, claims, cooldown); the elite itself is a live
    // bug and is never in the blob, and last-seen is runtime memory
    // the board honestly forgets.
    G.bn_serial=0; G.bn_done=0; G.bn_cool=0;
    G.bn_claim_pending=0; G.bn_live=0; G.bn_live_reg=0; G.bn_last_reg=-1;
    bn_flash_t=0; bn_flash_name=-1;
    if ((v=line_val(savebuf,"bn"))){ const char *q=v;
        G.bn_serial=parse_int(q); while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++;
        G.bn_done=parse_int(q); while (*q && *q!=',' && *q!='\n') q++; if (*q==',') q++;
        G.bn_cool=(float)parse_int(q);
        if (G.bn_serial<0) G.bn_serial=0;
        if (G.bn_done<0) G.bn_done=0;
        if (G.bn_cool<0) G.bn_cool=0;
    }
    // HAT PORTRAIT BOOTH (v1.25 feature 9): defaults before parsing
    // — a pre-feature save has no pt= tokens and simply has no
    // portrait; the dials sit at their defaults. The per-pl-line
    // temps in the tail walk below re-zero each player's record
    // before its own tokens are read.
    for (int i=0;i<MAXP;i++){ smemset(&portrait[i], 0, sizeof(Portrait)); port_bd[i]=0; port_hat[i]=1; }
    G.port_req_serial=0;
    // players: lines begin "pl= " ... find each occurrence
    const char *s = savebuf;
    while (*s){
        if (s[0]=='p' && s[1]=='l' && s[2]=='=' && s[3]==' '){
            const char *q = s+4;
            int idx = parse_int(q); while (*q && *q!=' ') q++; q++;
            char nm[13]; int ni=0; while (*q && *q!=' ' && ni<12){ nm[ni++]=*q++; } nm[ni]=0; q++;
            if (idx>=0 && idx<MAXP){
                Player *pl=&G.p[idx];
                player_fresh(pl, nm, 0, 0, 70);
                pl->connected=1; pl->is_local = (idx==0);
                pl->x=parse_flt(q); while (*q && *q!=' ') q++; q++;
                pl->y=parse_flt(q); while (*q && *q!=' ') q++; q++;
                pl->hp=parse_int(q); while (*q && *q!=' ') q++; q++;
                pl->alive=parse_int(q); while (*q && *q!=' ') q++; q++;
                pl->weapon=parse_int(q); while (*q && *q!=' ') q++; q++;
                if (savebuf[3]=='3'){                            // TSG3: raw copy counts
                    for (int w=0;w<NWEAP;w++){ pl->owned[w]=parse_int(q); while (*q && *q!=' ') q++; q++; }
                } else {
                    int ow=parse_int(q); while (*q && *q!=' ') q++; q++;
                    if (savebuf[3]=='1') unpack_owned_mask(pl, ow);   // TSG1 saves: bitmask
                    else unpack_copies(pl, ow);                      // TSG2: copy counts
                }
                for (int u=0;u<5;u++){ pl->upg[u]=parse_int(q); while (*q && *q!=' ') q++; q++; }
                pl->rollback=parse_int(q); while (*q && *q!=' ') q++; q++;
                pl->kills=parse_int(q); while (*q && *q!=' ') q++; q++;
                pl->heaviest=parse_flt(q); while (*q && *q!=' ') q++; q++;
                pl->color=parse_int(q); while (*q && *q!=' ') q++; q++;
                int sn=parse_int(q); while (*q && *q!=':') q++; q++;
                pl->stack_n=0; pl->carried=0;
                for (int s2=0;s2<sn;s2++){
                    int w2=parse_int(q); hat_push(pl, w2*0.5f);
                    // the last stack item is followed by a SPACE and the
                    // key= token tail (bk/hs/ml/bd/...): the skip must
                    // stop there too. It didn't — with any carcass on
                    // your hat (sn>=1) this ran to the end of the line
                    // (or the first comma inside cw=), devouring the
                    // whole tail: buddy, banked kg, story, craft — all
                    // lost on Continue while the slot list, which scans
                    // the text, still showed them (Daniel, Nov 5).
                    while (*q && *q!=',' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                }
                // optional key= tokens (bank / hat stand / mall / buddy)
                { int bsp=0, bxp=0, b1=0, b2=0, b3=0, bsk=0, bfn=0, bnm=-1, bcl=0, bnv=0, bwo=0, bwn=0, ben=100, bgx=0, bgy=56, bgz=0, ber=0, bgf=0;
                  int bsm=0, bsq=0, bsi=0, bsq2=0, bsb=0, bpd=0, btd=0, bcb=0, bcs=0, bcp=0, bcq=0, bcr=0, bwr=0, bcx=0, bcy=0, bcm=0, bcn=0, bcg=0, bct=0, bcf=0, bfp=0, be2=0;
                  int btp=0, btw=0;   // HAT TOPPERS temps (feat 5): absent token = nothing earned, nothing worn
                  int tcw_bn=0, tcw_x[48], tcw_y[48], tcw_k[48], tcw_h[48];
                  int tcd_gn=0, tcd_g[96];
                  int tmc[8]={0}, tmr[16]={0};   // MEDAL TIMES temps (feat 3): absent token = all zeros = no medals
                  int tmd[8]={0}, tmd_on=0;   // v1.27: earned Cart medals (md); absent token = a pre-v1.27 save, whose medals migrate from the mc times below
                  // HAT PORTRAIT BOOTH temps (feat 9): the pt= token's
                  // twelve ints + the frozen stack's kind digits.
                  // Absent token = dial 0, no portrait. This player's
                  // record re-zeros here, before its tokens are read.
                  int tpt_on=0, tpt[12]={0}, tptk_n=0; char tptk[MAXSTACK];
                  smemset(&portrait[idx], 0, sizeof(Portrait)); port_bd[idx]=0; port_hat[idx]=1;
                  // BUG BOWL LEAGUE temps (feat 10): the bl= token's
                  // fourteen ints. Absent token = an empty book. This
                  // player's book re-zeros here, before its token is
                  // read (the feat-9 portrait pattern).
                  int tbl_on=0, tbl[14]={0};
                  smemset(&G.blg[idx], 0, sizeof(BowlLeague));
                  // WAR! snapshot (v1.17): the six values ride as raw
                  // text (',' inside records, ';' between them, never
                  // a space) and are rebuilt into war_snap[idx] below.
                  // The nodes key is "wg", NOT the spec's "wn" — wn is
                  // already whale_on in this same token tail.
                  char twb[64], twu[4096], tws[2688], twg[192], twt[4800], twf[512];   // v1.18: caps re-audited — wt must hold 160 terrain entries now, and the old tws/twf caps could already clip a big battle's tail
                  int twar=0; twb[0]=twu[0]=tws[0]=twg[0]=twt[0]=twf[0]=0;
                  while (*q==' '){
                      q++;
                      char kk[4]; int ki=0;
                      while (*q && *q!='=' && *q!=' ' && *q!='\n' && ki<3) kk[ki++]=*q++;
                      kk[ki]=0;
                      if (*q=='=') q++;
                      if (kk[0]=='c' && kk[1]=='w' && kk[2]==0){   // compound build list: tx,ty,kind,hp;…
                          while (*q && *q!=' ' && *q!='\n' && tcw_bn<48){
                              tcw_x[tcw_bn]=parse_int(q); while (*q && *q!=',' && *q!=';' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                              tcw_y[tcw_bn]=parse_int(q); while (*q && *q!=',' && *q!=';' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                              tcw_k[tcw_bn]=parse_int(q); while (*q && *q!=',' && *q!=';' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                              tcw_h[tcw_bn]=parse_int(q); while (*q && *q!=';' && *q!=' ' && *q!='\n') q++; if (*q==';') q++;
                              tcw_bn++;
                          }
                      } else if (kk[0]=='c' && kk[1]=='d' && kk[2]==0){   // compound mined-node list: key;key;…
                          while (*q && *q!=' ' && *q!='\n' && tcd_gn<96){
                              tcd_g[tcd_gn++]=parse_int(q); while (*q && *q!=';' && *q!=' ' && *q!='\n') q++; if (*q==';') q++;
                          }
                      } else if (kk[0]=='m' && kk[1]=='c' && kk[2]==0){   // MEDAL TIMES: 8 comma-separated cart bests (ds)
                          int ti=0;
                          while (*q && *q!=' ' && *q!='\n' && ti<8){
                              tmc[ti++]=parse_int(q); while (*q && *q!=',' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                          }
                          while (*q && *q!=' ' && *q!='\n') q++;   // consume any overflow exactly to the delimiter (Nov 5 lesson)
                      } else if (kk[0]=='m' && kk[1]=='d' && kk[2]==0){   // v1.27: 8 comma-separated EARNED cart medals (0..3)
                          int ti=0; tmd_on=1;
                          while (*q && *q!=' ' && *q!='\n' && ti<8){
                              tmd[ti++]=parse_int(q); while (*q && *q!=',' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                          }
                          while (*q && *q!=' ' && *q!='\n') q++;
                      } else if (kk[0]=='m' && kk[1]=='r' && kk[2]==0){   // MEDAL TIMES: 16 comma-separated run bests (ds)
                          int ti=0;
                          while (*q && *q!=' ' && *q!='\n' && ti<16){
                              tmr[ti++]=parse_int(q); while (*q && *q!=',' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                          }
                          while (*q && *q!=' ' && *q!='\n') q++;
                      } else if (kk[0]=='b' && kk[1]=='l' && kk[2]==0){   // BUG BOWL LEAGUE (feat 10): 14 comma-separated league-book ints
                          tbl_on=1;
                          int ti=0;
                          while (*q && *q!=' ' && *q!='\n' && ti<14){
                              tbl[ti++]=parse_int(q); while (*q && *q!=',' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                          }
                          while (*q && *q!=' ' && *q!='\n') q++;   // consume any overflow exactly to the delimiter (Nov 5 lesson)
                      } else if (kk[0]=='p' && kk[1]=='t' && kk[2]==0){   // HAT PORTRAIT BOOTH (feat 9): dial,has,backdrop,haton,top,kg,stack_n,species,budcolor,nameidx,pcolor,whale : kind digits
                          tpt_on=1;
                          int ti=0;
                          while (*q && *q!=':' && *q!=' ' && *q!='\n' && ti<12){
                              tpt[ti++]=parse_int(q); while (*q && *q!=',' && *q!=':' && *q!=' ' && *q!='\n') q++; if (*q==',') q++;
                          }
                          if (*q==':'){
                              q++;
                              while (*q>='0' && *q<='9' && tptk_n<MAXSTACK) tptk[tptk_n++]=*q++;
                          }
                          while (*q && *q!=' ' && *q!='\n') q++;   // consume any overflow exactly to the delimiter (Nov 5 lesson)
                      } else if (kk[0]=='w' && kk[2]==0 && (kk[1]=='b' || kk[1]=='u' || kk[1]=='s' || kk[1]=='g' || kk[1]=='t' || kk[1]=='f')){
                          // a WAR! snapshot value: copy it raw (bounded);
                          // the generic tail-skip below then steps over it
                          char *dst; int cap;
                          if (kk[1]=='b'){ dst=twb; cap=(int)sizeof(twb); }
                          else if (kk[1]=='u'){ dst=twu; cap=(int)sizeof(twu); }
                          else if (kk[1]=='s'){ dst=tws; cap=(int)sizeof(tws); }
                          else if (kk[1]=='g'){ dst=twg; cap=(int)sizeof(twg); }
                          else if (kk[1]=='t'){ dst=twt; cap=(int)sizeof(twt); }
                          else { dst=twf; cap=(int)sizeof(twf); }
                          { int di=0; while (*q && *q!=' ' && *q!='\n' && di<cap-1) dst[di++]=*q++;
                            dst[di]=0; while (*q && *q!=' ' && *q!='\n') q++; }
                          if (kk[1]=='b') twar=1;   // the header anchors the snapshot; the rest are its luggage
                      }
                      int vv=parse_int(q);
                      if (kk[0]=='b' && kk[1]=='k' && kk[2]==0) pl->banked=vv*0.5f;
                      else if (kk[0]=='b' && kk[1]=='b' && kk[2]==0) pl->beans=vv;
                      else if (kk[0]=='f' && kk[1]=='p' && kk[2]==0) bfp=vv;
                      else if (kk[0]=='h' && kk[1]=='s') pl->hatstand=vv?1:0;
                      else if (kk[0]=='m' && kk[1]=='l') pl->in_mall=vv?1:0;
                      else if (kk[0]=='a' && kk[1]=='r') pl->in_arcade=vv?1:0;
                      else if (kk[0]=='w' && kk[1]=='o') bwo=vv&32767;
                      else if (kk[0]=='w' && kk[1]=='n') bwn=vv&32767;
                      else if (kk[0]=='b' && kk[1]=='d') bsp=vv;
                      else if (kk[0]=='b' && kk[1]=='x') bxp=vv;
                      else if (kk[0]=='b' && kk[1]=='1') b1=vv;
                      else if (kk[0]=='b' && kk[1]=='2') b2=vv;
                      else if (kk[0]=='b' && kk[1]=='3') b3=vv;
                      else if (kk[0]=='b' && kk[1]=='s') bsk=vv;
                      else if (kk[0]=='b' && kk[1]=='e') ben=vv;
                      else if (kk[0]=='g' && kk[1]=='x') bgx=vv;
                      else if (kk[0]=='g' && kk[1]=='y') bgy=vv;
                      else if (kk[0]=='g' && kk[1]=='z') bgz=vv;
                      else if (kk[0]=='g' && kk[1]=='f') bgf=vv;
                      else if (kk[0]=='b' && kk[1]=='f') bfn=vv;
                      else if (kk[0]=='b' && kk[1]=='n' && kk[2]=='v') bnv=vv;
                      else if (kk[0]=='b' && kk[1]=='n') bnm=vv;
                      else if (kk[0]=='b' && kk[1]=='c') bcl=vv;
                      else if (kk[0]=='s' && kk[1]=='m') bsm=vv;
                      else if (kk[0]=='s' && kk[1]=='q' && kk[2]=='2') bsq2=vv;
                      else if (kk[0]=='e' && kk[1]=='r') ber=vv;
                      else if (kk[0]=='e' && kk[1]=='2') be2=vv;   // DEEPER DOORS (feat 11): absent token = Chapter 2 unstarted
                      else if (kk[0]=='s' && kk[1]=='b') bsb=vv;
                      else if (kk[0]=='p' && kk[1]=='d') bpd=vv;
                      else if (kk[0]=='t' && kk[1]=='d') btd=vv;
                      else if (kk[0]=='c' && kk[1]=='b') bcb=vv;
                      else if (kk[0]=='c' && kk[1]=='s') bcs=vv;
                      else if (kk[0]=='c' && kk[1]=='p') bcp=vv;
                      else if (kk[0]=='c' && kk[1]=='q') bcq=vv;
                      else if (kk[0]=='c' && kk[1]=='r') bcr=vv;
                      else if (kk[0]=='w' && kk[1]=='r') bwr=vv;
                      else if (kk[0]=='t' && kk[1]=='p') btp=vv;
                      else if (kk[0]=='t' && kk[1]=='w') btw=vv;
                      else if (kk[0]=='c' && kk[1]=='x') bcx=vv;
                      else if (kk[0]=='c' && kk[1]=='y') bcy=vv;
                      else if (kk[0]=='c' && kk[1]=='m') bcm=vv;
                      else if (kk[0]=='c' && kk[1]=='n') bcn=vv;
                      else if (kk[0]=='c' && kk[1]=='g') bcg=vv;
                      else if (kk[0]=='c' && kk[1]=='t') bct=vv;
                      else if (kk[0]=='c' && kk[1]=='f') bcf=vv;
                      else if (kk[0]=='s' && kk[1]=='q') bsq=vv;
                      else if (kk[0]=='s' && kk[1]=='i') bsi=vv;
                      while (*q && *q!=' ' && *q!='\n') q++;
                  }
                  pl->whale_own=bwo; pl->whale_on=bwn;
                  G.fs_pool[idx]=bfp;
                  // BUG BOWL LEAGUE (feat 10): the league book,
                  // Player-level (it applies whether or not a buddy
                  // rides this pl-line), clamped field by field.
                  if (tbl_on){ BowlLeague *L=&G.blg[idx];
                      L->games = tbl[0]<0? 0 : tbl[0];
                      L->best = tbl[1]<0? 0 : tbl[1]>300? 300 : tbl[1];
                      L->pins = tbl[2]<0? 0 : tbl[2];
                      L->strikes = tbl[3]<0? 0 : tbl[3];
                      L->season = tbl[4]<0? 0 : tbl[4];
                      L->night = tbl[5]<0? 0 : tbl[5]>5? 5 : tbl[5];
                      L->spins = tbl[6]<0? 0 : tbl[6];
                      L->trophies = tbl[7]<0? 0 : tbl[7];
                      L->last_score = tbl[8]<0? 0 : tbl[8]>300? 300 : tbl[8];
                      L->last_place = tbl[9]<0? 0 : tbl[9]>4? 4 : tbl[9];
                      L->last_night = tbl[10]<0? 0 : tbl[10]>6? 6 : tbl[10];
                      L->last_season = tbl[11]<0? 0 : tbl[11];
                      L->fin_place = tbl[12]<0? 0 : tbl[12]>4? 4 : tbl[12];
                      L->fin_season = tbl[13]<0? 0 : tbl[13];
                  }
                  if (bsp>0){
                      Buddy *bd=&G.bud[idx];
                      smemset(bd, 0, sizeof(*bd));
                      bd->active=1; bd->species=(bsp-1)%BUDN; bd->xp=bxp; bd->on_wall=-1;
                      bd->up_hp=b1; bd->up_atk=b2; bd->up_def=b3; bd->skills=bsk;
                      bd->energy = ben<0? 0 : ben>100? 100 : ben;
                      bd->bw_x = bgx; bd->bw_y = bgy; bd->bw_seed = bgz; bd->bw_started = bgf? 1:0;
                      bd->fainted=bfn? 1:0;
                      if (bnm<0) bd->name_idx = BUD_DEF_NAME[bd->species];
                      else if (bnv>=2) bd->name_idx = bnm%BUD_NAMES_N;
                      else {   // legacy save: the index names a slot in the OLD list — carry the name across
                          bd->name_idx = BUD_DEF_NAME[bd->species];
                          const char *want = BUD_ELF_LEGACY[bnm%20];
                          for (int n2=0;n2<BUD_NAMES_N;n2++)
                              if (scmp(BUD_ELF[n2], want)==0){ bd->name_idx=n2; break; }
                      }
                      bd->color = bcl%BUD_COLORS_N;
                      bd->story_started = bsm? 1:0; bd->story_bits = bsq; bd->story_bits2 = bsq2;
                      bd->ent_best = ber % 10000; bd->ent_done = ber / 10000;
                      bd->ent_sub_best = bsb;
                      bd->ent2_best = be2 % 10000; bd->ent2_done = be2 / 10000;   // DEEPER DOORS (feat 11)
                      bd->plat_done = bpd<0? 0 : bpd;
                      bd->tank_done = btd<0? 0 : btd;
                      bd->cas_best = bcb<0? 0 : bcb;
                      bd->cas_shufn = bcs<0? 0 : bcs;   // CASTLE SHUFFLE (feat 7): absent on pre-feature saves -> 0
                      bd->crt_best_pos = bcp<0? 0 : bcp;
                      bd->crt_cupbest = bcr<0? 0 : bcr;
                      for (int ti=0; ti<8; ti++) bd->crt_tbest[ti] = tmc[ti]<0? 0 : tmc[ti];    // MEDAL TIMES (feat 3)
                      for (int ti=0; ti<8; ti++) bd->crt_medal_best[ti] = tmd[ti]<0? 0 : tmd[ti]>3? 3 : tmd[ti];   // v1.27: earned medals, clamped to tier range
                      if (!tmd_on) crt_migrate_medals(bd);   // v1.27: a pre-v1.27 save's medals stand — seed them from its best times
                      for (int ti=0; ti<16; ti++) bd->plat_tbest[ti] = tmr[ti]<0? 0 : tmr[ti];
                      bd->war_prog = bwr<0? 0 : bwr>8? 8 : bwr;
                      bd->top_earned = btp<0? 0 : (btp & TOP_ALL);   // HAT TOPPERS (feat 5)
                      bd->top_worn = (btw>=1 && btw<=TOP_N && (bd->top_earned & TOP_BIT(btw)))? btw : 0;
                      // HAT PORTRAIT BOOTH (feat 9): the dial and the
                      // frozen portrait, clamped field by field — a
                      // hand-edited or future token can ask for
                      // anything; the booth only ever shows a pose
                      // the game's own tables can draw.
                      port_bd[idx] = tpt_on? (tpt[0]<0? 0 : tpt[0]%PORT_BD_N) : 0;
                      if (tpt_on && tpt[1]){
                          Portrait *pt2=&portrait[idx];
                          pt2->has = 1;
                          pt2->backdrop = tpt[2]<0? 0 : tpt[2]%PORT_BD_N;
                          pt2->hat_on = tpt[3]? 1 : 0;
                          pt2->top_worn = (tpt[4]>=0 && tpt[4]<=TOP_N)? tpt[4] : 0;
                          pt2->kg = tpt[5]<0? 0 : tpt[5];
                          pt2->stack_n = tpt[6]<0? 0 : tpt[6]>MAXSTACK? MAXSTACK : tpt[6];
                          pt2->species = tpt[7]<0? 0 : tpt[7]%BUDN;
                          pt2->bud_color = tpt[8]<0? 0 : tpt[8]%BUD_COLORS_N;
                          pt2->name_idx = tpt[9]<0? 0 : tpt[9]%BUD_NAMES_N;
                          pt2->pcolor = tpt[10]&3;
                          pt2->whale_on = tpt[11]&32767;
                          for (int ki=0; ki<pt2->stack_n; ki++)
                              pt2->kinds[ki] = (ki<tptk_n)? (unsigned char)(tptk[ki]-'0')&3 : 0;
                      }
                      bd->crt_best_time = bcq<0? 0 : bcq;
                      bd->craft_x = bcx; bd->craft_y = bcy;
                      bd->craft_wood = bcm % 1000; bd->craft_stone = (bcm/1000) % 1000; bd->craft_fiber = bcm / 1000000;
                      bd->craft_car = bcn % 1000; bd->craft_pies = (bcn/1000) % 1000; bd->craft_stews = bcn / 1000000;
                      bd->craft_gear = bcg;
                      bd->craft_tod = bct * 0.1f; if (bd->craft_tod<0 || bd->craft_tod>240) bd->craft_tod = 40;
                      bd->craft_fish = bcf;
                      bd->craft_bn = tcw_bn;
                      for (int bi=0;bi<tcw_bn;bi++){ bd->craft_btx[bi]=tcw_x[bi]; bd->craft_bty[bi]=tcw_y[bi]; bd->craft_bk[bi]=tcw_k[bi]; bd->craft_bhp[bi]=tcw_h[bi]; }
                      if (twar){   // rebuild the canonical WAR! snapshot text for this player
                          char *sp = war_snap[idx];
                          sp = sappend(sp, "wb="); sp = sappend(sp, twb);
                          sp = sappend(sp, " wu="); sp = sappend(sp, twu);
                          sp = sappend(sp, " ws="); sp = sappend(sp, tws);
                          sp = sappend(sp, " wg="); sp = sappend(sp, twg);
                          sp = sappend(sp, " wt="); sp = sappend(sp, twt);
                          sp = sappend(sp, " wf="); sp = sappend(sp, twf);
                          *sp = 0; war_snap_on[idx] = 1;
                          war_snap_reg[idx] = parse_int(twb);   // v1.18: the wb header's first field is the region
                      }
                      bd->craft_gn = tcd_gn;
                      for (int gi=0;gi<tcd_gn;gi++) bd->craft_gone[gi]=tcd_g[gi];
                      { int pk=bsi; for (int i2=0;i2<4;i2++){ bd->items[i2]=pk%10; pk/=10; } }
                      buddy_refresh(idx);
                      topper_refresh(idx, 0);   // HAT TOPPERS: derive from the records just applied (silent at load)
                      bd->hp = bd->fainted? 0 : bd->maxhp;   // a fainted buddy loads still fainted
                      bd->x=pl->x+26; bd->y=pl->y+12;
                  }
                }
                pl->dmg_mul = 1.0f + 0.25f*pl->upg[0];
                pl->rate_mul = 1.0f + 0.18f*pl->upg[1];
                pl->shield_max = shield_for(pl->upg[2]);
                pl->shield = pl->shield_max;
                pl->maxhp = 100 + 25*pl->upg[3];
            }
        }
        while (*s && *s!='\n') s++;
        if (*s=='\n') s++;
    }
    s = savebuf;
    while (*s){
        if (s[0]=='w' && s[1]=='l' && s[2]=='=' && s[3]==' '){
            const char *q=s+4;
            for (int i=0;i<MAXWALLS;i++) if (!G.walls[i].alive){
                Wall *w=&G.walls[i];
                w->x=parse_flt(q); while (*q && *q!=' ') q++; q++;
                w->y=parse_flt(q); while (*q && *q!=' ') q++; q++;
                w->hp=parse_flt(q); while (*q && *q!=' ') q++; q++;
                w->maxhp=parse_flt(q); while (*q && *q!=' ') q++; q++;
                w->horiz=parse_int(q); while (*q && *q!=' ' && *q!='\n') q++; if (*q==' ') q++;
                // stack count rides at the end of the line (Nov 7);
                // older saves have nothing there — one wall, as saved
                w->stack = (*q && *q!='\n')? parse_int(q) : 1;
                if (w->stack<1) w->stack=1; if (w->stack>999) w->stack=999;
                w->alive=1;
                break;
            }
        }
        while (*s && *s!='\n') s++;
        if (*s=='\n') s++;
    }
    // absorb any legacy stacks (saves from before the absorb rule can
    // hold several entries on one spot): same place, same direction
    // becomes one wall with the summed strength before pools rebuild —
    // and (Nov 7) the merged pile keeps COUNT of its layers, so the
    // stack renders, tiers and sheds like the walls it was made of
    for (int i=0;i<MAXWALLS;i++) if (G.walls[i].alive==1)
        for (int j=i+1;j<MAXWALLS;j++) if (G.walls[j].alive==1 && G.walls[j].horiz==G.walls[i].horiz){
            float dx=G.walls[j].x-G.walls[i].x, dy=G.walls[j].y-G.walls[i].y;
            if (dx*dx+dy*dy < 36){ G.walls[i].hp += G.walls[j].hp; G.walls[i].maxhp += G.walls[j].maxhp;
                G.walls[i].stack += G.walls[j].stack>0? G.walls[j].stack : 1;
                if (G.walls[i].stack>999) G.walls[i].stack=999;
                G.walls[j].alive=0; }
        }
    walls_recompute_ex(1);   // pooled health rebuilds from the saved fractions
    if (!G.p[0].connected) player_fresh(&G.p[0], G.my_name, 0, 0, 70), G.p[0].connected=1, G.p[0].is_local=1;
    // Daniel's rule (Nov 2): a loaded run always resumes at the
    // centre of the Bug Mall, wherever the save was made. Restoring
    // in place put mall saves about 120px north of where the player
    // stood (the interior lives at MALL_CY, far from the world
    // spot the raw numbers suggest) and scattered everyone else.
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        G.p[i].x = i==0? 0.0f : 34.0f*i; G.p[i].y = MALL_CY + (i==0? 0.0f : 10.0f);
        G.p[i].in_mall=1; G.p[i].in_store=0; G.p[i].station=-1;
        // the resume point is the Mall centre, so the Arcade flag the
        // save carried (ar=, written wherever the run was saved) must
        // NOT survive the load: a player saved mid-Arcade used to
        // reload standing in the Mall still flagged in_arcade, and
        // arcade_door_at (which requires !in_arcade) then refused
        // them re-entry until they left by the exit mat (found by the
        // feat-7 battery; fixed in the v1.25 E2E pass).
        G.p[i].in_arcade=0;
        srcroom_reset_all();   // v1.26: the Source Room is a live visit, never a resume point (same rule as the Arcade flag above)
        // a fainted buddy can't follow its owner home, so the load used
        // to leave it lying wherever the save was made — invisible, and
        // indistinguishable from never having bought it (Nov 3). Carry
        // it home: it wakes up on the mall floor beside its owner.
        if (G.bud[i].active && G.bud[i].fainted){
            G.bud[i].x = G.p[i].x + 26; G.bud[i].y = G.p[i].y + 12;
        }
    }
    if (G.bud[0].active && G.bud[0].fainted)
        spawn_floater(G.p[0].x, G.p[0].y - 30, "YOUR BUDDY IS FAINTED - REVIVE IT AT BANZAI BUDDIES", COL(255,180,150));
    // local co-op: claim back the saved P2 (same slot, same run). If the
    // save predates co-op there is no P2 line — with two controllers
    // plugged in, P2 drops in fresh beside P1 instead.
    G.p2_slot=-1; coop_reset_views();
    if (G.coop_pref){
        for (int i=1;i<MAXP;i++)
            if (G.p[i].connected && scmp(G.p[i].name, "P2")==0){ G.p2_slot=i; break; }
        if (G.p2_slot<0 && IN.pad_count>=2) coop_spawn_p2(G.p[0].x-30, G.p[0].y);
    }
    // the local user's Whales unlocks come from the profile, not the run
    G.p[0].whale_own=G.whale_own; G.p[0].whale_on=G.whale_on;
    if (G.p2_slot>=0){ G.p[G.p2_slot].whale_own=G.whale_own; G.p[G.p2_slot].whale_on=G.whale_on; }
    // WEEKLY CHALLENGE (v1.25 feature 6): with every record the
    // baselines read now applied (and P2 claimed back), open the
    // current week for this slot — a save from an older week, or
    // from before the board existed, rolls over here, silently;
    // the board tells the story.
    wk_rollover();
    G.camx=G.p[0].x; G.camy=G.p[0].y;
    G.state=GS_PLAY;
}

