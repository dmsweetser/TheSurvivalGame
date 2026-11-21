// THE SURVIVAL GAME — mall/stations.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 380-766 in the Nov 2026 fine-grained refactor; code unchanged.

static int station_rows(int st, Player *p){
    if (st==5){ int ids[20]; return case_row_ids(p, ids); }   // TROPHY CASE
    if (st==6) return wk_rows();   // CHALLENGE BOARD (v1.25 feat 6): the row model lives in save.c with the week state
    if (st==7) return bn_rows();   // BOUNTY BOARD (v1.25 feat 8): the row model lives in save.c with the bounty state
    if (st==1) return WHALE_N + 1;   // the vanities + ENTER UNLOCK CODE (Whales Only, below)
    if (st==2) return 6;   // deposit all/half/quarter + withdraw all/half/quarter
    if (st==8){ int pi=(int)(p-G.p);   // PHOTO BOOTH (feat 9; its own station since the MALL ENLARGEMENT)
        return booth_stage[pi]? 1 : 5; }   // gallery: the one way back; booth: backdrop / hat / snap / your portrait / step out
    if (st==3){ int pi=(int)(p-G.p); Buddy *b=&G.bud[pi];
        int stage = bud_stage[pi];
        if (stage==1 || stage==3){ int rem = BUD_NAMES_N - bud_page[pi]*12; int n = rem>12? 12 : rem; return n + 1; }   // + the MORE row, on every page
        if (stage==2 || stage==4) return BUD_COLORS_N;
        return !b->active? BUDN : b->fainted? 1 : 11; }   // 11: the ten buddy rows + the HAT TOPPER wardrobe row (feat 5). The PHOTO BOOTH row left with the booth itself (MALL ENLARGEMENT: station 8)
    return 0;
}
static void store_sel_reset(int pi){
    if (G.p2_slot>=0 && pi==G.p2_slot) G.store_sel2=0; else G.store_sel=0;
}
static const char *station_row_name(int st, int row, Player *p, char *buf){
    if (st==5) return case_row_name(p, row, buf);   // TROPHY CASE
    if (st==6) return wk_row_name(row, buf);   // CHALLENGE BOARD
    if (st==7) return bn_row_name(row, buf);   // BOUNTY BOARD
    if (st==8){   // PHOTO BOOTH rows (feat 9; station 8 since the MALL ENLARGEMENT)
        if (booth_stage[(int)(p-G.p)]) return "BACK TO THE BOOTH";
        static const char *BN2[5] = { "BACKDROP", "THE HAT", "SNAP PORTRAIT", "YOUR PORTRAIT", "STEP OUT" };
        return BN2[row%5];
    }
    Buddy *b=&G.bud[p-G.p];
    char *q=buf;
    if (st==1){ return row<WHALE_N? WHALE_NAME[row] : "ENTER UNLOCK CODE"; }
    if (st==2){ static const char *BN[6] = { "DEPOSIT ALL", "DEPOSIT HALF", "DEPOSIT QUARTER",
                                             "WITHDRAW ALL", "WITHDRAW HALF", "WITHDRAW QUARTER" };
                return BN[row%6]; }
    { int pi=(int)(p-G.p);
      if (bud_stage[pi]==1 || bud_stage[pi]==3){
          int base = bud_page[pi]*12;
          if (row>=12 || base+row>=BUD_NAMES_N) return "MORE NAMES >>";
          return BUD_ELF[base+row];
      }
      if (bud_stage[pi]==2 || bud_stage[pi]==4) return BUD_COLOR_NAME[row%BUD_COLORS_N]; }
    if (!b->active){ q=sappend(q, "ADOPT A "); q=sappend(q, BUD_NAME[row%BUDN]); *q=0; return buf; }
    if (b->fainted){ q=sappend(q, "REVIVE "); q=sappend(q, BUD_NAME[b->species%BUDN]); *q=0; return buf; }
    switch (row){
        case 0: return "TRAIN HEALTH";
        case 1: return "TRAIN ATTACK";
        case 2: return "TRAIN DEFENSE";
        case 3: q=sappend(q, "TEACH: "); q=sappend(q, BUD_MOVE2[b->species%BUDN]); *q=0; return buf;
        case 4: q=sappend(q, "TEACH: "); q=sappend(q, BUD_SPEC[b->species%BUDN]); *q=0; return buf;
        case 5: q=sappend(q, "TEACH: "); q=sappend(q, BUD_MOVE3[b->species%BUDN]); *q=0; return buf;
        case 6: q=sappend(q, "TEACH: "); q=sappend(q, BUD_MOVE4[b->species%BUDN]); *q=0; return buf;
        case 10: return "HAT TOPPER";
        case 8: q=sappend(q, "NAME: "); q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); *q=0; return buf;
        case 9: q=sappend(q, "COLOR: "); q=sappend(q, BUD_COLOR_NAME[b->color%BUD_COLORS_N]); *q=0; return buf;
        default: q=sappend(q, "CHANGE TO "); q=sappend(q, BUD_NAME[(b->species+1)%BUDN]); *q=0; return buf;
    }
}
static const char *station_row_right(int st, int row, Player *p, char *buf){
    if (st==5) return case_row_right(p, row, buf);   // TROPHY CASE
    if (st==6) return wk_row_right(row, buf);   // CHALLENGE BOARD
    if (st==7) return bn_row_right(row, buf);   // BOUNTY BOARD
    if (st==8){   // PHOTO BOOTH (feat 9; station 8 since the MALL ENLARGEMENT): rights the generic stage rule below would blank
        int pi=(int)(p-G.p);
        if (booth_stage[pi]) return "";
        Portrait *pt2=&portrait[pi];
        if (row==0) return PORT_BD_NAME[port_bd[pi]%PORT_BD_N];
        if (row==1) return port_hat[pi]? "AS WORN" : "BARE";
        if (row==2) return "FREE";
        if (row==3) return pt2->has? PORT_BD_NAME[pt2->backdrop%PORT_BD_N] : "NONE YET";
        return "";
    }
    Buddy *b=&G.bud[p-G.p];
    char *q=buf;
    if (st==1){
        if (row>=WHALE_N) return "FREE";
        if (p->whale_own & (1<<row)) return (p->whale_on & (1<<row))? "ON" : "OFF";
        return "$0.67";
    }
    if (st==2){
        // the amount THIS row will move (Daniel, Nov 2), not the
        // whole balance on every row: deposits pop whole carcasses
        // until the fraction is covered (simulated in hat order);
        // withdraws pay exactly the fraction, in <=6 kg chunks.
        float frac = (row%3==0)? 1.0f : (row%3==1)? 0.5f : 0.25f;
        float amt;
        if (row<3){
            if (frac>=1.0f) amt = p->carried;
            else { float target = p->carried*frac; amt = 0;
                   for (int si=p->stack_n-1; si>=0 && amt<target; si--) amt += p->stack[si]; }
        } else amt = p->banked * frac;
        fmt_int(buf, "", (int)amt, " KG"); return buf;
    }
    if (bud_stage[(int)(p-G.p)]>0) return "";
    if (!b->active){ fmt_int(buf, "", BUD_PRICE[row%BUDN], " KG"); return buf; }
    if (b->fainted){ fmt_int(buf, "", bud_revive_price(b), " KG"); return buf; }
    switch (row){
        case 0: case 1: case 2: {
            int lvl = row==0? b->up_hp : row==1? b->up_atk : b->up_def;
            q=sapp_int(q, bud_train_cost(lvl)); q=sappend(q, " XP"); *q=0; return buf; }
        case 3: return (b->skills&1)? "TAUGHT" : "40 XP";
        case 4: return (b->skills&2)? "TAUGHT" : "80 XP";
        case 5: return (b->skills&4)? "TAUGHT" : "60 XP";
        case 6: return (b->skills&8)? "TAUGHT" : "120 XP";
        case 10: return b->top_worn? TOP_NAME[b->top_worn] : "NONE";
        case 8: case 9: return "FREE";
        default: fmt_int(buf, "", BUD_PRICE[(b->species+1)%BUDN], " KG"); return buf;
    }
}
static const char *station_blurb(int st, int row, Player *p){
    if (st==5) return case_row_blurb(p, row);   // TROPHY CASE
    if (st==6) return wk_row_blurb(row);   // CHALLENGE BOARD
    if (st==7) return bn_row_blurb(row);   // BOUNTY BOARD
    if (st==1) return row<WHALE_N? WHALE_BLURB[row]
                        : "PAID ON THE WEB? TYPE OR PASTE THE UNLOCK CODE IT GAVE YOU.";
    if (st==8){   // PHOTO BOOTH (feat 9; station 8 since the MALL ENLARGEMENT)
        if (booth_stage[(int)(p-G.p)]) return "THAT IS THE DAY YOU WORE IT. SNAP A NEW ONE AT THE BOOTH ANY TIME.";
        if (row==0) return "SIX BACKDROPS, ALL STOLEN FROM PLACES YOU HAVE BLED.";
        if (row==1) return "POSE WITH THE WHOLE ABSURD TOWER, OR BARE-HEADED AND PROUD.";
        if (row==2) return "THE PICTURE HANGS AT THE BOOTH, AND A COPY IS YOURS TO KEEP.";
        if (row==3) return "THE LAST PORTRAIT YOU SNAPPED, KEPT UNDER GLASS. IT NEVER FADES.";
        return "OUT THE CURTAIN, BACK TO THE MALL FLOOR.";
    }
    Buddy *b=&G.bud[p-G.p];
    if (st==2) return row<3? "SAFE FOREVER AND WEIGHS NOTHING - BUT NOT A HAT RECORD."
                            : "BACK ON YOUR HEAD: COUNTS AGAIN - AND AT RISK AGAIN.";
    { int stage = bud_stage[(int)(p-G.p)];
      if (stage==1 || stage==3) return "OLD NAMES FROM THE ELDER DAYS. PICK WELL - THEY STICK.";
      if (stage==2 || stage==4) return "A FRESH COAT. THE NEON RAINBOW IS... A LOT. WEAR IT PROUDLY."; }
    if (!b->active){
        if (row==0) return "LOYAL. FAST BITES, A POUNCE, AND A BARK THAT FREEZES BUGS.";
        if (row==1) return "FAST CLAWS, A THREE-SLASH RAKE, AND ONE WET HAIRBALL.";
        if (row==2) return "A WALKING WALL. ANTLERS, A CHARGE, AND A VERY LOUD STOMP.";
        if (row==3) return "VENOMOUS, SEMI-AQUATIC, UNBOTHERED. SPUR DIVE + VENOM STING.";
        if (row==4) return "IT IS A ROCK. IT ROLLS. IT ALMOST NEVER FAINTS. THAT IS ALL.";
        return "PORCELAIN. PLUMBED. SPLASHES EVERYWHERE. IT HAS SEEN THINGS.";
    }
    if (b->fainted) return "OUT COLD - NOT GONE. BUDDIES ONLY EVER FAINT. A REVIVE FIXES IT.";
    switch (row){
        case 0: return "+25 MAX HEALTH PER LEVEL. A STURDIER FRIEND.";
        case 1: return "+20% BUDDY DAMAGE PER LEVEL.";
        case 2: return "YOUR BUDDY TAKES 12% LESS DAMAGE PER LEVEL.";
        case 3: return "TEACH THE SECOND MOVE. YOUR BUDDY USES IT ON ITS OWN.";
        case 4: return "TEACH THE SPECIAL MOVE. BIG, LOUD, AND ON A LONG COOLDOWN.";
        case 5: return "TEACH THE THIRD MOVE. COSTS 30 ENERGY A POP IN A FIGHT.";
        case 6: return "TEACH THE FOURTH MOVE. THE BIG ONE - 55 ENERGY, NO APOLOGIES.";
        case 8: return "A NEW NAME FROM THE ELDER DAYS. FREE - NAMES WANT TO BE USED.";
        case 9: return "A NEW COAT, STANDARD OR NEON RAINBOW. FREE.";
        case 10: return "TROPHIES FOR YOUR HAT, EARNED OUT IN THE WORLD. FIRE CYCLES WHAT YOU HAVE WON.";
        default: return "SWAP SPECIES. TRAINING, NAME AND XP CARRY OVER.";
    }
}
static void station_note(int pi, const char *msg){
    if (pi==G.my_id || pi==G.p2_slot) store_note(msg);
}
// HAT PORTRAIT BOOTH (v1.25 feat 9): port_arm is set by a SNAP and
// consumed by the booth draw, which alone knows the frame's true
// rectangle in the current layout — see draw_booth_panel.
static int port_arm[MAXP];
static void booth_snap(int pi){
    Player *p=&G.p[pi]; Buddy *b=&G.bud[pi]; Portrait *pt2=&portrait[pi];
    pt2->has = 1;
    pt2->backdrop = port_bd[pi]%PORT_BD_N;
    pt2->hat_on = port_hat[pi]? 1 : 0;
    pt2->top_worn = p->top_worn;
    pt2->kg = (int)p->carried;
    pt2->stack_n = 0;
    if (pt2->hat_on){
        int n = p->stack_n>MAXSTACK? MAXSTACK : p->stack_n;
        pt2->stack_n = n;
        for (int i=0;i<n;i++){ int k=(int)p->stack[i]%4; pt2->kinds[i]=(unsigned char)(k<0? k+4 : k); }
    }
    pt2->species = b->species; pt2->bud_color = b->color; pt2->name_idx = b->name_idx;
    pt2->pcolor = p->color; pt2->whale_on = p->whale_on;
    port_arm[pi] = 1;
    if (pi==G.my_id || pi==G.p2_slot) do_save();
    station_note(pi, "PORTRAIT SNAPPED! IT HANGS AT THE BOOTH.");
    sfx(11);
}
static void station_action(int pi, int st, int row){
    Player *p=&G.p[pi];
    Buddy *b=&G.bud[pi];
    // while the till is counting THIS player's carcasses, the counter
    // is closed to them (Daniel, Nov 6): no second purchase, and no
    // bank run that drains the hat out from under the counting till
    if (pay_active && pay_player==pi){ station_note(pi, "THE TILL IS COUNTING YOUR CARCASSES..."); sfx(16); return; }
    if (st==5){
        // TROPHY CASE: a display, not a shop — FIRE admires, nothing more
        station_note(pi, "LOOK, DON'T TOUCH. THE PLAQUES ARE FRAGILE."); sfx(6);
        return;
    }
    if (st==6){
        // CHALLENGE BOARD: also a display — progress happens out in
        // the world, never by pressing the board (v1.25 feat 6)
        station_note(pi, "THE BOARD IS FOR READING. THE WEEK IS FOR DOING."); sfx(6);
        return;
    }
    if (st==7){
        // BOUNTY BOARD: also a display — the claim is settled in the
        // field the moment the named one falls (v1.25 feat 8)
        station_note(pi, "THE BOARD IS FOR READING. THE FIELD IS FOR HUNTING."); sfx(6);
        return;
    }
    if (st==1){
        if (row<WHALE_N){
            if (p->whale_own & (1<<row)){
                p->whale_on ^= (1<<row);
                if (pi==G.my_id || pi==G.p2_slot){   // a local player: the profile is the truth
                    G.whale_own=p->whale_own; G.whale_on=p->whale_on; do_save(); }
                station_note(pi, (p->whale_on>>row&1)? "SWITCHED ON" : "SWITCHED OFF"); sfx(6);
            } else station_note(pi, "BUY IT FROM THE LINK - $0.67");   // guests: the host can't open their browser
        }
        return;   // row 7 (ENTER CODE) is local-only, see station_input
    }
    if (st==4){
        // a LAN guest's bowling ball: the host just takes the carcass
        // off their head — the game itself plays out on their screen
        if (row==77 && p->stack_n>0) hat_pop(p);
        return;
    }
    if (st==2){
        // rows 0-2 deposit (all / half / quarter of the hat's weight),
        // rows 3-5 withdraw (all / half / quarter of the vault)
        float frac = (row%3==0)? 1.0f : (row%3==1)? 0.5f : 0.25f;
        if (row<3){
            if (p->stack_n<=0){ station_note(pi, "NOTHING ON YOUR HEAD TO DEPOSIT"); sfx(16); return; }
            if (frac>=1.0f){ p->banked += p->carried; p->stack_n=0; p->carried=0; }
            else { // whole carcasses pop until the fraction is covered
                float target = p->carried * frac, moved = 0;
                while (p->stack_n>0 && moved < target) moved += hat_pop(p);
                p->banked += moved;
            }
            station_note(pi, "DEPOSITED. SAFE AND SOUND."); sfx(11);
        } else {
            if (p->banked<=0.01f){ station_note(pi, "THE VAULT IS EMPTY"); sfx(16); return; }
            float target = p->banked * frac, moved = 0;
            while (moved < target - 0.01f){
                float w = target - moved; if (w>6.0f) w = 6.0f;
                hat_push(p, w); p->banked -= w; moved += w;
            }
            if (p->banked<0.01f) p->banked=0;
            station_note(pi, "WITHDRAWN. BACK ON YOUR HEAD."); sfx(11);
        }
        return;
    }
    if (st==8){
        // PHOTO BOOTH (feat 9; its own station since the MALL
        // ENLARGEMENT). Plain station actions, no picker — a LAN
        // guest's activate runs them here on the host like TRAIN
        // and the wardrobe do.
        if (booth_stage[pi]){
            booth_stage[pi]=0; store_sel_reset(pi); sfx(6); return;   // BACK TO THE BOOTH
        }
        if (row==0){ port_bd[pi] = (port_bd[pi]+1)%PORT_BD_N; if (pi==G.my_id || pi==G.p2_slot) do_save(); sfx(6); }
        else if (row==1){ port_hat[pi] = port_hat[pi]? 0 : 1; sfx(6); }
        else if (row==2){ booth_snap(pi); }
        else if (row==3){
            if (portrait[pi].has){ booth_stage[pi]=1; store_sel_reset(pi); sfx(6); }
            else { station_note(pi, "NO PORTRAIT YET - SNAP ONE FIRST."); sfx(16); }
        }
        else {   // STEP OUT: for a local shopper, close the panel the
                 // way the frame's own close paths do — the lstore
                 // flag in host co-op, the game state solo/client; a
                 // LAN guest's panel is theirs to close (their stage
                 // lives here).
            booth_stage[pi]=0; store_sel_reset(pi); sfx(6);
            if (pi==G.my_id){
                if (G.net_mode==NET_HOST && G.p2_slot>=0) lstore[0]=0;
                else G.state=GS_PLAY;
                p->in_store=0; p->station=-1;
            }
            else if (pi==G.p2_slot){ lstore[1]=0; p->in_store=0; p->station=-1; }
        }
        return;
    }
    if (st==3){
        // the name/color picker runs before (and after) the till:
        // adoption goes species -> name -> color -> payment
        if (bud_stage[pi]==1 || bud_stage[pi]==3){
            int base = bud_page[pi]*12;
            if (row>=12 || base+row>=BUD_NAMES_N){ bud_page[pi] = (bud_page[pi]+1) % ((BUD_NAMES_N+11)/12); store_sel_reset(pi); sfx(6); return; }
            if (bud_stage[pi]==1){ bud_dname[pi] = base+row; bud_stage[pi]=2; }
            else { b->name_idx = base+row; bud_stage[pi]=0; do_save(); station_note(pi, "A NAME WORTH SINGING OF."); }
            store_sel_reset(pi); sfx(11); return;
        }
        if (bud_stage[pi]==2 || bud_stage[pi]==4){
            if (bud_stage[pi]==4){ b->color = row%BUD_COLORS_N; bud_stage[pi]=0; do_save(); station_note(pi, "LOOK AT YOU. GORGEOUS."); }
            else {
                bud_color_draft[pi] = row%BUD_COLORS_N; bud_stage[pi]=0;
                if (pay_active) return;
                if (p->carried < BUD_PRICE[bud_dsp[pi]]){ station_note(pi, "NOT ENOUGH HAT FOR THAT BUDDY"); sfx(16); return; }
                start_payment(pi, 4, bud_dsp[pi], BUD_PRICE[bud_dsp[pi]]);
            }
            store_sel_reset(pi); sfx(11); return;
        }
        if (!b->active){
            if (p->carried < BUD_PRICE[row%BUDN]){ station_note(pi, "NOT ENOUGH HAT FOR THAT BUDDY"); sfx(16); return; }
            if (pi!=G.my_id && pi!=G.p2_slot){
                // a LAN guest's panel can't run the picker (it lives
                // here, host-side) — they adopt the classic way, with
                // the species' own name and coat
                if (pay_active) return;
                bud_dname[pi] = BUD_DEF_NAME[row%BUDN]; bud_color_draft[pi] = 0;
                start_payment(pi, 4, row%BUDN, BUD_PRICE[row%BUDN]);
                return;
            }
            bud_dsp[pi] = row%BUDN; bud_page[pi]=0; bud_stage[pi]=1;
            store_sel_reset(pi);
            station_note(pi, "FIRST - A NAME."); sfx(6);
            return;
        }
        if (b->fainted){
            // the only business a fainted buddy does: a revive
            if (pay_active) return;
            { int price = bud_revive_price(b);
              if (p->carried < price){ station_note(pi, "NOT ENOUGH HAT TO REVIVE"); sfx(16); return; }
              start_payment(pi, 6, 0, price); }
            return;
        }
        if (row<=2){
            int *lvl = row==0? &b->up_hp : row==1? &b->up_atk : &b->up_def;
            int cost = bud_train_cost(*lvl);
            if (b->xp < cost){ station_note(pi, "NOT ENOUGH BUDDY XP"); sfx(16); return; }
            b->xp -= cost; (*lvl)++;
            if (row==0){ buddy_refresh(pi); b->hp += 25; if (b->hp>b->maxhp) b->hp=b->maxhp; }
            station_note(pi, "TRAINED!"); sfx(11);
            return;
        }
        if (row==3){
            if (b->skills&1){ station_note(pi, "ALREADY TAUGHT"); sfx(16); return; }
            if (b->xp < 40){ station_note(pi, "NEEDS 40 BUDDY XP"); sfx(16); return; }
            b->xp -= 40; b->skills |= 1; station_note(pi, "MOVE TAUGHT!"); sfx(11);
            return;
        }
        if (row==4){
            if (b->skills&2){ station_note(pi, "ALREADY TAUGHT"); sfx(16); return; }
            if (b->xp < 80){ station_note(pi, "NEEDS 80 BUDDY XP"); sfx(16); return; }
            b->xp -= 80; b->skills |= 2; station_note(pi, "SPECIAL TAUGHT!"); sfx(11);
            return;
        }
        if (row==5){
            if (b->skills&4){ station_note(pi, "ALREADY TAUGHT"); sfx(16); return; }
            if (b->xp < 60){ station_note(pi, "NEEDS 60 BUDDY XP"); sfx(16); return; }
            b->xp -= 60; b->skills |= 4; station_note(pi, "THIRD MOVE TAUGHT!"); sfx(11);
            return;
        }
        if (row==6){
            if (b->skills&8){ station_note(pi, "ALREADY TAUGHT"); sfx(16); return; }
            if (b->xp < 120){ station_note(pi, "NEEDS 120 BUDDY XP"); sfx(16); return; }
            b->xp -= 120; b->skills |= 8; station_note(pi, "FOURTH MOVE TAUGHT!"); sfx(11);
            return;
        }
        if (row==10){
            // HAT TOPPERS (feat 5): the wardrobe row — FIRE cycles
            // NONE through the earned toppers in id order. No picker
            // UI, so a LAN guest can use it too (their activate
            // reaches the host like TRAIN does); the choice rides
            // the snapshot back to their screen.
            if (!b->top_earned){ station_note(pi, "NO TOPPERS YET - THE WORLD HOLDS THEM."); sfx(16); return; }
            int cur = b->top_worn;
            for (int k=0;k<TOP_N;k++){ cur = cur>=TOP_N? 0 : cur+1; if (cur==0 || (b->top_earned & TOP_BIT(cur))) break; }
            b->top_worn = cur; p->top_worn = cur;
            if (pi==G.my_id || pi==G.p2_slot) do_save();
            if (cur){ char nb[32]; char *q=nb; q=sappend(q, "TOPPER ON: "); q=sappend(q, TOP_NAME[cur]); *q=0; station_note(pi, nb); }
            else station_note(pi, "TOPPER OFF. BARE STACK.");
            sfx(11); return;
        }
        if (row==8 || row==9){
            if (pi!=G.my_id && pi!=G.p2_slot){ station_note(pi, "NAMES + COATS ARE CHOSEN AT THE HOST'S TABLE"); sfx(16); return; }
            if (row==8){ bud_page[pi] = b->name_idx/12; bud_stage[pi]=3; store_sel_reset(pi); station_note(pi, "A NEW NAME, THEN."); }
            else { bud_stage[pi]=4; store_sel_reset(pi); station_note(pi, "PICK A COLOR."); }
            sfx(6); return;
        }
        // change species
        if (pay_active) return;
        { int price = BUD_PRICE[(b->species+1)%BUDN];
          if (p->carried < price){ station_note(pi, "NOT ENOUGH HAT TO CHANGE"); sfx(16); return; }
          start_payment(pi, 5, (b->species+1)%BUDN, price); }
        return;
    }
}

// drive a mall station panel (station 0 delegates to the classic store)
// WHALES ONLY scrolls (Daniel, Nov 5): sixteen rows never fit at a
// readable size, so the panel windows over them like the craft menu —
// full-size text, a scrollbar, and the selection always on screen.
// Both the input (tap math) and the draw read the same window.
#define WHALE_VIS 12
static int whale_scroll[2];
