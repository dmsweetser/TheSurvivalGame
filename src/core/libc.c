// THE SURVIVAL GAME — core/libc.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from prelude.c lines 75-89 in the Nov 2026 fine-grained refactor; code unchanged.

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
