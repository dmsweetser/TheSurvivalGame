// THE SURVIVAL GAME — save/slots.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from save.c lines 1316-1437 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Save slots: probe at boot, summarize for the picker, run the picker.
// Legacy saves (one global blob with the run inside) become slot 1.
// ---------------------------------------------------------------------------
static void probe_slots(void){
    for (int s=1;s<=3;s++){
        int n = plat_load_slot(s, slotbuf, SAVE_CAP-1);
        G.slot_has[s] = 0;
        if (n>0){ slotbuf[n]=0; const char *v = line_val(slotbuf,"run");
            if (v && parse_int(v)==1) G.slot_has[s]=1; }
    }
    if (!G.slot_has[1]){
        // legacy: the global blob (still in savebuf from the records
        // load) carries the run — lift its run section into slot 1,
        // keeping the original format version character.
        const char *v = line_val(savebuf,"run");
        if (v && parse_int(v)==1){
            const char *rs = v - 4;                    // start of "run="
            char *q = slotbuf;
            q = sappend(q, "TSG"); *q++ = savebuf[3]; *q++='\n'; *q=0;
            while (*rs) *q++ = *rs++;
            *q = 0;
            plat_save_slot(1, slotbuf, (int)(q - slotbuf));
            G.slot_has[1] = 1;
        }
    }
    refresh_has_save();
}
// picker summaries, peeked from the slot blobs (never loads the run)
static char pk_name[4][13];
static int pk_hat[4], pk_kills[4], pk_time[4], pk_zen[4];
static char pk_bname[4][16];   // the slot's buddy, "" when none
static int pk_bspecies[4];
static void peek_slots(void){
    for (int s=1;s<=3;s++){
        pk_name[s][0]=0; pk_hat[s]=0; pk_kills[s]=0; pk_time[s]=0; pk_zen[s]=0;
        pk_bname[s][0]=0; pk_bspecies[s]=-1;
        if (!G.slot_has[s]) continue;
        int n = plat_load_slot(s, slotbuf, SAVE_CAP-1);
        if (n<=0){ G.slot_has[s]=0; continue; }
        slotbuf[n]=0;
        const char *v;
        if ((v=line_val(slotbuf,"time"))) pk_time[s]=parse_int(v);
        if ((v=line_val(slotbuf,"tkills"))) pk_kills[s]=parse_int(v);
        if ((v=line_val(slotbuf,"zrun"))) pk_zen[s]=parse_int(v)? 1:0;
        if ((v=line_val(slotbuf,"sum"))){
            const char *q=v;
            while (*q==' ') q++;   // the writer's separator space after
            // "sum=" — without this skip the name reads empty and every
            // field after it shifts one token early (the other half of
            // the phantom-pet bug: the tail walk then lands mid-list)
            int ni=0;
            while (*q && *q!=' ' && *q!='\n' && ni<12){ pk_name[s][ni++]=*q++; } pk_name[s][ni]=0;
            if (*q==' ') q++;
            pk_hat[s]=parse_int(q);
            // walk the rest of the summary: hat kills time zen, then
            // the buddy tail [bnameidx bspecies]. This once walked only
            // three fields, so it read ZEN as the name index and the
            // name index as the species — the picker then cheerfully
            // listed a pet the player does not have (Daniel, Nov 4)
            for (int adv=0; adv<4; adv++){ while (*q && *q!=' ' && *q!='\n') q++; if (*q==' ') q++; }
            if (*q && *q!='\n'){
                int bni=parse_int(q);
                while (*q && *q!=' ' && *q!='\n') q++; if (*q==' ') q++;
                int bsp=parse_int(q);
                if (bni>=0 && bsp>=0 && bsp<BUDN){
                    scpy(pk_bname[s], BUD_ELF[bni%BUD_NAMES_N], 16);
                    pk_bspecies[s]=bsp;
                }
            }
        } else {
            // legacy blob: the name rides on the first pl= line
            const char *ls = slotbuf;
            while (*ls){
                if (ls[0]=='p' && ls[1]=='l' && ls[2]=='=' && ls[3]==' '){
                    const char *q=ls+4;
                    parse_int(q); while (*q && *q!=' ') q++; if (*q) q++;
                    int ni=0; while (*q && *q!=' ' && *q!='\n' && ni<12){ pk_name[s][ni++]=*q++; } pk_name[s][ni]=0;
                    break;
                }
                while (*ls && *ls!='\n') ls++;
                if (*ls=='\n') ls++;
            }
        }
        if (!pk_name[s][0]) scpy(pk_name[s], "DEV", 13);
    }
    refresh_has_save();
}
static void open_slots(int mode){
    G.slots_mode=mode; G.slots_confirm=0;
    peek_slots();
    int pick = (G.slot>=1 && G.slot<=3)? G.slot-1 : 0;
    if (mode==0 && !G.slot_has[pick+1])
        for (int s=1;s<=3;s++) if (G.slot_has[s]){ pick=s-1; break; }
    G.slots_sel = pick;
    G.state=GS_SLOTS;
}
static void slots_activate(int i);   // defined with the menu code below
static void slots_input(void){
    if (IN.menu_up){ G.slots_sel=(G.slots_sel+3)%4; G.slots_confirm=0; }
    if (IN.menu_down){ G.slots_sel=(G.slots_sel+1)%4; G.slots_confirm=0; }
    if (IN.esc_pressed){ G.state=GS_MENU; return; }
    if (IN.tapped){
        for (int i=0;i<3;i++){ int y=116+i*52;
            if (IN.tap_y>=y-4 && IN.tap_y<=y+48){
                if (G.slots_mode==1 && G.slot_has[i+1] && G.slots_confirm!=i+1){
                    G.slots_sel=i; G.slots_confirm=i+1; return;
                }
                G.slots_sel=i; slots_activate(i); return;
            }
        }
        if (IN.tap_y>=272 && IN.tap_y<=300){ G.state=GS_MENU; return; }
    }
    if (IN.select_pressed || IN.interact_pressed) slots_activate(G.slots_sel);
}

// ---------------------------------------------------------------------------
// Networking — one packet format for both platforms.
//   client->host: 'J' name | 'I' keys,aim,weapon | 'B' item | 'L' leave
//   host->client: 'R' roster | 'A' your-id | 'S' snapshot | 'W' walls | 'G' over
// ---------------------------------------------------------------------------
static u8 pkt[65536];
