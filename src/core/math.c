// THE SURVIVAL GAME — core/math.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from prelude.c lines 90-142 in the Nov 2026 fine-grained refactor; code unchanged.

static float fsqrt(float x){
    if (x<=0.0f) return 0.0f;
    union { float f; u32 u; } v; v.f = x;
    v.u = (v.u>>1) + 0x1FC00000u;             // rough seed
    float y = v.f;
    for (int i=0;i<6;i++) y = 0.5f*(y + x/y);
    return y;
}
#define PI 3.14159265f
#define TAU 6.2831853f
// polynomial sin/cos on [-PI,PI]; angles wrapped with bounded arithmetic
static float sin_poly(float x){
    // fold into [-PI/2, PI/2] first (exact symmetry): the Taylor series is
    // poor near +-PI, and this keeps worst-case error under ~0.001.
    if (x > PI*0.5f) x = PI - x;
    else if (x < -PI*0.5f) x = -PI - x;
    float x2 = x*x;
    return x*(0.99997f + x2*(-0.16658f + x2*(0.00825f + x2*(-0.00019f))));
}
static float wrap_pi(float x){
    // full turn reduction in one cast step (no loops): correct for any
    // |x| up to ~1e6, which covers every phase accumulator in the game.
    // (An earlier version only corrected two turns and silently exploded
    //  past 3*PI — Taylor sin of -12 rad is in the thousands.)
    float k = (float)(int)(x/TAU + (x>=0? 0.5f : -0.5f));
    x -= k*TAU;
    if (x >  PI) x -= TAU; else if (x < -PI) x += TAU;
    return x;
}
static float fsin(float x){ return sin_poly(wrap_pi(x)); }
static float fcos(float x){ return sin_poly(wrap_pi(x + PI*0.5f)); }
static float fatan2(float y, float x){
    if (x==0.0f && y==0.0f) return 0.0f;
    float ax = ffabs(x), ay = ffabs(y);
    float ang;
    if (ax >= ay){
        float t = ay/ax; float s2 = t*t;
        float rr = ((-0.0464964749f*s2 + 0.15931422f)*s2 - 0.327622764f)*s2*t + t;
        ang = rr;
        if (x<0) ang = PI - ang;
        if (y<0) ang = -ang;
    } else {
        float t = ax/ay; float s2 = t*t;
        float rr = ((-0.0464964749f*s2 + 0.15931422f)*s2 - 0.327622764f)*s2*t + t;
        ang = PI*0.5f - rr;
        if (x<0) ang = PI - ang;
        if (y<0) ang = -ang;
    }
    return ang;
}

// RNG (all unsigned math — defined wraparound, see AGENTS.md wasm lessons)
static u32 rng_state = 0x2545F491u;
