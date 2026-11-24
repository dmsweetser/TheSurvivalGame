// THE SURVIVAL GAME — platform/mac.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from platform.c lines 1728-1790 in the Nov 2026 fine-grained refactor; code unchanged.


// ===========================================================================
// PLATFORM: macOS — interface for the pure-C Cocoa host in mac_main.c.
// That file drives NSApplication through the Objective-C runtime (this
// build environment has no AppKit headers), renders the framebuffer via
// CGBitmapContext, and pulls audio from synth_render via AudioQueue.
// ===========================================================================
#ifdef PLAT_MAC
void plat_save_slot(int slot, const char *d, int len){
    char nm[64]; save_slot_name(nm, slot);
    char path[1024]; const char *home = getenv("HOME"); if (!home) return;
    snprintf(path, sizeof(path), "%s/Library/Application Support/%s", home, nm);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fwrite(d, 1, (size_t)len, f); fclose(f);
}
int plat_load_slot(int slot, char *buf, int cap){
    char nm[64]; save_slot_name(nm, slot);
    char path[1024]; const char *home = getenv("HOME"); if (!home) return -1;
    snprintf(path, sizeof(path), "%s/Library/Application Support/%s", home, nm);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    int n = (int)fread(buf, 1, (size_t)cap, f); fclose(f);
    return n;
}
void plat_save(const char *d, int len){ plat_save_slot(0, d, len); }
int plat_load(char *buf, int cap){ return plat_load_slot(0, buf, cap); }
void plat_open_url(const char *url){   // Whales Only: only buy.stripe.com links ever reach here
    if (!url || strncmp(url, "https://buy.stripe.com/", 23)!=0) return;
    if (fork()==0){ execl("/usr/bin/open", "open", url, (char*)0); _exit(1); }
}
int plat_whale_poll(char *buf, int cap){   // whale-code.txt: cwd, else beside the saves
    char path[1024]; const char *home = getenv("HOME");
    FILE *f = fopen("whale-code.txt", "rb");
    if (!f && home){ snprintf(path, sizeof(path), "%s/Library/Application Support/whale-code.txt", home);
                     f = fopen(path, "rb"); }
    if (!f) return 0;
    int n = (int)fread(buf, 1, (size_t)(cap-1), f); fclose(f);
    remove("whale-code.txt");
    if (home){ snprintf(path, sizeof(path), "%s/Library/Application Support/whale-code.txt", home); remove(path); }
    if (n<=0) return 0;
    buf[n]=0; return n;
}
u32 *tsg_fb(void){ return fb; }
void tsg_frame(void){ frame(); }
double tsg_now(void){ return plat_now(); }
void tsg_web_start(void){ web_server_start(); }
void tsg_synth_render(float *out, int n, double now){ synth_render(out, n, now); }
void tsg_set_raw(int lo, int hi){ IN.raw = (u64)(u32)lo | ((u64)(u32)hi<<32); }
// mouse (v1.31, audit P5/H1): mac_main.c's view forwards mouse events
// here — the touch channel feeds the core under the web shell's
// reserved pointer id (4242), a click is a tap and a drag is a drag,
// and the position feeds the core's mouse fields.
void tsg_touch_point(int id, int x, int y, int phase){ core_touch_point(id, x, y, phase); }
void tsg_mouse_pos(int x, int y, int down){ IN.mx = x; IN.my = y; IN.mdown = down; }
void tsg_press(int pk){ if (pk>=0 && pk<64) IN.edges_in |= (1ULL<<pk); }
void tsg_act(int a, int down){
    if (a<0 || a>=ACT_COUNT) return;
    if (down){ if (!((IN.act_held>>a)&1)) IN.act_edges |= (1u<<a); IN.act_held |= (1u<<a); }
    else IN.act_held &= ~(1u<<a);
}
void tsg_act2(int a, int down){
    if (a<0 || a>=ACT_COUNT) return;
    if (down){ if (!((IN.act2_held>>a)&1)) IN.act2_edges |= (1u<<a); IN.act2_held |= (1u<<a); }
    else IN.act2_held &= ~(1u<<a);
}
void tsg_pads(int n){ IN.pad_count = n; }
void tsg_char(int c){ if (IN.typed_n<16) IN.typed[IN.typed_n++]=(char)c; }
#endif // PLAT_MAC
