// ============================================================================
//  TheSurvivalGame — macOS host (Apple Silicon), pure C.
//
//  This environment has no macOS SDK headers, so AppKit is driven through
//  the Objective-C runtime (objc_msgSend and friends — those headers DO
//  ship with the cross toolchain) and all framework symbols resolve at
//  runtime via -Wl,-undefined,dynamic_lookup after dlopen'ing the real
//  frameworks. On a real Mac everything referenced here is stock.
//
//  The game itself lives in survival.c (built with -DPLAT_MAC); this file
//  provides the window, keyboard, a CGBitmapContext blit of the game's
//  framebuffer, and AudioQueue output fed by the game's software synth.
// ============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <objc/runtime.h>
#include <objc/message.h>

// ---- interface provided by survival.c (PLAT_MAC) ----
typedef unsigned int u32;
extern u32   *tsg_fb(void);
extern void   tsg_frame(void);
extern double tsg_now(void);
extern void   tsg_web_start(void);
extern void   tsg_synth_render(float *out, int n, double now);
extern void   tsg_set_raw(int lo, int hi);
extern void   tsg_press(int pk);
extern void   tsg_char(int c);

// physical key ids — must match the game core (PK_*): A-Z = 0-25,
// digits 0-9 = 26-35, UP/DOWN/LEFT/RIGHT = 36-39, SPACE = 40,
// ENTER = 41, ESC = 42, SHIFT = 43, BACKSPACE = 44

typedef struct { double x, y, w, h; } TSGRect;   // NSRect / CGRect layout

// ---- CoreGraphics (prototypes declared by hand) ----
typedef struct CGContext *CGContextRef;
typedef struct CGImage *CGImageRef;
typedef struct CGColorSpace *CGColorSpaceRef;
extern CGColorSpaceRef CGColorSpaceCreateDeviceRGB(void);
extern CGContextRef CGBitmapContextCreate(void*, size_t, size_t, size_t, size_t, CGColorSpaceRef, unsigned);
extern CGImageRef CGBitmapContextCreateImage(CGContextRef);
extern void CGContextDrawImage(CGContextRef, TSGRect, CGImageRef);
extern void CGContextSetInterpolationQuality(CGContextRef, int);
extern void CGImageRelease(CGImageRef);

// ---- AudioToolbox / AudioQueue (types + prototypes declared by hand) ----
typedef struct OpaqueAudioQueue *AudioQueueRef;
typedef struct AudioQueueBuffer *AudioQueueBufferRef;
typedef struct {
    double mSampleRate; unsigned mFormatID, mFormatFlags, mBytesPerPacket,
           mFramesPerPacket, mBytesPerFrame, mChannelsPerFrame,
           mBitsPerChannel, mReserved;
} ASBD;
struct AudioQueueBuffer {
    unsigned mAudioDataBytesCapacity; void *mAudioData;
    unsigned mAudioDataByteSize; void *mUserData;
    unsigned mPacketDescriptionCapacity; void *mPacketDescriptions;
    unsigned mPacketDescriptionCount;
};
typedef void (*AQCallback)(void*, AudioQueueRef, AudioQueueBufferRef);
extern int AudioQueueNewOutput(const ASBD*, AQCallback, void*, void*, void*, unsigned, AudioQueueRef*);
extern int AudioQueueAllocateBuffer(AudioQueueRef, unsigned, AudioQueueBufferRef*);
extern int AudioQueueEnqueueBuffer(AudioQueueRef, AudioQueueBufferRef, unsigned, const void*);
extern int AudioQueueStart(AudioQueueRef, const void*);

// ---- tiny objc_msgSend shorthands ----
#define SEL_(n) sel_registerName(n)
#define MSG0(ret, o, s)        ((ret(*)(id, SEL))objc_msgSend)((o), SEL_(s))
#define MSG1(ret, o, s, a1)    ((ret(*)(id, SEL, __typeof__(a1)))objc_msgSend)((o), SEL_(s), (a1))

static id g_view = 0;
static unsigned long long g_raw = 0;

// ---- audio ----
static double aq_cursor = 0;
static void aq_fill(void *ctx, AudioQueueRef q, AudioQueueBufferRef buf){
    (void)ctx;
    int frames = (int)(buf->mAudioDataBytesCapacity / 4);
    tsg_synth_render((float*)buf->mAudioData, frames, aq_cursor);
    aq_cursor += frames / 44100.0;
    buf->mAudioDataByteSize = (unsigned)(frames * 4);
    AudioQueueEnqueueBuffer(q, buf, 0, 0);
}
static void audio_start(void){
    ASBD asbd; memset(&asbd, 0, sizeof(asbd));
    asbd.mSampleRate = 44100.0;
    asbd.mFormatID = 0x6C70636D;   // 'lpcm'
    asbd.mFormatFlags = 9;         // kLinearPCMFormatFlagIsFloat | IsPacked
    asbd.mBytesPerPacket = 4; asbd.mFramesPerPacket = 1;
    asbd.mBytesPerFrame = 4; asbd.mChannelsPerFrame = 1; asbd.mBitsPerChannel = 32;
    AudioQueueRef q = 0;
    if (AudioQueueNewOutput(&asbd, aq_fill, 0, 0, 0, 0, &q) != 0 || !q) return;
    aq_cursor = tsg_now();
    for (int i=0;i<3;i++){
        AudioQueueBufferRef b = 0;
        if (AudioQueueAllocateBuffer(q, 2048*4, &b) != 0 || !b) return;
        aq_fill(0, q, b);
    }
    AudioQueueStart(q, 0);
}

// ---- view: drawing + keyboard ----
static CGContextRef g_bctx = 0;
static void view_draw(id self, SEL _cmd, TSGRect dirty){
    (void)_cmd; (void)dirty;
    if (!g_bctx){
        CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
        // bitmapInfo 8194 = kCGImageByteOrder32Little | kCGImageAlphaNoneSkipFirst
        g_bctx = CGBitmapContextCreate(tsg_fb(), 1280, 720, 8, 1280*4, space, 8194);
        if (!g_bctx) return;
    }
    id gctx = MSG0(id, objc_getClass("NSGraphicsContext"), "currentContext");
    if (!gctx) return;
    CGContextRef cg = MSG0(CGContextRef, gctx, "CGContext");
    if (!cg) return;
    CGImageRef img = CGBitmapContextCreateImage(g_bctx);
    if (!img) return;
    TSGRect bounds = MSG0(TSGRect, self, "bounds");
    CGContextSetInterpolationQuality(cg, 3);   // kCGInterpolationHigh
    CGContextDrawImage(cg, bounds, img);
    CGImageRelease(img);
}
static int keycode_pk(unsigned short kc){
    // ANSI keycode -> physical key id (letters A-Z = 0-25, digits 0-9 = 26-35)
    switch (kc){
        case 0: return 0;   case 1: return 18;  case 2: return 3;   // A S D
        case 3: return 5;   case 4: return 7;   case 5: return 6;   // F H G
        case 6: return 25;  case 7: return 23;  case 8: return 2;   // Z X C
        case 9: return 21;  case 11: return 1;  case 12: return 16; // V B Q
        case 13: return 22; case 14: return 4;  case 15: return 17; // W E R
        case 16: return 24; case 17: return 19; case 31: return 14; // Y T O
        case 32: return 20; case 34: return 8;  case 35: return 15; // U I P
        case 37: return 11; case 38: return 9;  case 40: return 10; // L J K
        case 45: return 13; case 46: return 12;                     // N M
        case 18: return 27; case 19: return 28; case 20: return 29; // 1 2 3
        case 21: return 30; case 23: return 31; case 22: return 32; // 4 5 6
        case 26: return 33; case 28: return 34; case 25: return 35; // 7 8 9
        case 29: return 26;                                         // 0
        case 82: return 26; case 83: return 27; case 84: return 28; // keypad 0-9
        case 85: return 29; case 86: return 30; case 87: return 31;
        case 88: return 32; case 89: return 33; case 91: return 34;
        case 92: return 35;
        case 126: return 36; case 125: return 37;                   // arrows
        case 123: return 38; case 124: return 39;
        case 49: return 40;                                         // Space
        case 36: case 76: return 41;                                // Return / keypad Enter
        case 53: return 42;                                         // Escape
        case 56: case 60: return 43;                                // Shift keys
        case 51: return 44;                                         // Delete (backspace)
        case 48: return 45;                                         // Tab (v1.26: source editor)
        case 117: return 46;                                        // Forward Delete
        case 115: return 47;                                        // Home
        case 119: return 48;                                        // End
        case 116: return 49;                                        // Page Up
        case 121: return 50;                                        // Page Down
        case 59: case 62: return 51;                                // Control keys
    }
    return -1;
}
static void view_key(id self, SEL _cmd, id ev, int down){
    (void)self; (void)_cmd;
    unsigned short kc = MSG0(unsigned short, ev, "keyCode");
    int pk = keycode_pk(kc);
    if (pk>=0){
        if (down){
            signed char rep = MSG0(signed char, ev, "isARepeat");
            if (!rep) tsg_press(pk);          // one edge per physical press
            g_raw |= (1ULL<<pk);
        } else g_raw &= ~(1ULL<<pk);
        tsg_set_raw((int)(g_raw & 0xFFFFFFFFu), (int)(g_raw>>32));
    }
    if (down && kc != 51){
        // typed characters (the core decides if a text field is selected)
        id chars = MSG0(id, ev, "characters");
        if (chars){
            unsigned long len = MSG0(unsigned long, chars, "length");
            if (len > 0){
                unsigned c = MSG1(unsigned, chars, "characterAtIndex:", (unsigned long)0);
                if (c >= 32 && c < 127) tsg_char((int)c);
            }
        }
    }
}
static void view_key_down(id self, SEL cmd, id ev){ view_key(self, cmd, ev, 1); }
static void view_key_up(id self, SEL cmd, id ev){ view_key(self, cmd, ev, 0); }
static signed char view_first_responder(id self, SEL _cmd){ (void)self; (void)_cmd; return 1; }

// ---- timer target + app delegate ----
static void host_tick(id self, SEL _cmd, id timer){
    (void)self; (void)_cmd; (void)timer;
    tsg_frame();
    if (g_view) MSG1(void, g_view, "setNeedsDisplay:", (signed char)1);
}
static signed char delegate_last_closed(id self, SEL _cmd, id app){
    (void)self; (void)_cmd; (void)app; return 1;
}

int main(void){
    tsg_web_start();   // serve the game page + relay browser guests
    dlopen("/System/Library/Frameworks/Foundation.framework/Foundation", RTLD_NOW|RTLD_GLOBAL);
    dlopen("/System/Library/Frameworks/AppKit.framework/AppKit", RTLD_NOW|RTLD_GLOBAL);
    dlopen("/System/Library/Frameworks/AudioToolbox.framework/AudioToolbox", RTLD_NOW|RTLD_GLOBAL);

    Class NSApplication = objc_getClass("NSApplication");
    Class NSWindow = objc_getClass("NSWindow");
    Class NSView = objc_getClass("NSView");
    Class NSObject = objc_getClass("NSObject");
    Class NSTimer = objc_getClass("NSTimer");
    Class NSString = objc_getClass("NSString");
    if (!NSApplication || !NSWindow || !NSView || !NSObject){
        fprintf(stderr, "TheSurvivalGame: AppKit unavailable\n");
        return 1;
    }
    // view subclass
    Class viewCls = objc_allocateClassPair(NSView, "TSGView", 0);
    class_addMethod(viewCls, SEL_("drawRect:"), (IMP)view_draw, "v@:{CGRect={CGPoint=dd}{CGSize=dd}}");
    class_addMethod(viewCls, SEL_("keyDown:"), (IMP)view_key_down, "v@:@");
    class_addMethod(viewCls, SEL_("keyUp:"), (IMP)view_key_up, "v@:@");
    class_addMethod(viewCls, SEL_("acceptsFirstResponder"), (IMP)view_first_responder, "B@:");
    objc_registerClassPair(viewCls);
    // timer target subclass
    Class hostCls = objc_allocateClassPair(NSObject, "TSGHost", 0);
    class_addMethod(hostCls, SEL_("tick:"), (IMP)host_tick, "v@:@");
    objc_registerClassPair(hostCls);
    // delegate subclass: quit when the window closes
    Class delCls = objc_allocateClassPair(NSObject, "TSGDelegate", 0);
    class_addMethod(delCls, SEL_("applicationShouldTerminateAfterLastWindowClosed:"),
                    (IMP)delegate_last_closed, "B@:@");
    objc_registerClassPair(delCls);

    id app = MSG0(id, NSApplication, "sharedApplication");
    MSG1(void, app, "setActivationPolicy:", (long)0);   // regular app
    id delegate = MSG0(id, MSG0(id, delCls, "alloc"), "init");
    MSG1(void, app, "setDelegate:", delegate);

    TSGRect wr = { 120, 120, 1280, 720 };
    id win = MSG0(id, NSWindow, "alloc");
    win = ((id(*)(id, SEL, TSGRect, unsigned long, unsigned long, signed char))objc_msgSend)(
        win, SEL_("initWithContentRect:styleMask:backing:defer:"), wr, 15ul, 2ul, (signed char)0);
    id title = ((id(*)(id, SEL, const char*))objc_msgSend)(NSString, SEL_("stringWithUTF8String:"),
                                                           "THE SURVIVAL GAME");
    MSG1(void, win, "setTitle:", title);
    TSGRect vr = { 0, 0, 1280, 720 };
    id view = MSG0(id, viewCls, "alloc");
    view = MSG1(id, view, "initWithFrame:", vr);
    g_view = view;
    MSG1(void, win, "setContentView:", view);
    MSG1(void, win, "makeFirstResponder:", view);
    MSG0(void, win, "center");
    MSG1(void, win, "makeKeyAndOrderFront:", (id)0);
    MSG1(void, app, "activateIgnoringOtherApps:", (signed char)1);

    audio_start();

    id target = MSG0(id, MSG0(id, hostCls, "alloc"), "init");
    ((id(*)(id, SEL, double, id, SEL, id, signed char))objc_msgSend)(
        NSTimer, SEL_("scheduledTimerWithTimeInterval:target:selector:userInfo:repeats:"),
        1.0/60.0, target, SEL_("tick:"), (id)0, (signed char)1);

    MSG0(void, app, "run");
    return 0;
}
