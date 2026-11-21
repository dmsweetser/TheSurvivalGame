// THE SURVIVAL GAME — core/color.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from prelude.c lines 246-349 in the Nov 2026 fine-grained refactor; code unchanged.

// --- shading helpers: the "smoothed, detailed" pass. Channel access must
// respect each platform's COL() byte order, so it goes through macros.
#ifndef PLAT_WASM
#define COL_R(c) ((int)(((c)>>16)&0xFF))
#define COL_G(c) ((int)(((c)>>8)&0xFF))
#define COL_B(c) ((int)((c)&0xFF))
#else
#define COL_R(c) ((int)((c)&0xFF))
#define COL_G(c) ((int)(((c)>>8)&0xFF))
#define COL_B(c) ((int)(((c)>>16)&0xFF))
#endif
static u32 col_shade(u32 c, float f){
    int r=(int)(COL_R(c)*f), g=(int)(COL_G(c)*f), b=(int)(COL_B(c)*f);
    return COL(iclamp(r,0,255), iclamp(g,0,255), iclamp(b,0,255));
}
// linear mix toward another colour, f in 0..1 (v1.25 feat 12: the
// sky tints; generally useful)
static u32 col_mix(u32 a, u32 b, float f){
    if (f<=0.0f) return a; if (f>=1.0f) return b;
    int r=(int)(COL_R(a) + (COL_R(b)-COL_R(a))*f);
    int g=(int)(COL_G(a) + (COL_G(b)-COL_G(a))*f);
    int bl=(int)(COL_B(a) + (COL_B(b)-COL_B(a))*f);
    return COL(iclamp(r,0,255), iclamp(g,0,255), iclamp(bl,0,255));
}
static void pset_blend(int x, int y, u32 c, int alpha){   // alpha 0..255
    if (x<clip_x0 || x>=clip_x1 || y<clip_y0 || y>=clip_y1 || alpha<=0) return;
    if (alpha>255) alpha=255;
    int ia=255-alpha;
    for (int yy=y*RS; yy<(y+1)*RS; yy++) for (int xx=x*RS; xx<(x+1)*RS; xx++){
        u32 *p=&fb[yy*PW+xx]; u32 d=*p;
        *p = COL((COL_R(c)*alpha + COL_R(d)*ia)/255,
                 (COL_G(c)*alpha + COL_G(d)*ia)/255,
                 (COL_B(c)*alpha + COL_B(d)*ia)/255);
    }
}
static void blend_rect(int x, int y, int w, int h, u32 c, int alpha){
    for (int yy=y; yy<y+h; yy++) for (int xx=x; xx<x+w; xx++) pset_blend(xx, yy, c, alpha);
}
static void shadow_ellipse(int cx, int cy, int rx, int ry){
    // soft two-pass ground shadow
    for (int pass=0; pass<2; pass++){
        int prx = pass? rx*3/4 : rx, pry = pass? ry*3/4 : ry, al = pass? 110:60;
        if (prx<1) prx=1; if (pry<1) pry=1;
        int y0=iclamp(cy-pry,0,VH-1), y1=iclamp(cy+pry,0,VH-1);
        for (int yy=y0; yy<=y1; yy++){
            float t=(float)(yy-cy)/(float)pry;
            int dx=(int)(prx*fsqrt(1.0f-t*t));
            for (int xx=cx-dx; xx<=cx+dx; xx++) pset_blend(xx, yy, COL(18,26,18), al);
        }
    }
}
// a circle lit from the top, like a little sphere: rows run 1.14x bright at
// the top to 0.76x at the bottom, plus a soft white gloss top-left.
static void circle_shade(int cx, int cy, int r, u32 base){
    if (r<1) r=1;
    int y0=iclamp(cy-r,clip_y0,clip_y1-1), y1=iclamp(cy+r,clip_y0,clip_y1-1);
    int rr=r*r;
    for (int yy=y0; yy<=y1; yy++){
        int dy=yy-cy; int dx=(int)fsqrt((float)(rr-dy*dy));
        float f = 1.14f - 0.38f*((float)(dy+r)/(2.0f*r+1.0f));
        u32 c = col_shade(base, f);
        int x0=iclamp(cx-dx,clip_x0,clip_x1-1), x1=iclamp(cx+dx,clip_x0,clip_x1-1);
        span_fill(yy, x0, x1, c);
    }
}
static void ellipse_shade(int cx, int cy, int rx, int ry, u32 base){
    if (rx<1) rx=1; if (ry<1) ry=1;
    int y0=iclamp(cy-ry,clip_y0,clip_y1-1), y1=iclamp(cy+ry,clip_y0,clip_y1-1);
    for (int yy=y0; yy<=y1; yy++){
        float t=(float)(yy-cy)/(float)ry;
        int dx=(int)(rx*fsqrt(1.0f-t*t));
        float f = 1.13f - 0.36f*((float)(yy-(cy-ry))/(2.0f*ry+1.0f));
        u32 c = col_shade(base, f);
        int x0=iclamp(cx-dx,clip_x0,clip_x1-1), x1=iclamp(cx+dx,clip_x0,clip_x1-1);
        span_fill(yy, x0, x1, c);
    }
}
static void gloss(int cx, int cy, int r){
    // soft specular blob toward the top-left of a round thing
    int gx=cx-r/3, gy=cy-r/3;
    int grx=r/3<1?1:r/3, gry=r/4<1?1:r/4;
    int y0=iclamp(gy-gry,0,VH-1), y1=iclamp(gy+gry,0,VH-1);
    for (int yy=y0; yy<=y1; yy++){
        float t=(float)(yy-gy)/(float)gry;
        int dx=(int)(grx*fsqrt(1.0f-t*t));
        for (int xx=gx-dx; xx<=gx+dx; xx++) pset_blend(xx, yy, COL(255,255,255), 95);
    }
}
static void tri_fill(int x0,int y0,int x1,int y1,int x2,int y2,u32 c){
    int rminx = x0<x1?(x0<x2?x0:x2):(x1<x2?x1:x2), rmaxx = x0>x1?(x0>x2?x0:x2):(x1>x2?x1:x2);
    if (rmaxx < clip_x0 || rminx >= clip_x1) return;
    int minx=iclamp(rminx,clip_x0,clip_x1-1), maxx=iclamp(rmaxx,clip_x0,clip_x1-1);
    int miny=iclamp(y0<y1?(y0<y2?y0:y2):(y1<y2?y1:y2),clip_y0,clip_y1-1), maxy=iclamp(y0>y1?(y0>y2?y0:y2):(y1>y2?y1:y2),clip_y0,clip_y1-1);
    int d = (y1-y2)*(x0-x2) + (x2-x1)*(y0-y2);
    if (d==0) return;
    for (int yy=miny; yy<=maxy; yy++)
        for (int xx=minx; xx<=maxx; xx++){
            int w0 = (y1-y2)*(xx-x2) + (x2-x1)*(yy-y2);
            int w1 = (y2-y0)*(xx-x2) + (x0-x2)*(yy-y2);
            int w2 = d - w0 - w1;
            if ((w0>=0&&w1>=0&&w2>=0)||(w0<=0&&w1<=0&&w2<=0)) pset(xx, yy, c);
        }
}

