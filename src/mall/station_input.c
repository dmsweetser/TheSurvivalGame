// THE SURVIVAL GAME — mall/station_input.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 767-878 in the Nov 2026 fine-grained refactor; code unchanged.

static int case_scroll[2];   // TROPHY CASE (feat 4): the same windowing, its own scroll pair
static int whale_slot_of(int pi){ return (G.p2_slot>=0 && pi==G.p2_slot)? 1 : 0; }
static void whale_scroll_sync(int slot, int sel, int rows){
    int *sc = &whale_scroll[slot];
    if (sel < *sc) *sc = sel;
    if (sel > *sc + WHALE_VIS-1) *sc = sel - (WHALE_VIS-1);
    if (*sc > rows - WHALE_VIS) *sc = rows - WHALE_VIS;
    if (*sc < 0) *sc = 0;
}
static void case_scroll_sync(int slot, int sel, int rows){
    int *sc = &case_scroll[slot];
    if (sel < *sc) *sc = sel;
    if (sel > *sc + WHALE_VIS-1) *sc = sel - (WHALE_VIS-1);
    if (*sc > rows - WHALE_VIS) *sc = rows - WHALE_VIS;
    if (*sc < 0) *sc = 0;
}
static void srcterm_input(int pi, int up, int down, int activate);   // mall/srcterm.c (v1.26)
static void station_input(int pi, int st, int up, int down, int activate){
    if (st==9){ srcterm_input(pi, up, down, activate); return; }   // THE SOURCE terminal runs its own views
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
    if (st==5) case_scroll_sync(whale_slot_of(pi), *selp, rows);
    if (pi==G.my_id && IN.tapped){
        int w = G.ui_compact? 308 : STORE_W;
        int x = G.ui_compact? G.view_x0 + (G.view_w - w)/2 : STORE_X;
        if (IN.tap_x<x-6 || IN.tap_x>x+w+6 || IN.tap_y<STORE_Y-6 || IN.tap_y>STORE_Y+STORE_H+6) G.tap_close_store=1;
        else if (st==1 || st==5){
            // tap math follows the SCROLLED window the draw shows
            int sc = (st==1)? whale_scroll[whale_slot_of(pi)] : case_scroll[whale_slot_of(pi)];
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
