#!/usr/bin/env python3
# keyinject.py — send synthetic X11 key events (XSendEvent) to the
# Survival Game window under Xvfb. Usage:
#   keyinject.py list            print window names
#   keyinject.py space|enter|e|p|down|up|left|right|esc
import ctypes, sys, time

X11 = ctypes.CDLL("libX11.so.6")

class XKeyEvent(ctypes.Structure):
    _fields_ = [("type", ctypes.c_int), ("serial", ctypes.c_ulong),
                ("send_event", ctypes.c_int), ("display", ctypes.c_void_p),
                ("window", ctypes.c_ulong), ("root", ctypes.c_ulong),
                ("subwindow", ctypes.c_ulong), ("time", ctypes.c_ulong),
                ("x", ctypes.c_int), ("y", ctypes.c_int),
                ("x_root", ctypes.c_int), ("y_root", ctypes.c_int),
                ("state", ctypes.c_uint), ("keycode", ctypes.c_uint),
                ("same_screen", ctypes.c_int)]

class XEvent(ctypes.Union):
    _fields_ = [("xkey", XKeyEvent), ("pad", ctypes.c_long * 24)]

X11.XOpenDisplay.restype = ctypes.c_void_p
X11.XDefaultRootWindow.restype = ctypes.c_ulong
X11.XFetchName.restype = ctypes.c_int

def find_window(dpy, root):
    children = ctypes.c_void_p()
    n = ctypes.c_uint()
    X11.XQueryTree(dpy, root, ctypes.byref(ctypes.c_ulong()), ctypes.byref(ctypes.c_ulong()),
                   ctypes.byref(children), ctypes.byref(n))
    found = []
    arr = ctypes.cast(children, ctypes.POINTER(ctypes.c_ulong))
    for i in range(n.value):
        w = arr[i]
        name = ctypes.c_char_p()
        X11.XFetchName(dpy, w, ctypes.byref(name))
        nm = name.value.decode(errors="replace") if name.value else ""
        found.append((w, nm))
    return found

def main():
    dpy = X11.XOpenDisplay(b":99")
    if not dpy:
        print("no display"); return 1
    root = X11.XDefaultRootWindow(dpy)
    wins = find_window(dpy, root)
    if sys.argv[1] == "list":
        for w, nm in wins:
            print(hex(w), repr(nm))
        return 0
    target = None
    for w, nm in wins:
        if "survival" in nm.lower():
            target = w
    if target is None:
        print("game window not found", [(hex(w), nm) for w, nm in wins]); return 1
    keysyms = {"space": 0x20, "enter": 0xFF0D, "e": 0x65, "p": 0x70,
               "down": 0xFF54, "up": 0xFF52, "left": 0xFF51, "right": 0xFF53,
               "esc": 0xFF1B}
    ks = keysyms[sys.argv[1]]
    kc = X11.XKeysymToKeycode(dpy, ctypes.c_ulong(ks))
    for etype in (2, 3):   # KeyPress, KeyRelease
        ev = XEvent()
        ev.xkey.type = etype
        ev.xkey.display = dpy
        ev.xkey.window = target
        ev.xkey.root = root
        ev.xkey.subwindow = 0
        ev.xkey.time = int(time.time() * 1000) & 0xFFFFFFFF
        ev.xkey.x = ev.xkey.y = 10
        ev.xkey.x_root = ev.xkey.y_root = 10
        ev.xkey.state = 0
        ev.xkey.keycode = kc
        ev.xkey.same_screen = 1
        X11.XSendEvent(dpy, target, 1, 0x0001 if etype == 2 else 0x0002, ctypes.byref(ev))
        X11.XFlush(dpy)
        time.sleep(0.15)
    print("sent", sys.argv[1], "keycode", kc, "to", hex(target))
    return 0

sys.exit(main())
