// THE SURVIVAL GAME — platform/win32.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from platform.c lines 1093-1368 in the Nov 2026 fine-grained refactor; code unchanged.

// ===========================================================================
// PLATFORM: Windows — Win32 + GDI + winmm MIDI + Winsock UDP. Single .exe.
// ===========================================================================
#ifdef PLAT_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#undef IN
#undef OUT
#ifdef _MSC_VER
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "user32.lib")
#endif

static HWND g_hwnd=0;
static u64 g_raw=0;
static LARGE_INTEGER qpf; static int qpf_ok=0;
double plat_now(void){
    if (!qpf_ok){ QueryPerformanceFrequency(&qpf); qpf_ok=1; }
    LARGE_INTEGER t; QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)qpf.QuadPart;
}
int plat_daynum(void){
    SYSTEMTIME st; GetLocalTime(&st);
    return days_from_civil((int)st.wYear, (int)st.wMonth, (int)st.wDay);
}
float plat_minutes(void){
    SYSTEMTIME st; GetLocalTime(&st);
    return (float)st.wHour*60.0f + (float)st.wMinute + (float)st.wSecond/60.0f + (float)st.wMilliseconds/60000.0f;
}
// --- MIDI ---
static HMIDIOUT g_midi=0;
typedef struct { double off; int ch, note; } MidiOff;
static MidiOff g_offs[160]; static int g_offs_n=0;
static void midi_prog(int ch, int prog){
    if (g_midi) midiOutShortMsg(g_midi, 0xC0 | ch | (prog<<8));
}
void plat_note(int ch, int note, int vel, int instr){
    if (!g_midi) return;
    { static int kz_state = -1;   // WHALES ONLY: KAZOO MODE re-voices the music channels
      int kz = (G.whale_on & 16384)? 1 : 0;
      if (kz != kz_state){ kz_state = kz;
          midi_prog(0, kz? 71 : 80); midi_prog(1, kz? 70 : 38); midi_prog(2, kz? 71 : 81); } }
    int mc = ch, mn = note;
    if (ch==3 && instr==3){ mc=9; mn = vel>90? 49 : 42; }        // noise -> drums
    if (note<0 || note>127) return;
    midiOutShortMsg(g_midi, 0x90 | mc | (mn<<8) | (vel<<16));
    if (g_offs_n<160){ g_offs[g_offs_n].off = plat_now() + (ch==1? 0.22 : 0.16);
        g_offs[g_offs_n].ch=mc; g_offs[g_offs_n].note=mn; g_offs_n++; }
}
static void plat_audio_pump(void){
    double now = plat_now();
    for (int i=0;i<g_offs_n;i++) if (g_offs[i].off<=now){
        midiOutShortMsg(g_midi, 0x80 | g_offs[i].ch | (g_offs[i].note<<8));
        g_offs[i]=g_offs[--g_offs_n]; i--;
    }
}
// --- save file ---
// Saves live in ONE stable home (%APPDATA%) — they used to be loose
// files in whatever folder the exe ran from, so a fresh download
// (TheSurvivalGame(14).exe in Downloads) found a stale set from an
// older run, or none, and players "lost" progress that was sitting
// in another folder (Daniel's vanishing buddy, Nov 5). The legacy
// beside-the-exe file is adopted ONLY when the stable home has no
// copy yet (the one-time upgrade migration), and is then lifted into
// the stable home. It must never win just for being newer: a legacy
// file can only be written by an OLD build, so a Downloads folder
// full of old exes would otherwise keep rewinding the player's run
// to whichever stale set was touched last (Daniel, Nov 5 again —
// the buddy "lost" on Continue was the stable run being replaced
// by a fresher-mtime legacy set from an older exe in the folder).
static void win_save_paths(int slot, char *stable, char *legacy){
    save_slot_name(legacy, slot);
    const char *app = getenv("APPDATA");
    if (app && app[0]) snprintf(stable, 1024, "%s\\%s", app, legacy);
    else { int i=0; while (legacy[i]){ stable[i]=legacy[i]; i++; } stable[i]=0; }
}
void plat_save_slot(int slot, const char *d, int len){
    char sp[1024], lp[64]; win_save_paths(slot, sp, lp);
    HANDLE h = CreateFileA(sp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (h==INVALID_HANDLE_VALUE) h = CreateFileA(lp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);   // never lose a save
    if (h==INVALID_HANDLE_VALUE) return;
    DWORD wr; WriteFile(h, d, (DWORD)len, &wr, 0); CloseHandle(h);
}
int plat_load_slot(int slot, char *buf, int cap){
    char sp[1024], lp[64]; win_save_paths(slot, sp, lp);
    WIN32_FILE_ATTRIBUTE_DATA sa, la;
    int hs = GetFileAttributesExA(sp, GetFileExInfoStandard, &sa)!=0;
    int hl = GetFileAttributesExA(lp, GetFileExInfoStandard, &la)!=0;
    const char *use = 0; int import = 0;
    if (hs) use=sp;                          // the stable home is canonical once it exists
    else if (hl){ use=lp; import=1; }        // one-time migration only
    if (!use) return -1;
    HANDLE h = CreateFileA(use, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h==INVALID_HANDLE_VALUE) return -1;
    DWORD rd; BOOL ok = ReadFile(h, buf, (DWORD)cap, &rd, 0); CloseHandle(h);
    if (!ok) return -1;
    if (import){   // lift the legacy copy into the stable home
        HANDLE w = CreateFileA(sp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        if (w!=INVALID_HANDLE_VALUE){ DWORD wr; WriteFile(w, buf, rd, &wr, 0); CloseHandle(w); }
    }
    return (int)rd;
}
void plat_save(const char *d, int len){ plat_save_slot(0, d, len); }
int plat_load(char *buf, int cap){ return plat_load_slot(0, buf, cap); }
// THE SOURCE ROOM (v1.26): the mod overlay under %APPDATA%\tsg-modsrc\,
// mirroring the src-relative path with backslashes. tsg_mkdirs /
// tsg_rmrf are shared with srcbuild.c's staging (same names as the
// POSIX versions in posix.c; exactly one set compiles per build).
void plat_save_home_dir(char *out, int cap){
    const char *app = getenv("APPDATA");
    if (app && app[0]) snprintf(out, (size_t)cap, "%s", app);
    else if (cap>0) out[0]=0;
}
static void tsg_mkdirs(const char *dirpath){
    char tmp[1200]; snprintf(tmp, sizeof(tmp), "%s", dirpath);
    for (char *p = tmp; *p; p++){
        if (*p=='/' ) *p='\\';
        if (*p=='\\' && p>tmp && *(p-1)!=':'){ *p=0; CreateDirectoryA(tmp, 0); *p='\\'; }
    }
    CreateDirectoryA(tmp, 0);
}
static void tsg_rmrf(const char *path){
    DWORD at = GetFileAttributesA(path);
    if (at==INVALID_FILE_ATTRIBUTES) return;
    if (at & FILE_ATTRIBUTE_DIRECTORY){
        char pat[1200]; snprintf(pat, sizeof(pat), "%s\\*", path);
        WIN32_FIND_DATAA fd; HANDLE h = FindFirstFileA(pat, &fd);
        if (h!=INVALID_HANDLE_VALUE){
            do {
                if (fd.cFileName[0]=='.' && (!fd.cFileName[1] || (fd.cFileName[1]=='.' && !fd.cFileName[2]))) continue;
                char sub[1200]; snprintf(sub, sizeof(sub), "%s\\%s", path, fd.cFileName);
                tsg_rmrf(sub);
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
        RemoveDirectoryA(path);
    } else DeleteFileA(path);
}
static void mod_path(char *out, int cap, const char *rel){
    char home[1024]; plat_save_home_dir(home, (int)sizeof(home));
    if (!rel){ snprintf(out, (size_t)cap, "%s\\tsg-modsrc", home); return; }
    char conv[600]; int i=0;
    while (rel[i] && i<590){ conv[i] = rel[i]=='/'? '\\' : rel[i]; i++; }
    conv[i]=0;
    snprintf(out, (size_t)cap, "%s\\tsg-modsrc\\%s", home, conv);
}
int plat_mod_load(const char *path, char *buf, int cap){
    char fp[1200]; mod_path(fp, (int)sizeof(fp), path);
    HANDLE h = CreateFileA(fp, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h==INVALID_HANDLE_VALUE) return -1;
    DWORD rd; BOOL ok = ReadFile(h, buf, (DWORD)cap, &rd, 0); CloseHandle(h);
    return ok? (int)rd : -1;
}
void plat_mod_save(const char *path, const char *data, int len){
    char fp[1200]; mod_path(fp, (int)sizeof(fp), path);
    if (len<=0){ DeleteFileA(fp); return; }   // an empty overlay is no overlay
    char dir[1200]; snprintf(dir, sizeof(dir), "%s", fp);
    char *slash = strrchr(dir, '\\'); if (slash) *slash = 0;
    tsg_mkdirs(dir);
    HANDLE h = CreateFileA(fp, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (h==INVALID_HANDLE_VALUE) return;
    DWORD wr; WriteFile(h, data, (DWORD)len, &wr, 0); CloseHandle(h);
}
void plat_mod_clear(void){
    char dir[1200]; mod_path(dir, (int)sizeof(dir), 0);
    tsg_rmrf(dir);
}
int plat_mod_exists(const char *path){
    char fp[1200]; mod_path(fp, (int)sizeof(fp), path);
    DWORD at = GetFileAttributesA(fp);
    return at!=INVALID_FILE_ATTRIBUTES && !(at & FILE_ATTRIBUTE_DIRECTORY);
}
void plat_open_url(const char *url){   // Whales Only: only buy.stripe.com links ever reach here
    if (!url || strncmp(url, "https://buy.stripe.com/", 23)!=0) return;
    ShellExecuteA(0, "open", url, 0, 0, SW_SHOWNORMAL);
}
int plat_whale_poll(char *buf, int cap){   // a whale-code.txt: cwd, else beside the saves in %APPDATA%
    char sp[1024]; const char *app = getenv("APPDATA");
    HANDLE h = CreateFileA("whale-code.txt", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h==INVALID_HANDLE_VALUE && app && app[0]){
        snprintf(sp, sizeof(sp), "%s\\whale-code.txt", app);
        h = CreateFileA(sp, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    } else sp[0]=0;
    if (h==INVALID_HANDLE_VALUE) return 0;
    DWORD rd; BOOL ok = ReadFile(h, buf, (DWORD)(cap-1), &rd, 0); CloseHandle(h);
    DeleteFileA("whale-code.txt");
    if (sp[0]) DeleteFileA(sp);
    if (!ok || rd==0) return 0;
    buf[rd]=0; return (int)rd;
}
// (networking lives in the shared embedded-web-server section at the
// end of this file: the app serves the web build and browser guests
// relay straight into the host — there is no separate UDP transport.)

// --- window / input: report physical keys; the core applies bindings ---
static int vk_pk(int vk){
    if (vk>='A' && vk<='Z') return vk-'A';
    if (vk>='0' && vk<='9') return 26+(vk-'0');
    if (vk>=VK_NUMPAD0 && vk<=VK_NUMPAD9) return 26+(vk-VK_NUMPAD0);
    switch (vk){
        case VK_UP: return PK_UP; case VK_DOWN: return PK_DOWN;
        case VK_LEFT: return PK_LEFT; case VK_RIGHT: return PK_RIGHT;
        case VK_SPACE: return PK_SPACE; case VK_RETURN: return PK_ENTER;
        case VK_ESCAPE: return PK_ESC;
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: return PK_SHIFT;
        case VK_BACK: return PK_BACKSPACE;
        case VK_TAB: return PK_TAB;
        case VK_DELETE: return PK_DELETE;
        case VK_HOME: return PK_HOME;
        case VK_END: return PK_END;
        case VK_PRIOR: return PK_PGUP;
        case VK_NEXT: return PK_PGDN;
        case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL: return PK_CTRL;
    }
    return -1;
}
static void key_bit(int vk, int down){
    int pk = vk_pk(vk);
    if (pk>=0){ if (down) g_raw |= (1ULL<<pk); else g_raw &= ~(1ULL<<pk); }
}
// --- mouse (v1.31, audit P5/H1): the left button rides the core's
// touch channel under the web shell's reserved pointer id (4242),
// exactly as the shell's own mouse path does — a click is a tap and
// a drag is a drag. Client pixels scale to the 640x360 stage.
static int g_mouse_down=0, g_mouse_x=0, g_mouse_y=0;
static void win_mouse(LPARAM lp, int phase){
    RECT rc; GetClientRect(g_hwnd, &rc);
    int w = rc.right-rc.left, h = rc.bottom-rc.top; if (w<1) w=1; if (h<1) h=1;
    int x = (short)(lp & 0xFFFF), y = (short)((lp>>16) & 0xFFFF);
    int gx = x*640/w, gy = y*360/h;
    if (gx<0) gx=0; if (gx>639) gx=639; if (gy<0) gy=0; if (gy>359) gy=359;
    g_mouse_x=gx; g_mouse_y=gy;
    if (phase>=0) core_touch_point(4242, gx, gy, phase);
}
static LRESULT CALLBACK wndproc(HWND hw, UINT msg, WPARAM wp, LPARAM lp){
    switch (msg){
    case WM_DESTROY: PostQuitMessage(0); return 0;
    case WM_LBUTTONDOWN: g_mouse_down=1; win_mouse(lp, 0); return 0;
    case WM_MOUSEMOVE: win_mouse(lp, g_mouse_down? 1 : -1); return 0;
    case WM_LBUTTONUP: g_mouse_down=0; win_mouse(lp, 2); return 0;
    case WM_KEYDOWN: { int pk = vk_pk((int)wp);
        if (pk>=0 && !((g_raw>>pk)&1)) IN.edges_in |= (1ULL<<pk);   // one edge per physical press
        key_bit((int)wp, 1); return 0; }
    case WM_KEYUP: key_bit((int)wp, 0); return 0;
    case WM_CHAR:
        // typed characters: the game core decides whether a text field
        // is selected right now; everywhere else they are ignored.
        if (wp>=32 && wp<127 && IN.typed_n<16) IN.typed[IN.typed_n++]=(char)wp;
        return 0;
    }
    return DefWindowProcA(hw, msg, wp, lp);
}
// --- gamepad: XInput, loaded dynamically (no import lib needed) ---
typedef struct { u32 packet; u16 buttons; u8 lt, rt; short lx, ly, rx, ry; } XIState;
typedef u32 (__stdcall *XInputGetState_t)(u32, XIState*);
static XInputGetState_t xi_get=0; static int xi_tried=0;
static void pad_set(int a, int down){
    if (down){ if (!((IN.act_held>>a)&1)) IN.act_edges |= (1u<<a); IN.act_held |= (1u<<a); }
    else IN.act_held &= ~(1u<<a);
}
static void pad2_set(int a, int down){
    if (down){ if (!((IN.act2_held>>a)&1)) IN.act2_edges |= (1u<<a); IN.act2_held |= (1u<<a); }
    else IN.act2_held &= ~(1u<<a);
}
static void win32_pad_apply(XIState *s, int ch){
    u16 b = s->buttons;
    void (*set)(int,int) = ch? pad2_set : pad_set;
    if (ch==0){   // pad 1's raw sticks for WAR!'s cursor (v1.16); XInput's Y is up-positive, the game's axis order wants down-positive
        IN.pad_ax[0] = s->lx/32767.0f; IN.pad_ax[1] = -s->ly/32767.0f;
        IN.pad_ax[2] = s->rx/32767.0f; IN.pad_ax[3] = -s->ry/32767.0f;
    }
    if (ch==1){   // pad 2's raw sticks (v1.25 feat 13), same flip
        IN.pad2_ax[0] = s->lx/32767.0f; IN.pad2_ax[1] = -s->ly/32767.0f;
        IN.pad2_ax[2] = s->rx/32767.0f; IN.pad2_ax[3] = -s->ry/32767.0f;
    }
    set(ACT_UP,    (b&0x0001) || s->ly>12000);
    set(ACT_DOWN,  (b&0x0002) || s->ly<-12000);
    set(ACT_LEFT,  (b&0x0004) || s->lx<-12000);
    set(ACT_RIGHT, (b&0x0008) || s->lx>12000);
    set(ACT_FIRE,  (b&0x1000) || s->rt>30);
    set(ACT_INTERACT, (b&0x2000)!=0);
    set(ACT_WALL,  (b&0x4000)!=0);
    set(ACT_WPREV, (b&0x0100)!=0);
    set(ACT_WNEXT, (b&0x0200)!=0);
    set(ACT_PAUSE, (b&0x0010)!=0);
    set(ACT_ESC,   (b&0x0020)!=0);
    set(ACT_BEAN,  (b&0x8000)!=0);   // Y: the Backfire Bean
    set(ACT_SPRINT,(b&0x0040) || s->lt>30);
}
static void win32_pad_poll(void){
    if (!xi_tried){ xi_tried=1;
        HMODULE h = LoadLibraryA("xinput1_4.dll");
        if (!h) h = LoadLibraryA("xinput1_3.dll");
        if (h) xi_get = (XInputGetState_t)GetProcAddress(h, "XInputGetState");
    }
    if (!xi_get) return;
    // connected pads in user order: first drives P1, second drives P2
    int ch=0, count=0;
    for (int u=0; u<4 && ch<2; u++){
        XIState s; memset(&s, 0, sizeof(s));
        if (xi_get(u, &s)!=0) continue;
        count++;
        win32_pad_apply(&s, ch++);
    }
    if (!count) IN.pad_ax[0]=IN.pad_ax[1]=IN.pad_ax[2]=IN.pad_ax[3]=0;   // no pad: no phantom sticks
    if (count<2) IN.pad2_ax[0]=IN.pad2_ax[1]=IN.pad2_ax[2]=IN.pad2_ax[3]=0;   // no second pad: no phantom sticks either
    IN.pad_count = count;
}
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show){
    (void)hPrev; (void)cmd;
    web_server_start();   // serve the game page + relay browser guests
    WNDCLASSA wc; ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc=wndproc; wc.hInstance=hInst; wc.lpszClassName="TSGWindow";
    wc.hCursor=LoadCursorA(0, MAKEINTRESOURCEA(32515)); /* IDC_CROSSHAIR */
    wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassA(&wc);
    RECT r={0,0,1280,720}; AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
    g_hwnd = CreateWindowA("TSGWindow", "THE SURVIVAL GAME", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right-r.left, r.bottom-r.top, 0, 0, hInst, 0);
    if (!g_hwnd) return 1;
    ShowWindow(g_hwnd, show);
    if (midiOutOpen(&g_midi, MIDI_MAPPER, 0, 0, CALLBACK_NULL)==MMSYSERR_NOERROR){
        midi_prog(0, 80);   // square lead
        midi_prog(1, 38);   // synth bass
        midi_prog(2, 81);   // saw arp
        midi_prog(3, 80);   // sfx lead
    }
    BITMAPINFO bmi; ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth=PW; bmi.bmiHeader.biHeight=-PH;
    bmi.bmiHeader.biPlanes=1; bmi.bmiHeader.biBitCount=32; bmi.bmiHeader.biCompression=BI_RGB;
    int running=1;
    while (running){
        MSG m;
        while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)){
            if (m.message==WM_QUIT) running=0;
            TranslateMessage(&m); DispatchMessageA(&m);
        }
        if (!running) break;
        IN.raw = g_raw;
        IN.mx = g_mouse_x; IN.my = g_mouse_y; IN.mdown = g_mouse_down;   // v1.31: the mouse fields, live at last
        win32_pad_poll();
        frame();
        plat_audio_pump();
        HDC dc = GetDC(g_hwnd);
        RECT rc; GetClientRect(g_hwnd, &rc);
        SetStretchBltMode(dc, HALFTONE);                 // smooth upscale
        SetBrushOrgEx(dc, 0, 0, 0);
        StretchDIBits(dc, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
                      0, 0, PW, PH, fb, &bmi, DIB_RGB_COLORS, SRCCOPY);
        ReleaseDC(g_hwnd, dc);
        Sleep(1);
    }
    if (g_midi) midiOutClose(g_midi);
    plat_net_stop();
    return 0;
}

#endif // PLAT_WIN32
