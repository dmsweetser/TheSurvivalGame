// THE SURVIVAL GAME — net/host.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 1533-1595 in the Nov 2026 fine-grained refactor; code unchanged.

static void net_host_poll(void){
    u8 buf[2048]; int src=0;
    for (int iter=0; iter<10; iter++){
        int n = plat_net_recv(buf, 2048, &src);
        if (n<=0) break;
        if (buf[0]=='J'){
            // find the sender's slot or a free one
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot<0) for (int i=1;i<MAXP;i++) if (!G.p[i].connected){ slot=i; break; }
            if (slot<0) continue;
            char nm[13]; int ni=0;
            for (int k=0;k<12 && 1+k<n;k++){ char c=(char)buf[1+k]; nm[ni++]= c? c:' '; }
            nm[ni]=0; while (ni>0 && nm[ni-1]==' ') nm[--ni]=0;
            if (!G.p[slot].connected){
                player_fresh(&G.p[slot], nm[0]?nm:"DEV", slot, rng_range(-40,40), 80);
                G.p[slot].connected=1;
            }
            if (n>=15){ G.p[slot].whale_own = buf[13]; G.p[slot].whale_on = buf[14];
                if (n>=17){ G.p[slot].whale_own |= buf[15]<<8; G.p[slot].whale_on |= buf[16]<<8; }
                G.p[slot].whale_on &= G.p[slot].whale_own; }   // v1.32: a guest's on-mask never exceeds their own-mask
            pkt_src_token[slot]=src;
            int o=0; wr_u8(&o,'A'); wr_u8(&o,(u8)slot);
            plat_net_send(pkt, o, src);
            host_send_roster(-1); host_send_walls(src);
        } else if (buf[0]=='I'){
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot<0 || n<8) continue;
            int o=1;
            G.p[slot].net_keys = rd_u8(buf,&o);
            G.p[slot].net_aim = rd_flt(buf,&o);
            int wp = rd_u8(buf,&o);
            if (wp<NWEAP && G.p[slot].owned[wp]) G.p[slot].weapon=wp;
            int act = rd_u8(buf,&o);
            if (act&1) G.p[slot].want_wall=1;
            if (act&2){ /* interact: handled as buy context by client UI */ }
        } else if (buf[0]=='B' && n>=2){
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot>=0) try_buy(slot, buf[1]);
        } else if (buf[0]=='M' && n>=3){
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot>=0) station_action(slot, buf[1], buf[2]);
        } else if (buf[0]=='E'){
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot>=0){
                Player *p=&G.p[slot];
                if (!p->in_mall && near_store(p)) mall_enter(slot);
                else if (p->in_mall && on_exit_mat(p)) mall_exit(slot);
            }
        } else if (buf[0]=='T' && n>=2){
            int slot=-1;
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src) slot=i;
            if (slot>=0) G.p[slot].in_store = buf[1]? 1:0;
        } else if (buf[0]=='L'){
            for (int i=1;i<MAXP;i++) if (G.p[i].connected && pkt_src_token[i]==src){
                G.p[i].connected=0; host_send_roster(-1);
            }
        }
    }
}
