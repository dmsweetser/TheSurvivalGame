// THE SURVIVAL GAME — modes/craft/craft_render.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from craft.c lines 996-1486 in the Nov 2026 fine-grained refactor; code unchanged.

static void glow_ellipse(int cx, int cy, int rx, int ry, u32 c, int alpha){
    for (int pass=0; pass<2; pass++){
        int prx = pass? rx*2/3 : rx, pry = pass? ry*2/3 : ry, al = pass? alpha : alpha/2;
        if (prx<1) prx=1; if (pry<1) pry=1;
        for (int yy=cy-pry; yy<=cy+pry; yy++){
            float t=(float)(yy-cy)/(float)pry;
            int dx=(int)(prx*fsqrt(1.0f-t*t));
            for (int xx=cx-dx; xx<=cx+dx; xx++) pset_blend(xx, yy, c, al);
        }
    }
}
// row labels shared by the list rows and the selection detail strip,
// so the two can never disagree about a name or a price
static const char *craft_row_name(int id){
    if (id==103) return "EAT LAKE FISH (+35% HP)";
    if (id==100) return "EAT HEART PIE (+50% HP)";
    if (id==101) return "EAT CARAPACE STEW (+50% HP, SWIFT)";
    if (id==102) return "DEMOLISH MODE (HALF BACK)";
    return CREC[id].name;
}
static const char *craft_row_shortname(int id){
    if (id==103) return "LAKE FISH";
    if (id==100) return "HEART PIE";
    if (id==101) return "CARAPACE STEW";
    if (id==102) return "DEMOLISH MODE";
    return CREC[id].name;
}
static void craft_row_status(Buddy *b, int id, char *st){
    char *sq=st;
    if (id==103){ sq=sappend(sq, "HAVE "); sq=sapp_int(sq, b->craft_fish); }
    else if (id==100){ sq=sappend(sq, "HAVE "); sq=sapp_int(sq, b->craft_pies); }
    else if (id==101){ sq=sappend(sq, "HAVE "); sq=sapp_int(sq, b->craft_stews); }
    else if (id==102){ sq=sappend(sq, "ARM"); }
    else {
        const CRec *R = &CREC[id];
        int owned = R->build<0 && R->bit && (b->craft_gear & R->bit);
        int built = R->build>=0 && R->bit && (b->craft_gear & R->bit);
        if (owned) sq=sappend(sq, "OWNED");
        else if (built){ sq=sappend(sq, "BUILT "); sq=sapp_int(sq, craft_build_kind_count(b, R->build)); }
        else if (R->req && !(b->craft_gear & R->req)){ sq=sappend(sq, "NEED "); sq=sappend(sq, craft_reqname(R->req)); }
        else if (id==30){ if (b->craft_fish>0){ sq=sappend(sq, "FISH "); sq=sapp_int(sq, b->craft_fish); } else sq=sappend(sq, "NEED A FISH"); }
        else {
            if (R->w){ sq=sappend(sq, "W"); sq=sapp_int(sq, R->w); sq=sappend(sq, " "); }
            if (R->s){ sq=sappend(sq, "S"); sq=sapp_int(sq, R->s); sq=sappend(sq, " "); }
            if (R->f){ sq=sappend(sq, "F"); sq=sapp_int(sq, R->f); sq=sappend(sq, " "); }
            if (R->c){ sq=sappend(sq, "C"); sq=sapp_int(sq, R->c); }
        }
    }
    *sq=0;
}
static u32 craft_row_color(Buddy *b, int id){
    if (id==102) return COL(255,180,130);
    if (id>=100) return COL(240,244,250);
    const CRec *R = &CREC[id];
    int owned = R->build<0 && R->bit && (b->craft_gear & R->bit);
    int built = R->build>=0 && R->bit && (b->craft_gear & R->bit);
    if (owned || built) return COL(250,215,130);
    if (R->req && !(b->craft_gear & R->req)) return COL(240,150,130);
    if (id==30 && b->craft_fish<1) return COL(140,146,150);
    return craft_afford(b, R)? COL(240,244,250) : COL(140,146,150);
}
// little pictograms for the crafting list — you scan pictures, not text
static void craft_icon(int x, int y, int id){
    switch (id){
        case 0: case 2:   // axes (the stone one wears grey)
            line_thick(x+3, y+11, x+9, y+4, 2, COL(140,96,54));
            rect_fill(x+7, y+1, 6, 5, id==0? COL(120,124,132) : COL(150,152,158)); break;
        case 1: case 3:   // picks (the carapace one burns orange)
            line_thick(x+6, y+12, x+6, y+4, 2, COL(140,96,54));
            line_thick(x+1, y+5, x+6, y+1, 2, id==1? COL(150,152,158) : COL(255,150,60));
            line_thick(x+11, y+5, x+6, y+1, 2, id==1? COL(150,152,158) : COL(255,150,60)); break;
        case 4: case 7:   // rafts (the sail one flies a triangle)
            rect_fill(x+1, y+8, 12, 3, COL(156,116,66)); rect_fill(x+1, y+4, 12, 3, COL(146,106,58));
            if (id==7){ line_thick(x+7, y+8, x+7, y+0, 1, COL(110,76,44)); rect_fill(x+8, y+1, 4, 5, COL(240,238,225)); } break;
        case 5:   // lantern
            rect_fill(x+4, y+3, 6, 8, COL(240,210,110)); rect_line(x+4, y+3, 6, 8, COL(90,80,60)); disc_fill(x+7, y+7, 1, COL(255,250,220)); break;
        case 6:   // fishing rod
            line_thick(x+2, y+12, x+11, y+2, 2, COL(140,96,54)); line_thick(x+11, y+2, x+11, y+7, 1, COL(220,226,235)); disc_fill(x+11, y+9, 1, COL(255,120,130)); break;
        case 8:   // sword
            line_thick(x+3, y+11, x+10, y+3, 2, COL(210,214,220)); rect_fill(x+2, y+9, 5, 2, COL(198,164,84)); break;
        case 9:   // spear
            line_thick(x+2, y+12, x+10, y+4, 2, COL(140,96,54)); line_thick(x+10, y+4, x+12, y+1, 2, COL(255,170,80)); break;
        case 10: case 12:   // zappers (the storm one crackles twice)
            rect_fill(x+4, y+5, 6, 6, COL(120,124,134)); disc_fill(x+7, y+3, 2, COL(120,190,255));
            if (id==12) disc_fill(x+11, y+6, 2, COL(160,215,255)); break;
        case 11:   // boomerang
            disc_fill(x+3, y+9, 2, COL(230,160,70)); disc_fill(x+7, y+4, 2, COL(230,160,70)); disc_fill(x+11, y+9, 2, COL(230,160,70)); break;
        case 13: case 14: case 15:   // armor tiers
            rect_fill(x+3, y+3, 8, 9, id==13? COL(170,190,110) : id==14? COL(140,146,152) : COL(255,150,60));
            rect_fill(x+5, y+3, 4, 3, COL(18,22,20));
            if (id!=13) rect_fill(x+3, y+7, 8, 2, COL(90,94,102)); break;
        case 16: case 23:   // walls (the reinforced one wears iron)
            rect_fill(x+1, y+3, 12, 9, id==16? COL(156,110,62) : COL(120,84,48)); rect_fill(x+1, y+7, 12, 2, COL(96,64,32));
            if (id==23){ rect_fill(x+1, y+3, 2, 9, COL(96,100,108)); rect_fill(x+11, y+3, 2, 9, COL(96,100,108)); } break;
        case 17:   // stone wall
            rect_fill(x+1, y+3, 12, 9, COL(138,142,150)); disc_fill(x+4, y+6, 1, COL(96,100,108)); disc_fill(x+10, y+9, 1, COL(96,100,108)); break;
        case 18: case 19:   // fires
            line_thick(x+3, y+11, x+11, y+9, 2, COL(110,76,44));
            ellipse_fill(x+7, y+6, 3, 5, COL(255,140,50)); disc_fill(x+7, y+6, 1, COL(255,215,110));
            if (id==19){ rect_fill(x+1, y+3, 2, 9, COL(104,108,116)); rect_fill(x+11, y+3, 2, 9, COL(104,108,116)); } break;
        case 20:   // torch post
            rect_fill(x+6, y+5, 2, 7, COL(110,76,44)); ellipse_fill(x+7, y+3, 2, 3, COL(255,160,60)); break;
        case 21:   // dock
            rect_fill(x+1, y+4, 12, 3, COL(156,116,66));
            line_thick(x+1, y+10, x+4, y+9, 1, COL(120,190,235)); line_thick(x+5, y+11, x+8, y+10, 1, COL(120,190,235)); line_thick(x+9, y+10, x+12, y+9, 1, COL(120,190,235)); break;
        case 22:   // bedroll
            rect_fill(x+2, y+5, 10, 6, COL(150,60,60)); rect_fill(x+2, y+7, 10, 1, COL(240,230,200)); rect_fill(x+2, y+4, 4, 3, COL(240,244,250)); break;
        case 24:   // lantern post
            rect_fill(x+6, y+4, 2, 8, COL(84,66,44)); rect_fill(x+4, y+1, 6, 5, COL(240,210,110)); break;
        case 25:   // spike trap
            line_thick(x+3, y+11, x+4, y+4, 2, COL(168,172,180)); line_thick(x+7, y+11, x+7, y+3, 2, COL(168,172,180)); line_thick(x+11, y+11, x+10, y+4, 2, COL(120,124,132)); break;
        case 26:   // zapper tower
            rect_fill(x+3, y+6, 8, 6, COL(120,124,134)); disc_fill(x+7, y+3, 3, COL(120,190,255)); break;
        case 27:   // scarecrow
            line_thick(x+7, y+12, x+7, y+5, 2, COL(110,76,44)); rect_fill(x+2, y+5, 10, 2, COL(110,76,44)); disc_fill(x+7, y+3, 3, COL(255,150,50)); break;
        case 28: case 100:   // pies
            disc_fill(x+7, y+7, 5, COL(226,170,90)); line_thick(x+4, y+5, x+10, y+9, 1, COL(170,120,60)); line_thick(x+10, y+5, x+4, y+9, 1, COL(170,120,60)); break;
        case 29: case 101:   // stew pot
            ellipse_fill(x+7, y+8, 6, 4, COL(90,94,102)); rect_fill(x+3, y+4, 8, 2, COL(140,74,54)); break;
        case 30: case 103:   // fish
            ellipse_fill(x+6, y+7, 5, 3, COL(140,200,235)); line_thick(x+10, y+7, x+13, y+4, 2, COL(110,170,210)); line_thick(x+10, y+7, x+13, y+10, 2, COL(110,170,210)); disc_fill(x+4, y+6, 1, COL(20,30,40)); break;
        case 102:   // demolish hammer
            line_thick(x+4, y+12, x+9, y+6, 2, COL(140,96,54)); rect_fill(x+7, y+1, 6, 5, COL(255,150,90)); break;
        default:
            disc_fill(x+7, y+7, 3, COL(200,200,205)); break;
    }
}
static void craft_render(int pi){
    BwSim *S=&BWS[pi]; Buddy *b=&G.bud[pi];
    G.camx=b->x; G.camy=b->y;
    int vx=G.view_x0, vy=G.view_y0, vw2=G.view_w>0? G.view_w : VW, vh2=G.view_h>0? G.view_h : VH;
    int tx0=bw_tileof(b->x - vw2/2 - BW_TILE), tx1=bw_tileof(b->x + vw2/2 + BW_TILE);
    int ty0=bw_tileof(b->y - vh2/2 - BW_TILE), ty1=bw_tileof(b->y + vh2/2 + BW_TILE);
    for (int ty=ty0; ty<=ty1; ty++) for (int tx=tx0; tx<=tx1; tx++){
        int x=SX(tx*BW_TILE), y=SY(ty*BW_TILE);
        if (x+BW_TILE<vx || x>vx+vw2 || y+BW_TILE<vy || y>vy+vh2) continue;
        int biome = craft_biome_at(b, tx, ty);
        u32 h = hash2(tx, ty, craft_seed(b) ^ 0x7A55u);
        if (biome==5){
            u32 base = ((tx+ty)&1)==0? COL(44,104,164) : COL(38,96,154);
            rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.94f + (float)((h>>20)%14)*0.01f));
            if (h % 7 == 0){ int wxp = x + (int)((h>>6) % 30) + 6 + (int)(G.anim_t*6 + (h%40)) % 13 - 6,
                                wyp = y + (int)((h>>11) % 32) + 8;
                line_thick(wxp, wyp, wxp+4, wyp-2, 1, COL(150,200,235)); line_thick(wxp+4, wyp-2, wxp+8, wyp, 1, COL(150,200,235)); }
            { int dbi = craft_build_at(b, tx, ty);   // a dock rides the water tile itself
              if (dbi>=0 && b->craft_bk[dbi]==7){
                  rect_fill(x+1, y+8, BW_TILE-2, 30, COL(156,116,66));
                  rect_fill(x+1, y+17, BW_TILE-2, 3, COL(120,86,44));
                  rect_fill(x+1, y+28, BW_TILE-2, 3, COL(120,86,44));
                  rect_line(x+1, y+8, BW_TILE-2, 30, COL(96,66,34));
                  disc_fill(x+6, y+12, 1, COL(70,70,74)); disc_fill(x+BW_TILE-7, y+12, 1, COL(70,70,74));
                  disc_fill(x+6, y+33, 1, COL(70,70,74)); disc_fill(x+BW_TILE-7, y+33, 1, COL(70,70,74));
              } }
            // foam where water meets land (glow-up): a dashed lip
            // that breathes on the same slow clock as the sparkles
            { int fo = ((int)(G.anim_t*2) + ((h>>5)&1)) & 1; u32 fc = COL(225,242,250);
              if (craft_biome_at(b, tx, ty-1)!=5){ for (int s2=0;s2<3;s2++) rect_fill(x+4+s2*14+fo*4, y+1, 8, 2, fc); }
              if (craft_biome_at(b, tx, ty+1)!=5){ for (int s2=0;s2<3;s2++) rect_fill(x+4+s2*14+((fo^1)*4), y+BW_TILE-3, 8, 2, fc); }
              if (craft_biome_at(b, tx-1, ty)!=5){ for (int s2=0;s2<3;s2++) rect_fill(x+1, y+4+s2*14+fo*4, 2, 8, fc); }
              if (craft_biome_at(b, tx+1, ty)!=5){ for (int s2=0;s2<3;s2++) rect_fill(x+BW_TILE-3, y+4+s2*14+((fo^1)*4), 2, 8, fc); } }
            continue;
        }
        { u32 base;
          if (biome==0) base = ((tx+ty)&1)==0? COL(106,180,82) : COL(96,168,72);
          else if (biome==1) base = ((tx+ty)&1)==0? COL(66,134,60) : COL(58,124,52);
          else if (biome==2) base = ((tx+ty)&1)==0? COL(219,192,124) : COL(209,182,114);
          else if (biome==3) base = ((tx+ty)&1)==0? COL(228,234,240) : COL(216,224,232);
          else base = ((tx+ty)&1)==0? COL(90,112,68) : COL(80,102,60);
          rect_fill(x, y, BW_TILE+1, BW_TILE+1, col_shade(base, 0.94f + (float)((h>>20)%14)*0.01f));
          // sparse darker freckles so the flat biomes have grain (glow-up)
          if ((h>>3)%9==0){ u32 sc2 = col_shade(base, 0.86f);
              pset(x + (int)((h>>6)%38) + 5, y + (int)((h>>11)%38) + 5, sc2);
              pset(x + (int)((h>>16)%30) + 9, y + (int)((h>>21)%30) + 9, sc2); }
          // shore: land kissing water gets a sandy rim
          if (craft_biome_at(b, tx+1, ty)==5 || craft_biome_at(b, tx-1, ty)==5 || craft_biome_at(b, tx, ty+1)==5 || craft_biome_at(b, tx, ty-1)==5)
              rect_fill(x, y+BW_TILE-6, BW_TILE+1, 6, COL(226,204,140));
        }
        int nd = craft_node_at(pi, tx, ty);
        if (nd==1){   // a tree (a snowy pine in the snow)
            shadow_ellipse(x+24, y+41, 12, 3);
            rect_fill(x+21, y+26, 6, 14, COL(110,76,44));
            if (biome==3){ disc_fill(x+24, y+16, 13, COL(52,110,96)); disc_fill(x+24, y+10, 9, COL(66,128,112)); disc_fill(x+24, y+6, 5, COL(240,246,250)); }
            else { disc_fill(x+17, y+20, 10, COL(52,128,54)); disc_fill(x+31, y+20, 10, COL(58,138,58)); disc_fill(x+24, y+13, 11, COL(70,156,66)); }
        } else if (nd==2){
            ellipse_fill(x+24, y+30, 13, 9, COL(142,146,152));
            ellipse_fill(x+19, y+25, 7, 5, COL(164,168,174));
            disc_fill(x+27, y+24, 2, COL(110,114,120));
        } else if (nd==3){
            rect_fill(x+19, y+12, 10, 30, COL(74,160,84));
            rect_fill(x+12, y+20, 7, 4, COL(74,160,84)); rect_fill(x+12, y+16, 4, 8, COL(74,160,84));
            rect_fill(x+29, y+24, 7, 4, COL(74,160,84)); rect_fill(x+33, y+20, 4, 8, COL(74,160,84));
            disc_fill(x+24, y+10, 3, COL(240,120,150));
        } else if (nd==4){
            for (int g2=0;g2<5;g2++){ int gx2 = x+10+g2*7; line_thick(gx2, y+38, gx2 + (int)(fsin(G.anim_t*2 + g2*1.3f + tx)*2), y+24 - (g2%3)*3, 1, biome==4? COL(120,160,80) : COL(150,200,90)); }
        }
        int bi2 = craft_build_at(b, tx, ty);
        if (bi2>=0){
            int bk = b->craft_bk[bi2], bhp = b->craft_bhp[bi2], bmax = craft_buildhp_max(bk);
            if (bk!=7) shadow_ellipse(x+24, y+41, 20, 4);   // builds sit ON the ground now (glow-up)
            if (bk==0){   // wooden wall
                rect_fill(x+2, y+8, BW_TILE-4, 30, COL(156,110,62));
                rect_fill(x+2, y+20, BW_TILE-4, 3, COL(120,82,42));
                rect_line(x+2, y+8, BW_TILE-4, 30, COL(96,64,32));
                disc_fill(x+7, y+13, 1, COL(70,70,74)); disc_fill(x+BW_TILE-8, y+13, 1, COL(70,70,74));
                if (bhp < bmax){ line_thick(x+8, y+14, x+24, y+30, 1, COL(70,44,20)); line_thick(x+26, y+12, x+18, y+26, 1, COL(70,44,20)); }
            } else if (bk==2){   // stone wall
                rect_fill(x+2, y+8, BW_TILE-4, 30, COL(138,142,150));
                rect_fill(x+2, y+22, BW_TILE-4, 3, COL(104,108,116));
                rect_line(x+2, y+8, BW_TILE-4, 30, COL(78,82,90));
                disc_fill(x+12, y+15, 2, COL(96,100,108)); disc_fill(x+32, y+28, 2, COL(96,100,108));
                if (bhp < bmax*2/3){ line_thick(x+10, y+12, x+22, y+28, 1, COL(60,62,70)); }
                if (bhp < bmax/3){ line_thick(x+30, y+10, x+20, y+30, 1, COL(60,62,70)); line_thick(x+6, y+30, x+24, y+18, 1, COL(60,62,70)); }
            } else if (bk==1 || bk==3){   // campfire / fireplace
                int cxp=x+24, cyp=y+24;
                if (bk==3){ ellipse_fill(cxp, cyp+5, 15, 8, COL(104,108,116)); ellipse_fill(cxp, cyp+3, 11, 6, COL(70,72,80)); }
                else ellipse_fill(cxp, cyp+4, 13, 6, COL(120,124,128));
                line_thick(cxp-8, cyp+5, cxp+8, cyp+2, 3, COL(110,76,44)); line_thick(cxp-8, cyp+2, cxp+8, cyp+5, 3, COL(96,64,32));
                int fl = (int)(fsin(G.anim_t*11 + bi2*2)*2);
                int fh = bk==3? 13 : 9;
                ellipse_fill(cxp, cyp-4, bk==3? 8:6, fh+fl, COL(255,140,50));
                ellipse_fill(cxp, cyp-3, bk==3? 5:3, fh-3+fl, COL(255,205,90));
            } else if (bk==4){   // zapper tower
                int cxp=x+24, cyp=y+24;
                rect_fill(cxp-9, cyp-2, 18, 16, COL(120,124,134)); rect_line(cxp-9, cyp-2, 18, 16, COL(80,84,94));
                rect_fill(cxp-3, cyp-12, 6, 11, COL(96,100,110));
                int pu = 3 + (int)(fsin(G.anim_t*5 + bi2)*1.5f);
                disc_fill(cxp, cyp-15, pu+2, COL(120,190,255)); disc_fill(cxp, cyp-15, 2, COL(230,246,255));
                if (bhp < bmax){ line_thick(cxp-7, cyp+2, cxp+4, cyp+12, 1, COL(60,62,70)); }
            } else if (bk==5){   // spike trap: quiet grey teeth in the grass
                for (int s3=0;s3<4;s3++){ int sxp = x+8+s3*10;
                    line_thick(sxp, y+36, sxp+3, y+22, 2, COL(168,172,180)); line_thick(sxp+6, y+36, sxp+3, y+22, 2, COL(120,124,132)); }
            } else if (bk==6){   // torch post
                int cxp=x+24, cyp=y+24;
                rect_fill(cxp-2, cyp-2, 4, 16, COL(110,76,44));
                int fl = (int)(fsin(G.anim_t*13 + bi2*3)*1.5f);
                ellipse_fill(cxp, cyp-6, 4, 6+fl, COL(255,160,60));
                disc_fill(cxp, cyp-6, 2, COL(255,225,140));
            } else if (bk==10){   // reinforced wall: dark planks, iron bands
                rect_fill(x+2, y+8, BW_TILE-4, 30, COL(120,84,48));
                rect_fill(x+2, y+20, BW_TILE-4, 3, COL(88,60,30));
                rect_fill(x+2, y+8, 5, 30, COL(96,100,108)); rect_fill(x+BW_TILE-7, y+8, 5, 30, COL(96,100,108));
                rect_line(x+2, y+8, BW_TILE-4, 30, COL(70,48,24));
                if (bhp < bmax){ line_thick(x+10, y+12, x+26, y+32, 1, COL(56,36,16)); line_thick(x+28, y+10, x+16, y+28, 1, COL(56,36,16)); }
            } else if (bk==8){   // bedroll: a striped blanket, waiting
                rect_fill(x+8, y+16, 32, 18, COL(150,60,60));
                rect_fill(x+8, y+21, 32, 3, COL(240,230,200)); rect_fill(x+8, y+28, 32, 3, COL(240,230,200));
                rect_fill(x+8, y+13, 12, 6, COL(240,244,250));
            } else if (bk==9){   // the scarecrow stands its watch
                int cxp=x+24, cyp=y+24;
                rect_fill(cxp-2, cyp-4, 4, 20, COL(110,76,44));
                rect_fill(cxp-11, cyp-1, 22, 3, COL(110,76,44));
                disc_fill(cxp, cyp-10, 6, COL(255,150,50));
                rect_fill(cxp-7, cyp-16, 14, 3, COL(96,64,32)); rect_fill(cxp-3, cyp-19, 6, 4, COL(96,64,32));
                disc_fill(cxp-2, cyp-10, 1, COL(30,26,24)); disc_fill(cxp+2, cyp-10, 1, COL(30,26,24));
                if (bhp < bmax){ line_thick(cxp-8, cyp+2, cxp+6, cyp+12, 1, COL(70,44,20)); }
            } else if (bk==11){   // lantern post: taller, glassed, patient
                int cxp=x+24, cyp=y+24;
                rect_fill(cxp-2, cyp-4, 4, 20, COL(84,66,44));
                rect_fill(cxp-5, cyp-13, 10, 10, COL(240,210,110));
                rect_line(cxp-5, cyp-13, 10, 10, COL(90,80,60));
                disc_fill(cxp, cyp-8, 2, COL(255,240,190));
                rect_fill(cxp-6, cyp-15, 12, 2, COL(70,56,36));
            }
        }
    }
    // the supply cache, when one has drifted in
    if (S->craft_cache_on){
        int x=SX(S->craft_cache_x), y=SY(S->craft_cache_y);
        shadow_ellipse(x, y+8, 12, 3);
        rect_fill(x-11, y-8, 22, 15, COL(168,120,58)); rect_line(x-11, y-8, 22, 15, COL(110,76,34));
        rect_fill(x-11, y-2, 22, 3, COL(110,76,34));
        disc_fill(x, y-1, 2, COL(250,220,120));
    }
    // feral bugs: sun-bleached, gloom-dark, or golden
    for (int i=0;i<S->craft_bugn;i++){
        if (S->craft_bhp[i]<=0) continue;
        int x=SX(S->craft_bx[i]), y=SY(S->craft_by[i]);
        int bk = S->craft_bk[i];
        shadow_ellipse(x, y+8, 11, 3);
        if (bk==2){
            ellipse_fill(x, y, 11, 8, COL(232,180,60));
            disc_fill(x-4, y-3, 2, COL(255,255,255)); disc_fill(x+4, y-3, 2, COL(120,80,10));
            if (((int)(G.anim_t*3)+i)&1) disc_fill(x+7, y-8, 1, COL(255,246,200));
        } else if (bk==1){
            ellipse_fill(x, y, 12, 9, COL(74,44,96));
            disc_fill(x-4, y-3, 2, COL(255,70,60)); disc_fill(x+4, y-3, 2, COL(255,70,60));
        } else {
            ellipse_fill(x, y, 11, 8, COL(140,74,54));
            disc_fill(x-4, y-3, 2, COL(255,230,120)); disc_fill(x+4, y-3, 2, COL(255,230,120));
        }
        for (int l=0;l<3;l++){ int ph = (int)(fsin(G.anim_t*10 + i + l*2)*2); line_thick(x-9+l*8, y+5, x-11+l*8, y+9+ph, 1, COL(80,44,30)); }
    }
    // fins in the water — and the grey behind them
    for (int i=0;i<S->craft_shn;i++){
        if (S->craft_sh_hp[i]<=0) continue;
        int x=SX(S->craft_sh_x[i]), y=SY(S->craft_sh_y[i]);
        // the body stays UNDER the surface — a grey ghost in the
        // blue, drawn blended — and only the fin breaks the water,
        // pointing where he's actually headed (Daniel, Nov 6: they
        // rendered weird and showed up on the sand)
        glow_ellipse(x, y+3, 16, 7, COL(74,88,102), 120);
        float hd = S->craft_sh_hd[i];
        int px = (int)(-fsin(hd)*6), tipx = x + (int)(fcos(hd)*3);
        line_thick(x+px, y-1, tipx, y-11, 3, COL(122,134,148));
        line_thick(tipx, y-11, x-px, y-1, 3, COL(104,116,130));
    }
    // zapper bolts and tower beams
    for (int i=0;i<4;i++) if (S->craft_bolt_a[i]){
        int x=SX(S->craft_bolt_x[i]), y=SY(S->craft_bolt_y[i]);
        disc_fill(x, y, 4, COL(255,150,50)); disc_fill(x, y, 2, COL(255,225,150));
    }
    if (S->craft_bmr_a){   // the boomerang, mid-flight, spinning
        int x=SX(S->craft_bmr_x), y=SY(S->craft_bmr_y);
        float a = G.anim_t*22;
        for (int k2=0;k2<3;k2++){ float aa=a+k2*2.09f; disc_fill(x+(int)(fcos(aa)*5), y+(int)(fsin(aa)*5), 3, COL(230,160,70)); }
        disc_fill(x, y, 2, COL(255,220,150));
    }
    if (S->craft_beam_t>0){
        line_thick(SX(S->craft_beam_x1), SY(S->craft_beam_y1), SX(S->craft_beam_x2), SY(S->craft_beam_y2), 2, COL(170,215,255));
        line_thick(SX(S->craft_beam_x1), SY(S->craft_beam_y1), SX(S->craft_beam_x2), SY(S->craft_beam_y2), 1, COL(240,250,255));
    }
    // the raft, when he's afloat (not when the dock is under him)
    { int dbi2 = craft_build_at(b, bw_tileof(b->x), bw_tileof(b->y));
      if (craft_biome_at(b, bw_tileof(b->x), bw_tileof(b->y))==5 && (b->craft_gear&512) && !(dbi2>=0 && b->craft_bk[dbi2]==7)){
        int x=SX(b->x), y=SY(b->y);
        ellipse_fill(x, y+9, 20, 7, COL(146,102,58));
        rect_fill(x-18, y+6, 36, 3, COL(120,82,42));
        line_thick(x-14, y+3, x-14, y+12, 2, COL(110,76,44)); line_thick(x+12, y+3, x+12, y+12, 2, COL(110,76,44));
        if (b->craft_gear&262144){ line_thick(x+3, y+5, x+3, y-13, 2, COL(110,76,44)); rect_fill(x+4, y-12, 10, 13, COL(240,238,225)); rect_line(x+4, y-12, 10, 13, COL(190,186,170)); }
      } }
    draw_buddy(b);
    // night falls over everything but the fires
    float dk = craft_darkf(b);
    if (dk>0.02f){
        blend_rect(vx, vy, vw2, vh2, COL(8,10,28), (int)(dk*165));
        for (int i=0;i<b->craft_bn;i++){
            int bk = b->craft_bk[i];
            int cxp=SX(b->craft_btx[i]*BW_TILE+24), cyp=SY(b->craft_bty[i]*BW_TILE+24);
            if (bk==1) glow_ellipse(cxp, cyp, 92, 70, COL(255,170,80), (int)(95*dk));
            else if (bk==3) glow_ellipse(cxp, cyp, 128, 96, COL(255,176,84), (int)(105*dk));
            else if (bk==6) glow_ellipse(cxp, cyp, 82, 62, COL(255,190,96), (int)(85*dk));
            else if (bk==4) glow_ellipse(cxp, cyp-8, 40, 30, COL(140,200,255), (int)(55*dk));
            else if (bk==11) glow_ellipse(cxp, cyp, 122, 94, COL(255,200,110), (int)(95*dk));
        }
        glow_ellipse(SX(b->x), SY(b->y), (b->craft_gear&8192)? 88 : 56, (b->craft_gear&8192)? 68 : 44, COL(255,220,160), (int)(70*dk));
    }
    // HUD: two rows, nothing shares a line. Row 1: the pack (left)
    // and the hour (right). Row 2: the HP bar, then your gear. The old
    // one-row version right-aligned the gear INTO the pack text once
    // you owned a few tools — worse in split view (Daniel, Nov 5).
    rect_fill(vx, vy, vw2, 34, COL(16,20,28));
    { char buf[160]; char *q=buf;
      q=sappend(q, "WOOD "); q=sapp_int(q, b->craft_wood);
      q=sappend(q, "  STONE "); q=sapp_int(q, b->craft_stone);
      q=sappend(q, "  FIBER "); q=sapp_int(q, b->craft_fiber);
      q=sappend(q, "  CARAPACE "); q=sapp_int(q, b->craft_car);
      if (b->craft_fish>0){ q=sappend(q, "  FISH "); q=sapp_int(q, b->craft_fish); }
      if (b->craft_pies>0){ q=sappend(q, "  PIES "); q=sapp_int(q, b->craft_pies); }
      if (b->craft_stews>0){ q=sappend(q, "  STEWS "); q=sapp_int(q, b->craft_stews); }
      *q=0; draw_text(vx+8, vy+5, buf, 1, COL(240,244,250)); }
    { const char *tn = dk>0.55f? "NIGHT" : b->craft_tod<15 || b->craft_tod>=225? "DAWN" : b->craft_tod>=140? "DUSK" : "DAY";
      u32 tc = tn[0]=='N'? COL(170,190,240) : tn[1]=='A' && tn[0]=='D'? COL(250,200,130) : COL(255,226,140);
      draw_text(vx+vw2-8-text_w(tn,1), vy+5, tn, 1, tc); }
    rect_fill(vx+8, vy+21, 104, 6, COL(60,30,34));
    rect_fill(vx+8, vy+21, (int)(104 * (b->maxhp>0? b->hp/b->maxhp : 0)), 6, COL(230,84,96));
    { char buf[160]; char *q=buf;
      if (b->craft_gear&1) q=sappend(q, "AXE ");
      if (b->craft_gear&2) q=sappend(q, "PICK ");
      if (b->craft_gear&16) q=sappend(q, "S.AXE ");
      if (b->craft_gear&32) q=sappend(q, "C.PICK ");
      if (b->craft_gear&8192) q=sappend(q, "LAMP ");
      if (b->craft_gear&16384) q=sappend(q, "ROD ");
      if (b->craft_gear&512) q=sappend(q, "RAFT ");
      if (b->craft_gear&262144) q=sappend(q, "SAIL ");
      if (b->craft_gear&4) q=sappend(q, "SWD ");
      if (b->craft_gear&64) q=sappend(q, "SPEAR ");
      if (b->craft_gear&32768) q=sappend(q, "BOOM ");
      if (b->craft_gear&128) q=sappend(q, "ZAP ");
      if (b->craft_gear&65536) q=sappend(q, "S.ZAP ");
      if (b->craft_gear&8) q=sappend(q, "ARM ");
      if (b->craft_gear&256) q=sappend(q, "R.ARM ");
      if (b->craft_gear&131072) q=sappend(q, "SHELL");
      *q=0; if (q!=buf) draw_text(vx+124, vy+20, buf, 1, COL(250,215,130)); }
    if (S->craft_demolish>0 && !S->craft_menu){
        char buf[80]; char *q=buf; q=sappend(q, "DEMOLISH ARMED - "); q=sappend(q, lbl_int()); q=sappend(q, " REMOVES A BUILD"); *q=0;
        draw_text_center(vx+vw2/2, vy+40, buf, 1, COL(255,180,130));
    }
    if (S->sleep_t>0){
        float d = 1.0f - (S->sleep_t>0.7f? (S->sleep_t-0.7f)/0.7f : S->sleep_t/0.7f);
        blend_rect(vx, vy, vw2, vh2, COL(6,6,16), (int)(210*d));
        if (d>0.75f) draw_text_center(vx + vw2/2, vy + vh2/2-4, "Z Z Z", 2, COL(200,210,255));
    }
    if (S->craft_menu){
        int rows[64]; int n = craft_menu_rows(S, b, rows);
        int visn = n<7? n : 7;   // the list scrolls: seven rows, never a wall of text
        int w=380, h=40+visn*20+16+24+22;   // +24: the selection detail strip; +22: the search row
        int x=vx+(vw2-w)/2, y=vy+(vh2-h)/2;
        if (w>vw2-8){ w=vw2-8; x=vx+4; }
        rect_fill(x, y, w, h, COL(18,22,20)); rect_line(x, y, w, h, COL(198,164,84));
        draw_text_center(x+w/2, y+8, "CRAFTING", 2, COL(255,214,64));
        // the search row: tap it (or press P) and type — the list
        // filters flat to matching items, a mercy on a phone
        { int sy2 = y+28;
          rect_fill(x+10, sy2, w-20, 17, S->craft_search? COL(52,46,22) : COL(24,28,24));
          rect_line(x+10, sy2, w-20, 17, S->craft_search? COL(255,220,90) : COL(90,96,90));
          char sb[48]; char *q=sb; q=sappend(q, "SEARCH: ");
          if (S->craft_filter[0]) q=sappend(q, S->craft_filter);
          else if (!S->craft_search) q=sappend(q, mobile_ui()? "TAP HERE, THEN TYPE" : "PRESS /, THEN TYPE");
          if (S->craft_search && ((int)(G.anim_t*2.4f)&1)) q=sappend(q, "_");
          *q=0;
          draw_text(x+16, sy2+5, sb, 1, (S->craft_filter[0] || S->craft_search)? COL(255,240,200) : COL(150,170,190)); }
        for (int r=S->craft_scroll;r<S->craft_scroll+visn && r<n;r++){
            int id = rows[r], ry = y+52+(r-S->craft_scroll)*20;
            if (id==-99){   // the filter matched nothing
                draw_text_center(x+w/2, ry, "NOTHING MATCHES - TRY FEWER LETTERS", 1, COL(255,150,130));
                continue;
            }
            if (id<0){   // a segment header: a signpost in the list, never selectable
                rect_fill(x+10, ry-2, w-20, 13, COL(26,32,28));
                draw_text_center(x+w/2, ry, CRAFT_CAT_NAME[-id-1], 1, COL(198,164,84));
                continue;
            }
            char st[64]; craft_row_status(b, id, st);
            u32 colr = craft_row_color(b, id);
            if (S->craft_sel==r){ rect_fill(x+10, ry-3, w-20, 15, COL(70,58,30)); draw_text(x+14, ry, ">", 1, COL(255,220,90)); }
            craft_icon(x+26, ry-1, id);
            draw_text(x+46, ry, craft_row_name(id), 1, colr);
            draw_text(x+w-28-text_w(st,1), ry, st, 1, colr);
        }
        // the scrollbar: a track, and a thumb that says where you are
        if (n>visn){
            int track_y = y+52, track_h = visn*20-8;
            rect_fill(x+w-10, track_y, 3, track_h, COL(52,58,54));
            int th = track_h*visn/n; if (th<12) th=12;
            int ty2 = track_y + (track_h-th)*S->craft_scroll/(n-visn);
            rect_fill(x+w-10, ty2, 3, th, COL(255,220,90));
        }
        // the selection, up close: its icon in a gold chip, its name big —
        // what you are about to craft is never in doubt (Daniel, Nov 5)
        { int sid = rows[S->craft_sel]; int dy2 = y+h-40;
          rect_fill(x+10, dy2, w-20, 24, COL(24,28,24));
          if (sid==-99){
              draw_text(x+38, dy2+5, "NO MATCHES", 2, COL(255,150,130));
          } else {
          char sst[64]; craft_row_status(b, sid, sst);
          rect_line(x+14, dy2+4, 16, 16, COL(198,164,84));
          craft_icon(x+15, dy2+5, sid);
          draw_text(x+38, dy2+5, craft_row_shortname(sid), 2, craft_row_color(b, sid));
          draw_text(x+w-28-text_w(sst,1), dy2+8, sst, 1, COL(200,214,200)); } }
        { char buf[96]; char *q=buf;
          if (S->craft_search){
              draw_text_center(x+w/2, y+h-14, "TYPE TO FILTER - ESC WHEN DONE", 1, COL(150,170,190));
          } else if (mobile_ui() && !pad_mode()){
              draw_text_center(x+w/2, y+h-14, G.touch_ctl? "STICK PICKS - FIRE CRAFTS - TAP ROWS TOO" : "TAP A ROW - TAP IT AGAIN TO CRAFT - TAP OUTSIDE TO CLOSE", 1, COL(150,170,190));
          } else {
              q=sappend(q, pad_mode()? "DPAD/STICK" : "ARROWS"); q=sappend(q, " PICK - ");
              q=sappend(q, lbl_fire()); q=sappend(q, " CRAFTS - ");
              q=sappend(q, lbl_wall()); q=sappend(q, " CLOSES"); *q=0;
              draw_text_center(x+w/2, y+h-14, buf, 1, COL(150,170,190)); } }
    }
    // (no standing hint line here any more — controls live in the
    // Start-menu's CONTROLS panel; Daniel, Nov 6)
    // one contextual prompt (Daniel, Nov 6): what INTERACT would do if
    // pressed this instant, in the update's own priority order, named
    // for the device in hand. The note bar outranks it when speaking.
    if (!S->craft_menu && !S->bw_menu && S->note_t<=0 && S->sleep_t<=0){
        const char *verb = 0;
        int ttx = bw_tileof(b->x + fcos(b->aim)*34), tty = bw_tileof(b->y + fsin(b->aim)*34);
        if (S->craft_demolish>0) verb = "DEMOLISH";
        else if (craft_fire_near(b, b->x, b->y, 84, 0, 0) || craft_bedroll_near(b)) verb = "REST";
        else if ((b->craft_gear&16384) && craft_biome_at(b, ttx, tty)==5) verb = "FISH";
        else if (S->craft_cache_on){
            float dx=S->craft_cache_x-b->x, dy=S->craft_cache_y-b->y;
            if (dx*dx+dy*dy < 50*50) verb = "LOOT THE CACHE";
        }
        if (!verb){ int nd = craft_node_at(pi, ttx, tty);
            if (nd==1) verb = "CHOP"; else if (nd==2) verb = "MINE"; else if (nd) verb = "GATHER"; }
        if (verb){ char hb[72]; const char *hint = key_prompt(hb, lbl_int(), verb);
            int hw2 = text_w(hint, 1);
            // anchored like the main game's prompt (VH-34): in this
            // render the -22 slot lands half off the frame's bottom
            rect_fill(vx + vw2/2 - hw2/2 - 6, vy + vh2-38, hw2+12, 14, COL(20,26,20));
            draw_text_center(vx + vw2/2, vy + vh2-35, hint, 1, COL(240,248,240)); }
    }
    if (S->note_t>0){
        rect_fill(vx, vy+vh2-22, vw2, 22, COL(14,14,20));
        draw_text_center(vx+vw2/2, vy+vh2-15, S->note, 1, COL(255,240,200));
    }
}


