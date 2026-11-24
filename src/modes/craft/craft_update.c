// THE SURVIVAL GAME — modes/craft/craft_update.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from craft.c lines 531-995 in the Nov 2026 fine-grained refactor; code unchanged.

static void craft_update(int pi, float dt, const BwIn *in){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    if (in->exit_ && !S->craft_menu){ bw_exit(pi, 0); return; }
    if (S->craft_grace>0) S->craft_grace -= dt;
    if (b->atk_cd>0) b->atk_cd -= dt;
    if (b->lunge_t>0) b->lunge_t -= dt;
    if (b->hurt_t>0) b->hurt_t -= dt;
    if (S->craft_haste>0) S->craft_haste -= dt;
    if (S->craft_demolish>0) S->craft_demolish -= dt;
    if (S->craft_beam_t>0) S->craft_beam_t -= dt;
    // the day turns, in this mode only
    b->craft_tod += dt; if (b->craft_tod>=240) b->craft_tod -= 240;
    { float dk = craft_darkf(b);
      if (dk>0.6f && S->craft_darkp<=0.6f) craft_note(S, "NIGHT FALLS. THE WILD GETS HUNGRY.");
      S->craft_darkp = dk; }
    // woke from a rest (bw_update ran the sleep itself)
    if (S->slept){
        S->slept = 0;
        if (S->craft_nap){   // a bedroll nap: time passes, the night does not skip
            S->craft_nap = 0;
            b->craft_tod += 90; if (b->craft_tod>=240) b->craft_tod -= 240;
            scpy(S->note, "A FITFUL NAP UNDER THE STARS.", 72); S->note_t=2.6f;
            if (craft_darkf(b)>0.4f){   // and something noticed you sleeping out there
                craft_spawn_bug(pi, hash_f((int)(b->x/48), (int)(G.anim_t*10), craft_seed(b))*6.2832f, 300, -1);
                scpy(S->note, "YOU WAKE TO COMPANY. THE WILD FOUND YOU.", 72); S->note_t=2.6f;
            }
        }
        else if (craft_darkf(b)>0.4f){
            b->craft_tod = 232;   // slept through the dangerous hours
            scpy(S->note, "YOU SLEPT TILL DAWN. GOOD AS NEW.", 72); S->note_t=2.6f;
        }
    }
    // the crafting menu swallows the rest of the input
    if (S->craft_menu){
        // ONE list with segment headers (craft_all_rows). Up/down are
        // the only way the selection moves — left/right do nothing in
        // here on purpose, so a sensitive dpad can never flip lists.
        int rows[64]; int n = craft_menu_rows(S, b, rows);
        // SEARCH (Nov 14): "/" (or a tap on the search row) starts
        // it — P itself belongs to the BW menu and never reaches
        // this update. Typed letters filter the list flat: the fix
        // for scrolling thirty rows on a phone. While searching,
        // typed characters are text, never commands.
        int typed_now = 0;
        if (!S->craft_search && pi==G.my_id){
            for (int i=0;i<IN.typed_n;i++) if (IN.typed[i]=='/'){ S->craft_search = 1; break; }
        }
        // v1.31 (audit P8/H10): a pad player has no "/" key — the
        // pad's X button starts the search instead. Inside this menu
        // X (the WALL action) otherwise only closes it; the press
        // that OPENS the search must not read as that close, so it
        // is remembered and the close check below spares it.
        int search_started = 0;
        if (!S->craft_search && pi==G.my_id && ((IN.pad_edges>>ACT_WALL)&1)){ S->craft_search = 1; search_started = 1; }
        if (S->craft_search){
            if (pi==G.my_id){
                for (int i=0;i<IN.typed_n;i++){
                    char c = IN.typed[i];
                    if (c>='a' && c<='z') c -= 32;
                    if ((c>='A' && c<='Z') || (c>='0' && c<='9') || c==' '){
                        int L = slen(S->craft_filter);
                        if (L<12){ S->craft_filter[L]=c; S->craft_filter[L+1]=0; typed_now=1; }
                    }
                }
                if (IN.backspace){ int L = slen(S->craft_filter); if (L>0) S->craft_filter[L-1]=0; typed_now=1; }
            }
            if (typed_now){ S->craft_sel=0; S->craft_scroll=0; n = craft_menu_rows(S, b, rows); }
            // leaving search clears the filter: the soft-keyboard
            // session restarts from an empty field every time, so a
            // standing filter could never be edited again — done
            // searching means the whole list comes back
            if (!typed_now && in->exit_){ S->craft_search=0; S->craft_filter[0]=0; S->craft_sel=0; S->craft_scroll=0; S->craft_prevk = in->keys; return; }
        }
        if (S->craft_sel>=n) S->craft_sel=0;
        if (rows[S->craft_sel]<0) S->craft_sel = craft_step(rows, n, S->craft_sel, 1);
        if (in->up){ S->craft_sel = craft_step(rows, n, S->craft_sel, -1); sfx(6); }
        if (in->down){ S->craft_sel = craft_step(rows, n, S->craft_sel, 1); sfx(6); }
        S->craft_cat = craft_row_cat(rows[S->craft_sel]);   // kept truthful for the detail strip/tests
        // keep the selection inside the visible window of the list
        if (S->craft_sel < S->craft_scroll) S->craft_scroll = S->craft_sel;
        if (S->craft_sel > S->craft_scroll+6) S->craft_scroll = S->craft_sel-6;
        if (n>7){ if (S->craft_scroll > n-7) S->craft_scroll = n-7; } else S->craft_scroll = 0;
        if (S->craft_scroll<0) S->craft_scroll=0;
        // touch: the menu is a real touch menu (both schemes) — tap a
        // row to pick it, tap it again to craft, tap a segment header
        // to hop to that segment, tap outside to close. Geometry
        // mirrors craft_render exactly.
        if (pi==G.my_id && IN.tapped){
            int vx,vy,vw2,vh2; p1_view_rect(&vx,&vy,&vw2,&vh2);
            int visn = n<7? n : 7;
            int w=380, h=40+visn*20+16+24+22;
            int x=vx+(vw2-w)/2, y=vy+(vh2-h)/2;
            if (w>vw2-8){ w=vw2-8; x=vx+4; }
            int tx=IN.tap_x, ty=IN.tap_y; IN.tapped=0;
            if (tx>=x+10 && tx<=x+w-10 && ty>=y+28 && ty<y+50){
                // the search row toggles typing; ending a search
                // clears its filter (see the keyboard path)
                S->craft_search = !S->craft_search;
                if (!S->craft_search){ S->craft_filter[0]=0; S->craft_sel=0; S->craft_scroll=0; }
                sfx(6);
            }
            else if (tx<x || tx>x+w || ty<y || ty>y+h){ S->craft_menu=0; S->craft_search=0; sfx(6); }
            else for (int r=S->craft_scroll; r<S->craft_scroll+visn && r<n; r++){
                int ry = y+52+(r-S->craft_scroll)*20;
                if (ty>=ry-4 && ty<ry+15){
                    if (rows[r]<0){ S->craft_sel = craft_step(rows, n, r, 1); sfx(6); }
                    else if (r==S->craft_sel) craft_do(pi, rows[r]);
                    else { S->craft_sel=r; sfx(6); }
                    break;
                }
            }
            S->craft_prevk = in->keys;
            return;
        }
        if ((in->sel || in->interact) && !typed_now && rows[S->craft_sel]>=0) craft_do(pi, rows[S->craft_sel]);
        if ((in->exit_ || in->wall) && !search_started){ S->craft_menu=0; S->craft_search=0; }   // v1.31: the pad press that STARTED the search is not also its close
        S->craft_prevk = in->keys;
        return;
    }
    if (in->wall){ int rows[64]; int n = craft_all_rows(b, rows);
        S->craft_menu=1; S->craft_sel = n>1? craft_step(rows, n, 0, 1) : 0; S->craft_scroll=0; S->craft_demolish=0; S->craft_filter[0]=0; S->craft_search=0; sfx(6); S->craft_prevk = in->keys; return; }
    // movement, meadow-style, against the wild's own solids
    float mx=0, my=0; int k=in->keys;
    if (k&K_UP) my-=1; if (k&K_DOWN) my+=1; if (k&K_LEFT) mx-=1; if (k&K_RIGHT) mx+=1;
    int on_dock = 0;
    { int dbi = craft_build_at(b, bw_tileof(b->x), bw_tileof(b->y)); if (dbi>=0 && b->craft_bk[dbi]==7) on_dock = 1; }
    int on_water = craft_biome_at(b, bw_tileof(b->x), bw_tileof(b->y))==5 && (b->craft_gear&512) && !on_dock;
    if (mx!=0 || my!=0){
        float d=fsqrt(mx*mx+my*my); mx/=d; my/=d;
        b->aim = fatan2(my, mx);
        float sp = BUD_SPD[b->species%BUDN];
        if (on_water) sp *= (b->craft_gear&262144)? 0.88f : 0.62f;   // a sail makes the raft honest
        if (S->craft_haste>0) sp *= 1.35f;
        float nx = b->x + mx*sp*dt, ny = b->y + my*sp*dt;
        if (!craft_solid_at(pi, nx, b->y, 1)) b->x = nx;
        if (!craft_solid_at(pi, b->x, ny, 1)) b->y = ny;
        b->walk_ph += dt*9;
        b->mv_t = 0.15f;
    }
    // backstop: never leave the builder inside a solid tile — shove
    // him out by the nearest open edge (a placed wall used to be
    // able to entomb him; placement refuses now, this is the belt
    // to its suspenders)
    if (craft_solid_at(pi, b->x, b->y, 1)){
        int tx=bw_tileof(b->x), ty=bw_tileof(b->y);
        float cand[4][2] = { { tx*BW_TILE-16.0f, b->y }, { (tx+1)*BW_TILE+16.0f, b->y },
                             { b->x, ty*BW_TILE-16.0f }, { b->x, (ty+1)*BW_TILE+16.0f } };
        float best=1e9f; int bi2=-1;
        for (int c2=0;c2<4;c2++){
            if (craft_solid_at(pi, cand[c2][0], cand[c2][1], 1)) continue;
            float dx=cand[c2][0]-b->x, dy=cand[c2][1]-b->y, d2=dx*dx+dy*dy;
            if (d2<best){ best=d2; bi2=c2; }
        }
        if (bi2>=0){ b->x=cand[bi2][0]; b->y=cand[bi2][1]; }
    }
    if (b->energy<BUD_ENERGY_MAX){ b->energy += BUD_EN_REGEN*dt; if (b->energy>BUD_ENERGY_MAX) b->energy=BUD_ENERGY_MAX; }
    b->bw_x = b->x; b->bw_y = b->y;
    b->calm += dt;
    if (b->calm>4.0f && b->hp>0 && b->hp<b->maxhp){ b->hp += 2.2f*dt; if (b->hp>b->maxhp) b->hp=b->maxhp; }
    // fires warm you faster — the fireplace reaches further
    for (int i=0;i<b->craft_bn;i++){
        if (b->craft_bk[i]!=1 && b->craft_bk[i]!=3) continue;
        float r = b->craft_bk[i]==3? 112.0f : 74.0f, heal = b->craft_bk[i]==3? 10.0f : 6.0f;
        float dx=b->x-(b->craft_btx[i]*BW_TILE+24), dy=b->y-(b->craft_bty[i]*BW_TILE+24);
        if (dx*dx+dy*dy < r*r && b->hp>0 && b->hp<b->maxhp){ b->hp += heal*dt; if (b->hp>b->maxhp) b->hp=b->maxhp; }
    }
    // where are we?
    int biome = craft_biome_at(b, bw_tileof(b->x), bw_tileof(b->y));
    if (biome != S->craft_biome){
        S->craft_biome = biome;
        if (biome!=5){ scpy(S->note, CRAFT_BIOME_NAME[biome], 72); S->note_t = 2.4f; }
    }
    // hands: interact gathers, rests, loots, demolishes; the fire
    // wins ties — at camp, the interact button means REST
    int ttx = bw_tileof(b->x + fcos(b->aim)*34), tty = bw_tileof(b->y + fsin(b->aim)*34);
    if (in->interact && b->atk_cd<=0){
        b->atk_cd = 0.38f;
        if (S->craft_demolish>0) craft_demolish_at(pi);
        else if (craft_fire_near(b, b->x, b->y, 84, 0, 0)){ S->sleep_t = 1.4f; S->slept = 0; S->craft_nap = 0; sfx(11); }
        else if (craft_bedroll_near(b)){ S->sleep_t = 1.4f; S->slept = 0; S->craft_nap = 1; sfx(11); }
        else if ((b->craft_gear&16384) && craft_biome_at(b, ttx, tty)==5) craft_fish_at(pi);
        else if (S->craft_cache_on){
            float dx=S->craft_cache_x-b->x, dy=S->craft_cache_y-b->y;
            if (dx*dx+dy*dy < 50*50) craft_loot_cache(pi);
            else if (craft_node_at(pi, ttx, tty)) craft_hit_node(pi, ttx, tty);
        }
        else if (craft_node_at(pi, ttx, tty)) craft_hit_node(pi, ttx, tty);
    }
    if (in->sel && b->atk_cd<=0){
        if ((b->craft_gear&128) && S->craft_demolish<=0){   // the zapper shoots instead of swinging
            int storm = (b->craft_gear&65536)? 1 : 0;
            b->atk_cd = storm? 0.22f : 0.34f; b->lunge_t = 0.12f;
            for (int i=0;i<4;i++) if (!S->craft_bolt_a[i]){
                S->craft_bolt_a[i]=1; S->craft_bolt_t[i]=0.55f;
                S->craft_bolt_x[i]=b->x + fcos(b->aim)*16; S->craft_bolt_y[i]=b->y + fsin(b->aim)*16;
                S->craft_bolt_vx[i]=fcos(b->aim)*330; S->craft_bolt_vy[i]=fsin(b->aim)*330;
                S->craft_bolt_pc[i]=storm; S->craft_bolt_hc[i]=0;
                sfx(12); break;
            }
        } else if ((b->craft_gear&32768) && !S->craft_bmr_a && S->craft_demolish<=0){   // the boomerang goes out — and comes back
            b->atk_cd = 0.5f; b->lunge_t = 0.14f;
            S->craft_bmr_a=1; S->craft_bmr_t=0;
            S->craft_bmr_x=b->x + fcos(b->aim)*16; S->craft_bmr_y=b->y + fsin(b->aim)*16;
            S->craft_bmr_vx=fcos(b->aim)*360; S->craft_bmr_vy=fsin(b->aim)*360;
            sfx(12);
        } else {
            b->atk_cd = 0.42f; b->lunge_t = 0.16f;
            int dmg = (b->craft_gear&64)? 3 : (b->craft_gear&4)? 2 : 1;
            float reach = (b->craft_gear&64)? 70.0f : 52.0f;
            if (S->craft_cache_on){
                float dx=S->craft_cache_x-b->x, dy=S->craft_cache_y-b->y;
                if (dx*dx+dy*dy < (reach+22)*(reach+22)) craft_loot_cache(pi);
            }
            for (int i=0;i<S->craft_bugn;i++){
                if (S->craft_bhp[i]<=0) continue;
                float dx=S->craft_bx[i]-b->x, dy=S->craft_by[i]-b->y;
                float d2 = dx*dx+dy*dy;
                if (d2 < reach*reach){
                    float dl = fsqrt(d2)+0.01f;
                    if ((dx*fcos(b->aim) + dy*fsin(b->aim))/dl > 0.15f || d2 < 24*24){
                        S->craft_bhp[i] -= dmg; sfx(4);
                        splat(S->craft_bx[i], S->craft_by[i], COL(150,80,70), 5, 80);
                        if (S->craft_bhp[i]<=0) craft_bug_die(pi, i);
                    }
                }
            }
            for (int i=0;i<S->craft_shn;i++){   // a swing reaches the sharks at the raft's edge too
                if (S->craft_sh_hp[i]<=0) continue;
                float dx=S->craft_sh_x[i]-b->x, dy=S->craft_sh_y[i]-b->y;
                float d2 = dx*dx+dy*dy;
                if (d2 < (reach+8)*(reach+8)){
                    float dl = fsqrt(d2)+0.01f;
                    if ((dx*fcos(b->aim) + dy*fsin(b->aim))/dl > 0.15f || d2 < 26*26){
                        S->craft_sh_hp[i] -= dmg; sfx(4);
                        splat(S->craft_sh_x[i], S->craft_sh_y[i], COL(190,200,210), 5, 80);
                        if (S->craft_sh_hp[i]<=0) craft_shark_die(pi, i);
                    }
                }
            }
            if (craft_node_at(pi, ttx, tty)) craft_hit_node(pi, ttx, tty);
        }
    }
    // zapper bolts in flight (storm bolts pierce one extra target)
    for (int i=0;i<4;i++) if (S->craft_bolt_a[i]){
        S->craft_bolt_t[i] -= dt;
        if (S->craft_bolt_hc[i]>0) S->craft_bolt_hc[i] -= dt;
        S->craft_bolt_x[i] += S->craft_bolt_vx[i]*dt;
        S->craft_bolt_y[i] += S->craft_bolt_vy[i]*dt;
        if (S->craft_bolt_t[i]<=0 || craft_hard_at(pi, S->craft_bolt_x[i], S->craft_bolt_y[i])){ S->craft_bolt_a[i]=0; continue; }
        if (S->craft_bolt_hc[i]>0) continue;
        int consumed = 0;
        for (int j=0;j<S->craft_bugn && !consumed;j++){
            if (S->craft_bhp[j]<=0) continue;
            float dx=S->craft_bx[j]-S->craft_bolt_x[i], dy=S->craft_by[j]-S->craft_bolt_y[i];
            if (dx*dx+dy*dy < 15*15){
                S->craft_bhp[j] -= 2; sfx(4);
                splat(S->craft_bx[j], S->craft_by[j], COL(255,170,80), 5, 80);
                if (S->craft_bhp[j]<=0) craft_bug_die(pi, j);
                if (S->craft_bolt_pc[i]>0){ S->craft_bolt_pc[i]--; S->craft_bolt_hc[i]=0.18f; }
                else { S->craft_bolt_a[i]=0; consumed=1; }
            }
        }
        for (int j=0;j<S->craft_shn && !consumed && S->craft_bolt_a[i];j++){
            if (S->craft_sh_hp[j]<=0) continue;
            float dx=S->craft_sh_x[j]-S->craft_bolt_x[i], dy=S->craft_sh_y[j]-S->craft_bolt_y[i];
            if (dx*dx+dy*dy < 17*17){
                S->craft_sh_hp[j] -= 2; sfx(4);
                splat(S->craft_sh_x[j], S->craft_sh_y[j], COL(255,170,80), 5, 80);
                if (S->craft_sh_hp[j]<=0) craft_shark_die(pi, j);
                if (S->craft_bolt_pc[i]>0){ S->craft_bolt_pc[i]--; S->craft_bolt_hc[i]=0.18f; }
                else S->craft_bolt_a[i]=0;
            }
        }
    }
    // the boomerang: out fast, then it homes back to the hand
    if (S->craft_bmr_a){
        S->craft_bmr_t += dt;
        if (S->craft_bmr_t < 0.42f){
            S->craft_bmr_x += S->craft_bmr_vx*dt; S->craft_bmr_y += S->craft_bmr_vy*dt;
            if (craft_hard_at(pi, S->craft_bmr_x, S->craft_bmr_y)) S->craft_bmr_t = 0.42f;
        } else {
            float dx=b->x-S->craft_bmr_x, dy=b->y-S->craft_bmr_y, d2=dx*dx+dy*dy;
            if (d2 < 22*22 || S->craft_bmr_t > 2.2f){ S->craft_bmr_a=0; }
            else { float dl=fsqrt(d2)+0.01f; S->craft_bmr_x += dx/dl*400*dt; S->craft_bmr_y += dy/dl*400*dt; }
        }
        if (S->craft_bmr_a){
            for (int j=0;j<S->craft_bugn;j++){
                if (S->craft_bhp[j]<=0 || S->craft_btr[j]>0.25f) continue;
                float dx=S->craft_bx[j]-S->craft_bmr_x, dy=S->craft_by[j]-S->craft_bmr_y;
                if (dx*dx+dy*dy < 17*17){
                    S->craft_bhp[j] -= 2; S->craft_btr[j]=0.3f; sfx(4);
                    splat(S->craft_bx[j], S->craft_by[j], COL(255,190,90), 5, 80);
                    if (S->craft_bhp[j]<=0) craft_bug_die(pi, j);
                }
            }
            for (int j=0;j<S->craft_shn;j++){
                if (S->craft_sh_hp[j]<=0 || S->craft_sh_tr[j]>0) continue;
                float dx=S->craft_sh_x[j]-S->craft_bmr_x, dy=S->craft_sh_y[j]-S->craft_bmr_y;
                if (dx*dx+dy*dy < 18*18){
                    S->craft_sh_hp[j] -= 2; S->craft_sh_tr[j]=0.3f; sfx(4);
                    splat(S->craft_sh_x[j], S->craft_sh_y[j], COL(255,190,90), 5, 80);
                    if (S->craft_sh_hp[j]<=0) craft_shark_die(pi, j);
                }
            }
        }
    }
    // feral bugs: the wild is not empty, and the night is worse
    int night = craft_isnight(b);
    S->craft_bt -= dt;
    if (S->craft_bt<=0){
        S->craft_bt = night? 0.9f : 2.5f;
        if (S->craft_bugn < (night? 13 : 8)){
            float a = hash_f((int)(b->x/48), (int)(G.anim_t*10), craft_seed(b)) * 6.2832f;
            craft_spawn_bug(pi, a, 500, -1);
        }
    }
    // the occasional swarm keeps everyone honest
    S->craft_swarm_t -= dt;
    if (S->craft_swarm_t<=0){
        S->craft_swarm_t = 85 + hash_f((int)(b->x/48), (int)(b->y/48), craft_seed(b))*40;
        int n2 = night? 7 : 5;
        for (int s2=0;s2<n2;s2++) craft_spawn_bug(pi, s2*1.13f + b->x*0.01f, 260, -1);
        craft_note(S, "A SWARM IS COMING!"); S->note_t=2.4f; sfx(16);
    }
    // supply caches drift in now and then
    S->craft_cache_t -= dt;
    if (!S->craft_cache_on && S->craft_cache_t<=0){
        int placed=0;
        for (int t2=0;t2<8 && !placed;t2++){
            float a = hash_f((int)(G.anim_t*3)+t2*17, (int)(b->y/48), craft_seed(b)) * 6.2832f;
            float dd = 230 + hash_f(t2*31, (int)(b->x/48), craft_seed(b))*90;
            float cx2 = b->x + fcos(a)*dd, cy2 = b->y + fsin(a)*dd;
            if (!craft_solid_at(pi, cx2, cy2, 0) && craft_biome_at(b, bw_tileof(cx2), bw_tileof(cy2))!=5){
                S->craft_cache_x=cx2; S->craft_cache_y=cy2; S->craft_cache_on=1; S->craft_cache_t=75; placed=1;
            }
        }
        if (!placed) S->craft_cache_t=20;
    }
    if (S->craft_cache_on && S->craft_cache_t<=0){ S->craft_cache_on=0; S->craft_cache_t=50; }
    for (int i=0;i<S->craft_bugn;i++){
        if (S->craft_bhp[i]<=0) continue;
        int bk = S->craft_bk[i];
        float dx=b->x-S->craft_bx[i], dy=b->y-S->craft_by[i];
        float d2=dx*dx+dy*dy;
        S->craft_bph[i] += dt*0.7f;
        if (S->craft_btr[i]>0) S->craft_btr[i] -= dt;
        // standing on spikes hurts and slows (checked before moving)
        int on_trap = craft_build_at(b, bw_tileof(S->craft_bx[i]), bw_tileof(S->craft_by[i]))>=0
                   && b->craft_bk[craft_build_at(b, bw_tileof(S->craft_bx[i]), bw_tileof(S->craft_by[i]))]==5;
        float mvx, mvy, sp;
        float chase_r = bk==1? 300.0f : 240.0f;
        // a scarecrow is easier to catch than a buddy, and bugs know it
        float txp=b->x, typ=b->y, td2=d2; int decoy=-1;
        if (bk!=2){
            for (int j2=0;j2<b->craft_bn;j2++){
                if (b->craft_bk[j2]!=9) continue;
                float sx2=b->craft_btx[j2]*BW_TILE+24, sy2=b->craft_bty[j2]*BW_TILE+24;
                float sdx=sx2-S->craft_bx[i], sdy=sy2-S->craft_by[i], sd2=sdx*sdx+sdy*sdy;
                if (sd2<d2 && sd2<320*320){ txp=sx2; typ=sy2; td2=sd2; decoy=j2; }
            }
        }
        float tdx=txp-S->craft_bx[i], tdy=typ-S->craft_by[i];
        if (bk==2 && d2 < 210*210){ float dl=fsqrt(d2)+0.01f; mvx=-dx/dl; mvy=-dy/dl; sp=125.0f; }   // the golden one runs
        else if (td2 < chase_r*chase_r){ float dl=fsqrt(td2)+0.01f; mvx=tdx/dl; mvy=tdy/dl; sp = bk==1? 74.0f : 56.0f; }
        else { mvx=fcos(S->craft_bph[i]); mvy=fsin(S->craft_bph[i]); sp = bk==1? 30.0f : 22.0f; }
        if (on_trap) sp *= 0.55f;
        float nx = S->craft_bx[i] + mvx*sp*dt, ny = S->craft_by[i] + mvy*sp*dt;
        if (!craft_solid_at(pi, nx, S->craft_by[i], 0)) S->craft_bx[i]=nx;
        if (!craft_solid_at(pi, S->craft_bx[i], ny, 0)) S->craft_by[i]=ny;
        if (on_trap && S->craft_btr[i]<=0){
            S->craft_bhp[i] -= 1; S->craft_btr[i]=0.7f; sfx(4);
            if (S->craft_bhp[i]<=0){ craft_bug_die(pi, i); continue; }
        }
        if (decoy>=0 && td2 < 27*27 && S->craft_btr[i]<=0){
            S->craft_btr[i]=0.8f; b->craft_bhp[decoy] -= 1; sfx(4);
            if (b->craft_bhp[decoy]<=0){ craft_build_remove(b, decoy); craft_note(S, "THE SCARECROW IS DOWN!"); S->note_t=2.0f; do_save(); }
        }
        // after dark, bugs chew through builds that pen them in
        if (night && bk!=2){
            for (int j=0;j<b->craft_bn;j++){
                int kk2=b->craft_bk[j];
                if (kk2!=0 && kk2!=2 && kk2!=4 && kk2!=10) continue;
                float cx2=b->craft_btx[j]*BW_TILE+24, cy2=b->craft_bty[j]*BW_TILE+24;
                float ddx=S->craft_bx[i]-cx2, ddy=S->craft_by[i]-cy2;
                if (ddx*ddx+ddy*ddy < 42*42 && S->craft_btr[i]<=0){
                    b->craft_bhp[j] -= 1;
                    S->craft_btr[i] = (kk2==2||kk2==10)? 0.7f : kk2==4? 0.35f : 0.22f;
                    sfx(4);
                    if (b->craft_bhp[j]<=0){
                        craft_build_remove(b, j);
                        craft_note(S, "SOMETHING WAS CHEWED THROUGH IN THE DARK!"); S->note_t=2.2f; sfx(16); do_save();
                    }
                    break;
                }
            }
        }
        if (d2 < 25*25 && S->craft_grace<=0 && !b->fainted && bk!=2){
            float dmgf = (bk==1? 0.16f : 0.12f) * ((b->craft_gear&131072)? 0.15f : (b->craft_gear&256)? 0.3f : (b->craft_gear&8)? 0.5f : 1.0f);
            b->hp -= b->maxhp * dmgf; b->calm = 0; b->hurt_t = 0.3f;
            S->craft_grace = 1.3f; sfx(16);
            scpy(S->note, (b->craft_gear&8) || (b->craft_gear&256) || (b->craft_gear&131072)? "THE ARMOR TOOK THE BITE." : "A FERAL BUG BIT YOU!", 72); S->note_t=1.8f;
            if (b->hp<=0){ b->hp=0; b->fainted=1; bw_exit(pi, 1); return; }
        }
        if (d2 > 900*900) S->craft_bhp[i]=0;   // wandered off for good
    }
    // sharks: the water was never safe, it was just unguarded
    if (on_water){
        S->craft_sh_t -= dt;
        if (S->craft_sh_t<=0){
            S->craft_sh_t = 2.2f;
            if (S->craft_shn<4) craft_shark_spawn(pi, hash_f((int)(b->x/48), (int)(G.anim_t*10), craft_seed(b))*6.2832f, 300.0f);
        }
    }
    for (int i=0;i<S->craft_shn;i++){
        if (S->craft_sh_hp[i]<=0) continue;
        if (S->craft_sh_tr[i]>0) S->craft_sh_tr[i] -= dt;
        S->craft_sh_ph[i] += dt*0.9f;
        float dx=b->x-S->craft_sh_x[i], dy=b->y-S->craft_sh_y[i], d2=dx*dx+dy*dy;
        float mvx, mvy, sp;
        if (d2 < 380*380){ float dl=fsqrt(d2)+0.01f; mvx=dx/dl; mvy=dy/dl; sp=97.0f; }
        else { mvx=fcos(S->craft_sh_ph[i]); mvy=fsin(S->craft_sh_ph[i]); sp=26.0f; }
        S->craft_sh_hd[i] = fatan2(mvy, mvx);
        float nx=S->craft_sh_x[i]+mvx*sp*dt, ny=S->craft_sh_y[i]+mvy*sp*dt;
        // keep the whole fish in the water: the centre AND a
        // body's-length ahead of it must both be wet, or the grey
        // shows on the sand (Daniel, Nov 6)
        if (craft_biome_at(b, bw_tileof(nx), bw_tileof(S->craft_sh_y[i]))==5
         && craft_biome_at(b, bw_tileof(nx + mvx*15), bw_tileof(S->craft_sh_y[i]))==5) S->craft_sh_x[i]=nx;
        if (craft_biome_at(b, bw_tileof(S->craft_sh_x[i]), bw_tileof(ny))==5
         && craft_biome_at(b, bw_tileof(S->craft_sh_x[i]), bw_tileof(ny + mvy*15))==5) S->craft_sh_y[i]=ny;
        if (d2 < 27*27 && S->craft_grace<=0 && !b->fainted){
            float dmgf = 0.20f * ((b->craft_gear&131072)? 0.15f : (b->craft_gear&256)? 0.3f : (b->craft_gear&8)? 0.5f : 1.0f);
            b->hp -= b->maxhp*dmgf; b->calm=0; b->hurt_t=0.3f; S->craft_grace=1.3f; sfx(16);
            scpy(S->note, "SHARK BITE! THE WATER HAS TEETH.", 72); S->note_t=1.8f;
            splat(b->x, b->y, COL(200,215,225), 6, 90);
            if (b->hp<=0){ b->hp=0; b->fainted=1; bw_exit(pi, 1); return; }
        }
        if (d2 > 1000*1000) S->craft_sh_hp[i]=0;   // lost the scent
    }
    if (S->craft_shn>0 && S->craft_sh_hp[0]<=0){
        int w=0;
        for (int i=0;i<S->craft_shn;i++) if (S->craft_sh_hp[i]>0){ S->craft_sh_x[w]=S->craft_sh_x[i]; S->craft_sh_y[w]=S->craft_sh_y[i]; S->craft_sh_hp[w]=S->craft_sh_hp[i]; S->craft_sh_ph[w]=S->craft_sh_ph[i]; S->craft_sh_tr[w]=S->craft_sh_tr[i]; S->craft_sh_hd[w]=S->craft_sh_hd[i]; w++; }
        S->craft_shn=w;
    }
    // zapper towers stand guard over the base
    for (int j=0;j<b->craft_bn;j++){
        if (b->craft_bk[j]!=4) continue;
        if (S->craft_tz[j]>0){ S->craft_tz[j] -= dt; continue; }
        float cx2=b->craft_btx[j]*BW_TILE+24, cy2=b->craft_bty[j]*BW_TILE+24;
        int tgt=-1; float td=132*132;
        for (int i=0;i<S->craft_bugn;i++){
            if (S->craft_bhp[i]<=0) continue;
            float dx=S->craft_bx[i]-cx2, dy=S->craft_by[i]-cy2, d2=dx*dx+dy*dy;
            if (d2<td){ td=d2; tgt=i; }
        }
        if (tgt>=0){
            S->craft_tz[j]=1.1f; S->craft_beam_t=0.12f;
            S->craft_beam_x1=cx2; S->craft_beam_y1=cy2-16;
            S->craft_beam_x2=S->craft_bx[tgt]; S->craft_beam_y2=S->craft_by[tgt];
            S->craft_bhp[tgt] -= 1; sfx(12);
            splat(S->craft_bx[tgt], S->craft_by[tgt], COL(160,210,255), 4, 80);
            if (S->craft_bhp[tgt]<=0) craft_bug_die(pi, tgt);
        } else S->craft_tz[j]=0.25f;
    }
    // compact the bug list occasionally
    if (S->craft_bugn>0 && S->craft_bhp[0]<=0){
        int w=0;
        for (int i=0;i<S->craft_bugn;i++) if (S->craft_bhp[i]>0){ S->craft_bx[w]=S->craft_bx[i]; S->craft_by[w]=S->craft_by[i]; S->craft_bhp[w]=S->craft_bhp[i]; S->craft_bph[w]=S->craft_bph[i]; S->craft_bk[w]=S->craft_bk[i]; S->craft_btr[w]=S->craft_btr[i]; w++; }
        S->craft_bugn=w;
    }
    S->craft_prevk = in->keys;
}
