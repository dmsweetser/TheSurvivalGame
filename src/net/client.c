// THE SURVIVAL GAME — net/client.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 1596-1727 in the Nov 2026 fine-grained refactor; code unchanged.

static void net_client_poll(void){
    u8 buf[65536]; int src=0;
    for (int iter=0; iter<6; iter++){
        int n = plat_net_recv(buf, 65536, &src);
        if (n<=0) break;
        if (buf[0]=='A' && n>=2 && G.my_id<0){ G.my_id = buf[1]; }
        else if (buf[0]=='R'){
            int o=1; int cnt=rd_u8(buf,&o);
            for (int i=0;i<MAXP;i++) if (i!=G.my_id) G.p[i].connected=0;
            for (int k=0;k<cnt;k++){
                int id=rd_u8(buf,&o); int col=rd_u8(buf,&o);
                char nm[13]; for (int j=0;j<12;j++) nm[j]=(char)rd_u8(buf,&o); nm[12]=0;
                if (id>=0 && id<MAXP){
                    G.p[id].connected=1; G.p[id].color=col;
                    scpy(G.p[id].name, nm, 13);
                    if (id==G.my_id) G.p[id].is_local=1;
                }
            }
        } else if (buf[0]=='S'){
            int o=1; G.time = rd_flt(buf,&o);
            G.total_kills = rd_u16(buf,&o);
            G.diff_run = rd_u8(buf,&o); if (G.diff_run<0 || G.diff_run>3) G.diff_run=0;
            for (int i=0;i<MAXP;i++){
                int conn = rd_u8(buf,&o);
                if (!conn){ if (i!=G.my_id) G.p[i].connected=0; continue; }
                Player *p=&G.p[i];
                p->connected=1;
                p->x=rd_flt(buf,&o); p->y=rd_flt(buf,&o);
                { float av=rd_flt(buf,&o); if (i!=G.my_id) p->aim=av; }
                // (our own facing stays local + instant; the host echoes it
                // back in snapshots but we don't want the arm to lag)
                p->hp=rd_u16(buf,&o); p->shield=rd_u16(buf,&o);
                p->weapon=rd_u8(buf,&o); p->alive=rd_u8(buf,&o);
                int sn=rd_u16(buf,&o); p->carried=rd_flt(buf,&o); p->heaviest=rd_flt(buf,&o);
                p->kills=rd_u16(buf,&o); p->rollback=rd_u8(buf,&o);
                for (int w=0;w<NWEAP;w++) p->owned[w]=rd_u16(buf,&o);
                for (int u=0;u<5;u++) p->upg[u]=rd_u16(buf,&o);
                p->banked=rd_flt(buf,&o); p->hatstand=rd_u8(buf,&o); p->in_mall=rd_u8(buf,&o);
                p->on_wall = rd_u8(buf,&o)? 0 : -1;
                p->whale_own=rd_u8(buf,&o); p->whale_on=rd_u8(buf,&o);
                p->whale_own |= rd_u8(buf,&o)<<8; p->whale_on |= rd_u8(buf,&o)<<8;   // round 2 hi bytes
                p->top_worn = rd_u8(buf,&o); if (p->top_worn<0 || p->top_worn>TOP_N) p->top_worn=0;   // HAT TOPPERS (feat 5)
                p->stack_n=sn;
                // synthesize a plausible stack for the hat drawing
                float each = sn>0? p->carried/sn : 0;
                for (int s2=0;s2<sn && s2<MAXSTACK;s2++) p->stack[s2]=each*(0.6f+0.4f*((s2*7)%5)/4.0f);
                p->dmg_mul = 1.0f + 0.25f*p->upg[0];
                p->rate_mul = 1.0f + 0.18f*p->upg[1];
                p->shield_max = shield_for(p->upg[2]);
                p->maxhp = 100 + 25*p->upg[3];
                p->walk_ph += 0.1f;
            }
            int bn = rd_u16(buf,&o);
            smemset(G.bugs, 0, sizeof(G.bugs));
            for (int i=0;i<bn && i<MAXBUGS;i++){
                Bug *b=&G.bugs[i];
                b->alive=1;
                b->x=rd_flt(buf,&o); b->y=rd_flt(buf,&o); b->w=rd_flt(buf,&o);
                float hf=rd_u8(buf,&o)/255.0f; b->kind=rd_u8(buf,&o);
                b->bp_t=rd_u8(buf,&o)? 0.2f:0; b->hit_t=rd_u8(buf,&o)? 0.1f:0;
                b->leg_ph=rd_flt(buf,&o);
                b->elite=rd_u8(buf,&o); if (b->elite<0 || b->elite>ELITE_N) b->elite=0;   // NAMED ELITES (feat 8)
                b->maxhp=100; b->hp=hf*100;
                b->seed=(u32)(i*7919);
            }
            int evc = rd_u8(buf,&o);
            for (int i=0;i<evc;i++){
                int t=rd_u8(buf,&o); float ex=rd_flt(buf,&o), ey=rd_flt(buf,&o);
                if (t==1) splat(ex, ey, BUG_COL[i%4], 8, 110);
                if (t==2) splat(ex, ey, COL(170,120,70), 10, 110);
                if (t==3) sfx(11);
                if (t==4) sfx(5);
            }
            shield_remote = rd_u8(buf,&o);
            { int agn = rd_u8(buf,&o); (void)agn; }   // agents retired: count byte discarded
            { int bn2 = rd_u8(buf,&o);
              for (int i=0;i<MAXP;i++) G.bud[i].active=0;
              for (int i=0;i<bn2;i++){
                  int oi=rd_u8(buf,&o); int sp=rd_u8(buf,&o);
                  float bx=rd_flt(buf,&o), by=rd_flt(buf,&o);
                  int hf=rd_u8(buf,&o); int lu=rd_u8(buf,&o);
                  int xp=rd_u16(buf,&o);
                  int uh=rd_u8(buf,&o), ua=rd_u8(buf,&o), ud=rd_u8(buf,&o), sk=rd_u8(buf,&o);
                  int fn=rd_u8(buf,&o);
                  float wp=rd_flt(buf,&o);
                  int nm2=rd_u8(buf,&o); int cl2=rd_u8(buf,&o); int en2=rd_u8(buf,&o);
                  int ow2=rd_u8(buf,&o);
                  if (oi>=0 && oi<MAXP){
                      Buddy *bd=&G.bud[oi];
                      bd->active=1; bd->species=sp%BUDN; bd->x=bx; bd->y=by;
                      bd->on_wall = ow2-1;
                      bd->up_hp=uh; bd->up_atk=ua; bd->up_def=ud; bd->skills=sk; bd->xp=xp;
                      bd->fainted=fn? 1:0;
                      bd->name_idx=nm2%BUD_NAMES_N; bd->color=cl2%BUD_COLORS_N; bd->energy=en2;
                      buddy_refresh(oi);
                      bd->hp = bd->maxhp * hf/255.0f;
                      bd->lunge_t = lu? 0.15f : 0;
                      if (wp != bd->walk_ph) bd->mv_t = 0.15f;   // a guest's buddy flops on the client's screen too
                      bd->walk_ph = wp;
                  }
              } }
            if (G.my_id>=0 && G.state==GS_MENU) G.state=GS_PLAY;
        } else if (buf[0]=='W'){
            int o=1; int cnt=rd_u16(buf,&o);
            smemset(G.walls, 0, sizeof(G.walls));
            for (int i=0;i<cnt && i<MAXWALLS;i++){
                Wall *w=&G.walls[i];
                w->x=rd_flt(buf,&o); w->y=rd_flt(buf,&o);
                w->hp=rd_flt(buf,&o); w->maxhp=rd_flt(buf,&o);
                w->horiz=rd_u8(buf,&o); w->stack=rd_u8(buf,&o); if (w->stack<1) w->stack=1;
                w->alive=1; w->grp=-1; w->grp_old=-1;
            }
            walls_recompute();   // client side: groups/tiers only, no pools
        } else if (buf[0]=='G'){
            G.state=GS_OVER; G.gameover_t=0;
        }
    }
}
static void net_client_send_input(void){
    int o=0; wr_u8(&o,'I');
    wr_u8(&o, (u8)G.p[G.my_id<0?0:G.my_id].cur_keys);
    // facing, resolved from the movement keys in resolve_local_input
    wr_flt(&o, client_aim);
    wr_u8(&o, (u8)client_weapon);
    wr_u8(&o, (u8)(client_want_wall?1:0));
    plat_net_send(pkt, o, 0);
    client_want_wall=0;
}

// ---------------------------------------------------------------------------
// Store purchases + store input
// ---------------------------------------------------------------------------
