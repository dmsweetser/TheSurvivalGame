// THE SURVIVAL GAME — input/hints.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 531-623 in the Nov 2026 fine-grained refactor; code unchanged.

// ---- contextual HUD controls (v1.25): one device model, one label
// helper. Three device classes; a player's ACTIVE device is the one
// that last produced input from that player. P1's race runs on the
// stamps above (pad buttons/sticks, physical keys AND the mouse,
// touch entry points each stamp their own clock in derive_input /
// platform). The local P2 is structurally pad 2 — act2 is the only
// input P2 has — so P2's device is DEV_PAD whenever P2 exists. Net
// guests render their own screens, so nothing here needs theirs.
enum { DEV_KB=0, DEV_PAD=1, DEV_TOUCH=2 };
static int dev_local(void){
    if (IN.touch_last>0 && IN.touch_last>=IN.pad_last && IN.touch_last>=IN.key_last) return DEV_TOUCH;
    if (pad_mode()) return DEV_PAD;
    if (mobile_ui() && IN.pad_last<=0 && IN.key_last<=0) return DEV_TOUCH;   // touch-first boot, nothing touched yet
    return DEV_KB;
}
static int dev_of(int pi){
    if (pi>=0 && G.p2_slot>=0 && pi==G.p2_slot && pi!=G.my_id) return DEV_PAD;
    return dev_local();
}
// the device of the view/sim being drawn or updated right now; the
// render and update entry points set it per view/owner (bw_render,
// bw_update, bw_enter_mode, the split passes, render_store) and
// frame() resets it to the local device, so labels composed or drawn
// for P2 name P2's controls with no call-site changes.
static int g_dev_ctx = -1;
static int hint_dev(void){ return g_dev_ctx>=0? g_dev_ctx : dev_local(); }
// the action set the game actually prompts for
enum { H_FIRE, H_INT, H_WALL, H_BEAN, H_CONFIRM, H_BACK, H_PAUSE,
       H_MOVE, H_POINT };
// the one true label for an action on a given device. Pad map
// (standard): A fire/confirm, B interact/back, X wall, Y bean,
// START pause. Keyboard labels honour the player's own bindings.
// Touch: the prompts ARE the on-screen buttons, so stick-scheme
// hints name the deck buttons (FIRE / USE / WALL — the exact words
// drawn on them); in the drag scheme the finger is the control
// (TAP / DRAG); in menus the row itself is the button (TAP).
static const char *lbl2(int dev, int act){
    if (dev==DEV_PAD){
        switch (act){
            case H_FIRE: case H_CONFIRM: case H_POINT: return "A";
            case H_INT: case H_BACK: return "B";
            case H_WALL: return "X";
            case H_BEAN: return "Y";
            case H_PAUSE: return "START";
            case H_MOVE: return "STICK/DPAD";
        }
    } else if (dev==DEV_TOUCH){
        switch (act){
            case H_FIRE: return "FIRE";   // v1.33: the joystick deck's words, unconditionally (the DRAG "TAP"/"DRAG" variants died with the scheme)
            case H_INT:  return "USE";
            case H_WALL: return "WALL";
            case H_BEAN: return "TAP BEAN";
            case H_MOVE: return "STICK";
            default:     return "TAP";   // CONFIRM / BACK / PAUSE / POINT
        }
    }
    switch (act){
        case H_FIRE: case H_CONFIRM: return key_name(G.bind[ACT_FIRE]);
        case H_INT:  return key_name(G.bind[ACT_INTERACT]);
        case H_WALL: return key_name(G.bind[ACT_WALL]);
        case H_BEAN: return "6";
        case H_BACK: return "ESC";
        case H_PAUSE: return key_name(G.bind[ACT_PAUSE]);
        case H_MOVE: return "ARROWS";
        case H_POINT: return "CLICK";
    }
    return "?";
}
static const char *lbla(int act){ return lbl2(hint_dev(), act); }
// the steering phrase for the driving/running hints, per device
static const char *lbl_steer(void){
    int d = hint_dev();
    if (d==DEV_KB) return "STEER WITH THE ARROWS";
    return "STEER WITH THE STICK";
}
// the one true label for an action, for the device in hand (the
// wrappers every existing call site already speaks).
static const char *lbl_fire(void){ return lbla(H_FIRE); }
static const char *lbl_int(void){ return lbla(H_INT); }
static const char *lbl_wall(void){ return lbla(H_WALL); }
static const char *lbl_bean(void){ return lbla(H_BEAN); }
// a contextual key prompt: "<KEY> - <VERB>", composed on the spot
// (key_name's buffer is shared, so never store its pointer)
static const char *key_prompt(char *buf, const char *key, const char *verb){
    char *q=buf; q=sappend(q, key); q=sappend(q, " - "); q=sappend(q, verb); *q=0; return buf;
}
static const char *lbl_exit(void){ int d=hint_dev(); return d==DEV_PAD? "START" : (d==DEV_TOUCH? "TAP" : "ESC"); }

enum {
    K_UP=1<<0, K_DOWN=1<<1, K_LEFT=1<<2, K_RIGHT=1<<3,
    K_FIRE=1<<4, K_WALL=1<<5, K_INTERACT=1<<6, K_SPRINT=1<<7
};
