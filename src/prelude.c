// [UNITY-PIECE-BANNER]
// ===========================================================================
//  PIECE: prelude.c — platform interface, libc-free helpers, framebuffer renderer, bitmap font.
//  UNITY-INCLUDED by survival.c — not a standalone translation unit; do not compile directly.
// ===========================================================================
// [/UNITY-PIECE-BANNER]

#include <stdint.h>
#include <stddef.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int32_t  i32;

// ---------------------------------------------------------------------------
// Platform interface. The game core calls these; each platform provides them.
// ---------------------------------------------------------------------------
// audio: note event (channel, midi note, velocity, wave/instrument id)
void plat_note(int ch, int note, int vel, int instr);
// storage: persist / load the save block (NUL-terminated text)
void plat_save(const char *data, int len);
int  plat_load(char *buf, int cap);            // returns length or -1
// per-slot run storage: slot 1..3 (the global blob above is slot 0)
void plat_save_slot(int slot, const char *data, int len);
int  plat_load_slot(int slot, char *buf, int cap);
// "TheSurvivalGame_save.txt" for slot 0, "..._save1.txt" .. for slots
static void save_slot_name(char *out, int slot){
    const char *base = "TheSurvivalGame_save";
    int i=0; while (base[i]){ out[i]=base[i]; i++; }
    if (slot>=1 && slot<=3) out[i++] = (char)('0'+slot);
    const char *ext = ".txt"; int j=0; while (ext[j]) out[i++]=ext[j++];
    out[i]=0;
}
// network: mode 0=off 1=host 2=client. Packets are opaque byte blobs.
int  plat_net_start(int mode, const char *addr); // 0 ok
void plat_net_stop(void);
int  plat_net_send(const u8 *data, int len, int dest); // dest: -1 broadcast (host), 0 host (client)
int  plat_net_recv(u8 *buf, int cap, int *src);        // returns len or 0
// time in seconds (monotonic) and a "quit" flag for native builds
double plat_now(void);
// the local civil date as a day number (days since 1970-01-01, local
// time) — the WEEKLY CHALLENGE's only clock (v1.25 feat 6). plat_now
// is monotonic and never learns the date; this one does nothing else.
int plat_daynum(void);
// the local time of day as minutes since midnight (0..1440, seconds
// folded in as a fraction) — the LIVING SKY's only clock (v1.25 feat
// 12). Cosmetic only: nothing but colour ever reads it.
float plat_minutes(void);
// where friends point a browser to join this machine's game
// ("192.168.1.42:8379"), or 0 on builds that cannot host
const char *plat_join_addr(void);
// Whales Only: open a purchase page in the player's browser, and pick
// up an unlock code the player dropped off (web: ?whale= link / native:
// a whale-code.txt file beside the saves). poll returns length or 0.
void plat_open_url(const char *url);
int  plat_whale_poll(char *buf, int cap);
// native builds: start the built-in web server that serves the game
// page and relays browser guests into this host (called once at launch)
void web_server_start(void);
static void sfx(int id);   // sound effects, implemented with the audio section
static void try_buy(int pi, int sel);  // store purchases, implemented below
static void station_action(int pi, int station, int row);   // mall stations, below
static void draw_active_panel(void);                        // the open panel, below
static void ui_vignette(int x0, int y0, int w, int h);      // glow-up shading, below
static void bowl_update(int pi, float dt);                  // Bug Bowl tick, below
static void draw_bowling(void);                             // Bug Bowl view, below
static int act2_down(int a);                                // pad-2 held state, below
static void push_ev(int type, float x, float y); // net event queue (host)
static float client_aim=0; static int client_weapon=0, client_want_wall=0;
static int client_joined=0;   // client has sent its 'J' join packet
static float wall_send_t=0;   // host wall-broadcast timer (net section)

// ---------------------------------------------------------------------------
// Small libc-free helpers
// ---------------------------------------------------------------------------
static void *smemcpy(void *d, const void *s, int n){ u8 *a=(u8*)d; const u8 *b=(const u8*)s; for(int i=0;i<n;i++) a[i]=b[i]; return d; }
static void *smemset(void *d, int v, int n){ u8 *a=(u8*)d; for(int i=0;i<n;i++) a[i]=(u8)v; return d; }
static int slen(const char *s){ int n=0; while(s[n]) n++; return n; }
static int scmp(const char *a, const char *b){ while(*a && *a==*b){ a++; b++; } return (u8)*a - (u8)*b; }
static void scpy(char *d, const char *s, int cap){ int i=0; while(s[i] && i<cap-1){ d[i]=s[i]; i++; } d[i]=0; }
static float ffabs(float x){ return x<0?-x:x; }
static int   iabs(int x){ return x<0?-x:x; }
static float fclamp(float x, float lo, float hi){ return x<lo?lo:(x>hi?hi:x); }
static int   iclamp(int x, int lo, int hi){ return x<lo?lo:(x>hi?hi:x); }
static float flerpf(float a, float b, float t){ return a+(b-a)*t; }

// fixed-iteration Newton sqrt: bounded, no data-dependent loops (wasm-safe)
static float fsqrt(float x){
    if (x<=0.0f) return 0.0f;
    union { float f; u32 u; } v; v.f = x;
    v.u = (v.u>>1) + 0x1FC00000u;             // rough seed
    float y = v.f;
    for (int i=0;i<6;i++) y = 0.5f*(y + x/y);
    return y;
}
#define PI 3.14159265f
#define TAU 6.2831853f
// polynomial sin/cos on [-PI,PI]; angles wrapped with bounded arithmetic
static float sin_poly(float x){
    // fold into [-PI/2, PI/2] first (exact symmetry): the Taylor series is
    // poor near +-PI, and this keeps worst-case error under ~0.001.
    if (x > PI*0.5f) x = PI - x;
    else if (x < -PI*0.5f) x = -PI - x;
    float x2 = x*x;
    return x*(0.99997f + x2*(-0.16658f + x2*(0.00825f + x2*(-0.00019f))));
}
static float wrap_pi(float x){
    // full turn reduction in one cast step (no loops): correct for any
    // |x| up to ~1e6, which covers every phase accumulator in the game.
    // (An earlier version only corrected two turns and silently exploded
    //  past 3*PI — Taylor sin of -12 rad is in the thousands.)
    float k = (float)(int)(x/TAU + (x>=0? 0.5f : -0.5f));
    x -= k*TAU;
    if (x >  PI) x -= TAU; else if (x < -PI) x += TAU;
    return x;
}
static float fsin(float x){ return sin_poly(wrap_pi(x)); }
static float fcos(float x){ return sin_poly(wrap_pi(x + PI*0.5f)); }
static float fatan2(float y, float x){
    if (x==0.0f && y==0.0f) return 0.0f;
    float ax = ffabs(x), ay = ffabs(y);
    float ang;
    if (ax >= ay){
        float t = ay/ax; float s2 = t*t;
        float rr = ((-0.0464964749f*s2 + 0.15931422f)*s2 - 0.327622764f)*s2*t + t;
        ang = rr;
        if (x<0) ang = PI - ang;
        if (y<0) ang = -ang;
    } else {
        float t = ax/ay; float s2 = t*t;
        float rr = ((-0.0464964749f*s2 + 0.15931422f)*s2 - 0.327622764f)*s2*t + t;
        ang = PI*0.5f - rr;
        if (x<0) ang = PI - ang;
        if (y<0) ang = -ang;
    }
    return ang;
}

// RNG (all unsigned math — defined wraparound, see AGENTS.md wasm lessons)
static u32 rng_state = 0x2545F491u;
static void rng_seed(u32 s){ rng_state = s ? s : 0x2545F491u; }
static u32 rng_u32(void){ u32 x = rng_state; x ^= x<<13; x ^= x>>17; x ^= x<<5; rng_state = x; return x; }
static float rng_f(void){ return (float)(rng_u32()>>8) * (1.0f/16777216.0f); }
static float rng_range(float lo, float hi){ return lo + rng_f()*(hi-lo); }
static u32 hash2(i32 x, i32 y, u32 seed){
    u32 h = seed + (u32)x*374761393u + (u32)y*668265263u;
    h = (h ^ (h>>13)) * 1274126177u;
    return h ^ (h>>16);
}
static float hash_f(i32 x, i32 y, u32 seed){ return (float)(hash2(x,y,seed)>>8) * (1.0f/16777216.0f); }

// ---------------------------------------------------------------------------
// Framebuffer renderer — 640x360, cartoon shapes with dark outlines.
// Pixel layout: 0xFFBBGGRR stored little-endian == RGBA bytes in memory,
// which is exactly what both the JS ImageData blit and a Win32 32bpp DIB want.
// ---------------------------------------------------------------------------
#define VW 640
#define VH 360
// Render scale (Nov 3, 2026, Daniel): the framebuffer is RS times the
// logical resolution — 1280x720 physical. ALL game code keeps drawing
// in logical 640x360 coordinates; the primitives below scale into the
// big buffer, and the font is a true 16x16 raster, so text gets real
// extra detail while shapes keep the chunky pixel-art read.
#define RS 2
#define PW (VW*RS)
#define PH (VH*RS)
static u32 fb[PW*PH];
// split-screen clip: all primitives confine drawing to the current
// view's rectangle, [clip_x0, clip_x1) x [clip_y0, clip_y1). Full
// screen (the default) changes nothing anywhere.
static int clip_x0=0, clip_x1=VW;
static int clip_y0=0, clip_y1=VH;
#ifndef PLAT_WASM
// Native blitters (Win32 DIB, X11 XImage, macOS CGBitmapContext with
// byteOrder32Little) all want B,G,R,A bytes in memory:
#define COL(r,g,b) (0xFF000000u | ((u32)(r)<<16) | ((u32)(g)<<8) | (u32)(b))
#else
// JS ImageData wants R,G,B,A bytes in memory:
#define COL(r,g,b) (0xFF000000u | ((u32)(b)<<16) | ((u32)(g)<<8) | (u32)(r))
#endif
#define OUTLINE COL(28,24,38)

static void pset(int x, int y, u32 c){
    if (x<clip_x0 || x>=clip_x1 || y<clip_y0 || y>=clip_y1) return;
    int px=x*RS, py=y*RS;
    for (int yy=0; yy<RS; yy++){ u32 *row=&fb[(py+yy)*PW+px]; for (int xx=0; xx<RS; xx++) row[xx]=c; }
}
static void rect_fill(int x, int y, int w, int h, u32 c){
    int x0=iclamp(x,clip_x0,clip_x1), y0=iclamp(y,clip_y0,clip_y1), x1=iclamp(x+w,clip_x0,clip_x1), y1=iclamp(y+h,clip_y0,clip_y1);
    int px0=x0*RS, px1=x1*RS;
    for (int yy=y0*RS; yy<y1*RS; yy++){ u32 *row=&fb[yy*PW]; for (int xx=px0; xx<px1; xx++) row[xx]=c; }
}
// one logical row-span of a disc/ellipse raster, written at full res
static void span_fill(int yy, int x0, int x1, u32 c){
    int px0=x0*RS, px1=(x1+1)*RS;
    for (int py=yy*RS; py<(yy+1)*RS; py++){ u32 *row=&fb[py*PW]; for (int xx=px0; xx<px1; xx++) row[xx]=c; }
}
static void rect_line(int x, int y, int w, int h, u32 c){
    rect_fill(x,y,w,1,c); rect_fill(x,y+h-1,w,1,c); rect_fill(x,y,1,h,c); rect_fill(x+w-1,y,1,h,c);
}
static void circle_fill(int cx, int cy, int r, u32 c){
    if (r<0) r=0;
    if (cx+r < clip_x0 || cx-r >= clip_x1) return;   // fully outside the split view
    if (cy+r < clip_y0 || cy-r >= clip_y1) return;
    int y0=iclamp(cy-r,clip_y0,clip_y1-1), y1=iclamp(cy+r,clip_y0,clip_y1-1);
    int rr = r*r;
    for (int yy=y0; yy<=y1; yy++){
        int dy = yy-cy; int dx = (int)fsqrt((float)(rr-dy*dy));
        if (cx+dx < clip_x0 || cx-dx >= clip_x1) continue;
        int x0=iclamp(cx-dx,clip_x0,clip_x1-1), x1=iclamp(cx+dx,clip_x0,clip_x1-1);
        span_fill(yy, x0, x1, c);
    }
}
static void circle_out(int cx, int cy, int r, u32 fill){ // outlined cartoon circle
    circle_fill(cx, cy, r+2, OUTLINE);
    circle_fill(cx, cy, r, fill);
}
static void ellipse_fill(int cx, int cy, int rx, int ry, u32 c){
    if (rx<1) rx=1; if (ry<1) ry=1;
    if (cx+rx < clip_x0 || cx-rx >= clip_x1) return;
    if (cy+ry < clip_y0 || cy-ry >= clip_y1) return;
    int y0=iclamp(cy-ry,clip_y0,clip_y1-1), y1=iclamp(cy+ry,clip_y0,clip_y1-1);
    for (int yy=y0; yy<=y1; yy++){
        float t = (float)(yy-cy)/(float)ry;
        int dx = (int)(rx*fsqrt(1.0f-t*t));
        if (cx+dx < clip_x0 || cx-dx >= clip_x1) continue;
        int x0=iclamp(cx-dx,clip_x0,clip_x1-1), x1=iclamp(cx+dx,clip_x0,clip_x1-1);
        span_fill(yy, x0, x1, c);
    }
}
static void ellipse_out(int cx, int cy, int rx, int ry, u32 fill){
    ellipse_fill(cx, cy, rx+2, ry+2, OUTLINE);
    ellipse_fill(cx, cy, rx, ry, fill);
}
static void line_thick(int x0, int y0, int x1, int y1, int r, u32 c){
    int dx=x1-x0, dy=y1-y0;
    int steps = iabs(dx)>iabs(dy)? iabs(dx) : iabs(dy);
    if (steps<1) steps=1;
    for (int i=0;i<=steps;i++){
        int x = x0 + dx*i/steps, y = y0 + dy*i/steps;
        circle_fill(x, y, r, c);
    }
}
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

// ---------------------------------------------------------------------------
// 8x8 bitmap font, generated from Atkinson Hyperlegible (the Braille
// Institute's typeface designed for low-vision readers — distinct
// I/l/1, open apertures) by tools/genfont.py: the Bold face is
// rasterized with a high ink threshold, which erodes the strokes to
// a regular visual weight — full bold at 8px was too heavy to read.
// Rows are bytes, MSB = leftmost pixel. Glyphs: ASCII 32..126.
// ---------------------------------------------------------------------------
static const u16 FONT16[96][16] = {
/*   */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
 /* ! */ {0x0000,0x0000,0x0000,0x0000,0x0380,0x0380,0x0380,0x0380,0x0380,0x0100,0x0000,0x0100,0x0380,0x0380,0x0000,0x0000},
 /* " */ {0x0000,0x0000,0x0000,0x0000,0x06C0,0x06C0,0x06C0,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
 /* # */ {0x0000,0x0000,0x0000,0x0000,0x0660,0x0660,0x1FF8,0x1FF8,0x0E60,0x0CE0,0x3FF0,0x3FF0,0x0CC0,0x0CC0,0x0000,0x0000},
 /* $ */ {0x0000,0x0000,0x0100,0x0100,0x0FE0,0x1FE0,0x1D00,0x1F00,0x0FC0,0x03F0,0x0170,0x0D70,0x0FE0,0x07C0,0x0100,0x0000},
 /* % */ {0x0000,0x0000,0x0000,0x0000,0x1C60,0x3440,0x26C0,0x3680,0x1D30,0x0B78,0x02C8,0x06C8,0x0478,0x0C70,0x0000,0x0000},
 /* & */ {0x0000,0x0000,0x0000,0x0700,0x0FC0,0x0CC0,0x0CC0,0x0F80,0x0F30,0x1FB0,0x39F0,0x38E0,0x1FE0,0x0FF0,0x0000,0x0000},
 /* ' */ {0x0000,0x0000,0x0000,0x0000,0x0380,0x0180,0x0100,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
 /* ( */ {0x0000,0x0000,0x0000,0x0000,0x0180,0x0300,0x0300,0x0300,0x0600,0x0600,0x0600,0x0700,0x0300,0x0300,0x0380,0x0180},
 /* ) */ {0x0000,0x0000,0x0000,0x0000,0x0380,0x0180,0x0180,0x01C0,0x00C0,0x00C0,0x00C0,0x00C0,0x0180,0x0180,0x0180,0x0300},
 /* * */ {0x0000,0x0000,0x0000,0x0000,0x0100,0x07C0,0x0380,0x0380,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
 /* + */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0100,0x0100,0x0100,0x0FE0,0x0FF0,0x0380,0x0100,0x0100,0x0000,0x0000},
 /* , */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0100,0x0380,0x0380,0x0100,0x0100},
 /* - */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x07C0,0x07C0,0x0000,0x0000,0x0000,0x0000,0x0000},
 /* . */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0100,0x0380,0x0380,0x0000,0x0000},
 /* / */ {0x0000,0x0000,0x0000,0x0000,0x00C0,0x00C0,0x01C0,0x0180,0x0180,0x0300,0x0300,0x0700,0x0600,0x0600,0x0000,0x0000},
 /* 0 */ {0x0000,0x0000,0x0000,0x0000,0x07C0,0x0EE0,0x1E70,0x1F70,0x1F30,0x1DF0,0x1CF0,0x0C70,0x0FE0,0x07C0,0x0000,0x0000},
 /* 1 */ {0x0000,0x0000,0x0000,0x0000,0x00C0,0x07C0,0x07C0,0x00C0,0x00C0,0x00C0,0x00C0,0x00C0,0x00C0,0x00C0,0x0000,0x0000},
 /* 2 */ {0x0000,0x0000,0x0000,0x0000,0x07E0,0x0E60,0x0060,0x0060,0x00E0,0x01C0,0x0380,0x0E00,0x0FE0,0x0FF0,0x0000,0x0000},
 /* 3 */ {0x0000,0x0000,0x0000,0x0000,0x07E0,0x0CE0,0x0060,0x00E0,0x03C0,0x01E0,0x0060,0x0460,0x0FE0,0x07C0,0x0000,0x0000},
 /* 4 */ {0x0000,0x0000,0x0000,0x0000,0x00C0,0x01C0,0x03C0,0x07C0,0x0EC0,0x0CE0,0x1FF0,0x1FF0,0x00C0,0x00C0,0x0000,0x0000},
 /* 5 */ {0x0000,0x0000,0x0000,0x0000,0x0FE0,0x0FE0,0x0C00,0x0FC0,0x0FE0,0x0460,0x0070,0x0460,0x0FE0,0x07C0,0x0000,0x0000},
 /* 6 */ {0x0000,0x0000,0x0000,0x0000,0x07E0,0x0E40,0x0C00,0x0FC0,0x0FE0,0x1C60,0x0C70,0x0C60,0x0FE0,0x07C0,0x0000,0x0000},
 /* 7 */ {0x0000,0x0000,0x0000,0x0000,0x0FE0,0x0FE0,0x00E0,0x00C0,0x01C0,0x01C0,0x0380,0x0380,0x0300,0x0700,0x0000,0x0000},
 /* 8 */ {0x0000,0x0000,0x0000,0x0000,0x07C0,0x06C0,0x0440,0x06C0,0x07C0,0x0EE0,0x1C70,0x1C70,0x0FE0,0x07C0,0x0000,0x0000},
 /* 9 */ {0x0000,0x0000,0x0000,0x0000,0x07C0,0x0FE0,0x0C60,0x0C60,0x0E60,0x07E0,0x03C0,0x01C0,0x0180,0x0380,0x0000,0x0000},
 /* : */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0100,0x0380,0x0380,0x0000,0x0000,0x0100,0x0380,0x0380,0x0000,0x0000},
 /* ; */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0100,0x0380,0x0380,0x0000,0x0000,0x0100,0x0380,0x0380,0x0100,0x0100},
 /* < */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x00E0,0x03E0,0x0F00,0x0E00,0x07C0,0x01E0,0x0020,0x0000,0x0000},
 /* = */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0FF0,0x0FE0,0x0000,0x0FF0,0x0FE0,0x0000,0x0000,0x0000},
 /* > */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0800,0x0E00,0x0F80,0x01E0,0x00E0,0x07C0,0x0F00,0x0800,0x0000,0x0000},
 /* ? */ {0x0000,0x0000,0x0000,0x0000,0x0FE0,0x0EE0,0x0070,0x00E0,0x03C0,0x0300,0x0000,0x0300,0x0300,0x0300,0x0000,0x0000},
 /* @ */ {0x0000,0x0000,0x0000,0x0000,0x07E0,0x1830,0x1390,0x26C8,0x24C8,0x24D8,0x17F0,0x1220,0x0800,0x07C0,0x0000,0x0000},
 /* A */ {0x0000,0x0000,0x0000,0x0000,0x0380,0x03C0,0x07C0,0x06C0,0x0EE0,0x0C60,0x0FE0,0x1FF0,0x1C30,0x3838,0x0000,0x0000},
 /* B */ {0x0000,0x0000,0x0000,0x0000,0x1FE0,0x1FE0,0x1C70,0x1C60,0x1FE0,0x1FF0,0x1C70,0x1C70,0x1FF0,0x1FC0,0x0000,0x0000},
 /* C */ {0x0000,0x0000,0x0000,0x0000,0x07E0,0x0FF0,0x1C20,0x1C00,0x1800,0x1800,0x1C00,0x1C20,0x0FF0,0x07E0,0x0000,0x0000},
 /* D */ {0x0000,0x0000,0x0000,0x0000,0x1FC0,0x1FE0,0x1870,0x1830,0x1830,0x1830,0x1870,0x1870,0x1FE0,0x1F80,0x0000,0x0000},
 /* E */ {0x0000,0x0000,0x0000,0x0000,0x0FF0,0x0FE0,0x0C00,0x0C00,0x0F80,0x0F80,0x0C00,0x0C00,0x0FF0,0x0FF0,0x0000,0x0000},
 /* F */ {0x0000,0x0000,0x0000,0x0000,0x0FE0,0x0FE0,0x0C00,0x0E00,0x0FE0,0x0FE0,0x0C00,0x0C00,0x0C00,0x0C00,0x0000,0x0000},
 /* G */ {0x0000,0x0000,0x0000,0x0000,0x07E0,0x0FF0,0x1C00,0x1800,0x18F0,0x18F8,0x1838,0x1C38,0x0FF8,0x07D8,0x0000,0x0000},
 /* H */ {0x0000,0x0000,0x0000,0x0000,0x1830,0x1830,0x1830,0x1C70,0x1FF0,0x1FF0,0x1830,0x1830,0x1830,0x1830,0x0000,0x0000},
 /* I */ {0x0000,0x0000,0x0000,0x0000,0x07C0,0x07C0,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380,0x07C0,0x07C0,0x0000,0x0000},
 /* J */ {0x0000,0x0000,0x0000,0x0000,0x0060,0x0060,0x0060,0x0060,0x0060,0x0060,0x0060,0x0C60,0x0FE0,0x07C0,0x0000,0x0000},
 /* K */ {0x0000,0x0000,0x0000,0x0000,0x18E0,0x19C0,0x1980,0x1F00,0x1F00,0x1F80,0x1DC0,0x18C0,0x18E0,0x1870,0x0000,0x0000},
 /* L */ {0x0000,0x0000,0x0000,0x0000,0x0C00,0x0C00,0x0C00,0x0C00,0x0C00,0x0C00,0x0C00,0x0C00,0x0FE0,0x0FE0,0x0000,0x0000},
 /* M */ {0x0000,0x0000,0x0000,0x0000,0x3C78,0x3C78,0x3C78,0x3C78,0x36D8,0x36D8,0x33D8,0x3398,0x3398,0x3398,0x0000,0x0000},
 /* N */ {0x0000,0x0000,0x0000,0x0000,0x1C30,0x1E30,0x1E30,0x1F30,0x1B30,0x19B0,0x19F0,0x18F0,0x18F0,0x1870,0x0000,0x0000},
 /* O */ {0x0000,0x0000,0x0000,0x0000,0x0FE0,0x1EF0,0x1830,0x3838,0x3838,0x3838,0x3838,0x1C70,0x1FF0,0x07C0,0x0000,0x0000},
 /* P */ {0x0000,0x0000,0x0000,0x0000,0x1FE0,0x1FF0,0x1C70,0x1C70,0x1FF0,0x1FE0,0x1C00,0x1C00,0x1C00,0x1C00,0x0000,0x0000},
 /* Q */ {0x0000,0x0000,0x0000,0x0000,0x0FC0,0x1FE0,0x3870,0x3830,0x3030,0x39B0,0x39F0,0x18F0,0x1FF0,0x0FF8,0x0000,0x0000},
 /* R */ {0x0000,0x0000,0x0000,0x0000,0x1FC0,0x1FE0,0x1870,0x1870,0x1FE0,0x1FC0,0x19C0,0x19C0,0x18E0,0x1870,0x0000,0x0000},
 /* S */ {0x0000,0x0000,0x0000,0x0000,0x0FE0,0x1EE0,0x1C00,0x1E00,0x0FC0,0x03F0,0x0070,0x0C70,0x0FE0,0x07C0,0x0000,0x0000},
 /* T */ {0x0000,0x0000,0x0000,0x0000,0x1FF0,0x1FF0,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380,0x0000,0x0000},
 /* U */ {0x0000,0x0000,0x0000,0x0000,0x1C30,0x1C30,0x1C30,0x1C30,0x1C30,0x1C30,0x1C70,0x1C70,0x0FE0,0x07C0,0x0000,0x0000},
 /* V */ {0x0000,0x0000,0x0000,0x0000,0x1830,0x1C70,0x0C60,0x0C60,0x0EE0,0x06C0,0x06C0,0x07C0,0x0380,0x0380,0x0000,0x0000},
 /* W */ {0x0000,0x0000,0x0000,0x0000,0x3398,0x3398,0x1390,0x1BB0,0x1AB0,0x1EF0,0x1EF0,0x0EE0,0x0C60,0x0C60,0x0000,0x0000},
 /* X */ {0x0000,0x0000,0x0000,0x0000,0x1C70,0x0EE0,0x0FE0,0x07C0,0x0380,0x03C0,0x07C0,0x0EE0,0x1C70,0x1C70,0x0000,0x0000},
 /* Y */ {0x0000,0x0000,0x0000,0x0000,0x1C70,0x0C70,0x0EE0,0x07C0,0x07C0,0x0380,0x0380,0x0380,0x0380,0x0380,0x0000,0x0000},
 /* Z */ {0x0000,0x0000,0x0000,0x0000,0x1FF0,0x0FE0,0x00E0,0x01C0,0x0380,0x0700,0x0700,0x0E00,0x1FF0,0x1FF0,0x0000,0x0000},
 /* [ */ {0x0000,0x0000,0x0000,0x0000,0x03C0,0x0300,0x0300,0x0300,0x0300,0x0300,0x0300,0x0300,0x0300,0x0300,0x0300,0x03C0},
 /* \ */ {0x0000,0x0000,0x0000,0x0000,0x0600,0x0600,0x0700,0x0300,0x0380,0x0180,0x0180,0x01C0,0x00C0,0x00C0,0x0000,0x0000},
 /* ] */ {0x0000,0x0000,0x0000,0x0000,0x0380,0x0180,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0080,0x0180,0x0380},
 /* ^ */ {0x0000,0x0000,0x0000,0x0000,0x0380,0x0380,0x07C0,0x06C0,0x0C60,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
 /* _ */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0FE0,0x0FE0},
 /* ` */ {0x0000,0x0000,0x0000,0x0300,0x0180,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000},
 /* a */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x03C0,0x07E0,0x0060,0x01E0,0x0FE0,0x0C60,0x0EE0,0x0F60,0x0000,0x0000},
 /* b */ {0x0000,0x0000,0x0000,0x0000,0x0C00,0x0C00,0x0DC0,0x0FE0,0x0C60,0x0C60,0x0C70,0x0C60,0x0FE0,0x0FC0,0x0000,0x0000},
 /* c */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0380,0x07E0,0x0E40,0x0C00,0x0C00,0x0E00,0x0FE0,0x07C0,0x0000,0x0000},
 /* d */ {0x0000,0x0000,0x0000,0x0000,0x0060,0x0060,0x0760,0x0FE0,0x0C60,0x1C60,0x1C60,0x0C60,0x0FE0,0x07F0,0x0000,0x0000},
 /* e */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0380,0x07E0,0x0C60,0x0FE0,0x0FE0,0x0C00,0x0FE0,0x07C0,0x0000,0x0000},
 /* f */ {0x0000,0x0000,0x0000,0x01C0,0x03C0,0x0300,0x07C0,0x07C0,0x0380,0x0300,0x0300,0x0300,0x0300,0x0300,0x0000,0x0000},
 /* g */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0720,0x0FF0,0x0C70,0x1C70,0x0C70,0x0E70,0x0FF0,0x0370,0x0060,0x07E0},
 /* h */ {0x0000,0x0000,0x0000,0x0000,0x0C00,0x0C00,0x0DC0,0x0FE0,0x0E60,0x0C60,0x0C60,0x0C60,0x0C60,0x0C60,0x0000,0x0000},
 /* i */ {0x0000,0x0000,0x0080,0x0180,0x0180,0x0000,0x0380,0x0380,0x0180,0x0180,0x0180,0x0180,0x0180,0x0180,0x0000,0x0000},
 /* j */ {0x0000,0x0000,0x0080,0x01C0,0x0180,0x0000,0x0080,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x01C0,0x0780},
 /* k */ {0x0000,0x0000,0x0000,0x0000,0x0C00,0x0C00,0x0C60,0x0DC0,0x0D80,0x0F00,0x0F80,0x0FC0,0x0CC0,0x0CE0,0x0000,0x0000},
 /* l */ {0x0000,0x0000,0x0000,0x0000,0x0300,0x0300,0x0300,0x0300,0x0300,0x0300,0x0300,0x0300,0x0380,0x03C0,0x0000,0x0000},
 /* m */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x3770,0x3FF8,0x3398,0x3398,0x3398,0x3398,0x3398,0x3398,0x0000,0x0000},
 /* n */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0DC0,0x0FE0,0x0E60,0x0C60,0x0C60,0x0C60,0x0C60,0x0C60,0x0000,0x0000},
 /* o */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0380,0x0FE0,0x0C60,0x0C70,0x1C70,0x0C60,0x0FE0,0x07C0,0x0000,0x0000},
 /* p */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x09C0,0x0FE0,0x0C60,0x0C60,0x0C70,0x0C60,0x0FE0,0x0FC0,0x0C00,0x0C00},
 /* q */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0E40,0x1FE0,0x1CE0,0x18E0,0x18E0,0x18E0,0x1FE0,0x0FE0,0x00E0,0x00F8},
 /* r */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x06C0,0x07C0,0x0780,0x0700,0x0600,0x0600,0x0600,0x0600,0x0000,0x0000},
 /* s */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0380,0x0FE0,0x0C00,0x0F80,0x07E0,0x0060,0x0EE0,0x07C0,0x0000,0x0000},
 /* t */ {0x0000,0x0000,0x0000,0x0000,0x0100,0x0380,0x07C0,0x07C0,0x0380,0x0380,0x0380,0x0380,0x03C0,0x01C0,0x0000,0x0000},
 /* u */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0C60,0x0C60,0x0C60,0x0C60,0x0C60,0x0C60,0x0FE0,0x0760,0x0000,0x0000},
 /* v */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0C60,0x0C60,0x0E60,0x06C0,0x06C0,0x07C0,0x0380,0x0380,0x0000,0x0000},
 /* w */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x3118,0x3398,0x3BB8,0x1BB0,0x1EF0,0x1EF0,0x1EE0,0x0E60,0x0000,0x0000},
 /* x */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0C60,0x0EE0,0x07C0,0x0380,0x0380,0x07C0,0x0EE0,0x0C60,0x0000,0x0000},
 /* y */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0C60,0x0C60,0x0EE0,0x06C0,0x07C0,0x03C0,0x0380,0x0380,0x0300,0x0700},
 /* z */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0FE0,0x0FE0,0x01E0,0x01C0,0x0380,0x0700,0x0FE0,0x0FE0,0x0000,0x0000},
 /* { */ {0x0000,0x0000,0x0000,0x0000,0x01C0,0x0380,0x0300,0x0300,0x0300,0x0700,0x0700,0x0300,0x0300,0x0300,0x0380,0x01C0},
 /* | */ {0x0000,0x0000,0x0000,0x0000,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380,0x0380},
 /* } */ {0x0000,0x0000,0x0000,0x0000,0x0700,0x0380,0x0180,0x0180,0x0180,0x01C0,0x01C0,0x0180,0x0180,0x0180,0x0380,0x0700},
 /* ~ */ {0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x07E0,0x0FE0,0x0000,0x0000,0x0000,0x0000,0x0000},
};
static void draw_char(int x, int y, char ch, int scale, u32 c){
    int idx = (int)(u8)ch - 32;
    if (idx<0 || idx>=96) idx = 31; // '?'
    const u16 *g = FONT16[idx];
    // physical-space blit: the glyph cell is 16 source px spanning
    // 8*scale logical px, so one source px is exactly `scale` physical
    // px. Runs of set bits merge into single fills.
    int pcx0 = clip_x0*RS, pcx1 = clip_x1*RS, pcy0 = clip_y0*RS, pcy1 = clip_y1*RS;
    for (int row=0; row<16; row++){
        u16 bits = g[row];
        if (!bits) continue;
        int py = y*RS + row*scale;
        if (py+scale<=pcy0 || py>=pcy1) continue;
        int ay0 = py<pcy0? pcy0 : py, ay1 = py+scale>pcy1? pcy1 : py+scale;
        int col = 0;
        while (col<16){
            if (bits & (0x8000>>col)){
                int cs = col;
                while (col<16 && (bits & (0x8000>>col))) col++;
                int fx = x*RS + cs*scale, fw = (col-cs)*scale;
                int ax0 = fx<pcx0? pcx0 : fx, ax1 = fx+fw>pcx1? pcx1 : fx+fw;
                for (int yy=ay0; yy<ay1; yy++){ u32 *r2=&fb[yy*PW]; for (int xx=ax0; xx<ax1; xx++) r2[xx]=c; }
            } else col++;
        }
    }
}
static int text_w(const char *s, int scale){ return slen(s)*8*scale; }
// Text is drawn BARE — no drop shadow, no outline halo. Both were
// tried for readability and both read as BOLD at this size (Daniel,
// Nov 1 2026); the Atkinson letterforms at a regular weight carry
// the readability on their own.
static void draw_text(int x, int y, const char *s, int scale, u32 c){
    for (int i=0; s[i]; i++) draw_char(x+i*8*scale, y, s[i], scale, c);
}
static void draw_text_plain(int x, int y, const char *s, int scale, u32 c){
    draw_text(x, y, s, scale, c);
}
static void draw_text_center(int cx, int y, const char *s, int scale, u32 c){
    draw_text(cx - text_w(s,scale)/2, y, s, scale, c);
}
// draw_text_fit: the HUD's fit guarantee. If the string is wider than
// maxw pixels at this scale, it is trimmed with a ".." tail so what is
// drawn NEVER spills past the box it belongs to.
static void draw_text_fit(int x, int y, int maxw, const char *s, int scale, u32 c){
    if (text_w(s, scale) <= maxw){ draw_text(x, y, s, scale, c); return; }
    char buf[96]; int n=0;
    while (s[n] && n<94){ buf[n]=s[n]; n++; }
    buf[n]=0;
    while (n>0 && (n+2)*8*scale > maxw) buf[--n]=0;
    buf[n]='.'; buf[n+1]='.'; buf[n+2]=0;
    draw_text(x, y, buf, scale, c);
}
static void draw_text_center_fit(int cx, int y, int maxw, const char *s, int scale, u32 c){
    if (text_w(s, scale) <= maxw){ draw_text_center(cx, y, s, scale, c); return; }
    char buf[96]; int n=0;
    while (s[n] && n<94){ buf[n]=s[n]; n++; }
    buf[n]=0;
    while (n>0 && (n+2)*8*scale > maxw) buf[--n]=0;
    buf[n]='.'; buf[n+1]='.'; buf[n+2]=0;
    draw_text_center(cx, y, buf, scale, c);
}
// tiny string builders (no printf in freestanding land)
static char *sappend(char *p, const char *s){ while(*s) *p++ = *s++; *p = 0; return p; }
static char *sapp_int(char *p, int v){
    char tmp[16]; int n=0;
    if (v<0){ *p++='-'; v=-v; }
    if (v==0) tmp[n++]='0';
    while (v>0){ tmp[n++] = (char)('0' + v%10); v/=10; }
    while (n>0) *p++ = tmp[--n];
    *p = 0; return p;
}
static void fmt_int(char *out, const char *pre, int v, const char *post){
    char *p = sappend(out, pre); p = sapp_int(p, v); sappend(p, post);
}

