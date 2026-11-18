// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: mall.c — WHALES ONLY, Bug Bowl, menus / splash / help, settings + key rebinding, the controls panel, the frame state machine, local co-op, game-over portrait.
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]
// ---------------------------------------------------------------------------
// WHALES ONLY — the mall's fifth counter (in the old Gallery slot).
// Seven purely cosmetic perks, $0.67 each, sold through Stripe payment
// links. Tapping BUY opens the player's browser; after paying, the
// payment page hands back a signed unlock code (Ed25519 — verified
// offline against the public key below; the signing key never ships).
// Codes are the player's receipt: entitlements live in the save data
// (Daniel's warning, printed in the shop), and re-entering a code after
// a wipe restores the perk. NOTHING here touches gameplay numbers.
// ---------------------------------------------------------------------------
#define WHALE_N 15
static const char *WHALE_NAME[WHALE_N] = {
    "GOLDEN CARCASS", "COMICALLY ENORMOUS GLASSES", "ROYAL CAPE",
    "SPARKLE TRAIL", "THE FORBIDDEN MONOCLE", "DRAMATIC RAIN", "WHALE TITLE",
    "ROYAL BUDDY", "PARADE DUCK", "DISCO MALL", "CONFETTI SQUASH",
    "AURA OF IMPORTANCE", "SPACE BUDDY", "NAME IN LIGHTS", "KAZOO MODE" };
static const char *WHALE_BLURB[WHALE_N] = {
    "EVERY CARCASS YOU CARRY, CAST IN GOLD. SAME WEIGHT. MORE GLORY.",
    "THE NERD GLASSES, BUT MORE. ENORMOUS-ER, EVEN. PURE VANITY.",
    "A ROYAL CAPE THAT BILLOWS DRAMATICALLY. ROYALTY NOT INCLUDED.",
    "YOU LEAVE A TRAIL OF SPARKLES. THE BUGS ARE NOT IMPRESSED.",
    "A GOLD MONOCLE FOR YOUR BUDDY. IT HAS SEEN THINGS. TINY THINGS.",
    "IT RAINS ON YOU. ONLY YOU. DRAMATICALLY. EVERYONE ELSE IS DRY.",
    "THE WORD WHALE, FLOATING BY YOUR NAME. EARNED. PAID FOR. YOURS.",
    "A TINY GOLDEN CROWN FOR YOUR BUDDY. ROYALTY, AT LAST.",
    "A DUCK FOLLOWS YOU. IT DOES NOTHING. IT IS NEVER LATE.",
    "THE BUG MALL FLOOR, BUT DISCO. THE KEEPERS PRETEND NOT TO NOTICE.",
    "SQUASHED BUGS BURST INTO CONFETTI. CLEANUP IS SOMEONE ELSE'S PROBLEM.",
    "A GOLDEN RING OF GLOW BENEATH YOU. YOU LOOK IMPORTANT. YOU ARE.",
    "A BUBBLE HELMET FOR YOUR BUDDY. THE MUFFLE IS ADORABLE.",
    "YOUR NAME IN MARQUEE BULBS ON THE TITLE SCREEN. TOP BILLING.",
    "THE WHOLE BEETHOVEN LADDER, ON KAZOO. HE WOULD HAVE WANTED THIS." };
// Stripe payment links — one per perk; filled in when the links exist.
static const char *WHALE_LINK[WHALE_N] = {
    "https://buy.stripe.com/bJebJ215lelo3QPe7OdUY01",   /* 0 Golden Carcass */
    "https://buy.stripe.com/4gMbJ26pFfpsbjhaVCdUY02",   /* 1 Comically Enormous Glasses */
    "https://buy.stripe.com/7sY6oIbJZ0uycnlbZGdUY03",   /* 2 Royal Cape */
    "https://buy.stripe.com/eVqdRa29p5OSfzx8NudUY04",   /* 3 Sparkle Trail */
    "https://buy.stripe.com/fZufZi4hxfpsafdbZGdUY05",   /* 4 The Forbidden Monocle */
    "https://buy.stripe.com/00wbJ2bJZfpsdrp1l2dUY06",   /* 5 Dramatic Rain */
    "https://buy.stripe.com/00wfZiaFV0uygDB7JqdUY07",  /* 6 WHALE Title */
    "https://buy.stripe.com/14A14o01hcdgfzx2p6dUY08",  /* 7 Royal Buddy */
    "https://buy.stripe.com/dRmbJ29BR6SWafd8NudUY09",  /* 8 Parade Duck */
    "https://buy.stripe.com/8x2cN6dS7elo3QPfbSdUY0a",  /* 9 Disco Mall */
    "https://buy.stripe.com/dRmeVe3dt6SW4UTbZGdUY0b",  /* 10 Confetti Squash */
    "https://buy.stripe.com/28E14o4hx5OS9b97JqdUY0c",  /* 11 Aura of Importance */
    "https://buy.stripe.com/eVqcN6eWb3GK3QPd3KdUY0d",  /* 12 Space Buddy */
    "https://buy.stripe.com/9B65kE4hxa58bjh2p6dUY0e",  /* 13 Name in Lights */
    "https://buy.stripe.com/7sYdRaaFV4KO2ML1l2dUY0f" }; /* 14 Kazoo Mode */
// The issuer's Ed25519 public key (the private key lives server-side).
static const u8 WHALE_PUB[32] = {
    0xc6,0x92,0x89,0x6e,0x1b,0x3d,0x00,0x7a,0xc8,0x95,0x8d,0x45,0x46,0x57,0x98,0xd7,
    0x46,0x06,0xa9,0xbc,0xa1,0xa2,0xb0,0x95,0xec,0xe4,0xc4,0x1f,0x29,0x7b,0x33,0xe4 };
// code-entry panel state (per player; only the local player types)
static int  whale_code_mode[MAXP];
static int  whale_bs_hold[MAXP];   // frames Backspace has been held (hold-to-delete)
static char whale_code_buf[MAXP][168];
static int  whale_code_len[MAXP];

static int b32_val(int c){
    if (c>='a' && c<='z') c -= 32;
    if (c>='0' && c<='9') return c-'0';
    if (c=='O' || c=='o') return 0;
    if (c=='I' || c=='L' || c=='i' || c=='l') return 1;
    if (c>='A' && c<='H') return c-'A'+10;
    if (c=='J') return 18;
    if (c=='K') return 19;
    if (c=='M') return 20;
    if (c=='N') return 21;
    if (c>='P' && c<='T') return c-'P'+22;
    if (c>='V' && c<='Z') return c-'V'+27;
    return -1;
}
// Decode + verify one cleaned run of exactly 119 significant chars.
static int whale_try_sig(const char *s){
    u8 raw[74]; int nb=0; u32 acc=0; int accn=0;
    for (int i=0;i<119;i++){
        int v = b32_val((u8)s[i]);
        if (v<0) return -1;
        acc = (acc<<5) | (u32)v; accn += 5;
        if (accn>=8){ accn -= 8; if (nb<74) raw[nb++] = (u8)(acc>>accn); }
    }
    if (nb!=74 || raw[0]!=1) return -1;
    int sku = raw[1];
    if (sku>=WHALE_N) return -1;
    if (!tsg_ed25519_verify(raw+10, WHALE_PUB, raw, 10)) return -1;
    return sku;
}
// Decode + verify a pasted unlock code. Returns the sku (0..6) or -1.
// Format: Crockford base32 of [version=1, sku, 8-byte nonce] || signature.
// Copy accidents happen (Daniel's first real code arrived with 143 of
// 142 chars showing and would not redeem): hyphens, spaces and look-alike
// letters are forgiven, and ONE stray extra character anywhere in the
// paste is forgiven too — the game tries dropping each character in
// turn. A code with a character CHANGED still fails every try, so the
// signature's protection is untouched.
static int whale_redeem(const char *code){
    char sig[200]; int n=0;
    for (const char *s=code; *s && n<199; s++){
        char c=*s;
        if (c=='-' || c==' ' || c=='\n' || c=='\r' || c=='\t') continue;
        if (b32_val((u8)c)<0) continue;   // stray punctuation is skipped, not fatal
        sig[n++]=c;
    }
    sig[n]=0;
    if (n==119) return whale_try_sig(sig);
    if (n==120){
        for (int k=0;k<120;k++){
            char t[120]; int m=0;
            for (int i=0;i<120;i++) if (i!=k) t[m++]=sig[i];
            t[119]=0;
            int sku = whale_try_sig(t);
            if (sku>=0) return sku;
        }
        return -1;
    }
    if (n>120){ // several extra at a copy boundary: try both clean ends
        char t[120];
        for (int i=0;i<119;i++) t[i]=sig[i];
        t[119]=0;
        int sku = whale_try_sig(t);
        if (sku>=0) return sku;
        for (int i=0;i<119;i++) t[i]=sig[n-119+i];
        t[119]=0;
        return whale_try_sig(t);
    }
    return -1;
}
static void whale_grant(int sku){
    G.whale_own |= (1<<sku); G.whale_on |= (1<<sku);
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].is_local){
        G.p[i].whale_own=G.whale_own; G.p[i].whale_on=G.whale_on; }
    do_save();
}
// background pickup: the web build returns from the payment page with
// ?whale=<code>, and native players can drop the code into a
// whale-code.txt beside their saves. Checked a couple of times a second.
static float whale_poll_t = 0;
static void whale_poll(float dt){
    whale_poll_t -= dt;
    if (whale_poll_t>0) return;
    whale_poll_t = 0.4f;
    char buf[200];
    if (plat_whale_poll(buf, (int)sizeof(buf))<=0) return;
    int sku = whale_redeem(buf);
    if (sku>=0){ whale_grant(sku); store_note("UNLOCKED - CHECK THE WHALES ONLY COUNTER!"); sfx(11); }
    else store_note("A CODE ARRIVED BUT DID NOT CHECK OUT");
}


static int station_rows(int st, Player *p){
    if (st==1) return WHALE_N + 1;   // the vanities + ENTER UNLOCK CODE (Whales Only, below)
    if (st==2) return 6;   // deposit all/half/quarter + withdraw all/half/quarter
    if (st==3){ int pi=(int)(p-G.p); Buddy *b=&G.bud[pi];
        int stage = bud_stage[pi];
        if (stage==1 || stage==3){ int rem = BUD_NAMES_N - bud_page[pi]*12; int n = rem>12? 12 : rem; return n + 1; }   // + the MORE row, on every page
        if (stage==2 || stage==4) return BUD_COLORS_N;
        return !b->active? BUDN : b->fainted? 1 : 10; }
    return 0;
}
static void store_sel_reset(int pi){
    if (G.p2_slot>=0 && pi==G.p2_slot) G.store_sel2=0; else G.store_sel=0;
}
static const char *station_row_name(int st, int row, Player *p, char *buf){
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
        case 8: q=sappend(q, "NAME: "); q=sappend(q, BUD_ELF[b->name_idx%BUD_NAMES_N]); *q=0; return buf;
        case 9: q=sappend(q, "COLOR: "); q=sappend(q, BUD_COLOR_NAME[b->color%BUD_COLORS_N]); *q=0; return buf;
        default: q=sappend(q, "CHANGE TO "); q=sappend(q, BUD_NAME[(b->species+1)%BUDN]); *q=0; return buf;
    }
}
static const char *station_row_right(int st, int row, Player *p, char *buf){
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
        case 8: case 9: return "FREE";
        default: fmt_int(buf, "", BUD_PRICE[(b->species+1)%BUDN], " KG"); return buf;
    }
}
static const char *station_blurb(int st, int row, Player *p){
    if (st==1) return row<WHALE_N? WHALE_BLURB[row]
                        : "PAID ON THE WEB? TYPE OR PASTE THE UNLOCK CODE IT GAVE YOU.";
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
        default: return "SWAP SPECIES. TRAINING, NAME AND XP CARRY OVER.";
    }
}
static void station_note(int pi, const char *msg){
    if (pi==G.my_id || pi==G.p2_slot) store_note(msg);
}
static void station_action(int pi, int st, int row){
    Player *p=&G.p[pi];
    Buddy *b=&G.bud[pi];
    // while the till is counting THIS player's carcasses, the counter
    // is closed to them (Daniel, Nov 6): no second purchase, and no
    // bank run that drains the hat out from under the counting till
    if (pay_active && pay_player==pi){ station_note(pi, "THE TILL IS COUNTING YOUR CARCASSES..."); sfx(16); return; }
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
static int whale_slot_of(int pi){ return (G.p2_slot>=0 && pi==G.p2_slot)? 1 : 0; }
static void whale_scroll_sync(int slot, int sel, int rows){
    int *sc = &whale_scroll[slot];
    if (sel < *sc) *sc = sel;
    if (sel > *sc + WHALE_VIS-1) *sc = sel - (WHALE_VIS-1);
    if (*sc > rows - WHALE_VIS) *sc = rows - WHALE_VIS;
    if (*sc < 0) *sc = 0;
}
static void station_input(int pi, int st, int up, int down, int activate){
    if (st==0){ store_input(pi, up, down, activate); return; }
    if (st==4){ if (activate) G.bowl[pi].act=1; return; }   // Bug Bowl runs on its own input
    // WHALES ONLY code entry: the local player is typing/pasting a code
    if (st==1 && pi==G.my_id && whale_code_mode[pi]){
        for (int i=0;i<IN.typed_n;i++){
            char c = IN.typed[i];
            if (c>='a' && c<='z') c = (char)(c-32);
            if ((c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='-'){
                if (whale_code_len[pi]<160){ whale_code_buf[pi][whale_code_len[pi]++]=c; whale_code_buf[pi][whale_code_len[pi]]=0; }
            }
        }
        // Backspace: a tap deletes one character; HOLDING it down
        // (Daniel's ask) keeps deleting — first repeat after ~0.35s,
        // then about twenty a second. IN.backspace is only the press
        // edge; IN.raw carries the held state.
        { int held = (int)((IN.raw >> PK_BACKSPACE) & 1);
          if (IN.backspace){ if (whale_code_len[pi]>0) whale_code_buf[pi][--whale_code_len[pi]]=0; whale_bs_hold[pi]=0; }
          else if (held){
              whale_bs_hold[pi]++;
              if (whale_bs_hold[pi]>21 && (whale_bs_hold[pi]%3)==0 && whale_code_len[pi]>0)
                  whale_code_buf[pi][--whale_code_len[pi]]=0;
          } else whale_bs_hold[pi]=0; }
        if (IN.enter_pressed){
            int sku = whale_redeem(whale_code_buf[pi]);
            whale_code_mode[pi]=0;
            if (sku>=0){ whale_grant(sku); store_note("UNLOCKED! IT IS ON THE COUNTER."); sfx(11); }
            else { store_note("THAT CODE DOES NOT CHECK OUT"); sfx(16); }
        } else if (IN.esc_pressed){ whale_code_mode[pi]=0; sfx(5); }
        return;
    }
    int *selp = (G.p2_slot>=0 && pi==G.p2_slot)? &G.store_sel2 : &G.store_sel;
    int rows = station_rows(st, &G.p[pi]);
    if (rows<=0) return;
    if (*selp>=rows) *selp=0;
    if (up){ *selp = (*selp + rows-1)%rows; sfx(6); }
    if (down){ *selp = (*selp + 1)%rows; sfx(6); }
    if (st==1) whale_scroll_sync(whale_slot_of(pi), *selp, rows);
    if (pi==G.my_id && IN.tapped){
        int w = G.ui_compact? 308 : STORE_W;
        int x = G.ui_compact? G.view_x0 + (G.view_w - w)/2 : STORE_X;
        if (IN.tap_x<x-6 || IN.tap_x>x+w+6 || IN.tap_y<STORE_Y-6 || IN.tap_y>STORE_Y+STORE_H+6) G.tap_close_store=1;
        else if (st==1){
            // tap math follows the SCROLLED window the draw shows
            int sc = whale_scroll[whale_slot_of(pi)];
            for (int i=sc;i<sc+WHALE_VIS && i<rows;i++){
                int ry = MALL_ROW0 + (i-sc)*STORE_ROWH;
                if (IN.tap_y>=ry-5 && IN.tap_y<=ry+STORE_ROWH-2){
                    if (i==*selp) activate=1; else { *selp=i; sfx(6); }
                    break;
                }
            }
        }
        else for (int i=0;i<rows;i++){
            int ry = MALL_ROW0 + i*STORE_ROWH;
            if (IN.tap_y>=ry-5 && IN.tap_y<=ry+STORE_ROWH-2){
                if (i==*selp) activate=1; else { *selp=i; sfx(6); }
                break;
            }
        }
    }
    if (activate){
        // WHALES ONLY on the local screen: codes are typed here, and a
        // buy opens THIS player's browser (never the host's).
        if (st==1 && pi==G.my_id){
            int row = *selp;
            if (row==WHALE_N){ whale_code_mode[pi]=1; whale_code_len[pi]=0; whale_code_buf[pi][0]=0; sfx(6); return; }
            if (row<WHALE_N && !(G.p[pi].whale_own & (1<<row))){
                if (WHALE_LINK[row][0]){ plat_open_url(WHALE_LINK[row]); store_note("PAYMENT PAGE OPENED - YOUR CODE COMES AFTER"); }
                else store_note("THE WHALE TILL IS NOT OPEN YET");
                sfx(11); return;
            }
        }
        if (G.net_mode==NET_CLIENT){
            int o=0; wr_u8(&o,'M'); wr_u8(&o,(u8)st); wr_u8(&o,(u8)*selp);
            plat_net_send(pkt, o, 0);
            sfx(10);
        } else station_action(pi, st, *selp);
    }
}
// ---------------------------------------------------------------------------
// BUG BOWL — the mall's bowling alley. You bowl with bug carcasses: one
// carcass off your head per ball. The scoring is the real thing —
// strikes score 10 plus your next two balls, spares 10 plus the next
// one, and the tenth frame can run to three balls with fresh racks
// after strikes and spares.
// ---------------------------------------------------------------------------
// The bowling ball's radius, in lane units. ONE constant rules both
// sides of the throw: the carcass is DRAWN at this radius and it only
// knocks a pin down when the two actually touch (ball radius + the
// pin's own half-width). What you see is exactly what hits.
#define BOWL_BALL_R 8
#define BOWL_PIN_R 4
static const float PIN_X[10] = { 0, -15, 15, -30, 0, 30, -45, -15, 15, 45 };
static const float PIN_Y[10] = { 300, 322, 322, 344, 344, 344, 366, 366, 366, 366 };
static void bowl_rack(Bowl *bw){ for (int i=0;i<10;i++){ bw->standing[i]=1; bw->pft[i]=0; } }
static int bowl_pins_up(Bowl *bw){ int n=0; for (int i=0;i<10;i++) n+=bw->standing[i]; return n; }
static void bowl_new_game(int pi){
    Bowl *bw=&G.bowl[pi];
    smemset(bw, 0, sizeof(*bw));
    bowl_rack(bw);
    bw->active=1; bw->phase=0; bw->angle_dir=1; bw->power_dir=1;
    scpy(bw->msg, "FRAME 1 - YOUR THROW!", 40); bw->msg_t=1.4f;
    sfx(19);
}
// cumulative score through frame f, or -1 if it can't be known yet
static int bowl_cum(Bowl *bw, int f){
    int ri=0, tot=0;
    for (int fr=0; fr<=f; fr++){
        if (fr<9){
            if (ri>=bw->nrolls) return -1;
            if (bw->rolls[ri]==10){
                if (ri+2>=bw->nrolls) return -1;
                tot += 10 + bw->rolls[ri+1] + bw->rolls[ri+2]; ri+=1;
            } else {
                if (ri+1>=bw->nrolls) return -1;
                if (bw->rolls[ri]+bw->rolls[ri+1]==10){
                    if (ri+2>=bw->nrolls) return -1;
                    tot += 10 + bw->rolls[ri+2];
                } else tot += bw->rolls[ri] + bw->rolls[ri+1];
                ri+=2;
            }
        } else {
            if (!bw->game_over) return -1;
            while (ri<bw->nrolls){ tot += bw->rolls[ri]; ri++; }
        }
    }
    return tot;
}
// the roll index where frame f's rolls begin (for the scoresheet)
static int bowl_frame_ri(Bowl *bw, int f){
    int ri=0;
    for (int fr=0; fr<f && ri<bw->nrolls; fr++)
        ri += (fr<9 && bw->rolls[ri]==10)? 1 : 2;
    return ri;
}
static void bowl_record(Bowl *bw, int knocked){
    char m = knocked==0? '-' : (knocked==bw->pins_before? (bw->pins_before==10? 'X' : '/') : (char)('0'+knocked));
    if (bw->nrolls<21){ bw->rolls[bw->nrolls]=knocked; bw->marks[bw->nrolls]=m; bw->nrolls++; }
    int f=bw->frame;
    if (f<9){
        if (bw->ball==0 && knocked<10){ bw->ball=1; }
        else { bw->frame++; bw->ball=0; bw->ri_frame=bw->nrolls; bowl_rack(bw); }
    } else {
        int r1 = bw->rolls[bw->ri_frame];
        if (bw->ball==0){
            if (knocked==10) bowl_rack(bw);
            bw->ball=1;
        } else if (bw->ball==1){
            int r2 = knocked;
            if (r1==10){ if (r2==10) bowl_rack(bw); bw->ball=2; }
            else if (r1+r2==10){ bowl_rack(bw); bw->ball=2; }
            else bw->game_over=1;
        } else bw->game_over=1;
    }
    if (bw->game_over){
        bw->phase=5;
        bw->final = bowl_cum(bw, 9);
        if (bw->final > G.rec_bowl) G.rec_bowl = bw->final;
    } else bw->phase=0;
}
static void bowl_knock(Bowl *bw, int i, float dx, float dy, float speed){
    float len = fsqrt(dx*dx + dy*dy);
    if (len<0.01f){ dx=0; dy=1; len=1; }
    bw->standing[i]=0;
    bw->pfx[i]=PIN_X[i]; bw->pfy[i]=PIN_Y[i];
    bw->pfvx[i]=dx/len*speed + rng_range(-36.0f, 36.0f);
    bw->pfvy[i]=dy/len*speed;
    bw->pft[i]=0.30f;
}
static void bowl_throw(int pi){
    Bowl *bw=&G.bowl[pi]; Player *p=&G.p[pi];
    if (p->stack_n<=0){
        scpy(bw->msg, "NO CARCASSES ON YOUR HEAD!", 40); bw->msg_t=1.6f;
        sfx(16); bw->phase=0; return;
    }
    if (G.net_mode==NET_CLIENT){
        // the guest's game is local; the host just takes the carcass
        bw->bl_kind = ((int)p->stack[p->stack_n-1])%4;
        int o=0; wr_u8(&o,'M'); wr_u8(&o,4); wr_u8(&o,77); plat_net_send(pkt, o, 0);
    } else {
        float w = hat_pop(p);
        bw->bl_kind = ((int)w)%4;
    }
    bw->pins_before = bowl_pins_up(bw);
    float sp = 260.0f + bw->power*3.0f;
    bw->bx = bw->thrower_x; bw->by = 6;
    bw->bvy = fcos(bw->angle)*sp;
    bw->bvx = fsin(bw->angle)*sp*0.6f;
    bw->bl_live=1; bw->bl_gutter=0; bw->phase=3;
    sfx(15);
}
static int bowl_axis(int pi){
    if (G.p2_slot>=0 && pi==G.p2_slot)
        return (act2_down(ACT_RIGHT)?1:0) - (act2_down(ACT_LEFT)?1:0);
    int k = IN.keys | IN.touch_keys;
    return ((k&K_RIGHT)?1:0) - ((k&K_LEFT)?1:0);
}
static void bowl_update(int pi, float dt){
    Bowl *bw=&G.bowl[pi];
    int act = bw->act; bw->act=0;
    if (bw->msg_t>0) bw->msg_t-=dt;
    // toppling pins fly, and can take their neighbours down with them
    for (int i=0;i<10;i++) if (bw->pft[i]>0){
        bw->pft[i]-=dt;
        bw->pfx[i]+=bw->pfvx[i]*dt; bw->pfy[i]+=bw->pfvy[i]*dt;
        for (int j=0;j<10;j++) if (bw->standing[j]){
            float dx=PIN_X[j]-bw->pfx[i], dy=PIN_Y[j]-bw->pfy[i];
            if (dx*dx+dy*dy < 15*15)
                bowl_knock(bw, j, bw->pfvx[i], bw->pfvy[i],
                           fsqrt(bw->pfvx[i]*bw->pfvx[i]+bw->pfvy[i]*bw->pfvy[i])*0.75f);
        }
    }
    if (!bw->active){ if (act) bowl_new_game(pi); return; }
    if (bw->game_over){ if (act) bowl_new_game(pi); return; }
    switch (bw->phase){
    case 0: { int ax=bowl_axis(pi);
              bw->thrower_x = fclamp(bw->thrower_x + ax*150.0f*dt, -52.0f, 52.0f);
              if (act){ bw->phase=1; bw->angle=-0.42f; bw->angle_dir=1; sfx(6); } } break;
    case 1: bw->angle += bw->angle_dir*1.15f*dt;   // kid-paced meters: the aim
            if (bw->angle>0.42f){ bw->angle=0.42f; bw->angle_dir=-1; }   // sweep and the
            if (bw->angle<-0.42f){ bw->angle=-0.42f; bw->angle_dir=1; }  // power bar both
            if (act){ bw->phase=2; bw->power=0; bw->power_dir=1; sfx(6); } break;  // run slower
    case 2: bw->power += bw->power_dir*130.0f*dt;
            if (bw->power>100){ bw->power=100; bw->power_dir=-1; }
            if (bw->power<0){ bw->power=0; bw->power_dir=1; }
            if (act) bowl_throw(pi); break;
    case 3: { float sp2 = fsqrt(bw->bvx*bw->bvx + bw->bvy*bw->bvy);
              bw->bx += bw->bvx*dt; bw->by += bw->bvy*dt;
              float fr = 1.0f - 0.10f*dt; bw->bvx*=fr; bw->bvy*=fr;
              if (!bw->bl_gutter && (bw->bx<-60 || bw->bx>60)){
                  // the ball drops in when its CENTER crosses the lane's
                  // edge — the forgiving rule, so a throw from the far
                  // side of the lane isn't born in the gutter
                  bw->bl_gutter=1; bw->bx = bw->bx<0? -67 : 67; bw->bvx=0; sfx(16);
              }
              if (bw->bl_gutter) bw->bx = bw->bx<0? -67 : 67;
              else for (int i=0;i<10;i++) if (bw->standing[i]){
                  float dx=PIN_X[i]-bw->bx, dy=PIN_Y[i]-bw->by;
                  if (dx*dx+dy*dy < (BOWL_BALL_R+BOWL_PIN_R)*(BOWL_BALL_R+BOWL_PIN_R)){
                      bowl_knock(bw, i, dx, dy, 230.0f + sp2*0.28f);
                      bw->bvx = bw->bvx*0.90f + dx*0.5f; bw->bvy *= 0.90f;
                      sfx(3);
                  }
              }
              if (bw->by>415 || sp2<50){ bw->phase=4; bw->settle_t=0.85f; bw->bl_live=0; } } break;
    case 4: bw->settle_t-=dt;
            if (bw->settle_t<=0){
                int knocked = bw->pins_before - bowl_pins_up(bw);
                int was_frame=bw->frame, was_ball=bw->ball, was_gutter=bw->bl_gutter;
                bowl_record(bw, knocked);
                if (bw->game_over){ scpy(bw->msg, "GAME OVER!", 40); sfx(11); }
                else if (knocked==10 && bw->pins_before==10){ scpy(bw->msg, "STRIKE!", 40); sfx(11); }
                else if (was_ball==1 && was_frame<9 && knocked>0 && knocked==bw->pins_before){ scpy(bw->msg, "SPARE!", 40); sfx(11); }
                else if (was_gutter && knocked==0){ scpy(bw->msg, "INTO THE GUTTER...", 40); }
                else if (knocked==0){ scpy(bw->msg, "A COMPLETE WHIFF!", 40); }
                else { char *q=bw->msg; q=sapp_int(q, knocked); q=sappend(q, knocked==1? " PIN!" : " PINS!"); *q=0; }
                bw->msg_t=1.8f;
            } break;
    }
}
static void draw_pin_shape(int x, int y, float tilt){
    int tx = (int)tilt;
    ellipse_fill(x, y+7, 6, 3, COL(198,192,184));
    ellipse_fill(x+tx, y, 5, 8, COL(245,242,238));
    rect_fill(x+tx-4, y-3, 8, 3, COL(200,60,50));
    disc_fill(x+tx, y-6, 3, COL(245,242,238));
    pset(x+tx-1, y-6, COL(30,30,40)); pset(x+tx+1, y-6, COL(30,30,40));   // beady bug eyes
}
static void draw_bowling(void){
    Bowl *bw=&G.bowl[G.store_pi]; Player *me=&G.p[G.store_pi];
    int x0=G.view_x0, vw=G.view_w, compact=G.ui_compact;
    int cx = x0 + vw/2;
    rect_fill(x0, G.view_y0, vw, G.view_h>0? G.view_h : VH, COL(16,14,28));
    // header
    rect_fill(x0+4, 22, vw-8, 30, COL(50,70,120)); rect_line(x0+4, 22, vw-8, 30, OUTLINE);
    draw_text_center(cx, 26, "BUG BOWL", 2, COL(255,230,170));
    { char b[64]; char *q=b;
      if (!bw->active) q=sappend(q, "THE MALL LANES - ONE CARCASS A BALL");
      else if (bw->game_over){ q=sappend(q, "FINAL SCORE: "); q=sapp_int(q, bw->final); }
      else { if (bw->frame<9){ q=sappend(q, "FRAME "); q=sapp_int(q, bw->frame+1); }
             else q=sappend(q, "TENTH FRAME");
             q=sappend(q, " - BALL "); q=sapp_int(q, bw->ball+1);
             q=sappend(q, " - CARCASSES: "); q=sapp_int(q, me->stack_n); }
      *q=0; draw_text_center(cx, 42, b, 1, COL(200,220,255)); }
    // scoresheet: ten frame boxes, roll marks on top, running total below
    int sy=58, sh=compact? 36 : 40, fw=(vw-16)/10;
    for (int f=0;f<10;f++){
        int fx = x0+8+f*fw;
        rect_line(fx, sy, fw-2, sh, COL(150,160,190));
        int cells = f<9? 2 : 3, cw2=(fw-2)/cells;
        for (int c=1;c<cells;c++) rect_fill(fx+c*cw2, sy, 1, 13, COL(150,160,190));
        rect_fill(fx, sy+13, fw-2, 1, COL(150,160,190));
        int fri = bowl_frame_ri(bw, f);
        char mb[2]; mb[1]=0;
        if (f<9){
            int strike = (fri<bw->nrolls && bw->rolls[fri]==10);
            if (strike){ mb[0]='X'; draw_text_center(fx+cw2+cw2/2, sy+3, mb, 1, COL(255,230,170)); }
            else {
                if (fri<bw->nrolls){ mb[0]=bw->marks[fri]; draw_text_center(fx+cw2/2, sy+3, mb, 1, COL(255,230,170)); }
                if (fri+1<bw->nrolls){ mb[0]=bw->marks[fri+1]; draw_text_center(fx+cw2+cw2/2, sy+3, mb, 1, COL(255,230,170)); }
            }
        } else for (int c=0;c<3;c++)
            if (fri+c<bw->nrolls){ mb[0]=bw->marks[fri+c]; draw_text_center(fx+c*cw2+cw2/2, sy+3, mb, 1, COL(255,230,170)); }
        int cum = bowl_cum(bw, f);
        if (cum>=0){ char cb[8]; char *q=cb; q=sapp_int(q, cum); *q=0;
                     draw_text_center(fx+(fw-2)/2, sy+sh-14, cb, 1, COL(240,244,255)); }
    }
    // the lane
    float KX = compact? 1.15f : 1.5f, KY = 0.49f;
    int foul = VH-54;
    int lane_top = foul - (int)(392*KY);
    rect_fill(cx-(int)(76*KX), lane_top, (int)(152*KX), foul-lane_top+8, COL(96,80,66));
    rect_fill(cx-(int)(60*KX), lane_top, (int)(120*KX), foul-lane_top+8, COL(216,186,130));
    rect_fill(cx-(int)(60*KX), lane_top, (int)(120*KX), (int)(112*KY), COL(202,172,118));   // the deck
    for (int a=0;a<5;a++){ int ax = cx + (int)((a-2)*15*KX), ay = foul - (int)(150*KY) - (a==2? 10:0);
        tri_fill(ax, ay-6, ax-3, ay+4, ax+3, ay+4, COL(196,164,108)); }
    rect_fill(cx-(int)(60*KX), foul, (int)(120*KX), 3, COL(200,60,50));                    // foul line
    // pins, far row first
    for (int i=9;i>=0;i--){
        if (bw->standing[i]) draw_pin_shape(cx + (int)(PIN_X[i]*KX), foul - (int)(PIN_Y[i]*KY), 0);
        else if (bw->pft[i]>0) draw_pin_shape(cx + (int)(bw->pfx[i]*KX), foul - (int)(bw->pfy[i]*KY), bw->pfvx[i]*0.045f);
    }
    // the ball: a carcass, rolled. Before the throw it waits in hand.
    // Drawn from BOWL_BALL_R so the picture IS the physics: the sprite's
    // radius on screen is the radius that knocks pins (draw_carcass's
    // body radius is size+3 screen px, hence the -3).
    if (bw->bl_live) draw_carcass(cx + (int)(bw->bx*KX), foul - (int)(bw->by*KY), (int)(BOWL_BALL_R*KX)-3, bw->bl_kind, bw->by*0.06f);
    else if (bw->active && !bw->game_over && bw->phase<=2)
        draw_carcass(cx + (int)(bw->thrower_x*KX) + 10, foul+13, (int)(BOWL_BALL_R*KX)-3, 0, 0.5f);
    // the bowler: a mini dev in the player's own hoodie
    if (bw->active && !bw->game_over){
        int txp = cx + (int)(bw->thrower_x*KX), typ = foul+16;
        ellipse_fill(txp, typ+8, 7, 3, COL(40,36,50));
        rect_fill(txp-5, typ-2, 10, 9, HOODIE[me->color&3]); rect_line(txp-5, typ-2, 10, 9, OUTLINE);
        disc_fill(txp, typ-6, 4, COL(240,200,160));
        if (bw->phase==1 || bw->phase==2){
            int sxp = cx + (int)(bw->thrower_x*KX), syp = foul - (int)(6*KY);
            int ex = sxp + (int)(fsin(bw->angle)*54), ey = syp - (int)(fcos(bw->angle)*54);
            for (int d=1;d<=3;d++){ int px2=sxp+(ex-sxp)*d/3, py2=syp+(ey-syp)*d/3;
                disc_fill(px2, py2, 1, COL(255,240,200)); }
        }
        if (bw->phase==2){
            rect_fill(cx-70, VH-42, 140, 8, COL(60,60,70)); rect_line(cx-70, VH-42, 140, 8, OUTLINE);
            u32 pc = bw->power<50? COL(90,200,90) : bw->power<80? COL(240,200,60) : COL(230,80,60);
            rect_fill(cx-69, VH-41, (int)(bw->power*1.38f), 6, pc);
            draw_text_center(cx, VH-52, "POWER", 1, COL(200,210,230));
        }
    }
    if (bw->msg_t>0) draw_text_center(cx, foul-108, bw->msg, 2, COL(255,214,64));
    if (!bw->active && G.rec_bowl>0){ char b[48]; char *q=b; q=sappend(q, "BEST GAME SO FAR: "); q=sapp_int(q, G.rec_bowl); *q=0;
        draw_text_center(cx, foul+30, b, 1, COL(200,220,255)); }
    if (bw->active && !bw->game_over && me->stack_n<=0 && bw->phase<=2)
        draw_text_center(cx, foul+30, "NO CARCASSES - GO SQUASH SOME BUGS!", 1, COL(255,140,120));
    // the prompt line
    { const char *ak = (G.store_pi==G.my_id)? key_name(G.bind[ACT_FIRE]) : "A";
      char pb[96]; char *q=pb;
      if (!bw->active || bw->game_over){ q=sappend(q, ak); q=sappend(q, " - NEW GAME"); }
      else if (bw->phase==0){ q=sappend(q, "LEFT/RIGHT - POSITION - "); q=sappend(q, ak); q=sappend(q, " - AIM"); }
      else if (bw->phase==1){ q=sappend(q, ak); q=sappend(q, " - LOCK THE ANGLE"); }
      else if (bw->phase==2){ q=sappend(q, ak); q=sappend(q, " - THROW!"); }
      else q=sappend(q, "ROLLING...");
      *q=0; draw_text_center(cx, VH-20, pb, 1, COL(160,230,255)); }
}

static void draw_mall_panel(int st){
    Player *me=&G.p[G.store_pi];
    Buddy *bud=&G.bud[G.store_pi];
    int sel = (G.p2_slot>=0 && G.store_pi==G.p2_slot)? G.store_sel2 : G.store_sel;
    int y=STORE_Y, h=STORE_H;
    int w = G.ui_compact? 308 : STORE_W;
    int x = G.ui_compact? G.view_x0 + (G.view_w - w)/2 : STORE_X;
    static const u32 BAND[4] = { COL(74,36,34), COL(64,40,84), COL(36,74,52), COL(120,66,32) };
    if (!G.ui_compact) rect_fill(0, 0, VW, VH, COL(16,14,28));
    else rect_fill(G.view_x0, G.view_y0, G.view_w, G.view_h>0? G.view_h : VH, COL(16,14,28));
    rect_fill(x, y+1, w+8, h+8, COL(8,6,14));
    rect_fill(x-4, y-4, w+8, h+8, OUTLINE);
    rect_fill(x, y, w, h, COL(250,244,225));
    rect_line(x+3, y+3, w-6, h-6, COL(198,164,84));
    rect_fill(x, y, w, 32, BAND[st&3]);
    rect_fill(x, y+31, w, 2, COL(198,164,84));
    draw_text_center(x+w/2, y+3, MALL_STATION[st&3], 2, COL(255,226,150));
    { char b[96]; char *q=b;
      if (st==1){ q=sappend(q, "PURE VANITY - $0.67 EACH - NO GAMEPLAY EFFECT"); }
      else if (st==2){ q=sappend(q, "BANKED: "); q=sapp_int(q, (int)me->banked); q=sappend(q, " KG - ON YOUR HEAD: "); q=sapp_int(q, (int)me->carried); q=sappend(q, " KG"); }
      else if (bud_stage[G.store_pi]==1 || bud_stage[G.store_pi]==3){ q=sappend(q, "NAMES OF THE ELDER DAYS - PAGE "); q=sapp_int(q, bud_page[G.store_pi]+1); q=sappend(q, " OF "); q=sapp_int(q, (BUD_NAMES_N+11)/12); }
      else if (bud_stage[G.store_pi]==2 || bud_stage[G.store_pi]==4){ q=sappend(q, "STANDARD COATS - AND ONE VERY NEON RAINBOW"); }
      else if (bud->active){ q=sappend(q, BUD_ELF[bud->name_idx%BUD_NAMES_N]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[bud->species%BUDN]); q=sappend(q, " - XP "); q=sapp_int(q, bud->xp);
          q=sappend(q, " - HP "); q=sapp_int(q, (int)bud->hp); q=sappend(q, "/"); q=sapp_int(q, (int)bud->maxhp);
          if (bud->fainted) q=sappend(q, " - FAINTED"); }
      else q=sappend(q, "NO BUDDY YET - EVERY DEV NEEDS A FRIEND");
      draw_text_center(x+w/2, y+20, b, 1, COL(255,240,205)); }
    if (!G.ui_compact)
    { const char *sh = "SHIELD: UP";
      draw_text(x+w-14-text_w(sh,1), y+8, sh, 1, COL(150,220,255)); }
    if (st==1 && whale_code_mode[G.store_pi]){
        // typing/pasting an unlock code replaces the price list
        draw_text_center(x+w/2, y+64, "TYPE OR PASTE YOUR UNLOCK CODE", 2, COL(40,36,50));
        rect_fill(x+16, y+108, w-32, 46, COL(255,252,240));
        rect_line(x+16, y+108, w-32, 46, COL(198,164,84));
        { const char *cb = whale_code_buf[G.store_pi]; int cl = whale_code_len[G.store_pi];
          int show = cl>56? cl-56 : 0;
          char disp[64]; int dn=0; for (int i=show;i<cl && dn<56;i++) disp[dn++]=cb[i]; disp[dn]=0;
          draw_text_center(x+w/2, y+118, dn? disp : "...", 1, COL(40,36,50));
          char cb2[24]; char *q2=cb2; q2=sapp_int(q2, cl); q2=sappend(q2, " / 142 CHARS"); *q2=0;
          draw_text_center(x+w/2, y+136, cb2, 1, COL(120,110,90)); }
        draw_text_center(x+w/2, y+176, "THE CODE IS YOUR RECEIPT - KEEP IT SAFE.", 1, COL(90,80,60));
        draw_text_center(x+w/2, y+192, "UNLOCKS LIVE IN YOUR SAVE DATA; THE CODE RESTORES THEM.", 1, COL(90,80,60));
        draw_text_center(x+w/2, y+224, "ENTER - REDEEM     ESC - CANCEL", 1, COL(40,36,50));
        return;
    }
    int rows = station_rows(st, me);
    int rowh = STORE_ROWH;   // Whales Only scrolls at full size now (Nov 5)
    int wsc = 0;             // the visible window's first row (Whales Only)
    if (st==1){ whale_scroll_sync(whale_slot_of(G.store_pi), sel, rows); wsc = whale_scroll[whale_slot_of(G.store_pi)]; }
    int rows_end = rows;
    if (st==1 && rows_end > wsc+WHALE_VIS) rows_end = wsc+WHALE_VIS;
    char nb[40], rb[24];
    for (int i=wsc;i<rows_end;i++){
        int ry = MALL_ROW0 + (i-wsc)*rowh;
        if (i%2==0) rect_fill(x+8, ry-2, w-16, rowh, COL(243,236,210));
        rect_line(x+8, ry-2, w-16, rowh, COL(228,219,192));
        if (i==sel){
            rect_fill(x+8, ry-2, w-16, rowh, COL(255,224,138));
            rect_fill(x+8, ry-2, 3, rowh, COL(190,60,40));
            rect_line(x+8, ry-2, w-16, rowh, COL(198,164,84));
        }
        const char *nm = station_row_name(st, i, me, nb);
        const char *rt = station_row_right(st, i, me, rb);
        if (G.ui_compact) draw_text(x+16, ry+5, nm, 1, COL(40,36,50));
        else draw_text(x+16, ry+2, nm, 2, COL(40,36,50));
        if (st==3 && (bud_stage[G.store_pi]==2 || bud_stage[G.store_pi]==4)){
            // a live swatch beside each coat — the rainbow one cycles
            Buddy *pb = &G.bud[G.store_pi]; int keep = pb->color; pb->color = i;
            u32 sw = bud_coat(pb, COL(168,164,158)); pb->color = keep;
            int swx = x+w-40, swy = ry + (G.ui_compact? 4:6), sww = G.ui_compact? 14:18, swh = G.ui_compact? 9:12;
            rect_fill(swx, swy, sww, swh, sw); rect_line(swx, swy, sww, swh, COL(90,80,60));
        }
        u32 pc = COL(150,95,30);
        if (G.ui_compact) draw_text(x+w-14-text_w(rt,1), ry+5, rt, 1, pc);
        else draw_text(x+w-14-text_w(rt,2), ry+2, rt, 2, pc);
    }
    if (st==1 && rows>WHALE_VIS){
        // the scrollbar: a track in the panel margin, a thumb that
        // says where in the sixteen rows you are (craft menu style)
        int track_y = MALL_ROW0-2, track_h = WHALE_VIS*STORE_ROWH-4;
        rect_fill(x+w-6, track_y, 3, track_h, COL(214,204,178));
        int th = track_h*WHALE_VIS/rows; if (th<12) th=12;
        int ty2 = track_y + (track_h-th)*wsc/(rows-WHALE_VIS);
        rect_fill(x+w-6, ty2, 3, th, COL(190,60,40));
    }
    int by = y+h-40;
    int bh = G.ui_compact? 24 : 20;
    rect_fill(x+8, by-4, w-16, bh, COL(240,232,205));
    rect_line(x+8, by-4, w-16, bh, COL(198,164,84));
    rect_fill(x+8, by-4, 3, bh, COL(190,60,40));
    if (G.ui_compact){
        char bb[128]; scpy(bb, station_blurb(st, sel, me), 128);
        int cut=-1; for (int i=0; bb[i] && i<34; i++) if (bb[i]==' ') cut=i;
        if (cut>0 && bb[cut+1]){ bb[cut]=0;
            draw_text(x+14, by+1, bb, 1, COL(80,66,44));
            draw_text(x+14, by+12, bb+cut+1, 1, COL(80,66,44));
        } else draw_text(x+14, by+2, bb, 1, COL(80,66,44));
    }
    else draw_text(x+14, by+2, station_blurb(st, sel, me), 1, COL(80,66,44));
    if (pay_active) draw_text_center(x+w/2, y+h-13, "COUNTING CARCASSES...", 1, COL(190,60,40));
    else if (G.store_msg_t>0) draw_text_center(x+w/2, y+h-13, G.store_msg, 1, COL(60,140,60));
    else if (G.ui_compact) draw_text_center(x+w/2, y+h-13, "SHIELD UP - FIRE SELECTS - BACK LEAVES", 1, COL(88,76,54));
    else if (mobile_ui()) draw_text_center(x+w/2, y+h-13, "TAP A ROW - TAP AGAIN TO SELECT - TAP OUTSIDE TO LEAVE", 1, COL(88,76,54));
    else { char b[80]; char *q=b; q=sappend(q, "ARROWS CHOOSE - "); q=sappend(q, key_name(G.bind[ACT_FIRE]));
           q=sappend(q, " SELECTS - "); q=sappend(q, key_name(G.bind[ACT_INTERACT])); q=sappend(q, " LEAVES");
           draw_text_center(x+w/2, y+h-13, b, 1, COL(88,76,54)); }
}
static void draw_active_panel(void){
    if (G.store_station==0) draw_store_panel();
    else if (G.store_station==4) draw_bowling();
    else draw_mall_panel(G.store_station);
}

// ---------------------------------------------------------------------------
// Menus / splash / help / game over
// ---------------------------------------------------------------------------
static void draw_demo_scene(void){
    // a living diorama behind the menus: plaza, store, a dev with a tower
    // hat, and bugs wandering in circles
    G.camx = 0; G.camy = 30;
    render_world();                    // (no entities exist in menu state)
    static Bug demo[8];
    for (int i=0;i<8;i++){
        Bug *b=&demo[i];
        b->alive=1; b->kind=i%4; b->w=4+i*7; b->hp=b->maxhp=100;
        float a = G.anim_t*0.35f + i*1.7f;
        b->x = fcos(a)*(96+i*22); b->y = 128 + fsin(a*1.3f)*26 + i*5;
        b->leg_ph = G.anim_t*9 + i; b->wobble = a; b->hit_t=0; b->bp_t=0; b->seed=i*31;
        draw_bug_sprite(b);
    }
    // one absolute unit of a bug lumbering along the bottom, forever
    { static Bug big;
      float gx = G.anim_t*24.0f; gx -= (int)(gx/880.0f)*880.0f; gx -= 440.0f;
      big.alive=1; big.kind=1; big.w=64; big.hp=big.maxhp=100;
      big.x=gx; big.y=172; big.leg_ph=G.anim_t*5.0f; big.wobble=0.6f;
      big.hit_t=0; big.bp_t=0; big.seed=777;
      draw_bug_sprite(&big); }
    static Player hero;
    smemset(&hero, 0, sizeof(hero));
    hero.connected=1; hero.alive=1; hero.name[0]=0;
    hero.x=-52; hero.y=86; hero.weapon=0; hero.owned[0]=1;
    hero.hp=hero.maxhp=100; hero.walk_ph=G.anim_t*4;
    hero.aim=-0.5f + fsin(G.anim_t*0.45f)*0.45f;      // idly scanning for bugs
    hero.stack_n = 30 + (int)(fsin(G.anim_t*0.6f)*9.0f);   // the hat breathes
    hero.carried=190;
    for (int i=0;i<48;i++) hero.stack[i]=3+(i%5);
    draw_player_sprite(&hero);
    // drifting pollen in the menu light (glow-up)
    for (int i=0;i<22;i++){
        float mx = fsin(G.anim_t*0.11f + i*2.39f)*300 + fcos(G.anim_t*0.05f+i)*40;
        float my = 40 + (float)((i*67)%260) + fsin(G.anim_t*0.23f + i*1.7f)*9;
        pset_blend(VW/2 + (int)mx, (int)my, COL(255,240,200), 70 + (i%4)*30);
    }
}
static const char *menu_items(int *count){
    static char buf[8][24];
    int n=0;
    if (G.has_save){ scpy(buf[n++], "CONTINUE", 24); }
    scpy(buf[n++], "SINGLE PLAYER", 24);
    scpy(buf[n++], "HOST LAN GAME", 24);
    scpy(buf[n++], "JOIN LAN GAME", 24);
    scpy(buf[n++], "HOW TO PLAY", 24);
    *count=n;
    // return base pointer via a hack: caller uses menu_item(i)
    (void)buf;
    return 0;
}
// Every build has the same dead-simple menu — START and go. There is
// no LAN mode to pick: native builds are always hosts (friends join
// from a browser), and the web build is a guest when a native host
// served the page, single-player otherwise.
#define MENU_SIMPLE 1
#define MENU_FIELDS 1
#ifdef PLAT_WASM
#define START_MODE NET_OFF
#else
#define START_MODE NET_HOST
#endif
// the 2P CO-OP row exists only while two controllers are detected —
// Daniel's rule: the option appears automatically with the hardware.
static int coop_row(void){ return IN.pad_count>=2; }
// with gamepads in use the SETTINGS / CONTROLS row (keyboard bindings,
// touch scheme) leaves the menu — pad players never see it.
static int menu_base_count(void){ return (G.has_save? 7:6) - (menu_pad_mode()? 1:0); }
static const char *menu_item(int i){
    static char buf[24];
    static char dbuf[24];
    int idx=i;
    if (coop_row() && i==menu_base_count()) return G.coop_pref? "2P CO-OP: ON" : "2P CO-OP: OFF";
    if (G.has_save){ if (idx==0) return "CONTINUE"; idx--; }
    if (menu_pad_mode() && idx>=2) idx++;   // step over the hidden controls row
    if (MENU_SIMPLE){
        switch (idx){ case 0: return "START"; case 1: return "HOW TO PLAY";
                      case 2: return mobile_ui()? (G.touch_ctl? "CONTROLS: STICK" : "CONTROLS: DRAG") : "SETTINGS";
                      case 3: { char *q=dbuf; q=sappend(q, "DIFFICULTY: "); q=sappend(q, DIFF_NAME[G.difficulty&3]); *q=0; return dbuf; }
                      case 4: return G.zen? "ZEN MODE: ON" : "ZEN MODE: OFF";
                      default: return G.eddie? "EDDIE MODE: ON" : "EDDIE MODE: OFF"; }
    }
    switch (idx){
        case 0: return "SINGLE PLAYER";
        case 1: return "HOST LAN GAME";
        case 2: return "JOIN LAN GAME";
        default: return "HOW TO PLAY";
    }
    (void)buf;
}
static int menu_count(void){
    if (MENU_SIMPLE) return menu_base_count() + (coop_row()? 1:0);
    return G.has_save? 5:4;
}
static void slots_activate(int i){
    if (i==3){ G.state=GS_MENU; return; }                       // BACK row
    int s=i+1;
    if (G.slots_mode==0){                                       // load
        if (!G.slot_has[s]){ store_note("THAT SLOT IS EMPTY"); return; }
        continue_run(s);
    } else {                                                    // new game
        if (G.slot_has[s] && G.slots_confirm!=s){ G.slots_confirm=s; return; }
        G.slots_confirm=0; G.slot=s; start_run(START_MODE);
    }
}
static void menu_activate(int sel){
    int idx=sel;
    if (coop_row() && sel==menu_base_count()){ G.coop_pref ^= 1; do_save(); sfx(6); return; }
    if (G.has_save){ if (idx==0){ open_slots(0); return; } idx--; }
    if (menu_pad_mode() && idx>=2) idx++;   // step over the hidden controls row
    if (MENU_SIMPLE){
        if (idx==0) open_slots(1);
        else if (idx==1){ G.prev_state=G.state; G.state=GS_HELP; }
        else if (idx==3){ G.difficulty = (G.difficulty+1)&3; do_save(); sfx(6); }   // difficulty for the next run
        else if (idx==4){ G.zen ^= 1; do_save(); sfx(6); }              // zen for the next run
        else if (idx==5){ G.eddie ^= 1; do_save(); sfx(6); }            // eddie: puffball bugs (cosmetic)
        else if (mobile_ui()){ G.touch_ctl ^= 1; do_save(); sfx(6); }   // toggle drag <-> stick
        else { G.set_sel=0; G.set_capture=0; G.state=GS_SETTINGS; }
        return;
    }
    switch (idx){
        case 0: start_run(NET_OFF); break;
        case 1: start_run(NET_HOST); break;
        case 2:
            if (!G.join_ip[0]){ store_note("TYPE THE HOST IP FIRST"); G.menu_sel = menu_count()+1; }
            else start_run(NET_CLIENT);
            break;
        default: G.prev_state=G.state; G.state=GS_HELP; break;
    }
}
static void field_append(char *buf, int cap){
    int n=slen(buf);
    for (int i=0;i<IN.typed_n;i++){
        char c=IN.typed[i];
        if (n<cap-1){ buf[n++]=c; buf[n]=0; }
    }
    if (IN.backspace && n>0) buf[n-1]=0;
}
static void render_menu(void){
    draw_demo_scene();
    // dead bugs drift up past the scene like ash — the evidence rises
    for (int i=0;i<12;i++){
        unsigned h = (unsigned)i*2654435761u;
        float rise = G.anim_t*(12.0f + (float)((h>>16)%11)) + (float)(h%340);
        rise -= (int)(rise/400.0f)*400.0f;
        int cx = (int)((h>>8)%620) + 10 + (int)(fsin(G.anim_t*1.2f + (float)i*1.7f)*9.0f);
        draw_carcass(cx, 372 - (int)rise, 5 + (int)((h>>20)%3), (int)((h>>24)%4), G.anim_t*0.8f + (float)i);
    }
    // dim the top for the title (band shortened Nov 14, 2026 at
    // Daniel's word: the tall header was crowding the rows below it
    // into the records strip — the whole menu stack now starts higher)
    rect_fill(0, 0, VW, 96, COL(20,18,36));
    // twinkling stars in the title band
    for (int i=0;i<42;i++){
        unsigned h = (unsigned)i*2246822519u + 97u;
        int sx = (int)((h>>7)%636) + 2, sy = (int)((h>>15)%84) + 4;
        int ph = (int)(G.anim_t*2.0f + (float)((h>>24)%5));
        u32 c = (ph%5==0)? COL(255,250,220) : (ph%5==1)? COL(190,205,255) : COL(90,100,140);
        rect_fill(sx, sy, 2, 2, c);
    }
    rect_fill(0, 94, VW, 2, COL(255,205,40));
    glow_ellipse(VW/2, 28, 200, 24, COL(255,214,120), 26);   // a warm halo behind the title (glow-up)
    // the title, letter by letter on a slow wave, gold shimmering
    // (the font advances 8px per char times scale — 32px at scale 4)
    { const char *T = "THE SURVIVAL GAME";
      int x0 = VW/2 - (17*32)/2;
      for (int i=0;i<17;i++){
          if (T[i]==' ') continue;
          char ch[2]; ch[0]=T[i]; ch[1]=0;
          float ph2 = G.anim_t*3.0f - (float)i*0.55f;
          int y = 8 + (int)(fsin(ph2)*3.0f);
          float t2 = (fsin(ph2)+1.0f)*0.5f;
          draw_text(x0+i*32+2, y+3, ch, 4, COL(12, 10, 26));   // v1.22: a drop shadow grounds the logo
          draw_text(x0+i*32, y, ch, 4, COL(255, 214 + (int)(34.0f*t2), 64 + (int)(120.0f*t2)));
      } }
    { float tp = (fsin(G.anim_t*2.0f)+1.0f)*0.5f;
      draw_text_center(VW/2, 50, "SQUASH BUGS. WEAR THE EVIDENCE.", 1, COL(160 + (int)(70.0f*tp), 230 + (int)(20.0f*tp), 255)); }
    // menu items (selection also covers the two text fields below them:
    // rows n and n+1 are NAME and HOST IP — keyboard only, no mouse)
    int n=menu_count();
    if (G.menu_sel>=n+MENU_FIELDS) G.menu_sel=0;
    for (int i=0;i<n;i++){
        int y=108+i*22;
        const char *label=menu_item(i);
        if (i==G.menu_sel){
            rect_fill(0, y-4, VW, 18, COL(255,205,40));   // full-width bar: the whole row is the option
            draw_text_center(VW/2, y, label, 2, COL(30,26,44));
        } else draw_text_center(VW/2, y, label, 2, COL(235,238,245));
    }
    // name field (+ ip field on native builds)
    int fy=108+n*22+8;
    int caret = ((int)(G.anim_t*2))%2==0;
    int nx = MENU_SIMPLE? VW/2-82 : VW/2-150;
    draw_text(nx, fy+3, "NAME:", 1, G.menu_sel==n? COL(255,205,40):COL(220,225,235));
    rect_fill(nx+42, fy, 130, 13, G.menu_sel==n? COL(70,80,110):COL(44,46,66));
    rect_line(nx+42, fy, 130, 13, G.menu_sel==n? COL(255,205,40):COL(140,150,180));
    { char b[16]; char *q=b; q=sappend(q, G.my_name); if (G.menu_sel==n && caret) q=sappend(q, "_");
      draw_text(nx+46, fy+3, b, 1, COL(255,255,255)); }
    if (G.whale_on & 8192){   // WHALES ONLY: NAME IN LIGHTS — marquee bulbs chase around the name box
        static const u32 BULB[4] = { COL(255,90,90), COL(255,205,60), COL(120,230,255), COL(255,240,210) };
        int bx0 = nx+42, by0 = fy, bx1 = nx+42+130, by1 = fy+13, bi = 0;
        for (int bx=bx0; bx<=bx1; bx+=11){ disc_fill(bx, by0-3, 2, BULB[(bi + (int)(G.anim_t*6))&3]); bi++; disc_fill(bx, by1+3, 2, BULB[(bi + (int)(G.anim_t*6))&3]); bi++; }
        for (int by=by0+6; by<by1; by+=11){ disc_fill(bx0-4, by, 2, BULB[(bi + (int)(G.anim_t*6))&3]); bi++; disc_fill(bx1+4, by, 2, BULB[(bi + (int)(G.anim_t*6))&3]); bi++; }
    }
    if (!MENU_SIMPLE){
    draw_text(VW/2+34, fy+3, "HOST IP:", 1, G.menu_sel==n+1? COL(255,205,40):COL(220,225,235));
    rect_fill(VW/2+92, fy, 130, 13, G.menu_sel==n+1? COL(70,80,110):COL(44,46,66));
    rect_line(VW/2+92, fy, 130, 13, G.menu_sel==n+1? COL(255,205,40):COL(140,150,180));
    { char b[52]; char *q=b; q=sappend(q, G.join_ip[0]? G.join_ip : "192.168.1.X");
      if (G.menu_sel==n+1 && caret) q=sappend(q, "_");
      draw_text(VW/2+96, fy+3, b, 1, COL(255,255,255)); }
    }
    // hosting builds: tell the player how friends join — they just open
    // this machine's address in a browser. No install, no mode to pick.
    { const char *ja = plat_join_addr();
      if (ja && ja[0]){
          char b[96]; char *q=b;
          q=sappend(q, "http://"); q=sappend(q, ja);
          draw_text_center(VW/2, fy+24, "PLAY WITH FRIENDS - THEY JUST OPEN THIS IN A BROWSER:", 1, COL(150,160,185));
          draw_text_center(VW/2, fy+38, b, 2, COL(255,214,64));
      } }
    // records strip
    { char b[128]; char *q=b;
      q=sappend(q, "ALL-TIME HEAVIEST HAT: "); q=sappend(q, G.rec_name); q=sappend(q, " - ");
      q=sapp_int(q, (int)G.rec_hat); q=sappend(q, " KG  |  SQUASHED: ");
      q=sapp_int(q, G.rec_kills); q=sappend(q, "  |  BEST BUG BOWL: "); q=sapp_int(q, G.rec_bowl);
      draw_text_center(VW/2, VH-30, b, 1, COL(170,190,220)); }
    // (no standing controls line under the menu any more, Daniel,
    // Nov 6: the bottom of the screen stays clean — the pause menu's
    // CONTROLS panel is the reference, and menus work the same
    // everywhere: move, select, tap.)
    if (G.store_msg_t>0) draw_text_center(VW/2, 68, G.store_msg, 1, COL(255,130,130));
    (void)menu_items;
}
static void render_slots(void){
    draw_demo_scene();
    rect_fill(0, 0, VW, 96, COL(20,18,36));
    rect_fill(0, 94, VW, 2, COL(255,205,40));
    draw_text_center(VW/2+2, 16, "THE SURVIVAL GAME", 2, COL(12, 10, 26));   // v1.22: shadow, matching the title
    draw_text_center(VW/2, 14, "THE SURVIVAL GAME", 2, COL(255,214,64));
    draw_text_center(VW/2, 44, G.slots_mode==0? "LOAD A SAVE" : "NEW GAME - PICK A SLOT", 2, COL(235,238,245));
    for (int i=0;i<3;i++){
        int s=i+1, y=116+i*52, x=100, w=440;
        int sel = (G.slots_sel==i);
        ui_panel(x, y, w, 44);   // v1.22: slot cards wear the shared panel chrome
        if (sel){ blend_rect(x, y, w, 44, COL(255,205,40), 36);
                  rect_line(x, y, w, 44, COL(255,205,40)); rect_line(x+1, y+1, w-2, 44-2, COL(255,205,40)); }
        { char b[40]; char *q=b; q=sappend(q, "SLOT "); q=sapp_int(q, s);
          if (G.slot_has[s] && pk_name[s][0]){ q=sappend(q, " - "); q=sappend(q, pk_name[s]); }
          *q=0;
          draw_text(x+16, y+7, b, 2, sel? COL(255,214,64) : COL(220,225,235)); }
        if (G.slots_confirm==s && G.slots_mode==1){
            draw_text(x+16, y+28, "START FRESH HERE? THE OLD SAVE GOES AWAY.", 1, COL(255,120,120));
        } else if (!G.slot_has[s]){
            draw_text(x+16, y+28, "EMPTY", 1, COL(130,138,160));
        } else {
            char b[96]; char *q=b;
            q=sappend(q, pk_name[s]);
            if (pk_bname[s][0]){ q=sappend(q, " + "); q=sappend(q, pk_bname[s]); q=sappend(q, " THE "); q=sappend(q, BUD_NAME[pk_bspecies[s]%BUDN]); }
            if (pk_hat[s]>0){ q=sappend(q, " - HAT "); q=sapp_int(q, pk_hat[s]); q=sappend(q, " KG"); }
            q=sappend(q, " - "); q=sapp_int(q, pk_kills[s]); q=sappend(q, " SQUASHED - ");
            q=sapp_int(q, pk_time[s]/60); q=sappend(q, ":");
            if (pk_time[s]%60<10) q=sappend(q, "0"); q=sapp_int(q, pk_time[s]%60);
            if (pk_zen[s]) q=sappend(q, " - ZEN");
            draw_text(x+16, y+28, b, 1, COL(200,210,228));
        }
    }
    { int y=278;
      if (G.slots_sel==3){ rect_fill(0, y-4, VW, 19, COL(255,205,40)); draw_text_center(VW/2, y, "BACK", 2, COL(30,26,44)); }
      else draw_text_center(VW/2, y, "BACK", 2, COL(235,238,245)); }
    if (G.store_msg_t>0) draw_text_center(VW/2, 330, G.store_msg, 1, COL(255,130,130));
}
static void menu_input(void){
    // The selection walks the menu items AND the text fields below them
    // (row n = NAME, row n+1 = HOST IP, native only). On a field row,
    // typed letters go into the field; Space selects (it must not also
    // land in the field as a typed char). E is just a letter there.
    int n=menu_count();
    int rows=n+MENU_FIELDS;
    if (IN.menu_up){ G.menu_sel=(G.menu_sel+rows-1)%rows; }
    if (IN.menu_down){ G.menu_sel=(G.menu_sel+1)%rows; }
    // touch: tap an item to go straight to it, tap a field to select it
    if (IN.tapped){
        int fy=108+n*22+8;
        if (IN.tap_y>=fy-4 && IN.tap_y<=fy+17){
            G.menu_sel = (MENU_FIELDS==2 && IN.tap_x>=376)? n+1 : n;
        } else for (int i=0;i<n;i++){
            if (IN.tap_y>=108+i*22-6 && IN.tap_y<=108+i*22+16){ menu_activate(i); return; }
        }
    }
    if (G.menu_sel<n){
        if (IN.interact_pressed || IN.select_pressed) menu_activate(G.menu_sel);
    } else {
        if (IN.select_pressed){
            if (G.menu_sel==n){ if (MENU_FIELDS==2) G.menu_sel=n+1; }  // name -> ip
            else menu_activate(G.has_save? 3:2);                       // ip -> JOIN LAN GAME
        } else {
            if (G.menu_sel==n) field_append(G.my_name, 13);
            else field_append(G.join_ip, 48);
        }
    }
    G.edit_field = G.menu_sel==n? 1 : (G.menu_sel==n+1? 2 : 0);
}
static void move_key_str(char *b){
    // the four movement key names: concatenated when all are single
    // characters ("WASD"), slash-joined otherwise ("W/UP/S/DOWN" style).
    // key_name() returns a shared scratch buffer, so each name is copied
    // into the output before the next call.
    static const int idx[4] = { ACT_UP, ACT_LEFT, ACT_DOWN, ACT_RIGHT };
    int one=1;
    for (int i=0;i<4;i++) if (key_name(G.bind[idx[i]])[1]) one=0;
    char *q=b;
    for (int i=0;i<4;i++){ if (i && !one) q=sappend(q, "/"); q=sappend(q, key_name(G.bind[idx[i]])); }
    *q=0;
}
static void render_help(void){
    rect_fill(0,0,VW,VH,COL(24,22,38));
    draw_text_center(VW/2, 18, help_page? "THE MALL + MORE" : "HOW TO PLAY", 3, COL(255,214,64));
    if (help_page){
        const char *lines2[] = {
            "DIFFICULTY: EASY IS CLASSIC; THE REST MAKE BUGS MEANER.",
            "",
            "THE BUG BOUNTY IS NOW THE BUG MALL - PRESS INTERACT",
            "AT THE HUT TO STEP INSIDE. FIVE SHOPS, ONE BUILDING:",
            "THE COUNTER SELLS EVERYTHING IT ALWAYS DID.",
            "",
            "WHALES ONLY SELLS PURE VANITY FOR REAL MONEY: GOLD",
            "CARCASSES, A CAPE, RAIN THAT FALLS ONLY ON YOU. NO",
            "GAMEPLAY EFFECT WHATSOEVER - JUST GLORY. $0.67 EACH.",
            "",
            "THE BUG BANK KEEPS CARCASSES SAFE - THEY WEIGH NOTHING,",
            "NEVER COUNT TO YOUR HEAVIEST HAT, MOVE IN FRACTIONS.",
            "",
            "BUG BOWL: THE MALL'S OWN LANES. ONE CARCASS A BALL,",
            "FULL BOWLING SCORING: STRIKES, SPARES, TENTH FRAME.",
            "",
            "BANZAI BUDDIES: A PUPPY, KITTY, MOOSE, PLATYPUS, PET",
            "ROCK OR KITCHEN SINK FIGHTS AT YOUR SIDE, EARNING XP",
            "FROM KILLS AND ASSISTS TO SPEND ON TRAINING + MOVES.",
            "YOU PICK ITS NAME (ELDER DAYS, MOSTLY) AND ITS COAT -",
            "NEON RAINBOW INCLUDED. BUDDIES ONLY FAINT - NEVER DIE -",
            "AND THE SHOP REVIVES THEM - TRAINED ONES COST MORE.",
            "",
            "BIG HATS ARE FRAGILE: ONCE YOU CARRY 50 KG OR MORE,",
            "EVERY BITE SHAKES A CARCASS LOOSE. DON'T GET GREEDY.",
            "",
            "WALLS LINK UP: RUNS OF 5+ SHARE ONE BIG HEALTH POOL",
            "AND YOU CAN CLIMB UP AND WALK ALONG THE TOP. BUT A",
            "WALL MAY NEVER CLOSE INTO A LOOP - THE LOOP MUST",
            "STAY OPEN.",
        };
        for (int i=0;i<(int)(sizeof(lines2)/sizeof(lines2[0]));i++)
            draw_text_center(VW/2, 44+i*9, lines2[i], 1, COL(225,230,240));
        if (pad_mode()) draw_text_center(VW/2, VH-24, "B - BACK TO MENU", 1, COL(160,230,255));
        else if (mobile_ui()) draw_text_center(VW/2, VH-24, "TAP - BACK TO MENU", 1, COL(160,230,255));
        else { char b[48]; char *q=b; q=sappend(q, key_name(G.bind[ACT_INTERACT])); q=sappend(q, " - BACK TO MENU");
          draw_text_center(VW/2, VH-24, b, 1, COL(160,230,255)); }
        return;
    }
    char mv[40], l0[96], l1[96], l2[96], l3[96], l4[96], l5[96];
    scpy(l5, mobile_ui()? "CO-OP: A DESKTOP APP HOSTS - PHONES JOIN IN A BROWSER."
                        : "CO-OP: THE APP HOSTS - FRIENDS JOIN FROM A BROWSER.", 96);
    if (pad_mode()){
        scpy(l0, "STICK OR D-PAD - MOVE. YOU FACE THE WAY YOU MOVE,", 96);
        scpy(l1, "AND YOU FIRE THE WAY YOU FACE. A - FIRE + SELECT.", 96);
        scpy(l2, "LB / RB - WEAPONS - X WALL - LT SPRINT - START PAUSE", 96);
        scpy(l3, "B - ENTER AND LEAVE THE STORE. A PICKS, B BACKS OUT.", 96);
        scpy(l4, "TWO PADS PLUGGED IN? FLIP ON 2P CO-OP IN THE MENU.", 96);
    } else if (mobile_ui() && G.touch_ctl){
        scpy(l0, "PUSH YOUR LEFT THUMB TO STEER WITH THE", 96);
        scpy(l1, "STICK. YOU FIRE THE WAY YOU FACE.", 96);
        scpy(l2, "HOLD FIRE TO SHOOT. THE WALL BUTTON BUILDS", 96);
        scpy(l3, "A WALL FROM 3 CARCASSES. BY THE STORE,", 96);
        scpy(l4, "TAP USE TO GO IN - OR TAP THE STORE ITSELF.", 96);
    } else if (mobile_ui()){
        scpy(l0, "DRAG TO MOVE. YOU FACE THE WAY YOU DRAG,", 96);
        scpy(l1, "AND YOU FIRE THE WAY YOU FACE AS YOU GO.", 96);
        scpy(l2, "TAP WALL TO BUILD A WALL FROM 3 CARCASSES.", 96);
        scpy(l3, "WALK TO THE BUG MALL AND TAP TO GO IN.", 96);
        scpy(l4, "IN THE SHOPS: TAP A ROW, TAP IT AGAIN TO BUY.", 96);
    } else {
    move_key_str(mv);
    { char *q=l0; q=sappend(q, mv); q=sappend(q, " / ARROWS - MOVE. YOU FACE THE WAY YOU MOVE,"); }
    { char *q=l1; q=sappend(q, "AND YOU FIRE THE WAY YOU FACE. "); q=sappend(q, key_name(G.bind[ACT_FIRE])); q=sappend(q, " - FIRE + SELECT."); }
    { char *q=l2; q=sappend(q, key_name(G.bind[ACT_W1])); q=sappend(q, "-"); q=sappend(q, key_name(G.bind[ACT_W5]));
      q=sappend(q, " WEAPONS - "); q=sappend(q, key_name(G.bind[ACT_WALL])); q=sappend(q, " WALL - ");
      q=sappend(q, key_name(G.bind[ACT_SPRINT])); q=sappend(q, " SPRINT - "); q=sappend(q, key_name(G.bind[ACT_PAUSE])); q=sappend(q, " PAUSE (1P)"); }
    { char *q=l3; q=sappend(q, key_name(G.bind[ACT_INTERACT])); q=sappend(q, " - ENTER AND LEAVE THE STORE. REBIND KEYS IN SETTINGS."); }
    scpy(l4, "ON MOBILE: DRAG TO MOVE + SHOOT. TAP SELECTS + BUYS.", 96);
    }
    const char *lines[] = {
        l0, l1, l2, l3, l4,
        "",
        "SQUASH BUGS: THEIR CARCASSES LAND ON YOUR HEAD.",
        "YOUR HAT IS YOUR WALLET - AND IT SLOWS YOU DOWN.",
        "HAUL IT TO THE BUG MALL AT THE EPICENTER AND PAY BY",
        "WEIGHT AT THE COUNTER FOR WEAPONS, SHIELDS + MORE.",
        "HURT? A HOTFIX PATCHES YOU UP: +50 HP, 20 KG A POP.",
        "",
        "WHILE ANYONE IS IN THE MALL, A SHIELD SEALS THE",
        "EPICENTER - BUGS CAN'T CROSS THE RING. OTHERWISE IT'S",
        "NOT SAFE: BUGS WANDER RIGHT IN. NEAR HOME THEY ARE",
        "FEW, TINY AND SLOW: POCKET CHANGE.",
        "BUGS SCALE WITH DISTANCE: TO FIND THE BIG ONES YOU",
        "HAVE TO GO FAR OUT - WHERE THEY ARE BIGGER, MEANER,",
        "FAR MORE NUMEROUS, AND EVEN THE LITTLE ONES ARE FAST.",
        "",
        "WALLS OF CARCASS BLOCK BUGS UNTIL THEY CHEW THROUGH.",
        "YOUR SHOTS FLY OVER WALLS - BUGS HAVE TO CHEW THROUGH.",
        "IF EVERY PLAYER FALLS, THE RUN IS OVER. THE WORLD SAVES",
        "AS YOU GO. HEAVIEST HAT TOPS THE LEADERBOARD.",
        "",
        l5,
        "ZEN MODE (MENU): EVERY BUG IS GIGANTIC AND SLOW.",
        "A CALMER GAME - AND ZEN RUNS SET NO RECORDS.",
    };
    for (int i=0;i<(int)(sizeof(lines)/sizeof(lines[0]));i++)
        draw_text_center(VW/2, 48+i*10, lines[i], 1, COL(225,230,240));
    if (pad_mode()) draw_text_center(VW/2, VH-24, "B - PAGE 2: THE MALL + MORE", 1, COL(160,230,255));
    else if (mobile_ui()) draw_text_center(VW/2, VH-24, "TAP - PAGE 2: THE MALL + MORE", 1, COL(160,230,255));
    else { char b[48]; char *q=b; q=sappend(q, key_name(G.bind[ACT_INTERACT])); q=sappend(q, " - PAGE 2: THE MALL + MORE");
      draw_text_center(VW/2, VH-24, b, 1, COL(160,230,255)); }
}
// ---------------------------------------------------------------------------
// Settings: rebind every action. Bindings live in G.bind, are derived into
// semantic input centrally (derive_input), and persist in the save file.
// ---------------------------------------------------------------------------
#define SET_ROWS 15   // 14 actions + RESET DEFAULTS
static void settings_input(void){
    if (G.set_capture){
        // the next physical key pressed becomes the binding (ESC cancels)
        if (g_raw_edges){
            if (raw_edge(PK_ESC)) G.set_capture=0;
            else for (int pk=0;pk<PK_COUNT;pk++) if ((g_raw_edges>>pk)&1){
                G.bind[G.set_sel]=pk; G.set_capture=0; do_save(); break;
            }
        }
        return;
    }
    if (IN.menu_up) G.set_sel=(G.set_sel+SET_ROWS-1)%SET_ROWS;
    if (IN.menu_down) G.set_sel=(G.set_sel+1)%SET_ROWS;
    if (IN.tapped){
        for (int i=0;i<SET_ROWS;i++){ int ry=70+i*16;
            if (IN.tap_y>=ry-3 && IN.tap_y<=ry+13){
                if (i==G.set_sel && i<14) G.set_capture=1; else G.set_sel=i;
                return;
            }
        }
    }
    if (IN.esc_pressed || IN.interact_pressed){ G.state=GS_MENU; return; }
    if (IN.select_pressed){
        if (G.set_sel<14) G.set_capture=1;
        else { bind_defaults(G.bind); do_save(); }
    }
}
static void render_settings(void){
    rect_fill(0,0,VW,VH,COL(24,22,38));
    draw_text_center(VW/2, 14, "SETTINGS", 3, COL(255,214,64));
    draw_text_center(VW/2, 44, "KEY BINDINGS - ARROWS CHOOSE, SPACE REBINDS", 1, COL(160,230,255));
    for (int i=0;i<SET_ROWS;i++){
        int ry=70+i*16;
        int sel = (i==G.set_sel);
        if (sel) rect_fill(120, ry-3, 400, 15, COL(255,205,40));
        u32 tc = sel? COL(30,26,44) : COL(235,238,245);
        if (i<14){
            draw_text(150, ry, act_name(i), 1, tc);
            const char *kn = (sel && G.set_capture)? "PRESS A KEY..." : key_name(G.bind[i]);
            draw_text(490-text_w(kn,1), ry, kn, 1, tc);
        } else {
            draw_text(150, ry, "RESET DEFAULTS", 1, tc);
            draw_text(490-text_w("WASD + E F P",1), ry, "WASD + E F P", 1, tc);
        }
    }
    if (G.set_capture)
        draw_text_center(VW/2, 330, "PRESS ANY KEY FOR THE HIGHLIGHTED ACTION - ESC CANCELS", 1, COL(255,240,150));
    else
        draw_text_center(VW/2, 330, "GAMEPAD: STICK/DPAD MOVE - A FIRE - B INTERACT - X WALL - START PAUSE", 1, COL(140,150,170));
    { char b[80]; char *q=b; q=sappend(q, key_name(G.bind[ACT_INTERACT]));
      q=sappend(q, " - BACK TO MENU (ARROW KEYS ALWAYS WORK FOR MOVING)");
      draw_text_center(VW/2, 344, b, 1, COL(140,150,170)); }
}

static void pause_save_quit(void){
    do_save();                       // GS_PAUSE counts as in-run: writes the slot
    if (G.net_mode==NET_HOST) plat_net_stop();
    G.net_mode=NET_OFF;
    for (int i=0;i<MAXP;i++){ G.p[i].connected=0; G.p[i].is_local=0; }
    G.my_id=0; G.menu_sel=0;
    G.state=GS_MENU;
    sfx(6);
}
// ---------------------------------------------------------------------------
// THE CONTROLS PANEL (Daniel, Nov 6): the bottom-of-screen control lines
// are gone — they overlapped everything. Controls now live in menus:
// the pause menu's CONTROLS row in the main game, and the Start-menu
// inside Banzai World. One panel, drawn per place-you-are-standing
// (ctx), and the buttons named follow the device in your hands.
// ctx: 0 meadow+mall, 1 story/free, 2 entrances, 3 run, 4 craft, 5 bowl
// ---------------------------------------------------------------------------
static void draw_controls_panel(int vx, int vy, int vw2, int vh2, int ctx){
    static const char *CTX_NAME[9] = { "THE MEADOW + THE MALL", "STORY + FREE PLAY", "ENTRANCES", "SUPER BUDDY RUN", "BUDDY CRAFT", "BUG BOWL", "TANK RESCUE", "SUPER BUDDY CART", "WAR!" };
    if (ctx<0 || ctx>8) ctx = 0;
    const char *act[10]; const char *btn[10]; int n=0;
    static char mv[40], spr[8], pau[8], btnstore[10][24];
    if (pad_mode()) scpy(mv, "STICK / DPAD", 40);
    else if (mobile_ui()) scpy(mv, G.touch_ctl? "LEFT STICK" : "DRAG", 40);
    else move_key_str(mv);
    scpy(spr, pad_mode()? "LT" : key_name(G.bind[ACT_SPRINT]), 8);
    scpy(pau, pad_mode()? "START" : key_name(G.bind[ACT_PAUSE]), 8);
    const char *back = pad_mode()? "START" : (mobile_ui()? "THE MALL DOOR" : "ESC");
    // ROW copies the button label NOW: key_name() returns a shared
    // static buffer, so storing its pointer across calls would leave
    // every row showing the LAST key looked up (INTERACT read as F).
#define CTL_ROW(a, b) do{ act[n]=(a); scpy(btnstore[n], (b), 24); btn[n]=btnstore[n]; n++; }while(0)
    switch (ctx){
      case 0:
        CTL_ROW("MOVE", mv);
        CTL_ROW("FIRE", lbl_fire());
        CTL_ROW("INTERACT", lbl_int());
        CTL_ROW("THROW A WALL", lbl_wall());
        CTL_ROW("WEAPONS", pad_mode()? "LB / RB" : (mobile_ui()? "TAP THE CHIPS" : "1 - 5"));
        CTL_ROW("BACKFIRE BEAN", lbl_bean());
        if (!mobile_ui()) CTL_ROW("SPRINT", spr);
        if (!mobile_ui()) CTL_ROW("PAUSE MENU", pau);
        break;
      case 5:
        CTL_ROW("POSITION", mv);
        CTL_ROW("LOCK AIM, SET POWER, ROLL", lbl_fire());
        CTL_ROW("LEAVE THE LANE", lbl_int());
        break;
      case 1:
        CTL_ROW("MOVE", mv);
        CTL_ROW("CHOOSE / ATTACK", lbl_fire());
        CTL_ROW("TALK / USE", lbl_int());
        CTL_ROW("LOG + PACK", lbl_wall());
        CTL_ROW("LEAVE", back);
        break;
      case 2:
        CTL_ROW("MOVE", mv);
        CTL_ROW("THROW A STICK", lbl_fire());
        CTL_ROW("OPEN / UNLOCK", lbl_int());
        CTL_ROW("USE A PATCH NOTE", lbl_wall());
        CTL_ROW("LEAVE", back);
        break;
      case 3:
        CTL_ROW("RUN", mv);
        CTL_ROW("JUMP", lbl_fire());
        if (!mobile_ui()) CTL_ROW("STOMP (IN THE AIR)", "DOWN");
        CTL_ROW("LEAVE", back);
        break;
      case 6:
        CTL_ROW("DRIVE", mv);
        CTL_ROW("FIRE THE CANNON", lbl_fire());
        CTL_ROW("LEAVE", back);
        break;
      case 7:
        CTL_ROW("STEER", mv);
        CTL_ROW("GO (HOLD IT!)", lbl_fire());
        CTL_ROW("LEAVE", back);
        break;
      case 8:
        CTL_ROW("SELECT / ORDER", mobile_ui()? "TAP" : "CLICK / TAP");
        CTL_ROW("SELECT ALL OF A TYPE", "DOUBLE-TAP A UNIT");
        CTL_ROW("BUILD / TRAIN", "TAP THE BOTTOM BAR");
        CTL_ROW("PAN THE CAMERA", "DRAG / ARROW KEYS");
        CTL_ROW("LEAVE", back);
        break;
      default:
        CTL_ROW("MOVE", mv);
        CTL_ROW("SWING", lbl_fire());
        CTL_ROW("GATHER / REST", lbl_int());
        CTL_ROW("CRAFTING MENU", lbl_wall());
        CTL_ROW("IN THE MENU: PICK", mv);
        CTL_ROW("IN THE MENU: CRAFT", lbl_fire());
        CTL_ROW("LEAVE", back);
        break;
    }
#undef CTL_ROW
    int w = vw2<460? vw2-12 : 448; if (w<280) w=vw2-8;
    int h = 58 + n*19 + 24;
    int x = vx+(vw2-w)/2, y = vy+(vh2-h)/2; if (y<vy+2) y=vy+2;
    blend_rect(vx, vy, vw2, vh2, COL(8,8,14), 120);
    rect_fill(x, y, w, h, COL(16,18,28)); rect_line(x, y, w, h, COL(198,164,84));
    draw_text_center(x+w/2, y+8, "CONTROLS", 2, COL(255,214,64));
    draw_text_center(x+w/2, y+27, CTX_NAME[ctx], 1, COL(150,170,190));
    for (int i=0;i<n;i++){
        int ry = y+44+i*19;
        draw_text(x+16, ry, act[i], 1, COL(228,232,238));
        draw_text(x+w-16-text_w(btn[i],1), ry, btn[i], 1, COL(255,220,90));
    }
    draw_text_center(x+w/2, y+h-15, pad_mode()? "B / START - BACK" : (mobile_ui()? "TAP ANYWHERE - BACK" : "ESC / SPACE - BACK"), 1, COL(140,150,170));
}

static void render_pause(void){
    render_world();
    blend_rect(0, 0, VW, VH, COL(10,8,20), 150);
    if (G.pause_ctl){
        Player *me=&G.p[G.my_id];
        draw_controls_panel(0, 0, VW, VH, (me->in_mall && me->station==4)? 5 : 0);
        return;
    }
    draw_text_center(VW/2+3, 104+3, "PAUSED", 4, COL(10,8,20));
    draw_text_center(VW/2, 104, "PAUSED", 4, COL(255,214,64));
    { static const char *rows[3] = { "CONTINUE", "CONTROLS", "SAVE + QUIT TO TITLE" };
      for (int i=0;i<3;i++){
          int y=176+i*26;
          if (i==G.pause_sel){ rect_fill(0, y-4, VW, 19, COL(255,205,40)); draw_text_center(VW/2, y, rows[i], 2, COL(30,26,44)); }
          else draw_text_center(VW/2, y, rows[i], 2, COL(235,238,245));
      } }
    { char b[56]; char *q=b; q=sappend(q, "SAVES TO SLOT "); q=sapp_int(q, G.slot);
      q=sappend(q, " - YOUR HAT IS SAFE HERE.");
      draw_text_center(VW/2, 264, b, 1, COL(150,160,185)); }
    draw_text_center(VW/2, 282, pad_mode()? "STICK/DPAD + A - START ALSO RESUMES" : (mobile_ui()? "TAP A ROW - TAP SAVE + QUIT TWICE" : "ARROWS + SPACE - ESC RESUMES"), 1, COL(140,150,170));
}
static float over_stack[MAXSTACK];
static int over_stack_n=0;
static float over_carried=0;
static int over_have=0;
static void render_over(void){
    render_world();
    ui_panel(120, 60, 400, 220);   // v1.22: shared panel chrome under the red border
    rect_line(120, 60, 400, 220, COL(255,90,90));
    // you, as you fell — wearing the hat you died in (Daniel). Drawn
    // before the text so a tall hat never hides a word.
    if (over_have){
        static Player hero;
        Player *me=&G.p[G.my_id>=0? G.my_id : 0];
        smemset(&hero, 0, sizeof(hero));
        hero.connected=1; hero.alive=1; hero.name[0]=0;
        hero.color=me->color; hero.weapon=0; hero.owned[0]=1;
        hero.hp=hero.maxhp=100; hero.aim=-0.6f; hero.walk_ph=0;
        hero.whale_on=me->whale_on; hero.whale_own=me->whale_own;
        hero.stack_n=over_stack_n;
        for (int i=0;i<over_stack_n;i++) hero.stack[i]=over_stack[i];
        hero.carried=over_carried;
        // stand him at screen (168, 252) whatever the camera is doing:
        // invert SX/SY at zoom 1 so the portrait is always full size
        float pz=G.zoom; G.zoom=1.0f;
        int vw2 = G.view_w>0? G.view_w : VW, vh2 = G.view_h>0? G.view_h : VH;
        hero.x = G.camx + (float)(168 - G.view_x0 - vw2/2);
        hero.y = G.camy + (float)(252 - G.view_y0 - vh2/2);
        draw_player_sprite(&hero);
        G.zoom=pz;
    }
    draw_text_center(VW/2, 78, "GAME OVER", 3, COL(255,110,110));
    draw_text_center(VW/2, 106, "THE BUGS GOT THE WHOLE TEAM", 1, COL(230,235,245));
    { char b[64]; int t=(int)G.time; char *q=b;
      q=sappend(q, "SURVIVED "); q=sapp_int(q, t/60); q=sappend(q, ":");
      if (t%60<10) q=sappend(q,"0"); q=sapp_int(q, t%60);
      q=sappend(q, " - SQUASHED "); q=sapp_int(q, G.total_kills);
      q=sappend(q, " - CASHED "); q=sapp_int(q, (int)G.total_earned); q=sappend(q, " KG");
      draw_text_fit(238, 128, 272, b, 1, COL(255,240,180)); }
    int y=152;
    for (int i=0;i<MAXP;i++) if (G.p[i].connected){
        char b[64]; char *q=b;
        q=sappend(q, G.p[i].name); q=sappend(q, " - HEAVIEST HAT ");
        q=sapp_int(q, (int)G.p[i].heaviest); q=sappend(q, " KG - ");
        q=sapp_int(q, G.p[i].kills); q=sappend(q, " KILLS");
        draw_text_fit(238, y, 272, b, 1, COL(210,220,235)); y+=14;
    }
    { char b[64]; char *q=b;
      q=sappend(q, "ALL-TIME BEST HAT: "); q=sappend(q, G.rec_name); q=sappend(q, " ");
      q=sapp_int(q, (int)G.rec_hat); q=sappend(q, " KG");
      draw_text_fit(238, y+8, 272, b, 1, COL(150,200,255)); }
    if (mobile_ui()) draw_text_center(VW/2, 258, "TAP - BACK TO MENU", 1, COL(255,240,150));
    else { char b[48]; char *q=b; q=sappend(q, key_name(G.bind[ACT_FIRE])); q=sappend(q, " - BACK TO MENU");
      draw_text_center(VW/2, 258, b, 1, COL(255,240,150)); }
#ifdef PLAT_WASM
    // (the share buttons under the canvas speak for themselves —
    // no standing line pointing at them; Daniel, Nov 6)
#endif
}

// ---------------------------------------------------------------------------
// Frame — input resolution, state machine, camera. Platform entry point.
// ---------------------------------------------------------------------------
static void resolve_local_input(void){
    Player *me=&G.p[G.my_id];
    if (!me) return;
    me->cur_keys = IN.keys | IN.touch_keys;   // keyboard/pad bits + d-pad bits
    // facing follows the movement keys; standing still keeps the last facing.
    // (On a client this also aims our sprite between host snapshots.)
    float mx=0, my=0;
    if (IN.keys & K_UP) my-=1;
    if (IN.keys & K_DOWN) my+=1;
    if (IN.keys & K_LEFT) mx-=1;
    if (IN.keys & K_RIGHT) mx+=1;
    if (mx!=0 || my!=0) me->aim = fatan2(my, mx);
    client_aim = me->aim;
    // touch (mobile web): hold/drag to run toward the finger, firing as
    // you go. Finger right on top of the player = stand and shoot the
    // way you face.
    if (IN.touch_down){
        float psx = (me->x - G.camx) + VW/2, psy = (me->y - G.camy) + VH/2;
        float dx = IN.touch_x - psx, dy = IN.touch_y - psy;
        float d2 = dx*dx + dy*dy;
        me->cur_keys |= K_FIRE;
        if (d2 > 20*20){
            float d = fsqrt(d2), ax = dx/d, ay = dy/d;
            me->aim = fatan2(dy, dx); client_aim = me->aim;
            if (ax > 0.38f) me->cur_keys |= K_RIGHT; else if (ax < -0.38f) me->cur_keys |= K_LEFT;
            if (ay > 0.38f) me->cur_keys |= K_DOWN;  else if (ay < -0.38f) me->cur_keys |= K_UP;
        }
    }
    if (IN.weapon_key>=1 && IN.weapon_key<=5){
        int w=IN.weapon_key-1;
        if (me->owned[w]){ me->weapon=w; client_weapon=w; sfx(6); }
        else sfx(16);
    }
    if (IN.bean_key){ IN.bean_key=0; bean_use(G.my_id); }
    if (IN.weapon_cycle!=0){          // gamepad bumpers: next/prev owned weapon
        int w=me->weapon, tries=0;
        do { w=(w+IN.weapon_cycle+NWEAP)%NWEAP; tries++; } while (!me->owned[w] && tries<NWEAP);
        if (me->owned[w] && w!=me->weapon){ me->weapon=w; client_weapon=w; sfx(6); }
    }
    if (IN.wall_pressed){
        if (G.net_mode==NET_CLIENT) client_want_wall=1;
        else me->want_wall=1;
    }
}
// ---------------------------------------------------------------------------
// Local co-op: player 2 on the second controller, same machine, same run.
// P1 keeps the keyboard + first pad (resolve_local_input); P2 is fed
// from the pad-2 channel. The pair shares one camera while they stay
// close and splits the screen when they separate — and whenever either
// of them is in the store, so the shopper gets the Bug Bounty (shield
// and all) while the other keeps playing.
// ---------------------------------------------------------------------------
static float vcamx[2], vcamy[2], vzoom[2];
static int vsplit=0; static float vfit_t=0;
// the split is a place, not a switch: vsplit_f eases 0 (one shared
// view) toward 1 (two EVEN half views) — the seam sweeps in as the
// players separate and back out as they return, but it always comes
// to rest exactly in the middle: halves are never lopsided at rest
// (the kids were right — an uneven split feels broken). vorient
// picks the seam's direction from the direction of apartness
// (1 = side by side, 0 = stacked), vp1first remembers which side
// P1's view grew from.
static float vsplit_f=0; static int vorient=1; static int vp1first=1;
static int p2_rep_dir=0; static float p2_rep_t=0;
static int act2_edge(int a){ return (int)((IN.act2_edges>>a)&1); }
static int act2_down(int a){ return (int)((IN.act2_held>>a)&1); }
static void resolve_p2_input(void){
    Player *p=&G.p[G.p2_slot];
    u32 h=IN.act2_held; int k=0;
    if ((h>>ACT_UP)&1) k|=K_UP;
    if ((h>>ACT_DOWN)&1) k|=K_DOWN;
    if ((h>>ACT_LEFT)&1) k|=K_LEFT;
    if ((h>>ACT_RIGHT)&1) k|=K_RIGHT;
    if ((h>>ACT_FIRE)&1) k|=K_FIRE;
    if ((h>>ACT_SPRINT)&1) k|=K_SPRINT;
    p->cur_keys = k;
    float mx=0, my=0;
    if (k & K_UP) my-=1;
    if (k & K_DOWN) my+=1;
    if (k & K_LEFT) mx-=1;
    if (k & K_RIGHT) mx+=1;
    if (mx!=0 || my!=0) p->aim = fatan2(my, mx);
    int cyc = act2_edge(ACT_WNEXT)? 1 : act2_edge(ACT_WPREV)? -1 : 0;
    if (cyc){
        int w=p->weapon, tries=0;
        do { w=(w+cyc+NWEAP)%NWEAP; tries++; } while (!p->owned[w] && tries<NWEAP);
        if (p->owned[w] && w!=p->weapon){ p->weapon=w; sfx(6); }
    }
    if (act2_edge(ACT_WALL)) p->want_wall=1;
}
// P2's store cursor: edges plus the same hold-to-repeat feel as P1's menus
static void p2_store_nav(int *up, int *down, float dt){
    *up=0; *down=0;
    if (act2_edge(ACT_UP)){ *up=1; p2_rep_dir=-1; p2_rep_t=0.38f; }
    else if (act2_edge(ACT_DOWN)){ *down=1; p2_rep_dir=1; p2_rep_t=0.38f; }
    else if (p2_rep_dir!=0 && (act2_down(ACT_UP) || act2_down(ACT_DOWN))){
        p2_rep_t -= dt;
        if (p2_rep_t<=0){
            if (p2_rep_dir<0 && act2_down(ACT_UP)) *up=1;
            if (p2_rep_dir>0 && act2_down(ACT_DOWN)) *down=1;
            p2_rep_t=0.09f;
        }
    } else p2_rep_dir=0;
}
static float zoom_target_of(Player *p){
    if (p->in_mall) return 1.0f;
    float sn=(float)p->stack_n;
    float z = 1.0f - (sn*0.0016f>0.38f? 0.38f : sn*0.0016f);
    // as THREAT climbs the bugs swell without limit — the camera gives
    // ground in steps at set threat levels so the giants stay in frame
    float th = G.threat;
    z -= th>=8.0f? 0.16f : th>=5.0f? 0.12f : th>=3.0f? 0.08f : th>=2.0f? 0.04f : 0.0f;
    return z<0.45f? 0.45f : z;
}
// a simple strip of what you've got: one icon slot per weapon, in
// store order, gold frame on the one in your hands, Xn for stacks.
// P2's HUD is text-only otherwise — this is how they see their kit.
static void draw_weapon_icons(Player *p, int x, int y){
    for (int w=0; w<NWEAP; w++){
        if (!p->owned[w]) continue;
        int sx = x + w*26;
        rect_fill(sx, y, 22, 22, COL(16,14,28));
        rect_line(sx, y, 22, 22, p->weapon==w? COL(255,205,40) : COL(104,98,138));
        draw_item_icon(w, sx+11, y+11);
        if (p->owned[w]>1){ char b[8]; char *q=b; *q++='X'; q=sapp_int(q, p->owned[w]); *q=0;
            draw_text(sx+23-text_w(b,1), y+13, b, 1, COL(255,240,200)); }
    }
}
static void render_hud_compact(Player *p, int vx, int vy, int vw2, int vh2){
    // the split view's status card: what render_hud shows, in a card
    // that hugs the view's top-left corner — plus one context line,
    // what THIS player needs right now: safety in the mall, the threat
    // level out in the field.
    ui_panel(vx+6, vy+6, 236, 96);
    draw_text_fit(vx+12, vy+9, 218, p->name, 1, COL(255,255,255));
    draw_bar(vx+12, vy+20, 130, 7, p->maxhp? (float)p->hp/p->maxhp : 0, COL(120,230,90));
    { char b[24]; fmt_int(b, "HP ", p->hp, ""); draw_text(vx+148, vy+19, b, 1, COL(255,255,255)); }
    if (p->shield_max>0) draw_bar(vx+12, vy+29, 130, 4, p->shield_max? (float)p->shield/p->shield_max:0, COL(110,190,255));
    { char b[40]; char *q=b; q=sappend(q, WEAPON_NAME[p->weapon]);
      if (p->owned[p->weapon]>1){ q=sappend(q, " X"); q=sapp_int(q, p->owned[p->weapon]); }
      *q=0;
      draw_text_fit(vx+12, vy+36, 218, b, 1, COL(255,220,120)); }
    { char b[48]; char *q=b; q=sappend(q, "HAT "); q=sapp_int(q, (int)p->carried); q=sappend(q, " KG - ");
      q=sappend(q, hat_tier_name(p->stack_n, p->carried)); *q=0;
      draw_text_fit(vx+12, vy+47, 218, b, 1, COL(255,240,180)); }
    draw_weapon_icons(p, vx+12, vy+62);
    if (p->in_arcade) draw_text(vx+12, vy+82, "IN THE ARCADE - SAFE", 1, COL(150,230,255));
    else if (p->in_mall) draw_text(vx+12, vy+82, "IN THE MALL - SAFE", 1, COL(150,230,150));
    else { char b[32]; char *q=b; q=sappend(q, "THREAT X"); q=sapp_int(q, (int)G.threat);
           *q++='.'; *q++=(char)('0'+((int)(G.threat*10)%10)); *q=0;
           draw_text(vx+12, vy+82, b, 1, COL(255,150,90)); }
    if (!p->alive) draw_text_center(vx+vw2/2, vy+vh2/2-8, "DOWN", 2, COL(255,90,80));
    else { char pb[64]; const char *pr = interact_prompt(p, pb, (G.p2_slot>=0 && p==&G.p[G.p2_slot])? 1:0);
           if (pr) draw_text_center_fit(vx+vw2/2, vy+vh2-22, vw2-16, pr, 1, COL(255,240,180)); }
}
static void render_hud_mini(Player *p, int vx, int vy, int vw2){
    // a name-and-health strip for a view that is still peeling open:
    // the split is gradual now, and a sliver of screen can't hold the
    // full card yet
    int w = vw2-12; if (w>200) w=200; if (w<40) return;
    ui_panel(vx+6, vy+6, w, 28);
    draw_text_fit(vx+11, vy+9, w-10, p->name, 1, COL(255,255,255));
    draw_bar(vx+11, vy+21, w-22>10? w-22:10, 4, p->maxhp? (float)p->hp/p->maxhp : 0, COL(120,230,90));
}
static void render_p2_chip(void){
    Player *p=&G.p[G.p2_slot];
    ui_panel(VW-208, VH-74, 200, 66);
    draw_text_fit(VW-202, VH-68, 126, p->name, 1, COL(255,255,255));
    if (!p->alive) draw_text(VW-70, VH-68, "DOWN", 1, COL(255,90,80));
    draw_bar(VW-202, VH-57, 104, 6, p->maxhp? (float)p->hp/p->maxhp : 0, COL(120,230,90));
    { char b[24]; char *q=b; q=sappend(q, "HAT "); q=sapp_int(q, (int)p->carried); q=sappend(q, "KG"); *q=0;
      draw_text(VW-92, VH-58, b, 1, COL(255,240,180)); }
    draw_weapon_icons(p, VW-202, VH-46);
}
static void coop_render(float dt){
    Player *me=&G.p[G.my_id], *p2=&G.p[G.p2_slot];
    Player *tp[2]; tp[0]=me; tp[1]=p2;
    // one smoothed camera per player; a downed player's view follows
    // their partner so they can watch the rescue (or the end)
    for (int v=0; v<2; v++){
        Player *t = tp[v]->alive? tp[v] : tp[v^1];
        Buddy *tb = &G.bud[v? G.p2_slot : G.my_id];   // a BW view follows the buddy, not the DEV
        float txv = tb->in_bw? tb->x : t->x, tyv = tb->in_bw? tb->y : t->y;
        if (vzoom[v]<0.01f){ vcamx[v]=txv; vcamy[v]=tyv; vzoom[v]=1.0f; }
        float f = dt*6>1? 1 : dt*6;
        vcamx[v] += (txv - vcamx[v])*f;
        vcamy[v] += (tyv - vcamy[v])*f;
        vzoom[v] += ((tb->in_bw? 1.0f : zoom_target_of(t)) - vzoom[v])*0.045f;
    }
    // --- should the screen be split, and which way? -------------------
    // A panel open, or the players in different zones (one in the mall),
    // forces a full side-by-side split. Otherwise the distance decides:
    // they share a view while they fit in one, and the split's goal is
    // proportional to how far past fitting they have strayed.
    int forced = (lstore[0] || lstore[1] || (me->in_mall != p2->in_mall) || G.bud[G.my_id].in_bw || G.bud[G.p2_slot].in_bw);
    float z = vzoom[0]<vzoom[1]? vzoom[0] : vzoom[1];
    float dx = me->x - p2->x; if (dx<0) dx=-dx;
    float dy = me->y - p2->y; if (dy<0) dy=-dy;
    float fitx = (VW-110)/z, fity = (VH-110)/z;
    if (forced) { vsplit=1; vfit_t=0; }
    else if (dx <= fitx && dy <= fity){ vfit_t += dt; if (vfit_t>0.6f) vsplit=0; }
    else { vfit_t=0; vsplit=1; }
    // a death ends the split on the spot (Daniel): no easing-out —
    // one shared view of the rescue, right now
    if (!me->alive || !p2->alive){ vsplit=0; vsplit_f=0; }
    float goal = 0;
    if (vsplit){
        if (forced){ vorient=1; }
        else {
            int want_vert = dx >= dy;
            if (want_vert != vorient){
                // hysteresis: the other axis must clearly dominate
                // before the seam turns, so it never flip-flops
                if ((want_vert && dx > dy*1.35f) || (!want_vert && dy > dx*1.35f))
                    vorient = want_vert;
            }
        }
        goal = 1;   // always the full, even split — see the note above
        if (vsplit_f < 0.05f)   // a split is being born: remember the sides
            vp1first = vorient? (me->x <= p2->x) : (me->y <= p2->y);
    }
    // ease the amount toward its goal — quicker to open than to close
    if (vsplit_f < goal){ vsplit_f += dt*2.4f; if (vsplit_f>goal) vsplit_f=goal; }
    else if (vsplit_f > goal){ vsplit_f -= dt*1.6f; if (vsplit_f<goal) vsplit_f=goal; }

    if (vsplit_f <= 0.02f && !vsplit){
        // close together: one shared view centered between them
        clip_x0=0; clip_x1=VW; clip_y0=0; clip_y1=VH;
        G.view_x0=0; G.view_w=VW; G.view_y0=0; G.view_h=VH;
        G.camx=(vcamx[0]+vcamx[1])*0.5f; G.camy=(vcamy[0]+vcamy[1])*0.5f;
        G.zoom = vzoom[0]<vzoom[1]? vzoom[0] : vzoom[1];
        G.view_pi=G.my_id; G.cam_lock=1;
        render_world();
        G.cam_lock=0;
        render_hud();
        render_p2_chip();
        return;
    }
    // apart (or one is shopping): a view each. The seam sweeps in from
    // the edge the second view grows out of, and settles at the middle
    // only when the split is fully open.
    int rx[2], ry[2], rw[2], rh[2], seam;
    if (vorient){
        seam = vp1first? VW - (int)((VW/2)*vsplit_f) : (int)((VW/2)*vsplit_f);
        if (vp1first){ rx[0]=0; rw[0]=seam; rx[1]=seam; rw[1]=VW-seam; }
        else { rx[1]=0; rw[1]=seam; rx[0]=seam; rw[0]=VW-seam; }
        ry[0]=ry[1]=0; rh[0]=rh[1]=VH;
    } else {
        seam = vp1first? VH - (int)((VH/2)*vsplit_f) : (int)((VH/2)*vsplit_f);
        if (vp1first){ ry[0]=0; rh[0]=seam; ry[1]=seam; rh[1]=VH-seam; }
        else { ry[1]=0; rh[1]=seam; ry[0]=seam; rh[0]=VH-seam; }
        rx[0]=rx[1]=0; rw[0]=rw[1]=VW;
    }
    for (int v=0; v<2; v++){
        if (rw[v]<4 || rh[v]<4) continue;   // a hairline view draws nothing yet
        clip_x0=rx[v]; clip_x1=rx[v]+rw[v]; clip_y0=ry[v]; clip_y1=ry[v]+rh[v];
        G.view_x0=rx[v]; G.view_w=rw[v]; G.view_y0=ry[v]; G.view_h=rh[v];
        G.camx=vcamx[v]; G.camy=vcamy[v]; G.zoom=vzoom[v];
        G.view_pi = v? G.p2_slot : G.my_id; G.cam_lock=1;
        int bwv = G.bud[G.view_pi].in_bw;
        if (bwv) bw_render(G.view_pi); else render_world();
        G.cam_lock=0;
        if (bwv){ /* Banzai World draws its own card and hints */ }
        else if (lstore[v] && rw[v]>=316){
            G.store_pi = v? G.p2_slot : G.my_id;
            G.store_station = lstation[v];
            G.ui_compact=1;
            draw_active_panel();
            G.ui_compact=0;
        } else if (rw[v]>=280 && rh[v]>=120) render_hud_compact(tp[v], rx[v], ry[v], rw[v], rh[v]);
        else render_hud_mini(tp[v], rx[v], ry[v], rw[v]);
        if (bwgate[G.view_pi]) bw_gate_draw(G.view_pi, rx[v], ry[v], rw[v], rh[v]);
        if (arcgate[G.view_pi]) arc_gate_draw(G.view_pi, rx[v], ry[v], rw[v], rh[v]);
        // every split view gets its own way-back arrow (hidden near home)
        if (!bwv){ Player *t = tp[v]->alive? tp[v] : tp[v^1];
          draw_home_arrow(t, rx[v] + rw[v]/2, ry[v] + rh[v]/2); }
    }
    clip_x0=0; clip_x1=VW; clip_y0=0; clip_y1=VH;
    G.view_x0=0; G.view_w=VW; G.view_y0=0; G.view_h=VH;
    if (vorient) rect_fill(seam-1, 0, 2, VH, COL(10,9,18));   // the seam
    else rect_fill(0, seam-1, VW, 2, COL(10,9,18));
}
static void coop_frame(float dt){
    Player *me=&G.p[G.my_id];
    Player *p2=&G.p[G.p2_slot];
    // who was in Banzai World when this frame BEGAN: a Start/Esc that
    // leaves BW mid-frame must not also read as a pause press below
    int p1bw0 = G.bud[G.my_id].in_bw, p2bw0 = G.bud[G.p2_slot].in_bw;
    if (G.net_mode==NET_HOST)   // LAN guests still take their net input
        for (int i=1;i<MAXP;i++) if (G.p[i].connected && i!=G.p2_slot) G.p[i].cur_keys = G.p[i].net_keys;
    int p2up=0, p2down=0; p2_store_nav(&p2up, &p2down, dt);
    // P1 at the gate panel: it owns P1's menu input while open
    if (bwgate[G.my_id]){
        me->cur_keys=0;
        if (IN.menu_up){ bwgate_sel[G.my_id] = (bwgate_sel[G.my_id]+2)%3; sfx(6); }
        if (IN.menu_down){ bwgate_sel[G.my_id] = (bwgate_sel[G.my_id]+1)%3; sfx(6); }
        if (IN.tapped){ int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2); bw_gate_tap(G.my_id, vx, vy, vw2, vh2); }
        else if (IN.select_pressed || IN.enter_pressed) bw_gate_pick(G.my_id);
        else if (IN.esc_pressed || IN.pause_pressed){ bwgate[G.my_id]=0; sfx(6); }
    }
    // P1 at a cabinet panel: same deal, two rows
    if (arcgate[G.my_id]){
        me->cur_keys=0;
        if (IN.menu_up || IN.menu_down){ arcgate_sel[G.my_id] = (arcgate_sel[G.my_id]+1)%2; sfx(6); }
        if (IN.tapped){ int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2); arc_gate_tap(G.my_id, vx, vy, vw2, vh2); }
        else if (IN.select_pressed || IN.enter_pressed) arc_gate_pick(G.my_id);
        else if (IN.esc_pressed || IN.pause_pressed){ arcgate[G.my_id]=0; sfx(6); }
    }
    // P1 in Banzai World: P1's input drives the buddy in its own world
    else if (G.bud[G.my_id].in_bw){
        me->cur_keys=0;
        if (IN.tapped){ int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2); bw_battle_tap(G.my_id, vx, vy, vw2, vh2); }
        BwIn bin; bw_fill_p1(&bin, dt);
        bw_update(G.my_id, dt, &bin);
    }
    // P1's store (keyboard / pad 1)
    else if (bwgate[G.my_id]){ me->cur_keys=0; }
    else if (lstore[0]){
        me->cur_keys=0;
        station_input(G.my_id, lstation[0], IN.menu_up, IN.menu_down, IN.select_pressed);
        if (IN.esc_pressed || IN.interact_pressed || G.tap_close_store){
            // the till is counting YOUR carcasses: the panel stays
            // open until it finishes (Daniel, Nov 6)
            if (pay_active && pay_player==G.my_id){ store_note("THE TILL IS COUNTING - ONE MOMENT"); sfx(16); G.tap_close_store=0; }
            else { lstore[0]=0; me->in_store=0; me->station=-1; G.tap_close_store=0; bud_stage[G.my_id]=0; }
        }
    } else if (IN.interact_pressed && me->alive && downed_near(me)){
        try_revive(G.my_id);
    } else if ((IN.interact_pressed || IN.enter_pressed) && me->alive){
        if (!me->in_mall && near_store(me)){ mall_enter(G.my_id); }
        else if (me->in_mall){
            int st=station_at(me);
            if (st>=0){ lstore[0]=1; lstation[0]=st; G.store_sel=0; me->station=st; me->in_store=1; me->cur_keys=0; sfx(19); }
            else if (arcade_door_at(me)){ arcade_enter(G.my_id); }
            else if (bw_door_at(me)){ bw_gate_try(G.my_id); }
            else if (arcade_exit_at(me)){ arcade_exit(G.my_id); }
            else if (cabinet_at(me)>=0){ arc_cab[G.my_id]=cabinet_at(me); arc_gate_try(G.my_id); }
            else if (on_exit_mat(me)){ mall_exit(G.my_id); }
        }
    }
    // P2 at the gate panel
    if (bwgate[G.p2_slot]){
        p2->cur_keys=0;
        if (p2up){ bwgate_sel[G.p2_slot] = (bwgate_sel[G.p2_slot]+2)%3; sfx(6); }
        if (p2down){ bwgate_sel[G.p2_slot] = (bwgate_sel[G.p2_slot]+1)%3; sfx(6); }
        if (act2_edge(ACT_FIRE)) bw_gate_pick(G.p2_slot);
        else if (act2_edge(ACT_INTERACT) || act2_edge(ACT_ESC)){ bwgate[G.p2_slot]=0; sfx(6); }
    }
    // P2 at a cabinet panel
    if (arcgate[G.p2_slot]){
        p2->cur_keys=0;
        if (p2up || p2down){ arcgate_sel[G.p2_slot] = (arcgate_sel[G.p2_slot]+1)%2; sfx(6); }
        if (act2_edge(ACT_FIRE)) arc_gate_pick(G.p2_slot);
        else if (act2_edge(ACT_INTERACT) || act2_edge(ACT_ESC)){ arcgate[G.p2_slot]=0; sfx(6); }
    }
    // P2 in Banzai World: pad 2 drives P2's buddy in its own half
    else if (G.bud[G.p2_slot].in_bw){
        resolve_p2_input();
        BwIn bin; bin.keys = p2->cur_keys;
        bin.interact = act2_edge(ACT_INTERACT);
        bin.exit_ = act2_edge(ACT_ESC);
        bin.pause = act2_edge(ACT_PAUSE);
        bin.sel = act2_edge(ACT_FIRE);
        bin.up = p2up; bin.down = p2down;
        bin.wall = act2_edge(ACT_WALL);
        bin.sel_held = ((IN.act2_held>>ACT_FIRE)&1) || ((p2->cur_keys & K_UP) && G.bud[G.p2_slot].bw_mode!=5);   // mode 5: steering never fires (see bw_fill_p1)
        bw_update(G.p2_slot, dt, &bin);
        p2->cur_keys=0;   // P2's DEV stands at the gate meanwhile
    }
    // P2's store (pad 2)
    else if (bwgate[G.p2_slot]){ p2->cur_keys=0; }
    else if (lstore[1]){
        p2->cur_keys=0;
        station_input(G.p2_slot, lstation[1], p2up, p2down, act2_edge(ACT_FIRE));
        if (act2_edge(ACT_INTERACT) || act2_edge(ACT_ESC)){
            if (!(pay_active && pay_player==G.p2_slot)){ lstore[1]=0; p2->in_store=0; p2->station=-1; bud_stage[G.p2_slot]=0; }
        }
    } else if (act2_edge(ACT_INTERACT) && p2->alive && downed_near(p2)){
        try_revive(G.p2_slot);
    } else if (act2_edge(ACT_INTERACT) && p2->alive &&
               ((!p2->in_mall && near_store(p2)) || (p2->in_mall && (station_at(p2)>=0 || bw_door_at(p2) || on_exit_mat(p2) || arcade_door_at(p2) || arcade_exit_at(p2) || cabinet_at(p2)>=0)))){
        if (!p2->in_mall){ mall_enter(G.p2_slot); }
        else {
            int st=station_at(p2);
            if (st>=0){ lstore[1]=1; lstation[1]=st; G.store_sel2=0; p2->station=st; p2->in_store=1; p2->cur_keys=0; sfx(19); }
            else if (arcade_door_at(p2)){ arcade_enter(G.p2_slot); }
            else if (bw_door_at(p2)){ bw_gate_try(G.p2_slot); }
            else if (arcade_exit_at(p2)){ arcade_exit(G.p2_slot); }
            else if (cabinet_at(p2)>=0){ arc_cab[G.p2_slot]=cabinet_at(p2); arc_gate_try(G.p2_slot); }
            else mall_exit(G.p2_slot);
        }
    } else resolve_p2_input();
    // pause: either player's Start — but never out from under a shopper
    if (!lstore[0] && !lstore[1] && G.state==GS_PLAY &&
        (G.net_mode==NET_OFF || (G.net_mode==NET_HOST && !any_remote_connected())) &&
        ((IN.pause_pressed && !p1bw0) || (act2_edge(ACT_PAUSE) && !p2bw0))){
        G.state=GS_PAUSE; me->cur_keys=0; p2->cur_keys=0; G.pause_sel=0; G.pause_ctl=0;
        render_pause();
        return;
    }
    update_sim(dt);
    if (G.shake>0) G.shake -= dt*6;
    if (G.state==GS_OVER) render_over();
    else coop_render(dt);
    if (G.net_mode==NET_HOST){
        net_send_t -= dt;
        if (net_send_t<=0){ net_send_t=1.0f/12; host_snapshot(); }
        wall_send_t -= dt;
        if (wall_send_t<=0){ wall_send_t=2.5f; host_send_walls(-1); }
        if (G.state==GS_OVER){
            int o=0; wr_u8(&o,'G'); plat_net_send(pkt, o, -1);
        }
    }
}
static void game_init(void){
    smemset(&G, 0, sizeof(G));
    scpy(G.my_name, "DEV", 13);
    scpy(G.rec_name, "NOBODY", 13);
    G.my_id=0;
    G.p2_slot=-1; G.view_w=VW; G.view_y0=0; G.view_h=VH;
    bind_defaults(G.bind);
    load_records_and_check_save();
    if (G.slot<1 || G.slot>3) G.slot=1;
    probe_slots();
    G.state=GS_MENU;
}
// Turn the platform's physical-key report (+ gamepad actions) into the
// semantic input fields, through the player's bindings. Arrow keys are
// permanent alternates for movement and the only keyboard menu keys —
// that keeps typing in text fields from steering menus.
static void derive_input(float dt){
    // one-frame taps (the touch X button) must edge exactly once and
    // never latch into the held mask: a latched bit only edges the
    // first time, which made the X button work exactly once per load
    u64 pulse = IN.pulse; IN.pulse = 0;
    g_raw_edges = IN.edges_in | pulse | (IN.raw & ~prev_raw);
    IN.edges_in = 0;
    // last-device stamps (see pad_mode): any pad action — held or
    // edged, either player's pad — counts as gamepad use; any
    // physical key (or a touch, which renders keyboard-style labels
    // through the mobile branches) counts as keyboard use.
    IN.dev_t += dt;
    if (IN.act_held || IN.act_edges || IN.act2_held || IN.act2_edges) IN.pad_last = IN.dev_t;
    { float m=0; for (int i=0;i<4;i++){ float a=IN.pad_ax[i]; if (a<0) a=-a; if (a>m) m=a; }
      if (m>0.18f) IN.pad_last = IN.dev_t; }   // a stick pushed past the cursor deadzone is pad use too, even below the digital threshold
    if (IN.raw || g_raw_edges || IN.touch_seen) IN.key_last = IN.dev_t;
    int k=0;
    if (raw_dn(G.bind[ACT_UP])    || raw_dn(PK_UP)    || (IN.act_held>>ACT_UP&1))    k|=K_UP;
    if (raw_dn(G.bind[ACT_DOWN])  || raw_dn(PK_DOWN)  || (IN.act_held>>ACT_DOWN&1))  k|=K_DOWN;
    if (raw_dn(G.bind[ACT_LEFT])  || raw_dn(PK_LEFT)  || (IN.act_held>>ACT_LEFT&1))  k|=K_LEFT;
    if (raw_dn(G.bind[ACT_RIGHT]) || raw_dn(PK_RIGHT) || (IN.act_held>>ACT_RIGHT&1)) k|=K_RIGHT;
    if (raw_dn(G.bind[ACT_FIRE])   || (IN.act_held>>ACT_FIRE&1))   k|=K_FIRE;
    if (raw_dn(G.bind[ACT_SPRINT]) || (IN.act_held>>ACT_SPRINT&1)) k|=K_SPRINT;
    IN.keys = k;
    G.key_edges = k & ~prev_sem; prev_sem = k;
    IN.enter_pressed    = raw_edge(PK_ENTER) || (int)((IN.act_edges>>ACT_ENTER)&1);
    IN.select_pressed   = ((G.key_edges & K_FIRE)!=0) || raw_edge(G.bind[ACT_FIRE])
                          || (int)((IN.act_edges>>ACT_FIRE)&1) || IN.enter_pressed;
    IN.esc_pressed      = raw_edge(PK_ESC)   || (int)((IN.act_edges>>ACT_ESC)&1);
    IN.interact_pressed = raw_edge(G.bind[ACT_INTERACT]) || (int)((IN.act_edges>>ACT_INTERACT)&1);
    IN.wall_pressed     = raw_edge(G.bind[ACT_WALL])     || (int)((IN.act_edges>>ACT_WALL)&1);
    IN.pause_pressed    = raw_edge(G.bind[ACT_PAUSE])    || (int)((IN.act_edges>>ACT_PAUSE)&1);
    IN.backspace        = raw_edge(PK_BACKSPACE);
    IN.weapon_key = 0;
    for (int w=0;w<5;w++)
        if (raw_edge(G.bind[ACT_W1+w]) || ((IN.act_edges>>(ACT_W1+w))&1)) IN.weapon_key=w+1;
    IN.bean_key = raw_edge(26+6) || ((IN.act_edges>>ACT_BEAN)&1);   // the 6 key (digits are PK 26..35) or the pad's Y button
    IN.weapon_cycle = ((IN.act_edges>>ACT_WNEXT)&1)? 1 : ((IN.act_edges>>ACT_WPREV)&1)? -1 : 0;
    // menu navigation: arrow edges, synthesized auto-repeat while held
    // (identical repeat timing in every build), gamepad edges
    IN.menu_up=0; IN.menu_down=0;
    if (raw_edge(PK_UP) || ((IN.act_edges>>ACT_UP)&1)){ IN.menu_up=1; menu_rep_dir=-1; menu_rep_t=0.38f; }
    else if (raw_edge(PK_DOWN) || ((IN.act_edges>>ACT_DOWN)&1)){ IN.menu_down=1; menu_rep_dir=1; menu_rep_t=0.38f; }
    else if (menu_rep_dir!=0 && (raw_dn(PK_UP) || raw_dn(PK_DOWN))){
        menu_rep_t -= dt;
        if (menu_rep_t<=0){
            if (menu_rep_dir<0 && raw_dn(PK_UP)) IN.menu_up=1;
            if (menu_rep_dir>0 && raw_dn(PK_DOWN)) IN.menu_down=1;
            menu_rep_t=0.09f;
        }
    } else menu_rep_dir=0;
    prev_raw = IN.raw;
    IN.pad_edges = IN.act_edges;   // a snapshot for late readers (WAR!'s pad cursor) — act_edges itself is consumed below
    IN.act_edges = 0;   // consumed
}

// ---------------------------------------------------------------------------
// Game-over portrait: the hat you died in (Daniel). Death crashes the
// stack to zero, so while the local player lives we keep a running
// snapshot of the hat — the game-over screen wears it, and so does the
// share card. (The statics live up by render_over; this is the keeper.)
// ---------------------------------------------------------------------------
static void over_refresh(void){
    if (G.state!=GS_PLAY) return;
    Player *me=&G.p[G.my_id>=0? G.my_id : 0];
    if (!me->connected || !me->alive) return;
    over_stack_n = me->stack_n<MAXSTACK? me->stack_n : MAXSTACK;
    for (int i=0;i<over_stack_n;i++) over_stack[i]=me->stack[i];
    over_carried = me->carried;
    over_have = 1;
}

static double last_now=0; static int booted=0;
void frame(void){
    double now = plat_now();
    float dt = booted? (float)(now-last_now) : 0.016f;
    last_now = now;
    if (dt>0.05f) dt=0.05f; if (dt<0) dt=0;
    if (!booted){ booted=1; game_init(); }
    if (want_autostart){
        want_autostart = 0;
        if (G.state==GS_MENU && G.net_mode==NET_OFF) start_run(NET_CLIENT);
    }
    G.anim_t += dt;
    if (G.store_msg_t>0) G.store_msg_t--;
    derive_input(dt);
    G.view_pi = G.my_id>=0? G.my_id : 0;   // split passes override per view
    touch_update(dt);
    update_audio(dt);
    whale_poll(dt);

    if (G.net_mode==NET_HOST) net_host_poll();
    if (G.net_mode==NET_CLIENT) net_client_poll();
    over_refresh();
    if (G.state!=GS_MENU) G.menu_pad=0;   // title-menu layout latch resets off-menu

    switch (G.state){
    case GS_MENU: menu_input(); render_menu(); break;
    case GS_SETTINGS: settings_input(); render_settings(); break;
    case GS_HELP:
        if (IN.esc_pressed) G.state=GS_MENU;
        else if (IN.interact_pressed || IN.select_pressed || IN.tapped){
            help_page ^= 1;   // first press: page two. second: back to the menu
            if (!help_page) G.state=GS_MENU;
        }
        render_help(); break;
    case GS_PAUSE:
        if (any_remote_connected()){ G.state=GS_PLAY; G.pause_ctl=0; }   // a guest arrived: action resumes
        else if (G.pause_ctl){
            if (IN.esc_pressed || IN.pause_pressed || IN.select_pressed || IN.interact_pressed || IN.tapped || act2_edge(ACT_PAUSE) || act2_edge(ACT_ESC)) G.pause_ctl=0;
        }
        else if (IN.esc_pressed || IN.pause_pressed || act2_edge(ACT_PAUSE) || act2_edge(ACT_ESC)) G.state=GS_PLAY;
        else {
            if (IN.menu_up) G.pause_sel = (G.pause_sel+2)%3;
            if (IN.menu_down) G.pause_sel = (G.pause_sel+1)%3;
            if (IN.tapped){
                for (int i=0;i<3;i++){ int y=176+i*26;
                    if (IN.tap_y>=y-6 && IN.tap_y<=y+17){
                        if (i==2 && G.pause_sel!=2) G.pause_sel=2;    // first tap arms quit
                        else if (i==0) G.state=GS_PLAY;
                        else if (i==1) G.pause_ctl=1;
                        else pause_save_quit();
                        break; } }
            }
            else if (IN.select_pressed || IN.interact_pressed){
                if (G.pause_sel==0) G.state=GS_PLAY;
                else if (G.pause_sel==1) G.pause_ctl=1;
                else pause_save_quit();
            }
        }
        render_pause();
        break;
    case GS_SLOTS: slots_input(); render_slots(); break;
    case GS_PLAY: case GS_STORE: {
        if (G.net_mode==NET_CLIENT){
            if (!client_joined){
                client_joined=1;
                int o=0; wr_u8(&o,'J');
                for (int k2=0;k2<12;k2++) wr_u8(&o, (u8)G.my_name[k2]);
                wr_u8(&o, (u8)G.whale_own); wr_u8(&o, (u8)G.whale_on);
                wr_u8(&o, (u8)(G.whale_own>>8)); wr_u8(&o, (u8)(G.whale_on>>8));
                plat_net_send(pkt, o, 0);
            }
            if (G.my_id<0){
                // connecting screen
                rect_fill(0,0,VW,VH,COL(24,22,38));
                draw_demo_scene();
                rect_fill(0, 140, VW, 60, COL(20,18,36));
                draw_text_center(VW/2, 158, "CONNECTING TO HOST...", 2, COL(255,214,64));
                draw_text_center(VW/2, 184, pad_mode()? "BACK - CANCEL" : "ESC - CANCEL", 1, COL(190,200,220));
                if (IN.esc_pressed){ plat_net_stop(); G.net_mode=NET_OFF; G.state=GS_MENU; }
                break;
            }
            // client: send inputs, render the host's world
            Player *me=&G.p[G.my_id];
            me->is_local=1;
            resolve_local_input();
            if (store_open_local){
                station_input(G.my_id, client_station, IN.menu_up, IN.menu_down, IN.select_pressed);
                if (client_station==4) bowl_update(G.my_id, dt);   // guests bowl on their own screen
                G.store_station = client_station;
                render_store();
                if (IN.esc_pressed || (IN.interact_pressed && !(client_station==1 && whale_code_mode[G.my_id])) || G.tap_close_store){ store_open_local=0; G.tap_close_store=0; whale_code_mode[G.my_id]=0; client_tell_store(0); }
            } else {
                if ((IN.interact_pressed || IN.enter_pressed || (IN.tapped && !me->in_mall && tap_on_store())) && me->alive){
                    if (!me->in_mall && near_store(me)){ int o=0; wr_u8(&o,'E'); plat_net_send(pkt, o, 0); sfx(19); }
                    else if (me->in_mall){
                        int st = station_at(me);
                        if (st>=0){ store_open_local=1; client_station=st; G.store_sel=0; client_tell_store(1); sfx(19); }
                        else if (on_exit_mat(me)){ int o=0; wr_u8(&o,'E'); plat_net_send(pkt, o, 0); sfx(19); }
                    }
                }
                // particles/floaters still animate locally
                update_fx(dt);
                G.camx += (me->x - G.camx)*(dt*6>1?1:dt*6);
                G.camy += (me->y - G.camy)*(dt*6>1?1:dt*6);
                render_world(); render_hud();
            }
            net_send_t -= dt;
            if (net_send_t<=0){ net_send_t=1.0f/30; net_client_send_input(); }
            break;
        }
        // single player or host: full simulation
        {
            Player *me=&G.p[G.my_id];
            resolve_local_input();
            // local co-op: with the option on and two controllers in,
            // a press on pad 2 drops P2 into the run beside P1
            if (G.p2_slot<0 && G.coop_pref && G.net_mode!=NET_CLIENT && G.state==GS_PLAY && IN.pad_count>=2 && IN.act2_edges)
                coop_spawn_p2(me->x-30, me->y);
            if (G.p2_slot>=0 && G.net_mode!=NET_CLIENT){ coop_frame(dt); break; }
            // remote players take their input from the net
            if (G.net_mode==NET_HOST)
                for (int i=1;i<MAXP;i++) if (G.p[i].connected){
                    G.p[i].cur_keys = G.p[i].net_keys;
                }
            // pause whenever you're effectively solo (the sim simply
            // stops advancing): offline, or hosting with no guests yet
            if (G.state==GS_PLAY && !bwgate[G.my_id] && !arcgate[G.my_id] && !G.bud[G.my_id].in_bw && (G.net_mode==NET_OFF ||
                (G.net_mode==NET_HOST && !any_remote_connected())) &&
                (IN.esc_pressed || IN.pause_pressed)){
                G.state=GS_PAUSE; me->cur_keys=0; G.pause_sel=0; G.pause_ctl=0; render_pause(); break;
            }
            int gate_was = bwgate[G.my_id] || arcgate[G.my_id];
            if (G.state==GS_PLAY && bwgate[G.my_id]){
                me->cur_keys=0;
                if (IN.menu_up){ bwgate_sel[G.my_id] = (bwgate_sel[G.my_id]+2)%3; sfx(6); }
                if (IN.menu_down){ bwgate_sel[G.my_id] = (bwgate_sel[G.my_id]+1)%3; sfx(6); }
                if (IN.tapped) bw_gate_tap(G.my_id, 0, 0, VW, VH);
                else if (IN.select_pressed || IN.enter_pressed) bw_gate_pick(G.my_id);
                else if (IN.esc_pressed || IN.pause_pressed){ bwgate[G.my_id]=0; sfx(6); }
            }
            if (G.state==GS_PLAY && arcgate[G.my_id]){
                me->cur_keys=0;
                if (IN.menu_up || IN.menu_down){ arcgate_sel[G.my_id] = (arcgate_sel[G.my_id]+1)%2; sfx(6); }
                if (IN.tapped) arc_gate_tap(G.my_id, 0, 0, VW, VH);
                else if (IN.select_pressed || IN.enter_pressed) arc_gate_pick(G.my_id);
                else if (IN.esc_pressed || IN.pause_pressed){ arcgate[G.my_id]=0; sfx(6); }
            }
            if (G.state==GS_PLAY && G.bud[G.my_id].in_bw){
                // my buddy is off in Banzai World: my input drives it,
                // and my DEV just stands at the gate in the mall
                me->cur_keys=0;
                if (IN.tapped) bw_battle_tap(G.my_id, 0, 0, VW, VH);
                BwIn bin; bw_fill_p1(&bin, dt);
                bw_update(G.my_id, dt, &bin);
            }
            if (G.state==GS_PLAY && !G.bud[G.my_id].in_bw && !gate_was){
                if ((IN.interact_pressed || IN.enter_pressed || (IN.tapped && !me->in_mall && tap_on_store())) && me->alive){
                    if (IN.interact_pressed && downed_near(me)){ try_revive(G.my_id); }
                    else if (!me->in_mall && near_store(me)){ mall_enter(G.my_id); me->cur_keys=0; }
                    else if (me->in_mall){
                        int st = station_at(me);
                        if (st>=0){ G.state=GS_STORE; G.store_station=st; G.store_sel=0; me->station=st; me->in_store=1; me->cur_keys=0; sfx(19); }
                        else if (arcade_door_at(me)){ arcade_enter(G.my_id); me->cur_keys=0; }
                        else if (bw_door_at(me)){ bw_gate_try(G.my_id); me->cur_keys=0; }
                        else if (arcade_exit_at(me)){ arcade_exit(G.my_id); me->cur_keys=0; }
                        else if (cabinet_at(me)>=0){ arc_cab[G.my_id]=cabinet_at(me); arc_gate_try(G.my_id); me->cur_keys=0; }
                        else if (on_exit_mat(me)){ mall_exit(G.my_id); me->cur_keys=0; }
                    }
                }
            } else if (G.state==GS_STORE) {
                // This was a bare else once: with the gate panel open
                // (or the buddy away in Banzai World) the frame fell
                // into station_input against the STALE G.store_station
                // — picking STORY MODE also withdrew the bank, and
                // taps landed on a phantom panel (Daniel, Nov 4:
                // "the menus got mixed up")
                me->cur_keys=0;
                station_input(G.my_id, G.store_station, IN.menu_up, IN.menu_down, IN.select_pressed);
                // While an unlock code is being typed, the Interact key
                // must not close the panel — codes are full of E's, and
                // the E that types the letter was also slamming the panel
                // shut mid-code (this is why a hand-typed code could
                // never be finished). Esc still backs out.
                if (IN.esc_pressed || (IN.interact_pressed && !(G.store_station==1 && whale_code_mode[G.my_id])) || G.tap_close_store){
                    if (pay_active && pay_player==G.my_id){ store_note("THE TILL IS COUNTING - ONE MOMENT"); sfx(16); G.tap_close_store=0; }
                    else { G.state=GS_PLAY; me->in_store=0; me->station=-1; G.tap_close_store=0; bud_stage[G.my_id]=0; whale_code_mode[G.my_id]=0; }
                }
            }
            update_sim(dt);
            G.camx += (me->x - G.camx)*(dt*6>1?1:dt*6);
            G.camy += (me->y - G.camy)*(dt*6>1?1:dt*6);
            if (G.shake>0) G.shake -= dt*6;
            if (G.state==GS_STORE) render_store();
            else if (G.state==GS_OVER) render_over();
            else if (G.bud[G.my_id].in_bw) bw_render(G.my_id);
            else { render_world(); render_hud(); if (bwgate[G.my_id]) bw_gate_draw(G.my_id, 0, 0, VW, VH); if (arcgate[G.my_id]) arc_gate_draw(G.my_id, 0, 0, VW, VH); }
            if (G.net_mode==NET_HOST){
                net_send_t -= dt;
                if (net_send_t<=0){ net_send_t=1.0f/12; host_snapshot(); }
                wall_send_t -= dt;
                if (wall_send_t<=0){ wall_send_t=2.5f; host_send_walls(-1); }
                if (G.state==GS_OVER){ // tell the clients the run ended
                    int o=0; wr_u8(&o,'G'); plat_net_send(pkt, o, -1);
                }
            }
        }
        break; }
    case GS_OVER:
        G.gameover_t += dt;
        render_over();
        if ((IN.esc_pressed || IN.interact_pressed || IN.select_pressed || IN.tapped) && G.gameover_t>0.6f){
            if (G.net_mode!=NET_OFF){ plat_net_stop(); G.net_mode=NET_OFF; }
            for (int i=0;i<MAXP;i++) G.p[i].connected=0;
            G.p2_slot=-1; coop_reset_views();
            G.state=GS_MENU;
        }
        break;
    }
    // clear one-frame input edges
    IN.act2_edges=0;   // pad-2 edges live exactly one frame, like P1's
    IN.mclicked=0; IN.rclicked=0; IN.wall_pressed=0; IN.interact_pressed=0;
    IN.esc_pressed=0; IN.enter_pressed=0; IN.typed_n=0; IN.backspace=0; IN.weapon_key=0; IN.bean_key=0;
    IN.menu_up=0; IN.menu_down=0; IN.select_pressed=0; IN.pause_pressed=0; IN.tapped=0;
    G.tap_close_store=0;
}

