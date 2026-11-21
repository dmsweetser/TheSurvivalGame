// THE SURVIVAL GAME — banzai/entrances.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from banzai.c lines 1847-2559 in the Nov 2026 fine-grained refactor; code unchanged.

// --- ENTRANCES (Nov 5, Daniel) -------------------------------------------------
// A Doors-like hotel run inside Banzai World, built for Wesley: the loop
// of Doors (numbered doors, one-rule monsters, hide, hunt for keys and
// loot) with the horror drained out — getting caught is a bonk and a
// kazoo, never a scare. Daniel's verdict on the original: too scary.
// Rooms are uniform 12x8-tile slabs laid end to end along +x; room k's
// right wall holds ENTRANCE k+1. Everything about a room derives from
// hash(seed, k), so the hotel is endless and needs no stored map.
// Tiles: 0 floor, 1 wall, 2 door (closed), 3 bush (hide in it),
// 4 drawer, 5 shelf, 6 tent, 7 the EXIT door, 8 the lobby door home.
#define ENT_STRIDE 15        // room width in tiles (was 12 — bigger rooms, longer runs to the door and the bushes: Daniel, Nov 5)
static int ent_duet_door(int pi, int k);   // DEEPER DOORS (v1.25 feat 11): defined with the door machinery below
static void ent_plate_pos(int pi, int k, int which, float *px, float *py);
static u32 ent_rnd(Buddy *b, int k, int salt){
    // DEEPER DOORS (v1.25 feat 11): a Chapter 2 run re-deals the
    // hotel from the same seed — the chapter constant mixes into
    // the seed word, so every derived fact (door rows, locks,
    // dupes, furniture, keys) deals fresh while Chapter 1's deal
    // is bit-for-bit the shipped one (its word never sees the XOR).
    u32 sd = (u32)b->bw_seed ^ 0xE77u;
    if (BWS[b - G.bud].ent_chap==2) sd ^= 0xDEE9u;
    return hash2(k*31+7, salt*131+3, sd);
}
// the run's exit room: 51 for Chapter 1, 40 (a landing — tent,
// confetti and all) for DEEPER DOORS (v1.25 feat 11)
static int ent_final_room(BwSim *S){ return S->ent_chap==2? 40 : 51; }
static int ent_room_of(float x){ int k = (int)(x / (ENT_STRIDE*BW_TILE)) + 1; return k<1? 1 : k; }
static float ent_room_x0(int k){ return (float)((k-1)*ENT_STRIDE*BW_TILE); }   // left wall face
static int ent_in_chase_seg(int k){ for (int d=0; d<5; d++){ int s=k-d; if (s>=19 && s%30==19) return 1; } return 0; }   // Inspector segments: rooms 19-23, 49-53, 79-83 — at least 30 doors apart, five doors long (Daniel, Nov 6)
static int ent_door_locked(int pi, int k){
    Buddy *b=&G.bud[pi]; BwSim *S=&BWS[pi];
    if (ent_duet_door(pi, k)) return 0;   // DEEPER DOORS (feat 11): the duet is the lock — mechanics don't stack
    if (S->ent_sub) return ent_rnd(b, k, 2400) % 5 == 0 && k>=3;
    if (k<3 || k==25 || k==50 || k==51) return 0;   // 25 is the Library: the fuses are its lock
    if (ent_in_chase_seg(k)) return 0;
    return ent_rnd(b, k, 400) % 5 == 0;
}
static int ent_dupe_room(Buddy *b, int k){
    if (k<4 || k==25 || k%10==0 || ent_in_chase_seg(k)) return 0;
    return ent_rnd(b, k, 305) % 10 < 6;   // painted doors in most rooms now (Daniel, Nov 6) — and they look the part
}
// the real door is not always in the middle any more (Daniel, Nov 6):
// its row wanders per room, and the west door of the next room is the
// same doorway seen from the other side
static int ent_door_ly(Buddy *b, int k){
    static const int ROWS[5] = { 2, 3, 4, 5, 6 };
    return ROWS[ent_rnd(b, k, 350) % 5];
}
static int ent_doory(Buddy *b, int k){ return ent_door_ly(b, k)*BW_TILE + 24; }
// DEEPER DOORS (v1.25 feat 11): a duet door opens for two friends
// at once — the buddy on one brass plate, Lumen on the other.
// Door 4 is always one (the tutorial); beyond it, one door in
// four by the hash, never out of a landing (landings stay pure
// rest, the exclusion painted dupes use) and never inside the
// Inspector's segment (chase doors fly open — a duet door there
// would lie).
static int ent_duet_door(int pi, int k){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->ent_chap!=2 || S->ent_sub) return 0;
    if (k==4) return 1;
    if (k<5 || k%10==0 || ent_in_chase_seg(k)) return 0;
    return ent_rnd(b, k, 4500) % 4 == 0;
}
// a duet door's two brass plates flank its doorway on the room
// side: one north of the door row, one south, a short step west
// of the wall — near enough to see the door, far enough to stand
static void ent_plate_pos(int pi, int k, int which, float *px, float *py){
    Buddy *b=&G.bud[pi];
    *px = ent_room_x0(k) + (ENT_STRIDE-1)*BW_TILE - 16;
    *py = ent_doory(b, k) + (which? 58 : -58);
}
// painted doors share the real door's row set, so row alone tells
// you nothing: 1-2 dupes, never on the real row
static int ent_dupe_n(Buddy *b, int k){ return ent_dupe_room(b, k)? 1 + (int)(ent_rnd(b, k, 307) % 2) : 0; }
static int ent_dupe_ly_i(Buddy *b, int k, int i){
    static const int ROWS[5] = { 2, 3, 4, 5, 6 };
    int real = ent_door_ly(b, k), cand[4], n = 0;
    for (int j=0;j<5;j++) if (ROWS[j]!=real) cand[n++] = ROWS[j];
    int a = cand[ent_rnd(b, k, 300) % 4];
    if (!i) return a;
    int c2 = cand[(ent_rnd(b, k, 300) % 4 + 1 + (int)(ent_rnd(b, k, 302) % 3)) % 4];
    if (c2==a) c2 = cand[(ent_rnd(b, k, 300) % 4 + 2) % 4];
    return c2;
}
static int ent_dupe_ly(Buddy *b, int k){ return ent_dupe_ly_i(b, k, 0); }
// the noise-sensitive Big Bug keeps a few ordinary rooms too now
// (Daniel, Nov 6) — nowhere the chase, a landing, or the Library
static int ent_bigbug_room(int k){
    if (k<6 || k==25 || k%10==0 || ent_in_chase_seg(k)) return 0;
    return k%13==7 || k%17==9;
}
static int ent_furn(int pi, int tx, int ty){
    Buddy *b=&G.bud[pi]; BwSim *S=&BWS[pi];
    if (tx<0) return 0;
    int k = tx/ENT_STRIDE + 1, lx = tx%ENT_STRIDE, ly = ty;
    if (S->ent_sub){   // the service floor: sparser, greyer — vents to hide in, the odd crate, a cot every 20
        if (k%20==0 && lx==3 && ly==1) return 6;
        int nd = (int)(ent_rnd(b, k, 2200) % 2);
        for (int i=0;i<nd;i++){
            int dx = 2 + (int)(ent_rnd(b, k, 2210+i) % (ENT_STRIDE-4)), dy = (ent_rnd(b, k, 2220+i)&1)? 1 : ENT_ROWS-2;
            if (lx==dx && ly==dy) return 4;
        }
        int nb = 3 + (int)(ent_rnd(b, k, 2100) % 3);   // the service floor gets its share too (3-5 vents)
        for (int i=0;i<nb;i++){
            int bx = 1 + (int)(ent_rnd(b, k, 2110+i) % (ENT_STRIDE-2)), by = 1 + (int)((ent_rnd(b, k, 2120+i)>>3) % (ENT_ROWS-2));
            if ((by==ent_door_ly(b, k-1) && bx<=2) || (by==ent_door_ly(b, k) && bx>=ENT_STRIDE-3)) continue;
            if (lx==bx && ly==by) return 3;
        }
        return 0;
    }
    if (k==25){   // the library: shelf rows with two gaps each
        if ((ly==3 || ly==6) && lx>=2 && lx<=ENT_STRIDE-3){
            int g1 = 2 + (int)(ent_rnd(b, 25, 900 + ly) % (ENT_STRIDE-4));
            int g2 = 2 + (int)(ent_rnd(b, 25, 940 + ly) % (ENT_STRIDE-4)); if (g2==g1) g2 = g1>=ENT_STRIDE-3? 2 : g1+1;
            if (lx!=g1 && lx!=g2) return 5;
        }
        return 0;
    }
    if (k%10==0 && lx==3 && ly==1) return 6;   // a landing's tent
    int nd = 1 + (int)(ent_rnd(b, k, 200) % 2);
    for (int i=0;i<nd;i++){
        int dx = 2 + (int)(ent_rnd(b, k, 210+i) % (ENT_STRIDE-4)), dy = (ent_rnd(b, k, 220+i)&1)? 1 : ENT_ROWS-2;
        if (lx==dx && ly==dy) return 4;
    }
    int nb = 4 + (int)(ent_rnd(b, k, 100) % 3);   // 4-6 bushes a room: more shelter, more often (Daniel, Nov 6)
    for (int i=0;i<nb;i++){
        int bx = 1 + (int)(ent_rnd(b, k, 110+i) % (ENT_STRIDE-2)), by = 1 + (int)((ent_rnd(b, k, 120+i)>>3) % (ENT_ROWS-2));
        if ((by==ent_door_ly(b, k-1) && bx<=2) || (by==ent_door_ly(b, k) && bx>=ENT_STRIDE-3)) continue;   // keep the doorway runs clear
        if (lx==bx && ly==by) return 3;
    }
    return 0;
}
static void ent_key_pos(int pi, int k, float *wx, float *wy){
    Buddy *b=&G.bud[pi]; BwSim *S=&BWS[pi];
    if (S->ent_sub){   // downstairs keys hide on the room's crate, rolled with downstairs salts
        int dx = 2 + (int)(ent_rnd(b, k, 2210) % (ENT_STRIDE-4)), dy = (ent_rnd(b, k, 2220)&1)? 1 : ENT_ROWS-2;
        *wx = ((k-1)*ENT_STRIDE + dx)*BW_TILE + 24; *wy = dy*BW_TILE + 10;
        return;
    }
    int nd = 1 + (int)(ent_rnd(b, k, 200) % 2);
    if (nd>=1){ int dx = 2 + (int)(ent_rnd(b, k, 210) % (ENT_STRIDE-4)), dy = (ent_rnd(b, k, 220)&1)? 1 : ENT_ROWS-2;
        *wx = ((k-1)*ENT_STRIDE + dx)*BW_TILE + 24; *wy = dy*BW_TILE + 10; }
    else { *wx = ((k-1)*ENT_STRIDE + 2 + (int)(ent_rnd(b, k, 410) % 7))*BW_TILE + 24; *wy = (ENT_ROWS-2)*BW_TILE + 24; }
}
static const int ENT_FUSE_TX[5] = { 2, 12, 7, 2, 12 };
static const int ENT_FUSE_TY[5] = { 1, 1, 4, 8, 8 };
static int ent_tile(int pi, int tx, int ty){
    Buddy *b=&G.bud[pi]; BwSim *S=&BWS[pi];
    if (tx<0 || ty<0 || ty>=ENT_ROWS) return 1;
    int k = tx/ENT_STRIDE + 1, lx = tx%ENT_STRIDE, ly = ty;
    if (!S->ent_sub && k==ent_final_room(S) && lx==ENT_STRIDE/2 && ly==ENT_ROWS-1) return 7;
    if (ly==0 || ly==ENT_ROWS-1) return 1;
    if (lx==0 || lx==ENT_STRIDE-1){
        if (lx==0 && k==1 && ly==ENT_DOOR_LY) return 8;   // the stairs home: always the middle of room 1's west wall
        int door = (lx==ENT_STRIDE-1)? k : k-1;
        if (door>=1 && ly==ent_door_ly(b, door)){
            int upto = S->ent_sub? S->ent_door_sub : S->ent_door_upto;
            if (door <= upto) return 0;
            if (!S->ent_sub && door==25 && S->ent_libdone) return 0;
            return 2;
        }
        if (!S->ent_sub && lx==ENT_STRIDE-1 && !S->ent_dupe_used && ent_dupe_room(b, k)
            && (ly==ent_dupe_ly_i(b, k, 0) || (ent_dupe_n(b, k)>1 && ly==ent_dupe_ly_i(b, k, 1)))) return 2;
        return 1;
    }
    if (!S->ent_sub && k==25 && S->ent_chap!=2 && lx==ENT_STRIDE-2 && ly==ENT_ROWS-2) return 9;   // the tall bookcase (the secret is behind it) — Chapter 1's only (feat 11)
    return ent_furn(pi, tx, ty);
}
static int ent_solid(int pi, float x, float y){
    int t = ent_tile(pi, bw_tileof(x), bw_tileof(y));
    return t==1 || t==2 || t==4 || t==5 || t==6 || t==7 || t==8 || t==9;
}
static int ent_solid_insp(int pi, float x, float y){
    // the Inspector's version: a real doorway (tile 2 on that door's
    // own row) is NOT solid to him — he comes through doors (Daniel,
    // Nov 6). A painted dupe door may share the row set but never the
    // right column-and-row at once, and stays a wall to him.
    Buddy *b=&G.bud[pi];
    int tx = bw_tileof(x), ty = bw_tileof(y);
    int t = ent_tile(pi, tx, ty);
    if (t==2){
        int k = tx/ENT_STRIDE + 1, lx = tx%ENT_STRIDE;
        int door = (lx==ENT_STRIDE-1)? k : k-1;
        return !(door>=1 && ty==ent_door_ly(b, door));
    }
    return t==1 || t==4 || t==5 || t==6 || t==7 || t==8 || t==9;
}
static int ent_room_looted(BwSim *S, int k){
    for (int i=0;i<S->ent_loot_n;i++) if (S->ent_loot_rooms[i]==k) return 1;
    return 0;
}
static void ent_reset(int pi){
    BwSim *S=&BWS[pi];
    S->ent_room = 1; S->ent_door_upto = 0; S->ent_key = 0; S->ent_bits = 0;
    S->ent_sticks = 0; S->ent_lant = 0; S->ent_ev = 0; S->ent_phase = 0;
    S->ent_dark = 0; S->ent_dupe_used = 0; S->ent_libdone = 0; S->ent_fuse_mask = 0;
    S->ent_t = 0; S->ent_grace = 0; S->ent_zpass = 0; S->ent_typs = 0;
    S->ent_sub = 0; S->ent_door_sub = 0; S->ent_passes = 1; S->ent_tamb = 7; S->ent_zt = 0;
    S->ent_chase = 0; S->ent_noise = 0; S->ent_loot_n = 0; S->ent_finished = 0;
    S->ent_chap = 1; S->ent_lux = 0; S->ent_luy = 0; S->ent_duet_t = 0;   // DEEPER DOORS (feat 11): bw_enter_mode applies the chapter after this reset
    S->ent_ix = 0; S->ent_iy = 0; S->ent_istun = 0; S->ent_bbx = 0; S->ent_bby = 0;
    S->ent_fn = 0; S->ent_mt = 0; S->ent_mstep = 0;
    S->ent_rvx = 0; S->ent_rvy = 0;
}
static void ent_room_setup(int pi, int k){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->ent_dupe_used = 0; S->ent_ev = 0; S->ent_phase = 0; S->ent_noise = 0; S->ent_fn = 0;
    if (S->ent_sub){   // the service floor has its own, worse manners
        S->ent_dark = (k%3==1)? 1 : 0;
        if (k%20==0){ /* a maintenance landing: quiet */ }
        else if (ent_rnd(b, k, 3300) % 100 < 14){ S->ent_ev = 7; S->ent_phase = 1; S->ent_t = 0.45f; S->ent_passes = 1;
            scpy(S->note, "THE AIR TEARS. SOMETHING IS COMING THROUGH.", 72); S->note_t = 2.2f; sfx(21); }
        else if (k%6==4){ S->ent_ev = 2; S->ent_phase = 1; S->ent_t = 3.0f; S->ent_zpass = 0;
            S->ent_passes = 2 + (int)(ent_rnd(b, k, 3350) % 3);
            scpy(S->note, "MANY LEGS. MANY FACES. THE MULTIBUG! INTO A VENT!", 72); S->note_t = 2.8f; sfx(20); }
        else if (S->ent_dark){ S->ent_ev = 3; S->ent_phase = 1; S->ent_twait = 1.3f; S->ent_typs = 3; }
        return;
    }
    S->ent_dark = (k%5==2 && k!=25)? 1 : 0;
    if (k>=19 && k%30==19 && !S->ent_chase){ S->ent_chase = k; S->ent_ev = 5; S->ent_phase = 0; }
    else if (S->ent_chase && k>=S->ent_chase && k<=S->ent_chase+4){ S->ent_ev = 5; S->ent_phase = 1; }
    // phase stays 1 across the segment rooms: he has always spawned by
    // the time one is entered (the buddy crosses the start room's
    // middle to leave it), and his position is world-continuous, so he
    // walks through each doorway in plain sight. Resetting to phase 0
    // here was the vanish Daniel saw (Nov 6): undrawn and idle until
    // the buddy crossed the new room's middle, then re-formed at its
    // west edge as if he'd teleported.
    else if (S->ent_chase && k>=S->ent_chase+5){ S->ent_chase = 0; S->ent_bits += 10;
        scpy(S->note, "YOU LOST THE INSPECTOR! +10 BITS", 72); S->note_t = 2.4f; sfx(11); }
    if (S->ent_ev==5){   // a chase room burns: floor fires to dodge, his idea of decor (Daniel, Nov 6)
        S->ent_fn = 0;
        int nf = 2 + (int)(ent_rnd(b, k, 5100) % 2);
        for (int i=0;i<nf && S->ent_fn<4;i++){
            int fx = 2 + (int)(ent_rnd(b, k, 5110+i*7) % (ENT_STRIDE-4));
            int fy = 1 + (int)((ent_rnd(b, k, 5120+i*7)>>3) % (ENT_ROWS-2));
            if ((fy==ent_door_ly(b, k-1) && fx<=3) || (fy==ent_door_ly(b, k) && fx>=ENT_STRIDE-4)) continue;   // never wall off a doorway
            if (ent_furn(pi, (k-1)*ENT_STRIDE + fx, fy)) continue;          // not inside the furniture
            S->ent_fx[S->ent_fn] = ent_room_x0(k) + fx*BW_TILE + 24;
            S->ent_fy[S->ent_fn] = fy*BW_TILE + 24;
            S->ent_fn++;
        }
    }
    if (S->ent_ev) return;
    if (k==25 && S->ent_chap!=2){   // the Library is Chapter 1's set piece (feat 11): Deeper room 25 is a quiet breather
        S->ent_ev = 6;
        if (!S->ent_libdone) S->ent_fuse_mask = 0;
        S->ent_bbx = ent_room_x0(25) + (ENT_STRIDE/2)*BW_TILE + 24; S->ent_bby = 1.6f*BW_TILE;
        S->ent_bbdir = 0; S->ent_bbcharge = 0; S->ent_t = 1.0f;
    }
    else if (k%10==0){ /* a landing: quiet, lamplit, safe */ }
    else if (k>=12 && ent_rnd(b, k, 3300) % 100 < 6){ S->ent_ev = 7; S->ent_phase = 1; S->ent_t = 0.45f; S->ent_passes = 1;
        scpy(S->note, "THE AIR TEARS. SOMETHING IS COMING THROUGH.", 72); S->note_t = 2.2f; sfx(21); }
    else if (k%7==3){ S->ent_ev = 1; S->ent_phase = 1; S->ent_t = 3.0f; S->ent_passes = 1;
        scpy(S->note, "YOU HEAR IT SCREAMING UP THE HALL... HIDE IN A BUSH!", 72); S->note_t = 2.8f; sfx(20); }
    else if (k%11==6){ S->ent_ev = 2; S->ent_phase = 1; S->ent_t = 3.0f; S->ent_zpass = 0; S->ent_passes = 3;
        scpy(S->note, "A SCREAM, BOTH WAYS... IT KEEPS COMING BACK. BUSH!", 72); S->note_t = 2.8f; sfx(20); }
    else if (S->ent_dark){ S->ent_ev = 3; S->ent_phase = 1; S->ent_twait = 1.3f; S->ent_typs = 3; }
    else if (k%8==5){ S->ent_ev = 4;
        S->ent_rvx = ent_room_x0(k) + (2 + (int)(ent_rnd(b, k, 700) % (ENT_STRIDE-4)))*BW_TILE + 24;
        S->ent_rvy = (2 + (int)(ent_rnd(b, k, 710) % (ENT_ROWS-4)))*BW_TILE + 24;
        scpy(S->note, "THE CODE REVIEWER IS IN. DO NOT MEET ITS EYES.", 72); S->note_t = 2.8f; }
    else if (ent_bigbug_room(k)){ S->ent_ev = 6;   // a Big Bug keeps this room too: mind your noise (Daniel, Nov 6)
        S->ent_bbx = ent_room_x0(k) + (3 + (int)(ent_rnd(b, k, 800) % (ENT_STRIDE-6)))*BW_TILE + 24;
        S->ent_bby = ((ent_rnd(b, k, 810)&1)? 2 : ENT_ROWS-3)*BW_TILE + 24;
        S->ent_bbdir = 0; S->ent_bbcharge = 0; S->ent_t = 1.0f;
        scpy(S->note, "SOMETHING ENORMOUS IS SNORING HERE. SOFT FEET.", 72); S->note_t = 2.8f; }
}
static void ent_enter_room(int pi, int k){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    S->ent_room = k;
    ent_room_setup(pi, k);
    if (S->ent_sub){ if (k > b->ent_sub_best) b->ent_sub_best = k; return; }
    // DEEPER DOORS (feat 11): each chapter keeps its own records —
    // a Deeper run never moves Chapter 1's best or done counts
    if (S->ent_chap==2){ if (k > b->ent2_best) b->ent2_best = k; }
    else if (k > b->ent_best) b->ent_best = k;
    if (k==ent_final_room(S) && !S->ent_finished){
        S->ent_finished = 1;
        if (S->ent_chap==2) b->ent2_done++; else b->ent_done++;
        buddy_award_xp(pi, 20);   // v1.19: a completed Entrances run pays the buddy, on top of the quest XP (Chapter 2 pays the same terms, feat 11)
        scpy(S->note, S->ent_chap==2? (b->active? "YOU MADE IT OUT! EVERY DEEPER DOOR, CLEARED! +20 BUDDY XP" : "YOU MADE IT OUT! EVERY DEEPER DOOR, CLEARED!")
                                    : (b->active? "YOU MADE IT OUT! EVERY ENTRANCE, CLEARED! +20 BUDDY XP" : "YOU MADE IT OUT! EVERY ENTRANCE, CLEARED!"), 72); S->note_t = 4.5f;
        do_save(); sfx(11);
    }
    else if (k==25 && S->ent_chap!=2){ scpy(S->note, "ENTRANCE 25: THE LIBRARY. SNEAK. FIND 5 FUSES.", 72); S->note_t = 3.6f; }
    else if (k==ent_final_room(S)){ scpy(S->note, "THE EXIT ROOM. GO HOME PROUD, OR KEEP GOING.", 72); S->note_t = 3.4f; }
    else if (S->ent_chap==2 && k==4){ scpy(S->note, "A DUET DOOR AHEAD. TWO PLATES, TWO FRIENDS.", 72); S->note_t = 3.2f; }
    else if (k%10==0){ scpy(S->note, "A SAFE LANDING. THE TENT RESTS AND SAVES.", 72); S->note_t = 3.0f; }
    else if (S->ent_dark && S->ent_ev!=1 && S->ent_ev!=2){ scpy(S->note, S->ent_chap==2? "IT IS DARK IN HERE. LUMEN GLOWS. LISTEN FOR A PSST." : "IT IS DARK IN HERE. LISTEN FOR A PSST.", 72); S->note_t = 2.8f; }
}
static void ent_bonk(int pi, float frac, float kx){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (S->ent_grace>0) return;
    b->hp -= b->maxhp * frac; if (b->hp<0) b->hp = 0;
    b->calm = 0; S->ent_grace = 1.6f;
    if (kx!=0){ float nx = b->x + kx; if (!bw_solid_b(b, nx, b->y)) b->x = nx; }
    sfx(4);
}
static void ent_neutralize(int pi, int via_ctrlz){
    BwSim *S=&BWS[pi];
    if ((S->ent_ev==1 || S->ent_ev==2 || S->ent_ev==7) && S->ent_phase==2){
        int wasev = S->ent_ev; S->ent_ev = 0; S->ent_phase = 0;
        scpy(S->note, wasev==7? "BONK! THE GLITCH SPITS IT BACK OUT." : via_ctrlz? "CTRL-Z! THE ZIP BUG UNDOES ITSELF." : "BONK! THE ZIP BUG SPINS AWAY DIZZY.", 72); S->note_t = 2.2f;
    } else if (S->ent_ev==5 && S->ent_phase==1){
        if (via_ctrlz){ S->ent_chase = 0; S->ent_ev = 0; S->ent_phase = 0;
            scpy(S->note, "CTRL-Z! THE INSPECTOR UNDOES HIMSELF.", 72); S->note_t = 2.4f; }
        else { S->ent_istun = 3.5f; S->ent_ix -= 110;
            scpy(S->note, "BONK! THE INSPECTOR SEES STARS.", 72); S->note_t = 2.2f; }
    } else if (S->ent_ev==3 && S->ent_phase==2){
        S->ent_bits += 2; S->ent_typs--;
        S->ent_phase = S->ent_typs>0? 1 : 0; if (!S->ent_typs) S->ent_ev = 0; S->ent_twait = 1.5f;
        scpy(S->note, "BONK! TYPO CORRECTED. +2 BITS", 72); S->note_t = 2.0f;
    } else if (S->ent_ev==6){
        S->ent_bbcharge = 0; S->ent_noise = 0;
        if (S->ent_room==25 && !S->ent_sub){ S->ent_bbx = ent_room_x0(25) + (ENT_STRIDE/2)*BW_TILE + 24; S->ent_bby = 1.6f*BW_TILE; }
        else { Buddy *b=&G.bud[pi]; int k = S->ent_room;   // a resident Big Bug just shuffles back to its corner
            S->ent_bbx = ent_room_x0(k) + (3 + (int)(ent_rnd(b, k, 800) % (ENT_STRIDE-6)))*BW_TILE + 24;
            S->ent_bby = ((ent_rnd(b, k, 810)&1)? 2 : ENT_ROWS-3)*BW_TILE + 24; }
        scpy(S->note, via_ctrlz? "CTRL-Z! THE BIG BUG BLINKS BACK TO ITS DESK." : "BONK! THE BIG BUG BACKS OFF, HUFFING.", 72); S->note_t = 2.2f;
    }
}
static void ent_update(int pi, float dt, const BwIn *in){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (in->exit_){ bw_exit(pi, 0); return; }
    if (S->ent_grace>0) S->ent_grace -= dt;
    if (b->atk_cd>0) b->atk_cd -= dt;
    // movement (the buddy's own BW walk, hotel tiles)
    float mx=0, my=0; int k2=in->keys;
    if (k2&K_UP) my-=1; if (k2&K_DOWN) my+=1; if (k2&K_LEFT) mx-=1; if (k2&K_RIGHT) mx+=1;
    int moving = (mx!=0 || my!=0);
    if (moving){
        float d=fsqrt(mx*mx+my*my); mx/=d; my/=d;
        b->aim = fatan2(my, mx);
        float sp = BUD_SPD[b->species%BUDN];
        float nx = b->x + mx*sp*dt, ny = b->y + my*sp*dt;
        if (!bw_solid_b(b, nx, b->y)) b->x = nx;
        if (!bw_solid_b(b, b->x, ny)) b->y = ny;
        b->walk_ph += dt*9;
        b->mv_t = 0.15f;
    }
    if (b->lunge_t>0) b->lunge_t-=dt;
    if (b->energy<BUD_ENERGY_MAX){ b->energy += BUD_EN_REGEN*dt; if (b->energy>BUD_ENERGY_MAX) b->energy=BUD_ENERGY_MAX; }
    b->bw_x = b->x; b->bw_y = b->y;
    // breath regen: the hotel is not as kind as the meadow (Daniel,
    // Nov 5 — harder) — six calm seconds, and a slower trickle
    b->calm += dt;
    if (b->calm>6 && b->hp>0 && b->hp<b->maxhp){ b->hp += 1.4f*dt; if (b->hp>b->maxhp) b->hp=b->maxhp; }
    // room tracking
    int rk = ent_room_of(b->x);
    if (rk != S->ent_room) ent_enter_room(pi, rk);
    int room = S->ent_room;
    // DEEPER DOORS (feat 11): Lumen follows the buddy, and a duet
    // door opens when two friends stand on its plates together
    if (S->ent_chap==2){
        float txp = b->x - 26, typ = b->y - 22, cap = 340;   // his hover point: just behind the shoulder
        int duet = ent_duet_door(pi, room) && room > S->ent_door_upto;
        float pax=0, pay=0, pbx=0, pby=0; int onplate = -1;
        if (duet){
            ent_plate_pos(pi, room, 0, &pax, &pay);
            ent_plate_pos(pi, room, 1, &pbx, &pby);
            float dax=b->x-pax, day=b->y-pay, dbx=b->x-pbx, dby=b->y-pby;
            if (dax*dax+day*day < 22*22) onplate = 0;
            else if (dbx*dbx+dby*dby < 22*22) onplate = 1;
            if (onplate>=0){ txp = onplate? pax : pbx; typ = onplate? pay : pby; cap = 420; }   // called to his plate, he hurries
        }
        float ldx = txp - S->ent_lux, ldy = typ - S->ent_luy;
        float ldl = fsqrt(ldx*ldx + ldy*ldy);
        if (ldl > 1){ float step = cap*dt; if (step > ldl) step = ldl; S->ent_lux += ldx/ldl*step; S->ent_luy += ldy/ldl*step; }
        if (duet && onplate>=0){
            float ox = onplate? pax : pbx, oy = onplate? pay : pby;
            float odx = S->ent_lux - ox, ody = S->ent_luy - oy;
            if (odx*odx + ody*ody < 16*16){
                S->ent_duet_t += dt;
                if (S->ent_duet_t >= 0.6f){
                    S->ent_duet_t = 0; S->ent_door_upto = room;
                    scpy(S->note, "TOGETHER! THE DUET DOOR OPENS.", 72); S->note_t = 2.6f; sfx(19); sfx(11);
                }
            } else if (S->ent_duet_t > 0) S->ent_duet_t -= dt*2;
        } else if (S->ent_duet_t > 0) S->ent_duet_t -= dt*2;
        if (S->ent_duet_t < 0) S->ent_duet_t = 0;
    }
    // ambient dread: now and then the hotel reminds you it is a hotel
    S->ent_tamb -= dt;
    if (S->ent_tamb<=0){
        S->ent_tamb = 9.0f + (float)(ent_rnd(b, room, 2900 + (int)(G.anim_t*3)) % 8);
        int landing = S->ent_sub? (room%20==0) : (room%10==0);
        if (!landing && S->note_t<=0){
            static const char *AMB[4] = { "SOMEWHERE, A DOOR CREAKED.", "THE LAMPS BUZZ. THEN, BRIEFLY, DON'T.",
                                          "SOMETHING SMALL SCUTTLED AWAY.", "BREATHING, SOMEWHERE. PROBABLY YOURS." };
            scpy(S->note, AMB[ent_rnd(b, room, 2950 + (int)(G.anim_t*5)) % 4], 72); S->note_t = 2.4f; sfx(6);
        }
    }
    float rx0 = ent_room_x0(room);
    int hidden = ent_tile(pi, bw_tileof(b->x), bw_tileof(b->y))==3;
    // --- the room's resident trouble ---
    if (S->ent_ev==1 || S->ent_ev==2 || S->ent_ev==7){
        if (S->ent_phase==1){
            S->ent_t -= dt;
            if (S->ent_t<=0){
                S->ent_phase = 2;
                S->ent_zdir = (S->ent_ev==2 && (S->ent_zpass&1))? -1 : 1;
                if (S->ent_ev==7) S->ent_zdir = (ent_rnd(b, room, 3400)&1)? -1 : 1;
                S->ent_zx = S->ent_zdir>0? rx0 + BW_TILE + 10 : rx0 + (ENT_STRIDE-1)*BW_TILE - 10;
                S->ent_zy = ent_doory(b, room);   // the pass rides the door row: that is the run the moths mark
                S->ent_zspd = S->ent_ev==7? 470.0f : S->ent_sub? 440.0f : 400.0f;
                S->ent_zt = 0;
                if (S->ent_ev!=7) sfx(20);   // the glitch brings its own noise
            }
        } else if (S->ent_phase==2){
            float prevzx = S->ent_zx;
            S->ent_zx += S->ent_zdir * S->ent_zspd * dt;
            // the roar (Daniel: LOUDER): a wall of rushing noise that
            // swells as the thing bears down on you
            S->ent_zt -= dt;
            if (S->ent_zt<=0){
                S->ent_zt = 0.09f;
                float adx = b->x - S->ent_zx; if (adx<0) adx=-adx;
                int vel = 92 - (int)(adx*0.07f); if (vel<52) vel=52; if (vel>92) vel=92;
                qnote(0,3,26,vel,3); qnote(0.02f,3,47,vel-25,1);
            }
            // like Rush, it owns the whole room: anywhere not hidden
            // when it sweeps past your position, it gets you (45% —
            // Doors does not do love taps)
            if (!hidden && S->ent_grace<=0 && prevzx != S->ent_zx && (prevzx - b->x)*(S->ent_zx - b->x) <= 0.0f){
                ent_bonk(pi, 0.45f, -S->ent_zdir*26);
                scpy(S->note, S->ent_sub? "IT GOT YOU! VENTS ARE FOR HIDING." : "THE ZIP BUG GOT YOU! BUSHES ARE FOR HIDING.", 72); S->note_t = 2.4f;
            }
            if (S->ent_zx < rx0 + 10 || S->ent_zx > rx0 + ENT_STRIDE*BW_TILE - 10){
                S->ent_zpass++;
                if (S->ent_zpass>=S->ent_passes){ int wasev = S->ent_ev; S->ent_ev = 0; S->ent_phase = 0;
                    if (wasev==2){ scpy(S->note, S->ent_sub? "THE MULTIBUG PEELS AWAY. FOR NOW." : "THE BOOMERANG BUG GIVES UP AT LAST.", 72); S->note_t = 2.2f; } }
                else { S->ent_phase = 1; S->ent_t = 2.0f; sfx(20); }   // every return pass gets a real warning too (Daniel, Nov 6)
            }
        }
    }
    else if (S->ent_ev==3){
        if (S->ent_phase==1){
            S->ent_twait -= dt;
            if (S->ent_twait<=0){
                u32 h = ent_rnd(b, room, 500 + S->ent_typs*17 + (int)(G.anim_t*7));
                S->ent_tyx = rx0 + (1 + (int)(h % (ENT_STRIDE-2)))*BW_TILE + 24;
                S->ent_tyy = (1 + (int)((h>>4) % (ENT_ROWS-2)))*BW_TILE + 24;
                S->ent_tt = 2.4f; S->ent_phase = 2; sfx(6);
                scpy(S->note, "psst... GO SAY HI BEFORE IT NIBBLES.", 72); S->note_t = 1.8f;
            }
        } else if (S->ent_phase==2){
            S->ent_tt -= dt;
            float dx=b->x-S->ent_tyx, dy=b->y-S->ent_tyy;
            if (dx*dx+dy*dy < 24*24){
                S->ent_bits += 2; S->ent_typs--; sfx(19);
                scpy(S->note, "YOU FOUND THE TYPO! +2 BITS", 72); S->note_t = 1.8f;
                S->ent_phase = S->ent_typs>0? 1 : 0; if (!S->ent_typs) S->ent_ev = 0; S->ent_twait = 1.5f;
            } else if (S->ent_tt<=0){
                ent_bonk(pi, 0.10f, 0);
                scpy(S->note, "THE TYPO NIBBLED YOU. TOO SLOW!", 72); S->note_t = 2.0f;
                S->ent_typs--;
                S->ent_phase = S->ent_typs>0? 1 : 0; if (!S->ent_typs) S->ent_ev = 0; S->ent_twait = 1.5f;
            }
        }
    }
    else if (S->ent_ev==4){
        float dx=S->ent_rvx-b->x, dy=S->ent_rvy-b->y; float d2=dx*dx+dy*dy;
        if (d2 < 180*180 && d2 > 1){
            float dl = fsqrt(d2);
            if ((fcos(b->aim)*dx + fsin(b->aim)*dy)/dl > 0.55f){
                b->hp -= b->maxhp * 0.16f * dt; if (b->hp<0) b->hp = 0; b->calm = 0;
            }
        }
    }
    else if (S->ent_ev==5 && S->ent_chase){
        if (S->ent_phase==0){
            if (b->x > rx0 + (ENT_STRIDE/2)*BW_TILE){
                S->ent_phase = 1;
                S->ent_ix = rx0 + BW_TILE + 14; S->ent_iy = ent_doory(b, S->ent_chase - 1);
                scpy(S->note, "THE INSPECTOR! RUN!", 72); S->note_t = 2.2f; sfx(16);
            }
        } else {
            // doors in a chase just fly open — no stopping for keys
            float ddx = (rx0 + (ENT_STRIDE-1)*BW_TILE + 24) - b->x, ddy = ent_doory(b, room) - b->y;
            if (ddx*ddx+ddy*ddy < 52*52 && room > S->ent_door_upto){ S->ent_door_upto = room; sfx(19); }
            if (room >= S->ent_chase+5){
                S->ent_chase = 0; S->ent_ev = 0; S->ent_phase = 0; S->ent_bits += 10;
                scpy(S->note, "YOU LOST THE INSPECTOR! +10 BITS", 72); S->note_t = 2.4f; sfx(11);
            } else {
                // the chase has a sound: a low pulse that keeps
                // missing the beat, and something far off screaming
                // about it (Daniel, Nov 6: scary music)
                S->ent_mt -= dt;
                if (S->ent_mt<=0){
                    S->ent_mt = 0.21f;
                    static const int CHN[8] = { 38, 38, 39, 38, 38, 41, 39, 36 };
                    int cn = CHN[S->ent_mstep & 7]; S->ent_mstep++;
                    qnote(0, 3, cn, 68, 2); qnote(0, 3, cn-12, 42, 3);
                    if ((S->ent_mstep & 7)==5) qnote(0.03f, 3, 87, 24, 0);
                }
                // floor fires: his rooms, his rules — step wide
                for (int i=0;i<S->ent_fn;i++){
                    float fdx=b->x-S->ent_fx[i], fdy=b->y-S->ent_fy[i];
                    if (fdx*fdx+fdy*fdy < 17*17 && S->ent_grace<=0){
                        ent_bonk(pi, 0.12f, 0);
                        scpy(S->note, "FIRE ON THE FLOOR! STEP WIDE!", 72); S->note_t = 2.0f;
                    }
                }
                if (S->ent_istun>0) S->ent_istun -= dt;
                else {
                    // he comes THROUGH the doorways (Daniel, Nov 6):
                    // in another room, he makes for the door tile
                    // between you first. His position lives in the
                    // world, never in a room setup, so crossing a
                    // threshold no longer vanishes and re-forms him —
                    // he walks through, in plain sight. A smidge
                    // faster than the buddy (1.10x), and quicker
                    // still while he is a room behind (1.18x more),
                    // so the pressure never lets up. Run.
                    float tx2 = b->x, ty2 = b->y;
                    int ik = ent_room_of(S->ent_ix), bk2 = ent_room_of(b->x);
                    if (ik < bk2){ tx2 = ent_room_x0(ik) + (ENT_STRIDE-1)*BW_TILE + 24; ty2 = ent_doory(b, ik); }
                    else if (ik > bk2){ tx2 = ent_room_x0(ik) + 24; ty2 = ent_doory(b, ik-1); }
                    float isp = BUD_SPD[b->species%BUDN] * 1.10f;
                    if (ik!=bk2) isp *= 1.18f;
                    float dx=tx2-S->ent_ix, dy=ty2-S->ent_iy; float dl=fsqrt(dx*dx+dy*dy);
                    if (dl>1){
                        float nx = S->ent_ix + dx/dl*isp*dt, ny = S->ent_iy + dy/dl*isp*dt;
                        if (!ent_solid_insp(pi, nx, S->ent_iy)) S->ent_ix = nx;
                        if (!ent_solid_insp(pi, S->ent_ix, ny)) S->ent_iy = ny;
                    }
                    float cx=b->x-S->ent_ix, cy=b->y-S->ent_iy;
                    if (cx*cx+cy*cy < 22*22 && S->ent_grace<=0){
                        ent_bonk(pi, 0.40f, 20);
                        scpy(S->note, "CAUGHT! THE INSPECTOR CONFISCATES SOME HP.", 72); S->note_t = 2.2f;
                        int back = room-1; if (back < S->ent_chase) back = S->ent_chase;
                        S->ent_ix = ent_room_x0(back) + BW_TILE + 14; S->ent_iy = ent_doory(b, back-1);
                        S->ent_istun = 2.2f;
                    }
                }
            }
        }
    }
    else if (S->ent_ev==6){
        // fuses first: walk over one to pocket it (the Library only —
        // a resident Big Bug elsewhere has no fuses to guard)
        if (room==25 && !S->ent_libdone){
            for (int i=0;i<5;i++){
                if (S->ent_fuse_mask>>i & 1) continue;
                float fx = ent_room_x0(25) + ENT_FUSE_TX[i]*BW_TILE + 24, fy = ENT_FUSE_TY[i]*BW_TILE + 24;
                float dx=b->x-fx, dy=b->y-fy;
                if (dx*dx+dy*dy < 24*24){
                    S->ent_fuse_mask |= 1<<i;
                    int n=0; for (int j=0;j<5;j++) if (S->ent_fuse_mask>>j & 1) n++;
                    if (n>=5){ S->ent_libdone = 1; scpy(S->note, "ALL 5 FUSES! THE LIBRARY DOOR CLICKS OPEN.", 72); S->note_t = 3.2f; sfx(11); }
                    else { char *q=S->note; q=sappend(q, "FUSE "); q=sapp_int(q, n); q=sappend(q, " OF 5."); *q=0; S->note_t = 1.8f; sfx(19); }
                }
            }
        }
        // the Big Bug: blind, but it hears footsteps
        float bdx = b->x - S->ent_bbx, bdy = b->y - S->ent_bby; float bd = fsqrt(bdx*bdx + bdy*bdy);
        if (S->ent_bbcharge>0){
            S->ent_bbcharge -= dt;
            float nx = S->ent_bbx + fcos(S->ent_bbdir)*235*dt, ny = S->ent_bby + fsin(S->ent_bbdir)*235*dt;
            if (!bw_solid_b(b, nx, S->ent_bby)) S->ent_bbx = nx;
            if (!bw_solid_b(b, S->ent_bbx, ny)) S->ent_bby = ny;
            if (bd < 27 && S->ent_grace<=0){
                ent_bonk(pi, 0.40f, 0);
                scpy(S->note, "THE BIG BUG SAT ON YOU. MOVE QUIETLY!", 72); S->note_t = 2.2f;
                S->ent_bbcharge = 0; S->ent_noise = 0;
            }
        } else {
            S->ent_t -= dt;
            if (S->ent_t<=0){ S->ent_t = 1.3f + (float)(ent_rnd(b, room, 800 + (int)(G.anim_t*3)) % 10) * 0.13f;
                S->ent_bbdir = (float)(ent_rnd(b, room, 810 + (int)(G.anim_t*5)) % 628) * 0.01f; }
            float nx = S->ent_bbx + fcos(S->ent_bbdir)*27*dt, ny = S->ent_bby + fsin(S->ent_bbdir)*27*dt;
            if (!bw_solid_b(b, nx, S->ent_bby)) S->ent_bbx = nx; else S->ent_bbdir += 1.7f;
            if (!bw_solid_b(b, S->ent_bbx, ny)) S->ent_bby = ny; else S->ent_bbdir += 1.7f;
            if (bd < 27 && S->ent_grace<=0){   // like Figure, it does not need to be charging to end you
                ent_bonk(pi, 0.40f, 0);
                scpy(S->note, "YOU BUMPED INTO THE BIG BUG. IT NOTICED.", 72); S->note_t = 2.2f;
                S->ent_noise = 0;
            }
            if (moving && bd < 160) S->ent_noise += 58*dt; else S->ent_noise -= 26*dt;
            if (S->ent_noise<0) S->ent_noise = 0;
            if (S->ent_noise>=100){
                S->ent_noise = 0; S->ent_bbcharge = 0.8f; S->ent_bbdir = fatan2(bdy, bdx);
                scpy(S->note, "IT HEARD YOU!", 72); S->note_t = 1.6f; sfx(16);
            }
        }
    }
    // --- hands: swing, patch, open, search ---
    if (in->sel && b->atk_cd<=0){
        b->atk_cd = 0.5f;
        int intarget = 0;
        if ((S->ent_ev==1 || S->ent_ev==2 || S->ent_ev==7) && S->ent_phase==2){ float dx=b->x-S->ent_zx, dy=b->y-S->ent_zy; intarget = dx*dx+dy*dy < 58*58; }
        else if (S->ent_ev==5 && S->ent_phase==1){ float dx=b->x-S->ent_ix, dy=b->y-S->ent_iy; intarget = dx*dx+dy*dy < 58*58; }
        else if (S->ent_ev==3 && S->ent_phase==2){ float dx=b->x-S->ent_tyx, dy=b->y-S->ent_tyy; intarget = dx*dx+dy*dy < 58*58; }
        else if (S->ent_ev==6){ float dx=b->x-S->ent_bbx, dy=b->y-S->ent_bby; intarget = dx*dx+dy*dy < 58*58; }
        if (intarget){
            if (S->ent_sticks>0){ S->ent_sticks--; sfx(4); ent_neutralize(pi, 0); }
            else if (b->items[3]>0){ b->items[3]--; sfx(11); ent_neutralize(pi, 1); }
            else { scpy(S->note, "NOTHING TO SWING. FIND A STICK IN A DRAWER!", 72); S->note_t = 2.0f; sfx(16); }
        }
    }
    if (in->wall && b->atk_cd<=0){
        b->atk_cd = 0.5f;
        if (b->items[0]>0 && b->hp < b->maxhp){ b->items[0]--; b->hp += b->maxhp*0.4f; if (b->hp>b->maxhp) b->hp=b->maxhp;
            scpy(S->note, "PATCH NOTES. MUCH BETTER.", 72); S->note_t = 1.8f; sfx(11); }
        else if (b->items[1]>0 && b->energy < BUD_ENERGY_MAX){ b->items[1]--; b->energy = BUD_ENERGY_MAX;
            scpy(S->note, "ENERGY DRINK! ZOOM.", 72); S->note_t = 1.8f; sfx(11); }
        else if (b->items[0]>0 || b->items[1]>0){ scpy(S->note, "ALL TOPPED UP ALREADY.", 72); S->note_t = 1.6f; }
        else { scpy(S->note, "NO SUPPLIES. CHECK THE DRAWERS.", 72); S->note_t = 1.8f; sfx(16); }
    }
    if (in->interact){
        float doorx = rx0 + (ENT_STRIDE-1)*BW_TILE + 24, doory = ent_doory(b, room);
        float dx=b->x-doorx, dy=b->y-doory;
        int upto = S->ent_sub? S->ent_door_sub : S->ent_door_upto;
        if (dx*dx+dy*dy < 54*54 && room > upto){
            if (ent_duet_door(pi, room)){ scpy(S->note, "A DUET DOOR. IT OPENS FOR TWO FRIENDS AT ONCE - STAND ON A PLATE!", 72); S->note_t = 2.6f; sfx(16); return; }
            if (!S->ent_sub && room==25 && !S->ent_libdone){ scpy(S->note, "IT NEEDS 5 FUSES BEFORE IT BUDGES. SHHH.", 72); S->note_t = 2.4f; sfx(16); return; }
            if (ent_door_locked(pi, room)){
                if (S->ent_key){ S->ent_key = 0; if (S->ent_sub) S->ent_door_sub = room; else S->ent_door_upto = room; scpy(S->note, "UNLOCKED!", 72); S->note_t = 1.6f; sfx(19); }
                else { scpy(S->note, "LOCKED. THE KEY IS HIDING IN THIS ROOM.", 72); S->note_t = 2.4f; sfx(16); }
                return;
            }
            if (S->ent_sub) S->ent_door_sub = room; else S->ent_door_upto = room; sfx(19); return;
        }
        if (!S->ent_sub && !S->ent_dupe_used && ent_dupe_room(b, room)){
            for (int di=0; di<ent_dupe_n(b, room); di++){
                float fx = doorx, fy = ent_dupe_ly_i(b, room, di)*BW_TILE + 24;
                float ddx=b->x-fx, ddy=b->y-fy;
                if (ddx*ddx+ddy*ddy < 54*54){
                    S->ent_dupe_used = 1; S->ent_grace = 0;
                    ent_bonk(pi, 0.12f, -20);
                    scpy(S->note, "DUPE! THAT DOOR WAS PAINTED ON. RUDE.", 72); S->note_t = 2.6f; sfx(16);
                    return;
                }
            }
        }
        if (room==1){
            // the west door of room 1 (the stairs home, or B-1's
            // stairwell) sits on its own fixed middle row (tile 8,
            // ENT_DOOR_LY) — NOT on room 1's east door row, which
            // wanders per room since batch C. Measuring from the east
            // row stranded the climb in B-1 whenever the east row was
            // off-centre (Nov 6: the buddy could press himself against
            // the stairs forever and never come up).
            float mdx=b->x-24, mdy=b->y-ENT_DOORY;
            if (mdx*mdx+mdy*mdy < 54*54){
                if (S->ent_sub){   // B-1's west door is a stairwell, not the mall
                    S->ent_sub = 0; S->ent_key = 0;
                    b->x = ent_room_x0(25) + (ENT_STRIDE-3)*BW_TILE + 24; b->y = (ENT_ROWS-2)*BW_TILE + 24; b->bw_x = b->x; b->bw_y = b->y;
                    scpy(S->note, "UP THE STAIRS. THE LIBRARY AGAIN. IT MISSED YOU.", 72); S->note_t = 2.8f; sfx(12);
                    return;
                }
                bw_exit(pi, 0); return;
            }
        }
        if (!S->ent_sub && room==ent_final_room(S)){
            float ex = ent_room_x0(room) + (ENT_STRIDE/2)*BW_TILE + 24, ey = (ENT_ROWS-1)*BW_TILE + 24;
            float edx=b->x-ex, edy=b->y-ey;
            if (edx*edx+edy*edy < 58*58){ bw_exit(pi, 0); return; }
        }
        if (!S->ent_sub && room==25 && S->ent_chap!=2){   // the tall bookcase, and what is behind it (Chapter 1's secret, feat 11)
            float bx2 = ent_room_x0(25) + (ENT_STRIDE-2)*BW_TILE + 24, by2 = (ENT_ROWS-2)*BW_TILE + 24;
            float ddx=b->x-bx2, ddy=b->y-by2;
            if (ddx*ddx+ddy*ddy < 58*58){
                if (!S->ent_libdone){ scpy(S->note, "A TALL BOOKCASE. IT WILL NOT BUDGE. THE ROOM FEELS UNFINISHED.", 72); S->note_t = 2.4f; sfx(16); return; }
                S->ent_sub = 1; S->ent_door_sub = 0; S->ent_key = 0;
                b->x = 2*BW_TILE + 24; b->y = ENT_DOORY; b->bw_x = b->x; b->bw_y = b->y;
                scpy(S->note, "THE BOOKCASE SLIDES! A STAIRWELL. DOWN YOU GO.", 72); S->note_t = 3.2f; sfx(12);
                return;
            }
        }
        // drawers: search the room's furniture for one in reach
        int done_int = 0;
        for (int lyy=1; lyy<=ENT_ROWS-2 && !done_int; lyy++) for (int lxx=1; lxx<=ENT_STRIDE-2 && !done_int; lxx++){
            int tx = (room-1)*ENT_STRIDE + lxx, ty = lyy;
            if (ent_furn(pi, tx, ty)==6){
                float txx = tx*BW_TILE + 24, tyy = ty*BW_TILE + 24;
                float ddx=b->x-txx, ddy=b->y-tyy;
                if (ddx*ddx+ddy*ddy < 50*50){ S->sleep_t = 1.4f; S->slept = 0; sfx(11); done_int = 1; }
            } else if (ent_tile(pi, tx, ty)==4){
                float txx = tx*BW_TILE + 24, tyy = ty*BW_TILE + 24;
                float ddx=b->x-txx, ddy=b->y-tyy;
                if (ddx*ddx+ddy*ddy < 48*48){
                    done_int = 1;
                    int lk = S->ent_sub? room+1000 : room;   // downstairs loot is its own ledger
                    if (ent_room_looted(S, lk)){ scpy(S->note, "JUST DUST. AND A BUTTON.", 72); S->note_t = 1.6f; }
                    else {
                        if (S->ent_loot_n < 96) S->ent_loot_rooms[S->ent_loot_n++] = lk;
                        int roll = (int)(ent_rnd(b, room, 600 + S->ent_loot_n) % 100);
                        if (roll < 55){ int nb2 = 3 + (int)(ent_rnd(b, room, 610) % 6); S->ent_bits += nb2;
                            char *q=S->note; q=sappend(q, "BITS IN THE DRAWER! +"); q=sapp_int(q, nb2); *q=0; S->note_t = 1.8f; }
                        else if (roll < 75){ S->ent_sticks++; { char *q=S->note; q=sappend(q, "A GOOD STICK! "); q=sappend(q, lbl_fire()); q=sappend(q, " SWINGS IT."); *q=0; } S->note_t = 2.0f; }
                        else if (roll < 88){ b->items[0]++; scpy(S->note, "PATCH NOTES IN THE DRAWER!", 72); S->note_t = 1.8f; }
                        else if (roll < 95){ b->items[1]++; scpy(S->note, "AN ENERGY DRINK, STILL FIZZY!", 72); S->note_t = 1.8f; }
                        else if (S->ent_lant < 2){ S->ent_lant++; scpy(S->note, "A BRIGHTER LANTERN!", 72); S->note_t = 2.0f; }
                        else { S->ent_bits += 5; scpy(S->note, "A VERY SHINY BUTTON. +5 BITS", 72); S->note_t = 1.8f; }
                        sfx(11);
                    }
                }
            }
        }
        if (done_int) return;
    }
    // the brass key just sits there waiting to be pocketed
    if (!S->ent_key && room > (S->ent_sub? S->ent_door_sub : S->ent_door_upto) && ent_door_locked(pi, room)){
        float kx, ky; ent_key_pos(pi, room, &kx, &ky);
        float dx=b->x-kx, dy=b->y-ky;
        if (dx*dx+dy*dy < 26*26){ S->ent_key = 1; scpy(S->note, "GOT THE BRASS KEY!", 72); S->note_t = 2.0f; sfx(19); }
    }
    if (b->hp<=0){ b->hp = 0; b->fainted = 1; bw_exit(pi, 1); return; }
}


