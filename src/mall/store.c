// THE SURVIVAL GAME — mall/store.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 1728-1814 in the Nov 2026 fine-grained refactor; code unchanged.

static void store_note(const char *msg){
    scpy(G.store_msg, msg, 64); G.store_msg_t = 150;
}
static void try_buy(int pi, int sel){
    Player *p=&G.p[pi];
    if (pay_active) return;
    if (G.state!=GS_STORE && G.net_mode!=NET_HOST) { /* local buys happen in store UI */ }
    if (!near_store(p) && !p->in_store){
        if (pi==G.my_id) store_note("YOU MUST BE AT THE STORE");
        return;
    }
    int owned; int price = store_item_price(sel, p, &owned);
    if (owned){
        if (pi==G.my_id){ store_note(sel<5? "FULLY STACKED"
                                          : (sel==11? "ALREADY AT FULL HP" : "FULLY UPGRADED")); sfx(16); }
        return;
    }
    if (p->carried < (float)price){
        if (pi==G.my_id){
            char b[64]; char *q=b;
            q=sappend(q, "NOT ENOUGH HAT - NEED ");
            q=sapp_int(q, (int)(price - p->carried)); q=sappend(q, " KG MORE");
            store_note(b); sfx(16);
        }
        return;
    }
    start_payment(pi, sel<5?0:1, sel<5? sel : sel-5, price);
}
static void client_tell_store(int open){
    // let the host know we're browsing, so the Epicenter shield goes up
    int o=0; wr_u8(&o,'T'); wr_u8(&o,(u8)open); plat_net_send(pkt, o, 0);
}
// drive one player's store browsing. The nav inputs come from whoever
// owns the store session: P1's keyboard/pad-1 fields, or P2's pad-2
// edges in local co-op — each shopper keeps their own cursor.
static void store_input(int pi, int up, int down, int activate){
    int *selp = (G.p2_slot>=0 && pi==G.p2_slot)? &G.store_sel2 : &G.store_sel;
    if (up){ *selp = (*selp + STORE_ROWS-1)%STORE_ROWS; sfx(6); }
    if (down){ *selp = (*selp + 1)%STORE_ROWS; sfx(6); }
    // touch: tap a row to select it, tap the selected row to buy it,
    // tap anywhere outside the panel to leave the store (P1 only)
    if (pi==G.my_id && IN.tapped){
        if (IN.tap_x<STORE_X-6 || IN.tap_x>STORE_X+STORE_W+6 ||
            IN.tap_y<STORE_Y-6 || IN.tap_y>STORE_Y+STORE_H+6) G.tap_close_store=1;
        else for (int i=0;i<STORE_ROWS;i++){
            int ry = STORE_ROW0 + (i<5? i*STORE_ROWH : i*STORE_ROWH+STORE_UPGOFF);
            if (IN.tap_y>=ry-5 && IN.tap_y<=ry+STORE_ROWH-2){
                if (i==*selp) activate=1;
                else { *selp=i; sfx(6); }
                break;
            }
        }
    }
    if (activate){
        if (G.net_mode==NET_CLIENT){
            int o=0; wr_u8(&o,'B'); wr_u8(&o,(u8)*selp);
            plat_net_send(pkt, o, 0);
            sfx(10);
        } else try_buy(pi, *selp);
    }
}

// ---------------------------------------------------------------------------
// Mall stations — the panels for the Gallery, the Bug Bank and the
// Banzai Buddies store. Station 0 is the Bug Bounty counter and keeps
// the original store panel + input. Everything buys with hat weight or
// buddy XP, through the same one-at-a-time till.
// ---------------------------------------------------------------------------
#define HATSTAND_PRICE 150
// a revive is priced off the buddy itself (Daniel, Nov 2): its
// species, its training, and its taught moves — a fresh pet rock
// is cheap to wake, a trained moose with both moves is not. A
// faint never costs the buddy anything it earned: training, XP
// and skills all ride through untouched (see hurt_buddy).
static int bud_revive_price(Buddy *b){
    int skills = (b->skills&1) + ((b->skills>>1)&1) + ((b->skills>>2)&1) + ((b->skills>>3)&1);
    return 10 + BUD_PRICE[b->species%BUDN]/5
         + 6*(b->up_hp + b->up_atk + b->up_def) + 15*skills;
}
#define MALL_ROW0 (STORE_ROW0 + 26)
// training prices (Daniel): the first lessons are cheap; mastery is a
// fortune. Flat 12/24/36... became roughly x1.45 a level past the basics.
static int bud_train_cost(int lvl){
    float c = 10.0f;
    for (int i=0;i<lvl;i++) c = c*1.45f + 2.0f;
    return (int)(c + 0.5f);
}
