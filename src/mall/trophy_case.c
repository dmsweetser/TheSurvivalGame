// THE SURVIVAL GAME — mall/trophy_case.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 157-379 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// TROPHY CASE (v1.25 feat 4, station 5): a pure VIEW over records the
// game already keeps — per-buddy cabinet records on the pl-line, the
// medal bests of feature #3, the Walk of Fame house records in the
// global blob, the First Shift's slot state. It writes nothing and
// adds no save state: a pre-feature save has nothing to tolerate.
// One row model feeds the count, the name, the right-hand value, the
// blurb and the earned test, so the panel can never disagree with
// itself. Rows are shelves: headers, then the plaques on them.
// ---------------------------------------------------------------------------
enum { CR_HDR_CAB, CR_CASTLE, CR_STORY, CR_WAR, CR_CUPS, CR_TANK, CR_ENT, CR_SUB,
       CR_HDR_MED, CR_CMED, CR_RMED, CR_HAT, CR_KILLS, CR_BOWL, CR_LONG,
       CR_HDR_CREW, CR_FS, CR_BUDDY, CR_TRAIN };
static int case_row_ids(Player *p, int *ids){
    int n=0;
    ids[n++]=CR_HDR_CAB;
    ids[n++]=CR_CASTLE; ids[n++]=CR_STORY; ids[n++]=CR_WAR; ids[n++]=CR_CUPS;
    ids[n++]=CR_TANK; ids[n++]=CR_ENT; ids[n++]=CR_SUB;
    ids[n++]=CR_HDR_MED;
    ids[n++]=CR_CMED; ids[n++]=CR_RMED; ids[n++]=CR_HAT; ids[n++]=CR_KILLS; ids[n++]=CR_BOWL; ids[n++]=CR_LONG;
    ids[n++]=CR_HDR_CREW;
    if (G.fs_state) ids[n++]=CR_FS;   // a slot never offered the chain gets no row — "NOT STARTED" would be a lie
    ids[n++]=CR_BUDDY;
    if (G.bud[p-G.p].active) ids[n++]=CR_TRAIN;
    return n;
}
static int case_row_id(Player *p, int row){
    int ids[20]; int n=case_row_ids(p, ids);
    return (row>=0 && row<n)? ids[row] : -1;
}
static int case_story_done(Buddy *b){   // quests at state 2 (done), of 10
    int n=0; for (int q=0;q<10;q++) if (bw_qstate_g(b,q)==2) n++;
    return n;
}
static int case_story_started(Buddy *b){
    if (b->story_started) return 1;
    for (int q=0;q<10;q++) if (bw_qstate_g(b,q)) return 1;
    return (b->story_bits>>17)&1;   // Zed is down: the story was certainly started
}
static const char *case_rank(int r){   // a cup rank as the case says it
    static const char *R[7] = { "--", "1ST", "2ND", "3RD", "4TH", "5TH", "6TH" };
    return R[r<0? 0 : r>6? 6 : r];
}
static int case_earned(Player *p, int id){
    Buddy *b=&G.bud[p-G.p];
    switch (id){
        case CR_CASTLE: return b->cas_best>0;
        case CR_STORY: return (b->story_bits>>17)&1;
        case CR_WAR: return b->war_prog>=8;
        case CR_CUPS: return (b->crt_cupbest%8)==1 || (b->crt_cupbest/8)==1;
        case CR_TANK: return b->tank_done>0;
        case CR_ENT: return b->ent_done>0;
        case CR_SUB: return b->ent_sub_best>0;
        case CR_CMED: return crt_medal_count(b,1)+crt_medal_count(b,2)+crt_medal_count(b,3)>0;
        case CR_RMED: return plat_medal_count(b,1)+plat_medal_count(b,2)+plat_medal_count(b,3)>0;
        case CR_HAT: return G.rec_hat>0;
        case CR_KILLS: return G.rec_kills>0;
        case CR_BOWL: return G.rec_bowl>0;
        case CR_LONG: return G.rec_time>0;
        case CR_FS: return G.fs_state==2;
        case CR_BUDDY: return b->active;
        case CR_TRAIN: return b->up_hp+b->up_atk+b->up_def>0 || b->skills!=0;
        default: return 0;
    }
}
static u32 case_chip_col(Player *p, int id){   // an earned row's plaque-chip colour (medal rows wear their best tier)
    Buddy *b=&G.bud[p-G.p];
    if (id==CR_CMED){ for (int t=3;t>=1;t--) if (crt_medal_count(b,t)>0) return medal_col(t); }
    if (id==CR_RMED){ for (int t=3;t>=1;t--) if (plat_medal_count(b,t)>0) return medal_col(t); }
    return COL(250,200,50);
}
static const char *case_row_name(Player *p, int row, char *buf){
    Buddy *b=&G.bud[p-G.p];
    char *q=buf;
    switch (case_row_id(p,row)){
        case CR_HDR_CAB: return "THE CABINETS";
        case CR_HDR_MED: return "MEDALS + WALK OF FAME";
        case CR_HDR_CREW: return "THE CREW";
        case CR_CASTLE: return "CASTLE";
        case CR_STORY: return "STORY MODE";
        case CR_WAR: return "WAR!";
        case CR_CUPS: return "CART CUPS";
        case CR_TANK: return "TANK RESCUE";
        case CR_ENT: return "ENTRANCES";
        case CR_SUB: return "THE SUBFLOOR";
        case CR_CMED: return "CART MEDALS";
        case CR_RMED: return "RUN MEDALS";
        case CR_HAT: return "HEAVIEST HAT";
        case CR_KILLS: return "MOST BUGS SQUASHED";
        case CR_BOWL: return "BEST BUG BOWL";
        case CR_LONG: return "LONGEST RUN";
        case CR_FS: return "FIRST SHIFT";
        case CR_BUDDY:
            if (b->active){ q=sappend(q,"BUDDY: "); q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); *q=0; return buf; }
            return "YOUR BUDDY";
        case CR_TRAIN: return "TRAINING";
        default: return "";
    }
}
static const char *case_row_right(Player *p, int row, char *buf){
    Buddy *b=&G.bud[p-G.p];
    char *q=buf;
    switch (case_row_id(p,row)){
        case CR_CASTLE:
            if (b->cas_best>0){ q=sappend(q,"BEST "); q=sapp_int(q,b->cas_best); *q=0; return buf; }
            return "NOT YET";
        case CR_STORY:
            if ((b->story_bits>>17)&1) return "ZED DEFEATED";
            if (case_story_started(b)){ q=sappend(q,"QUESTS "); q=sapp_int(q,case_story_done(b)); q=sappend(q,"/10"); *q=0; return buf; }
            return "NOT STARTED";
        case CR_WAR:
            if (b->war_prog>=8) return "CONQUERED";
            if (b->war_prog>0){ q=sapp_int(q,b->war_prog); q=sappend(q,"/8 REGIONS"); *q=0; return buf; }
            return "NOT YET";
        case CR_CUPS:
            q=sappend(q,"HOME "); q=sappend(q,case_rank(b->crt_cupbest%8));
            q=sappend(q," - FAR "); q=sappend(q,case_rank(b->crt_cupbest/8)); *q=0; return buf;
        case CR_TANK:
            if (b->tank_done>0){ q=sappend(q,"LEVEL "); q=sapp_int(q,b->tank_done); *q=0; return buf; }
            return "NOT YET";
        case CR_ENT:
            if (b->ent_done>0){ q=sappend(q,"OUT x"); q=sapp_int(q,b->ent_done); *q=0; return buf; }
            if (b->ent_best>0){ q=sappend(q,"ROOM "); q=sapp_int(q,b->ent_best); q=sappend(q,"/51"); *q=0; return buf; }
            return "NOT YET";
        case CR_SUB:
            if (b->ent_sub_best>0){ q=sappend(q,"B-"); q=sapp_int(q,b->ent_sub_best); *q=0; return buf; }
            return "NOT YET";
        case CR_CMED: case CR_RMED: {
            int g,s,br;
            if (case_row_id(p,row)==CR_CMED){ g=crt_medal_count(b,3); s=crt_medal_count(b,2); br=crt_medal_count(b,1); }
            else { g=plat_medal_count(b,3); s=plat_medal_count(b,2); br=plat_medal_count(b,1); }
            if (!g && !s && !br) return "NONE YET";
            q=sapp_int(q,g); q=sappend(q,"G - "); q=sapp_int(q,s); q=sappend(q,"S - "); q=sapp_int(q,br); q=sappend(q,"B"); *q=0; return buf; }
        case CR_HAT:
            if (G.rec_hat>0){ q=sapp_int(q,(int)G.rec_hat); q=sappend(q," KG"); *q=0; return buf; }
            return "NO RECORD";
        case CR_KILLS:
            if (G.rec_kills>0){ q=sapp_int(q,G.rec_kills); *q=0; return buf; }
            return "NO RECORD";
        case CR_BOWL:
            if (G.rec_bowl>0){ q=sapp_int(q,G.rec_bowl); *q=0; return buf; }
            return "NO RECORD";
        case CR_LONG:
            if (G.rec_time>0){ int t=(int)G.rec_time; q=sapp_int(q,t/60); q=sappend(q,":"); if (t%60<10) q=sappend(q,"0"); q=sapp_int(q,t%60); *q=0; return buf; }
            return "NO RECORD";
        case CR_FS:
            if (G.fs_state==2) return "COMPLETE";
            if (G.fs_state==3) return "SKIPPED";
            q=sappend(q,"STEP "); q=sapp_int(q,G.fs_step+1); q=sappend(q," OF 6"); *q=0; return buf;
        case CR_BUDDY:
            if (b->active){ q=sappend(q,"XP "); q=sapp_int(q,b->xp); *q=0; return buf; }
            return "NONE YET";
        case CR_TRAIN:
            q=sappend(q,"HP"); q=sapp_int(q,b->up_hp); q=sappend(q," ATK"); q=sapp_int(q,b->up_atk); q=sappend(q," DEF"); q=sapp_int(q,b->up_def); *q=0; return buf;
        default: return "";
    }
}
static char case_blurb_buf[128];
static const char *case_row_blurb(Player *p, int row){
    Buddy *b=&G.bud[p-G.p];
    int id=case_row_id(p,row);
    char *q;
    switch (id){
        case CR_HDR_CAB: return "EVERY CABINET IN THE ARCADE KEEPS A PLAQUE HERE.";
        case CR_HDR_MED: return "MEDALS ARE YOURS ALONE. THE WALK OF FAME BELONGS TO THE HOUSE.";
        case CR_HDR_CREW: return "THE CREW THAT WEARS THE EVIDENCE.";
        case CR_CASTLE: return case_earned(p,id)? "OUT THE COURTYARD GATE WITH THE TREASURE. THE CASE KEEPS THE SCORE."
                                                : "ESCAPE THE CASTLE, TREASURE IN HAND. THE PLAQUE IS WAITING.";
        case CR_STORY:
            if ((b->story_bits>>17)&1) return "ZERO-DAY ZED, FUNERALED. THE MEADOW IS PATCHED, AND THE CASE KNOWS.";
            if (case_story_started(b)) return "PATCHINA'S ERRANDS, PARTLY RUN. ZED IS STILL OUT THERE, UNPATCHED.";
            return "STORY MODE WAITS IN BANZAI WORLD. TEN QUESTS, ONE FINAL BUG.";
        case CR_WAR:
            if (b->war_prog>=8) return "ALL EIGHT REGIONS. THE HIVE SENDS ITS REGARDS. IT HAS NONE LEFT.";
            if (b->war_prog>0) return "REGIONS FALL ONE AT A TIME. THE MAP REMEMBERS WHERE YOU STOPPED.";
            return "CONQUER ALL EIGHT REGIONS OF THE WAR! MAP. THE PLAQUE IS WAITING.";
        case CR_CUPS:
            if (case_earned(p,id)) return "A CUP ON THE SHELF. THE OTHER KARTS CAN BE HEARD SULKING FROM HERE.";
            if (b->crt_cupbest) return "FINISH A GRAND PRIX ON TOP AND THE CUP GOES IN THE CASE.";
            return "WIN A GRAND PRIX - HOME OR FAR COUNTRY - AND THE CUP LIVES HERE.";
        case CR_TANK: return b->tank_done>0? "THE TANK CAME BACK. THEY ALWAYS COME BACK - DEEPER, NEXT TIME."
                                           : "RESCUE DEEP IN TANK RESCUE. THE PLAQUE COUNTS YOUR DEEPEST LEVEL.";
        case CR_ENT:
            if (b->ent_done>0) return "EVERY ENTRANCE, CLEARED. THE DOORS BEHAVE THEMSELVES NOW. MOSTLY.";
            if (b->ent_best>0) return "FIFTY-ONE ROOMS DEEP, THE EXIT. HOW FAR DID YOU GET LAST TIME?";
            return "GET OUT OF ENTRANCES - ALL 51 ROOMS. GENTLE RULES, LONG HALLS.";
        case CR_SUB: return b->ent_sub_best>0? "BELOW THE FLOORS, THE B-ROOMS. YOU WENT DOWN AND CAME BACK UP."
                                             : "SOMEWHERE UNDER ENTRANCES THERE ARE B-ROOMS. FIND THE WAY DOWN.";
        case CR_CMED: case CR_RMED: {
            int g,s,br;
            if (id==CR_CMED){ g=crt_medal_count(b,3); s=crt_medal_count(b,2); br=crt_medal_count(b,1); }
            else { g=plat_medal_count(b,3); s=plat_medal_count(b,2); br=plat_medal_count(b,1); }
            if (!g && !s && !br) return id==CR_CMED? "BEAT A TRACK'S MEDAL TIME IN SUPER BUDDY CART. BRONZE IS KIND."
                                                   : "BEAT A LEVEL'S MEDAL TIME IN SUPER BUDDY RUN. BRONZE IS KIND.";
            q=case_blurb_buf;
            q=sapp_int(q,g); q=sappend(q," GOLD, "); q=sapp_int(q,s); q=sappend(q," SILVER, "); q=sapp_int(q,br);
            q=sappend(q, id==CR_CMED? " BRONZE ACROSS THE EIGHT TRACKS." : " BRONZE ACROSS THE FIRST SIXTEEN LEVELS.");
            *q=0; return case_blurb_buf; }
        case CR_HAT:
            if (G.rec_hat>0){ q=case_blurb_buf; q=sappend(q,"SET BY "); q=sappend(q,G.rec_name); q=sappend(q,". THE WHOLE HOUSE WALKS TALLER."); *q=0; return case_blurb_buf; }
            return "THE HEAVIEST HAT EVER WORN, HOUSE-WIDE. NOBODY HAS SET IT YET.";
        case CR_KILLS: return G.rec_kills>0? "THE BUSIEST RUN THIS HOUSE HAS EVER SEEN. CARCASSES EVERYWHERE."
                                           : "MOST BUGS SQUASHED IN ONE RUN, HOUSE-WIDE. NO RECORD YET.";
        case CR_BOWL: return G.rec_bowl>0? "THE BEST GAME THESE LANES HAVE HOSTED. THE PINS STILL TALK."
                                          : "BEST BUG BOWL GAME, HOUSE-WIDE. THE LANES ARE WAITING.";
        case CR_LONG: return G.rec_time>0? "THE LONGEST SINGLE RUN ON THE BOOKS. A PROPER SHIFT."
                                         : "THE LONGEST SINGLE RUN, HOUSE-WIDE. NO RECORD YET.";
        case CR_FS:
            if (G.fs_state==2) return "SIX JOBS, ALL DONE. YOU ARE ON THE CREW NOW.";
            if (G.fs_state==3) return "SKIPPED, NO HARD FEELINGS. THE CREW FOUND ITS OWN WAY.";
            return "THE GUIDED FIRST DAY: SIX LITTLE JOBS, PAID IN BUDDY XP.";
        case CR_BUDDY:
            if (b->active){ q=case_blurb_buf; q=sappend(q,"YOUR "); q=sappend(q,BUD_NAME[b->species%BUDN]); q=sappend(q,". EVERY CABINET WIN TRAINS THE SAME FRIEND."); *q=0; return case_blurb_buf; }
            return "NO BUDDY YET. BANZAI BUDDIES, ON THE NORTH WALL, FIXES THAT.";
        case CR_TRAIN: {
            int k = (b->skills&1?1:0) + (b->skills&2?1:0) + (b->skills&4?1:0) + (b->skills&8?1:0);
            q=case_blurb_buf;
            q=sappend(q,"MOVES TAUGHT "); q=sapp_int(q,k); q=sappend(q," OF 4 - TRAIN AT BANZAI BUDDIES, NORTH WALL.");
            *q=0; return case_blurb_buf; }
        default: return "";
    }
}


// THE SOURCE ROOM gate (v1.26 Item 1a): the earned-trophy count the
// Mall's Source door reads — earned non-header rows of this very case.
static int trophy_earned_count(Player *p){
    int ids[20]; int n=case_row_ids(p, ids), c=0;
    for (int i=0;i<n;i++){
        if (ids[i]==CR_HDR_CAB || ids[i]==CR_HDR_MED || ids[i]==CR_HDR_CREW) continue;
        if (case_earned(p, ids[i])) c++;
    }
    return c;
}
// test pin (dbg_givetrophy): raise the underlying records so the case
// genuinely reads n earned trophies. Cumulative, cheapest rows first.
static void dbg_pin_trophies(int n){
    Buddy *b=&G.bud[G.my_id];
    if (n>=1) G.rec_hat=1;
    if (n>=2) G.rec_kills=1;
    if (n>=3) b->cas_best=1;
    if (n>=4) G.rec_bowl=1;
    if (n>=5) G.rec_time=1;
    if (n>=6) b->tank_done=1;
    if (n>=7) b->ent_done=1;
}
