// THE SURVIVAL GAME — core/render.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from prelude.c lines 154-245 in the Nov 2026 fine-grained refactor; code unchanged.

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
