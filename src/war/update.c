// THE SURVIVAL GAME — war/update.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from war.c lines 2498-2582 in the Nov 2026 fine-grained refactor; code unchanged.

static void war_update(int pi, float dt, const BwIn *in){
    BwSim *S = &BWS[pi]; Buddy *b = &G.bud[pi];
    if (S->war_msgt > 0) S->war_msgt -= dt;
    if (S->war_grace > 0) S->war_grace -= dt;
    if (S->war_guest){ war_guest_update(pi, dt, in); return; }   // feat 13: a guest commands the owner's battle, never their own
    if (S->war_duo>=0){   // feat 13: link upkeep — a guest who left WAR! (any door) dissolves the link within a frame
        int g = S->war_duo;
        BwSim *GS = (g>=0 && g<MAXP)? &BWS[g] : 0;
        if (!GS || g!=war_other_local(pi) || !G.bud[g].active || !G.bud[g].in_bw || G.bud[g].bw_mode!=8 || GS->war_guest!=pi+1){
            if (S->war_pause && S->war_pause_by==g) S->war_pause_by = pi;   // a departed pauser hands the menu to the owner
            war_link_break(pi);
        }
    }
    if (S->war_state==0){ war_map_update(pi, dt, in); return; }
    if (S->war_state==4){ war_setup_update(pi, dt, in); return; }   // v1.19: the skirmish setup panel — the battle clock does not run here
    if (S->war_state==2 || S->war_state==3){
        S->war_endt -= dt;
        if (pi==G.my_id && IN.tapped){ IN.tapped=0; if (S->war_endt > 0.4f) S->war_endt = 0.4f; }
        if (S->war_endt <= 0){
            war_clear_field(S);
            S->war_state=0; S->war_map_sel = b->war_prog<8? b->war_prog : 7; S->war_grace=0.3f;
            if (S->war_duo>=0){ int g=S->war_duo; war_link_break(pi); BWS[g].war_state=0; BWS[g].war_grace=0.3f; }   // feat 13: a decided battle releases the guest to their own map
        }
        return;
    }
    if (S->war_pause){   // v1.17: the battle holds its breath — only the pause menu's inputs run; war_t, waves, units and structures all stay frozen
        int linked = S->war_duo>=0;   // feat 13: shared pause — only the pauser resumes it or drives it; the other commander waits
        if (in->exit_ && (!linked || S->war_pause_by==pi)){ S->war_pause=0; S->war_grace=0.3f; sfx(6); return; }   // Esc again = RESUME
        if (!linked || S->war_pause_by==pi) war_pause_input(pi, dt);
        else if (pi==G.my_id){ IN.tapped=0; IN.weapon_key=0; }   // feat 13: the waiting commander's taps/keys are eaten, not queued into the resume frame
        return;
    }
    S->war_t += dt;
    if (S->war_demoarm>=0 && S->war_demoarm != S->war_selstruct) S->war_demoarm=-1;   // v1.21: an armed DEMOLISH lives only while its building stays selected
    for (int i=0;i<WAR_MAXU;i++) if (S->war_uflash[i]>0) S->war_uflash[i] -= dt;   // cosmetic timers (hit flashes + particles): they only ever feed the renderer
    for (int s=0;s<WAR_MAXS;s++) if (S->war_sflash[s]>0) S->war_sflash[s] -= dt;
    for (int i=0;i<24;i++) if (S->war_ft[i]>0) S->war_ft[i] -= dt;
    for (int t=0;t<S->war_tn;t++) if (S->war_tdeadt[t] > 0) S->war_tdeadt[t] -= dt;   // scorch marks fade
    if (in->exit_){   // Esc in a battle (v1.17): the pause menu — RESUME / RESTART / SAVE & QUIT — not an abandonment
        S->war_pause=1; S->war_pause_by=pi; S->war_pause_sel=0; sfx(6);   // feat 13: the pauser is recorded — the pause is theirs to drive. v1.31: the keyboard selection starts on RESUME
        return;
    }
    war_command_input(pi, dt, in);   // feat 13: the drag / key-pan / tap / weapon-key / pad section, extracted verbatim — see war_command_input above (the old P2 centre-tap line is gone there, deliberately)
    S->war_wavet -= dt;
    if (S->war_wavet <= 0){
        S->war_wavet = war_wave_dt(S->war_region);
        // v1.21: a wave is BOUGHT, not conjured — each hive spends
        // its own purse, in this order: (1) replace dead bug
        // harvesters first, because a hive that cannot harvest
        // cannot fight for long; (2) buy the wave, bug by bug,
        // stopping the moment the purse can't cover the next one —
        // a partial wave is a real wave, and a hive that buys
        // nothing sends nothing and says nothing; (3) refill the
        // guard post only from real surplus (>= 60 left over).
        int sent = 0;
        for (int s=0;s<WAR_MAXS;s++)
            if (S->war_shp[s]>0 && S->war_stype[s]==WAR_S_HIVE){
                while (war_hive_bharv(S, s) < 2 && S->war_hbits[s] >= WAR_COST_BHARV){
                    if (war_spawn_bharv(S, s) < 0) break;
                    S->war_hbits[s] -= WAR_COST_BHARV;
                }
                int k = 2 + (S->war_region>=4? 1:0);
                for (int j=0;j<k;j++){
                    int type = war_bug_type_next(S);
                    int price = (type==WAR_U_SPIT)? WAR_COST_SPIT : WAR_COST_CRAWL;
                    if (S->war_hbits[s] < price) break;
                    if (war_spawn_bug_t(S, s, 1, type) < 0) break;   // capped/full: nothing bought, nothing charged
                    S->war_hbits[s] -= price;
                    sent = 1;
                }
                int gar = S->war_region>=2? 3 : 2;
                while (war_hive_guards(S, s) < gar && S->war_hbits[s] >= 60){
                    if (war_spawn_bug_t(S, s, 0, WAR_U_CRAWL) < 0) break;
                    S->war_hbits[s] -= WAR_COST_CRAWL;
                }
            }
        if (sent) war_note(S, "THE HIVE SENDS A WAVE!");
    }
    for (int s=0;s<WAR_MAXS;s++) if (S->war_shp[s]>0) war_struct_update(pi, s, dt);
    for (int i=0;i<WAR_MAXU;i++) if (S->war_uhp[i]>0) war_unit_update(pi, i, dt);
    war_separate(S);
    war_proj_update(pi, dt);
    war_fog_update(S);
    if (S->war_ordt > 0) S->war_ordt -= dt;
}
