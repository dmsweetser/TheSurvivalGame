// THE SURVIVAL GAME — sim/sim.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from world.c lines 1508-1541 in the Nov 2026 fine-grained refactor; code unchanged.

static void update_sim(float dt){
    G.time += dt;
    { static int prev_shield = 0; int sh = shield_active();
      if (sh && !prev_shield) sfx(8);                 // shield rises: firewall ping
      prev_shield = sh; }
    update_players(dt);
    update_projectiles(dt);
    update_bugs(dt);
    // Bug Bowl games tick for whoever is standing at the lanes
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].in_store && G.p[i].station==4)
        bowl_update(i, dt);
    update_buddies(dt);
    update_fx(dt);
    update_payment(dt);
    spawner(dt);
    // game over: everyone connected is dead
    int any_conn=0, any_alive=0;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){ any_conn=1; if (G.p[i].alive) any_alive=1; }
    if (any_conn && !any_alive && (G.state==GS_PLAY || G.state==GS_STORE)){
        G.state=GS_OVER; G.gameover_t=0; G.has_save=0;
        // the run is finished: empty its slot so CONTINUE can't
        // resurrect it (records are banked by save_records below)
        if (G.slot>=1 && G.slot<=3 && G.slot_has[G.slot]){
            plat_save_slot(G.slot, "TSG3\n", 5);
            G.slot_has[G.slot]=0; refresh_has_save();
        }
        save_records();
        sfx(18);
    }
    // autosave
    G.autosave_t -= dt;
    if (G.autosave_t<=0){ G.autosave_t=5.0f; if (G.state==GS_PLAY||G.state==GS_STORE){ for (int i=0;i<MAXP;i++) if (G.p[i].connected) topper_refresh(i, 1); do_save(); } }
}

