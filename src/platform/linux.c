// THE SURVIVAL GAME — platform/linux.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from platform.c lines 1460-1727 in the Nov 2026 fine-grained refactor; code unchanged.


// ===========================================================================
// PLATFORM: Linux — X11 window, ALSA synth output, stdio save file.
// ===========================================================================
#ifdef PLAT_LINUX
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/joystick.h>
#include <X11/XKBlib.h>

// Saves live in ONE stable home (~/.local/share) — see the Windows
// backend for why (loose beside-the-exe files lost players' runs).
// The legacy beside-the-exe file is adopted only when the stable
// home has no copy yet, and is then lifted into the stable home;
// it never wins just for being newer (an old build's save set must
// not rewind the canonical one).
static void lin_save_paths(int slot, char *stable, char *legacy){
    save_slot_name(legacy, slot);
    const char *home = getenv("HOME");
    if (home && home[0]) snprintf(stable, 1024, "%s/.local/share/%s", home, legacy);
    else { int i=0; while (legacy[i]){ stable[i]=legacy[i]; i++; } stable[i]=0; }
}
static void lin_save_mkdir(void){
    const char *home = getenv("HOME"); if (!home || !home[0]) return;
    char p[1024];
    snprintf(p, sizeof(p), "%s/.local", home); mkdir(p, 0755);
    snprintf(p, sizeof(p), "%s/.local/share", home); mkdir(p, 0755);
}
void plat_save_slot(int slot, const char *d, int len){
    char sp[1024], lp[64]; lin_save_paths(slot, sp, lp);
    lin_save_mkdir();
    FILE *f = fopen(sp, "wb");
    if (!f) f = fopen(lp, "wb");   // never lose a save
    if (!f) return;
    fwrite(d, 1, (size_t)len, f); fclose(f);
}
int plat_load_slot(int slot, char *buf, int cap){
    char sp[1024], lp[64]; lin_save_paths(slot, sp, lp);
    struct stat ss, ls;
    int hs = stat(sp, &ss)==0, hl = stat(lp, &ls)==0;
    const char *use = 0; int import = 0;
    if (hs) use=sp;                          // the stable home is canonical once it exists
    else if (hl){ use=lp; import=1; }        // one-time migration only
    if (!use) return -1;
    FILE *f = fopen(use, "rb");
    if (!f) return -1;
    int n = (int)fread(buf, 1, (size_t)cap, f); fclose(f);
    if (import){ lin_save_mkdir();
        FILE *w = fopen(sp, "wb");
        if (w){ fwrite(buf, 1, (size_t)n, w); fclose(w); } }
    return n;
}
void plat_save(const char *d, int len){ plat_save_slot(0, d, len); }
int plat_load(char *buf, int cap){ return plat_load_slot(0, buf, cap); }
void plat_open_url(const char *url){   // Whales Only: only buy.stripe.com links ever reach here
    if (!url || strncmp(url, "https://buy.stripe.com/", 23)!=0) return;
    if (fork()==0){ execlp("xdg-open", "xdg-open", url, (char*)0); _exit(1); }
}
int plat_whale_poll(char *buf, int cap){   // a whale-code.txt: cwd, else beside the saves
    char sp[1024]; const char *home = getenv("HOME");
    FILE *f = fopen("whale-code.txt", "rb");
    if (!f && home && home[0]){ snprintf(sp, sizeof(sp), "%s/.local/share/whale-code.txt", home); f = fopen(sp, "rb"); }
    else sp[0]=0;
    if (!f) return 0;
    int n = (int)fread(buf, 1, (size_t)(cap-1), f); fclose(f);
    remove("whale-code.txt");
    if (sp[0]) remove(sp);
    if (n<=0) return 0;
    buf[n]=0; return n;
}

// --- ALSA output thread: pulls the software synth in blocks ---
static void *alsa_thread(void *arg){
    (void)arg;
    snd_pcm_t *pcm=0;
    if (snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)!=0) return 0;
    if (snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                           1, 44100, 1, 120000)!=0){ snd_pcm_close(pcm); return 0; }
    static float fbuf[1024]; static short sbuf[1024];
    double rt = plat_now();
    for (;;){
        synth_render(fbuf, 1024, rt);
        rt += 1024/44100.0;
        for (int i=0;i<1024;i++) sbuf[i] = (short)(fbuf[i]*32767.0f);
        int w = (int)snd_pcm_writei(pcm, sbuf, 1024);
        if (w<0) snd_pcm_prepare(pcm);
    }
    return 0;
}

// --- window / input ---
#define WIN_W 1280
#define WIN_H 720
static Display *g_dpy=0;
static u64 g_raw=0;
static u32 g_scaled[WIN_W*WIN_H];
static void blit_scale(void){
    // bilinear 640x360 -> window size (exactly 2x at the default size)
    for (int y=0;y<WIN_H;y++){
        float sy = (y+0.5f)*(float)PH/WIN_H - 0.5f;
        int y0 = (int)floorf(sy); float fy = sy - y0;
        if (y0<0){ y0=0; fy=0; } if (y0>PH-2){ y0=PH-2; fy=1; }
        for (int x=0;x<WIN_W;x++){
            float sx = (x+0.5f)*(float)PW/WIN_W - 0.5f;
            int x0 = (int)floorf(sx); float fx = sx - x0;
            if (x0<0){ x0=0; fx=0; } if (x0>PW-2){ x0=PW-2; fx=1; }
            u32 c00=fb[y0*PW+x0], c10=fb[y0*PW+x0+1], c01=fb[(y0+1)*PW+x0], c11=fb[(y0+1)*PW+x0+1];
            int r = (int)((COL_R(c00)*(1-fx)*(1-fy) + COL_R(c10)*fx*(1-fy) + COL_R(c01)*(1-fx)*fy + COL_R(c11)*fx*fy) + 0.5f);
            int g = (int)((COL_G(c00)*(1-fx)*(1-fy) + COL_G(c10)*fx*(1-fy) + COL_G(c01)*(1-fx)*fy + COL_G(c11)*fx*fy) + 0.5f);
            int b = (int)((COL_B(c00)*(1-fx)*(1-fy) + COL_B(c10)*fx*(1-fy) + COL_B(c01)*(1-fx)*fy + COL_B(c11)*fx*fy) + 0.5f);
            g_scaled[y*WIN_W+x] = COL(r, g, b);
        }
    }
}
// --- mouse (v1.31, audit P5/H1): the left button rides the core's
// touch channel under the web shell's reserved pointer id (4242),
// exactly as the shell's own mouse path does — a click is a tap and
// a drag is a drag. Window pixels scale to the 640x360 stage.
static int g_mouse_down=0, g_mouse_x=0, g_mouse_y=0;
static void lin_mouse(int wx, int wy, int phase){
    int gx = wx*640/WIN_W, gy = wy*360/WIN_H;
    if (gx<0) gx=0; if (gx>639) gx=639; if (gy<0) gy=0; if (gy>359) gy=359;
    g_mouse_x=gx; g_mouse_y=gy;
    core_touch_point(4242, gx, gy, phase);
}
static int ks_pk(KeySym ks){
    if (ks>=XK_a && ks<=XK_z) return (int)(ks-XK_a);
    if (ks>=XK_A && ks<=XK_Z) return (int)(ks-XK_A);
    if (ks>=XK_0 && ks<=XK_9) return 26+(int)(ks-XK_0);
    if (ks>=XK_KP_0 && ks<=XK_KP_9) return 26+(int)(ks-XK_KP_0);
    switch (ks){
        case XK_Up: case XK_KP_Up: return PK_UP;
        case XK_Down: case XK_KP_Down: return PK_DOWN;
        case XK_Left: case XK_KP_Left: return PK_LEFT;
        case XK_Right: case XK_KP_Right: return PK_RIGHT;
        case XK_space: return PK_SPACE;
        case XK_Return: case XK_KP_Enter: return PK_ENTER;
        case XK_Escape: return PK_ESC;
        case XK_Shift_L: case XK_Shift_R: return PK_SHIFT;
        case XK_BackSpace: return PK_BACKSPACE;
        case XK_Tab: return PK_TAB;
        case XK_Delete: return PK_DELETE;
        case XK_Home: return PK_HOME;
        case XK_End: return PK_END;
        case XK_Prior: return PK_PGUP;
        case XK_Next: return PK_PGDN;
        case XK_Control_L: case XK_Control_R: return PK_CTRL;
    }
    return -1;
}
static void key_bit(KeySym ks, int down){
    int pk = ks_pk(ks);
    if (pk>=0){ if (down) g_raw |= (1ULL<<pk); else g_raw &= ~(1ULL<<pk); }
}
// --- gamepad: /dev/input/js0 (standard joystick API) ---
static void pad_set(int a, int down){
    if (down){ if (!((IN.act_held>>a)&1)) IN.act_edges |= (1u<<a); IN.act_held |= (1u<<a); }
    else IN.act_held &= ~(1u<<a);
}
static int js_fd=-1, js_btn[16], js_ax[8], js_retry=0;
static int js2_fd=-1, js2_btn[16], js2_ax[8], js2_retry=0;
static void pad2_set(int a, int down){
    if (down){ if (!((IN.act2_held>>a)&1)) IN.act2_edges |= (1u<<a); IN.act2_held |= (1u<<a); }
    else IN.act2_held &= ~(1u<<a);
}
static int linux_js_drain(int *fdp, const char *path, int *retry, int *btn, int *ax){
    if (*fdp<0){
        if (--(*retry)>0) return 0;
        *retry = 300;
        *fdp = open(path, O_RDONLY|O_NONBLOCK);
        if (*fdp<0) return 0;
    }
    struct js_event ev;
    ssize_t r;
    int alive=1;
    while ((r=read(*fdp, &ev, sizeof(ev)))==(ssize_t)sizeof(ev)){
        int type = ev.type & ~JS_EVENT_INIT;
        if (type==JS_EVENT_BUTTON && ev.number<16) btn[ev.number]=ev.value;
        else if (type==JS_EVENT_AXIS && ev.number<8) ax[ev.number]=ev.value;
    }
    if (r==0) alive=0;
    if (!alive){ close(*fdp); *fdp=-1; memset(btn,0,16*sizeof(int)); memset(ax,0,8*sizeof(int)); return 0; }
    return 1;
}
static void linux_pad_apply(int *btn, int *ax, int ch){
    void (*set)(int,int) = ch? pad2_set : pad_set;
    if (ch==0){   // pad 1's raw sticks for WAR!'s cursor (v1.16); js axes 0/1 are the left stick (down-positive, as the digital map below reads them), 3/4 the right stick on the common mapping
        IN.pad_ax[0] = ax[0]/32767.0f; IN.pad_ax[1] = ax[1]/32767.0f;
        IN.pad_ax[2] = ax[3]/32767.0f; IN.pad_ax[3] = ax[4]/32767.0f;
    }
    if (ch==1){   // pad 2's raw sticks (v1.25 feat 13), same mapping
        IN.pad2_ax[0] = ax[0]/32767.0f; IN.pad2_ax[1] = ax[1]/32767.0f;
        IN.pad2_ax[2] = ax[3]/32767.0f; IN.pad2_ax[3] = ax[4]/32767.0f;
    }
    set(ACT_UP,    ax[1]<-12000 || ax[7]<-12000);
    set(ACT_DOWN,  ax[1]>12000  || ax[7]>12000);
    set(ACT_LEFT,  ax[0]<-12000 || ax[6]<-12000);
    set(ACT_RIGHT, ax[0]>12000  || ax[6]>12000);
    set(ACT_FIRE,  btn[0] || ax[5]>0);
    set(ACT_INTERACT, btn[1]!=0);
    set(ACT_WALL,  btn[2]!=0);
    set(ACT_WPREV, btn[4]!=0);
    set(ACT_WNEXT, btn[5]!=0);
    set(ACT_ESC,   btn[6]!=0);
    set(ACT_PAUSE, btn[7]!=0);
    set(ACT_BEAN,  btn[3]!=0);   // Y: the Backfire Bean
    set(ACT_SPRINT, btn[9] || ax[2]>0);
}
static void linux_pad_poll(void){
    int n=0, n2=0;
    if (linux_js_drain(&js_fd, "/dev/input/js0", &js_retry, js_btn, js_ax)){ linux_pad_apply(js_btn, js_ax, 0); n++; }
    if (linux_js_drain(&js2_fd, "/dev/input/js1", &js2_retry, js2_btn, js2_ax)){ linux_pad_apply(js2_btn, js2_ax, 1); n++; n2=1; }
    if (!n) IN.pad_ax[0]=IN.pad_ax[1]=IN.pad_ax[2]=IN.pad_ax[3]=0;   // no pad: no phantom sticks
    if (!n2) IN.pad2_ax[0]=IN.pad2_ax[1]=IN.pad2_ax[2]=IN.pad2_ax[3]=0;   // no second pad: no phantom sticks either
    IN.pad_count = n;
}
int main(int argc, char **argv){
    (void)argc; (void)argv;
    web_server_start();   // serve the game page + relay browser guests
    g_dpy = XOpenDisplay(0);
    if (!g_dpy){ fprintf(stderr, "TheSurvivalGame: cannot open X display\n"); return 1; }
    // (keyboard auto-repeat arrives as release+press pairs with identical
    // timestamps; the event loop swallows those pairs, so a held key reads
    // as held and produces exactly one edge — no XKB mode needed.)
    int screen = DefaultScreen(g_dpy);
    Window win = XCreateSimpleWindow(g_dpy, RootWindow(g_dpy, screen), 0, 0, WIN_W, WIN_H, 0, 0, 0);
    XStoreName(g_dpy, win, "THE SURVIVAL GAME");
    { XSizeHints h; memset(&h, 0, sizeof(h));
      h.flags = PMinSize|PMaxSize; h.min_width=WIN_W; h.max_width=WIN_W; h.min_height=WIN_H; h.max_height=WIN_H;
      XSetWMNormalHints(g_dpy, win, &h); }
    Atom wm_delete = XInternAtom(g_dpy, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(g_dpy, win, &wm_delete, 1);
    XSelectInput(g_dpy, win, KeyPressMask|KeyReleaseMask|ExposureMask|StructureNotifyMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask);
    XMapWindow(g_dpy, win);
    GC gc = XCreateGC(g_dpy, win, 0, 0);
    Visual *vis = DefaultVisual(g_dpy, screen);
    XImage *img = XCreateImage(g_dpy, vis, (unsigned)DefaultDepth(g_dpy, screen), ZPixmap, 0,
                               (char*)g_scaled, WIN_W, WIN_H, 32, WIN_W*4);
    pthread_t th; pthread_create(&th, 0, alsa_thread, 0); pthread_detach(th);
    int running=1;
    while (running){
        while (XPending(g_dpy)){
            XEvent ev; XNextEvent(g_dpy, &ev);
            if (ev.type==KeyRelease && XPending(g_dpy)){
                // auto-repeat pair (release+press, same keycode, same
                // timestamp): swallow both — the key never really came up
                XEvent nx; XPeekEvent(g_dpy, &nx);
                if (nx.type==KeyPress && nx.xkey.keycode==ev.xkey.keycode && nx.xkey.time==ev.xkey.time){
                    XNextEvent(g_dpy, &nx);
                    continue;
                }
            }
            if (ev.type==ClientMessage && (Atom)ev.xclient.data.l[0]==wm_delete) running=0;
            else if (ev.type==ButtonPress && ev.xbutton.button==1){ g_mouse_down=1; lin_mouse(ev.xbutton.x, ev.xbutton.y, 0); }
            else if (ev.type==MotionNotify && g_mouse_down){ lin_mouse(ev.xmotion.x, ev.xmotion.y, 1); }
            else if (ev.type==MotionNotify){ g_mouse_x=ev.xmotion.x*640/WIN_W; g_mouse_y=ev.xmotion.y*360/WIN_H; }
            else if (ev.type==ButtonRelease && ev.xbutton.button==1){ g_mouse_down=0; lin_mouse(ev.xbutton.x, ev.xbutton.y, 2); }
            else if (ev.type==KeyPress || ev.type==KeyRelease){
                int down = (ev.type==KeyPress);
                char chbuf[8]; KeySym ks=0;
                int chn = XLookupString(&ev.xkey, chbuf, sizeof(chbuf), &ks, 0);
                { int pk = ks_pk(ks);
                  if (down && pk>=0 && !((g_raw>>pk)&1)) IN.edges_in |= (1ULL<<pk); }
                key_bit(ks, down);
                if (down && chn>0){
                    unsigned char c = (unsigned char)chbuf[0];
                    if (c>=32 && c<127 && IN.typed_n<16) IN.typed[IN.typed_n++]=(char)c;
                }
            }
        }
        if (!running) break;
        IN.raw = g_raw;
        IN.mx = g_mouse_x; IN.my = g_mouse_y; IN.mdown = g_mouse_down;   // v1.31: the mouse fields, live at last
        linux_pad_poll();
        frame();
        blit_scale();
        XPutImage(g_dpy, win, gc, img, 0, 0, 0, 0, WIN_W, WIN_H);
        XFlush(g_dpy);
        usleep(1000);
    }
    plat_net_stop();
    XCloseDisplay(g_dpy);
    return 0;
}
#endif // PLAT_LINUX
