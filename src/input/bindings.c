// THE SURVIVAL GAME — input/bindings.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 411-530 in the Nov 2026 fine-grained refactor; code unchanged.

static void refresh_has_save(void);
static int want_autostart = 0;   // shell asks to skip the menu (hosted page)

// Physical keys — the ONLY keyboard vocabulary the platform layers speak
// now. Each backend reports which physical keys are held (IN.raw); the
// core turns them into game actions through the player's key bindings,
// so every build has exactly the same controls. Letters A-Z are 0-25,
// digits 0-9 are 26-35, then the named keys.
enum {
    PK_UP=36, PK_DOWN, PK_LEFT, PK_RIGHT, PK_SPACE, PK_ENTER, PK_ESC,
    PK_SHIFT, PK_BACKSPACE,
    // v1.26: the SOURCE ROOM editor's keys — reported by every backend
    // alongside the original set (indices stay < 64 for the u64 masks)
    PK_TAB, PK_DELETE, PK_HOME, PK_END, PK_PGUP, PK_PGDN, PK_CTRL,
    PK_COUNT
};
// Game actions. The first 14 are rebindable in Settings; WPREV/WNEXT/ESC
// exist so gamepads can speak in actions directly.
enum {
    ACT_UP=0, ACT_DOWN, ACT_LEFT, ACT_RIGHT, ACT_FIRE, ACT_INTERACT,
    ACT_WALL, ACT_PAUSE, ACT_SPRINT, ACT_W1, ACT_W2, ACT_W3, ACT_W4,
    ACT_W5, ACT_WPREV, ACT_WNEXT, ACT_ESC, ACT_ENTER, ACT_BEAN, ACT_COUNT
};
static const char *act_name(int a){
    static const char *n[] = { "MOVE UP","MOVE DOWN","MOVE LEFT","MOVE RIGHT",
        "FIRE","INTERACT","BUILD WALL","PAUSE","SPRINT",
        "WEAPON 1","WEAPON 2","WEAPON 3","WEAPON 4","WEAPON 5" };
    return n[a];
}
static const char *key_name(int pk){
    static char b[8];
    if (pk>=0 && pk<26){ b[0]=(char)('A'+pk); b[1]=0; return b; }
    if (pk>=26 && pk<36){ b[0]=(char)('0'+pk-26); b[1]=0; return b; }
    switch (pk){
        case PK_UP: return "UP"; case PK_DOWN: return "DOWN";
        case PK_LEFT: return "LEFT"; case PK_RIGHT: return "RIGHT";
        case PK_SPACE: return "SPACE"; case PK_ENTER: return "ENTER";
        case PK_ESC: return "ESC"; case PK_SHIFT: return "SHIFT";
        case PK_BACKSPACE: return "BKSP";
    }
    return "?";
}
static void bind_defaults(int *bind){
    bind[ACT_UP]= 'W'-'A'; bind[ACT_DOWN]='S'-'A';
    bind[ACT_LEFT]='A'-'A'; bind[ACT_RIGHT]='D'-'A';
    bind[ACT_FIRE]=PK_SPACE; bind[ACT_INTERACT]='E'-'A';
    bind[ACT_WALL]='F'-'A'; bind[ACT_PAUSE]='P'-'A';
    bind[ACT_SPRINT]=PK_SHIFT;
    for (int w=0;w<5;w++) bind[ACT_W1+w]=26+1+w;   // digits 1-5
}

// input, filled by the platform each frame
typedef struct {
    u64 raw;                          // held physical keys (PK_* bits)
    u64 edges_in;                     // keydown events since last frame:
                                      // lossless edges (a tap between two
                                      // frames still counts exactly once)
    u64 pulse;                        // one-frame key taps (touch buttons)
    u32 act_held, act_edges;          // gamepad actions, in ACT_* space
    u32 act2_held, act2_edges;        // second gamepad (local co-op P2)
    float pad_ax[4];                  // pad 1's analog sticks, raw -1..1 in web-axes order (LX, LY, RX, RY; +Y is DOWN) — WAR!'s cursor flies on these (v1.16)
    float pad2_ax[4];                 // pad 2's analog sticks, same order/convention (v1.25 feat 13: WAR! co-op's second cursor flies on these)
    u32 pad_edges;                    // this frame's act_edges, snapshotted by derive_input before it clears them (WAR!'s pad buttons read here)
    u32 pad2_edges;                   // the same snapshot for act2_edges (pad 2)
    int pad_count;                    // controllers the platform can see
    float dev_t, pad_last, key_last, touch_last;  // last-device stamps: which input the player is actually USING (drives hint labels)
    int weapon_cycle;                 // derived: -1/0/+1 from gamepad bumpers
    int keys;                         // bits below
    float mx, my;                     // mouse in screen coords
    int mdown, mclicked, rclicked;
    char typed[16]; int typed_n;      // characters typed this frame (for fields)
    int backspace;
    int weapon_key;                   // 0 none, else 1..5 pressed this frame
    int bean_key;                     // the Backfire Bean trigger (key 6 / BEAN tap)
    int wall_pressed, interact_pressed, esc_pressed;
    int enter_pressed;                // Enter key specifically (E is interact;
                                      // menus need to tell them apart so you
                                      // can type the letter E into a field)
    int select_pressed;               // Space (the select key) or Enter, one frame
    int pause_pressed;                // P key, one frame (single-player pause)
    int touch_x, touch_y, touch_down; // mobile web: finger pos (fb coords) + held
    int touch_seen;                   // mobile web: touch used at least once (prompt wording)
    int mobile_boot;                  // mobile web: shell reported a touch-first device at boot
    int tap_x, tap_y, tapped;         // mobile web: a quick tap landed this frame
    struct { int active, id, x, y, x0, y0, ax, ay, moved, zone; float age; } tp[6];  // live touch points (ax/ay: joystick anchor)
    int touch_keys;                   // stick scheme: K_* bits held via on-screen controls
    int tp_act_edge, tp_wpn_edge, tp_wall_edge;   // button taps, latched for the next frame
    int tp_held;                      // button zones currently held (for rendering)
    int joy_on, joy_ax, joy_ay, joy_kx, joy_ky;   // floating joystick render state
    int menu_up, menu_down;           // ARROW keys only, as one-frame edges.
                                      // Menus navigate with these, never with
                                      // the WASD bits — otherwise typing a
                                      // name like WES would steer the menu.
} Input;
static Input IN;
// Touch-first presentation: no keyboard exists, so the Settings (key
// binding) screen is hidden and every instruction is written for taps.
static int mobile_ui(void){ return IN.touch_seen || IN.mobile_boot; }
// gamepad presentation: the keyboard/touch control hints and the
// menu's settings (key-binding) option hide themselves — Daniel's
// rule. This used to be "a pad is plugged in", which lied constantly:
// a pad sitting connected flipped every hint to pad labels while the
// player was on keys (and vice versa). Now it follows the LAST DEVICE
// actually touched: pad buttons/sticks stamp pad_last in
// derive_input, physical keys stamp key_last, and the newer stamp
// wins. Before either is touched, a connected pad still means pad
// labels (the boot default Daniel expects on the couch).
static int pad_mode(void){
    if (IN.pad_count<1) return 0;
    if (IN.pad_last<=0 && IN.key_last<=0) return 1;
    return IN.pad_last>=IN.key_last;
}
// The title menu's ROW LAYOUT latches its pad/keyboard flavour when the
// menu opens. pad_mode() follows the last device touched, which is right
// for hint labels — but letting it restructure the menu mid-navigation
// silently moves rows under the cursor: one keyboard Down press
// re-inserted the SETTINGS row and pushed 2P CO-OP down a row, so a
// player counting rows toggled EDDIE MODE instead and co-op never
// engaged (the mallfix P2 drop-in failure, Nov 14 2026). The latch
// resets whenever the game leaves the title menu.
static int menu_pad_mode(void){
    if (!G.menu_pad) G.menu_pad = pad_mode()? 2 : 1;
    return G.menu_pad==2;
}
