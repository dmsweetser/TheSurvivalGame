// THE SURVIVAL GAME — mall/panels.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 1770-1817 in the Nov 2026 fine-grained refactor; code unchanged.

static void draw_srcterm_panel(void);   // mall/srcterm.c (v1.26)
static void draw_active_panel(void){
    if (G.store_station==0) draw_store_panel();
    else if (G.store_station==4) draw_bowling();
    else if (G.store_station==8) draw_booth_panel();
    else if (G.store_station==9) draw_srcterm_panel();
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
