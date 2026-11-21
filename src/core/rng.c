// THE SURVIVAL GAME — core/rng.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from prelude.c lines 143-153 in the Nov 2026 fine-grained refactor; code unchanged.

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

