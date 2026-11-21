// THE SURVIVAL GAME — music/engine.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from music.c lines 575-623 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// COMFORT (v1.25 feature 2): the two gain knobs + mute, applied at the
// only two places notes are born — the event queue (every sfx() and
// qnote() one-shot: clicks, splats, win jingles) and the sequencer
// (the music itself). -1 means the bus is silent (mute on, or that
// volume at 0) and the note is NOT emitted at all, so a muted game
// leaves the platform synth/MIDI completely quiet. Otherwise the
// velocity scales linearly, vel * vol / 10 — at the default 10 both
// helpers are the identity and the mix is byte-identical to before.
// ---------------------------------------------------------------------------
static int ev_vel(int vel){ if (G.mute_all || G.vol_sfx<=0) return -1; return vel * G.vol_sfx / 10; }
static int mus_vel(int vel){ if (G.mute_all || G.vol_music<=0) return -1; return vel * G.vol_music / 10; }
static void mus_note(int ch, int note, int vel, int instr){
    int v = mus_vel(vel); if (v>0) plat_note(ch, note, v, instr);
}
static void update_audio(float dt){
    mus_meter(dt);   // feat 15: the danger meter runs every frame; the latch moves at bars
    // sfx queue
    for (int i=0;i<evq_n;i++){
        evq[i].delay -= dt;
        if (evq[i].delay<=0 && evq[i].note>=0){
            int v = ev_vel(evq[i].vel);
            if (v>0) plat_note(evq[i].ch, evq[i].note, v, evq[i].instr);
            evq[i].note=-1;
        }
    }
    int w=0; for (int i=0;i<evq_n;i++) if (evq[i].note>=0) evq[w++]=evq[i];
    evq_n=w;
    // music sequencer — the Beethoven ladder (see mus_piece): serene
    // while the run is calm, fiercer as the threat climbs
    mus_step_t -= dt;
    if (mus_step_t<=0){
        int pc = mus_piece();
        const Piece *P = &PIECES[pc];
        mus_step_t += P->step;
        if (mus_step_t < -P->step) mus_step_t = 0;      // long stall: resync, don't machine-gun
        if ((mus_step % 16)==0) mus_lvl = mus_desired;   // feat 15: the intensity latch moves at bar boundaries only
        int s = mus_step % P->len;
        int menu = (pc==0 && G.state!=GS_PLAY && G.state!=GS_STORE);
        int kz = (G.whale_on & 16384)? 4 : -1;   // WHALES ONLY: KAZOO MODE
        int sparse = (mus_ctx_now!=0 && mus_lvl==0);   // feat 15: SPARSE = the lead alone; bass + accompaniment rest
        int b = P->bass[s]; if (b>=0 && !sparse) mus_note(1, b, menu? 60 : 78, kz>=0? 4 : 1);
        int m = P->mid[s];  if (m>=0 && !sparse) mus_note(2, m, menu? 40 : (pc==0? 56 : 32), kz>=0? 4 : 2);
        int l = P->lead[s]; if (l>=0) mus_note(0, l, menu? (P->lv*3)/4 : P->lv, kz>=0? 4 : 0);
        if ((pc==2 || pc==3) && s%4==0 && !menu && !sparse) mus_note(2, 42, 8, 3);   // a heartbeat under the storm
        mus_step++;
    }
}

