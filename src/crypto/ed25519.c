// THE SURVIVAL GAME — crypto/ed25519.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from music.c lines 687-875 in the Nov 2026 fine-grained refactor; code unchanged.

static void fe_carry(u64 *h){
    u64 c;
    c=h[0]>>51; h[0]&=M51; h[1]+=c; c=h[1]>>51; h[1]&=M51; h[2]+=c;
    c=h[2]>>51; h[2]&=M51; h[3]+=c; c=h[3]>>51; h[3]&=M51; h[4]+=c;
    c=h[4]>>51; h[4]&=M51; h[0]+=c*19; c=h[0]>>51; h[0]&=M51; h[1]+=c;
}
static void fe_copy(fe o, const fe a){ for(int i=0;i<5;i++) o[i]=a[i]; }
static void fe_0(fe o){ for(int i=0;i<5;i++) o[i]=0; }
static void fe_1(fe o){ o[0]=1; o[1]=o[2]=o[3]=o[4]=0; }
static void fe_add(fe o, const fe a, const fe b){ for(int i=0;i<5;i++) o[i]=a[i]+b[i]; }
static void fe_sub(fe o, const fe a, const fe b){
    // a + 2p - b, with 2p limbs below; keeps everything positive
    static const u64 TP[5] = { 0xFFFFFFFFFFFDAULL, 0xFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFEULL };
    for(int i=0;i<5;i++) o[i]=a[i]+TP[i]-b[i];
}
static void fe_neg(fe o, const fe a){
    static const u64 TP[5] = { 0xFFFFFFFFFFFDAULL, 0xFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFEULL };
    for(int i=0;i<5;i++) o[i]=TP[i]-a[i];
}
static void fe_mul(fe o, const fe a, const fe b){
    u128 s0 = (u128)a[0]*b[0] + 19*((u128)a[1]*b[4] + (u128)a[2]*b[3] + (u128)a[3]*b[2] + (u128)a[4]*b[1]);
    u128 s1 = (u128)a[0]*b[1] + (u128)a[1]*b[0] + 19*((u128)a[2]*b[4] + (u128)a[3]*b[3] + (u128)a[4]*b[2]);
    u128 s2 = (u128)a[0]*b[2] + (u128)a[1]*b[1] + (u128)a[2]*b[0] + 19*((u128)a[3]*b[4] + (u128)a[4]*b[3]);
    u128 s3 = (u128)a[0]*b[3] + (u128)a[1]*b[2] + (u128)a[2]*b[1] + (u128)a[3]*b[0] + 19*(u128)a[4]*b[4];
    u128 s4 = (u128)a[0]*b[4] + (u128)a[1]*b[3] + (u128)a[2]*b[2] + (u128)a[3]*b[1] + (u128)a[4]*b[0];
    u64 r[5];
    r[0]=(u64)s0 & M51; s1 += (u64)(s0>>51);
    r[1]=(u64)s1 & M51; s2 += (u64)(s1>>51);
    r[2]=(u64)s2 & M51; s3 += (u64)(s2>>51);
    r[3]=(u64)s3 & M51; s4 += (u64)(s3>>51);
    r[4]=(u64)s4 & M51; u64 c = (u64)(s4>>51);
    r[0] += c*19;
    fe_carry(r);
    for(int i=0;i<5;i++) o[i]=r[i];
}
static void fe_sq(fe o, const fe a){ fe_mul(o, a, a); }
// fully reduce and pack to 32 bytes little-endian
static void fe_tobytes(u8 *s, const fe a){
    u64 h[5]; fe_copy(h, a);
    // h = h mod p: carry, then subtract p if h >= p
    fe_carry(h); fe_carry(h);
    // compute h - p
    u64 q0 = h[0] - (M51 - 18);          // p0 = 2^51-19 = M51-18
    u64 q1 = h[1] - M51, q2 = h[2] - M51, q3 = h[3] - M51, q4 = h[4] - M51;
    // borrow chain: if any subtraction underflowed, keep original
    // (do it with signed checks via carry propagation on a copy)
    u64 t[5]; fe_copy(t, h);
    { u64 borrow = 0;
      u64 p[5] = { M51-18, M51, M51, M51, M51 };
      for (int i=0;i<5;i++){ u128 d = (u128)t[i] - p[i] - borrow; t[i] = (u64)d; borrow = (d >> 127) & 1; }
      if (!borrow){ fe_copy(h, t); }
    }
    (void)q0;(void)q1;(void)q2;(void)q3;(void)q4;
    // pack 51-bit limbs into bytes
    u128 v0=h[0], v1=h[1], v2=h[2], v3=h[3], v4=h[4];
    u64 w0 = (u64)v0 | ((u64)v1<<51);
    // do it byte by byte from a 5-limb -> bit stream
    { u64 limbs[5]; for(int i=0;i<5;i++) limbs[i]=h[i];
      for (int i=0;i<32;i++){ int bit=i*8; int limb=bit/51; int off=bit%51;
        u64 lo = limbs[limb] >> off; u64 hi = 0;
        if (off>43 && limb<4) hi = limbs[limb+1] << (51-off);
        s[i] = (u8)(lo | hi); } }
    (void)w0; (void)v2; (void)v3; (void)v4;
}
static void fe_frombytes(fe o, const u8 *s){
    // load 32 bytes as 5 radix-51 limbs (ref10-style shifts)
    u64 t0=0,t1=0,t2=0,t3=0,t4=0;
    for (int i=0;i<32;i++){
        int bit=i*8, limb=bit/51, off=bit%51;
        u64 v = s[i];
        if (limb==0) t0 |= v<<off; else if (limb==1) t1 |= v<<off; else if (limb==2) t2 |= v<<off; else if (limb==3) t3 |= v<<off; else t4 |= v<<off;
        if (off>43 && limb<4){ u64 hi = v >> (51-off); if (limb==0) t1|=hi; else if (limb==1) t2|=hi; else if (limb==2) t3|=hi; else t4|=hi; }
    }
    o[0]=t0&M51; o[1]=t1&M51; o[2]=t2&M51; o[3]=t3&M51; o[4]=t4&M51;
}
static int fe_iszero(const fe a){ u8 s[32]; fe_tobytes(s, a); u8 r=0; for(int i=0;i<32;i++) r|=s[i]; return r==0; }
static int fe_eq(const fe a, const fe b){ fe d; fe_sub(d, a, b); fe_carry(d); return fe_iszero(d); }
static int fe_lowbit(const fe a){ u8 s[32]; fe_tobytes(s, a); return s[0]&1; }
// exponentiation by a 32-byte little-endian exponent (square & multiply)
static void fe_pow(fe o, const fe a, const u8 *e){
    fe r; fe_1(r); fe base; fe_copy(base, a);
    for (int i=0;i<255;i++){
        fe_sq(r, r);
        int bit = (e[(254-i)/8] >> ((254-i)%8)) & 1;
        if (bit){ fe t; fe_mul(t, r, base); fe_copy(r, t); }
    }
    fe_copy(o, r);
}
static void fe_inv(fe o, const fe a){
    // a^(p-2), p-2 = 2^255 - 21
    static const u8 E[32] = { 0xeb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f };
    fe_pow(o, a, E);
}
// d = -121665/121666 mod p, computed once
static fe ED_D, ED_D2, ED_I; static int ed_ready=0;
static void fe_from_u32(fe o, u32 v){ fe_0(o); o[0]=v; }
static void ed_init(void){
    if (ed_ready) return; ed_ready=1;
    fe a, b, ai;
    fe_from_u32(a, 121665); fe_from_u32(b, 121666);
    fe_inv(ai, b); fe_mul(a, a, ai); fe_neg(ED_D, a); fe_carry(ED_D);
    fe_add(ED_D2, ED_D, ED_D); fe_carry(ED_D2);
    // I = sqrt(-1) = 2^((p-1)/4)
    static const u8 EI[32] = { 0xfb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1f };
    fe t; fe_from_u32(t, 2); fe_pow(ED_I, t, EI);
}

// ---------------- points: extended coordinates ----------------
typedef struct { fe X, Y, Z, T; } point;
static void pt_identity(point *o){ fe_0(o->X); fe_1(o->Y); fe_1(o->Z); fe_0(o->T); }
static void pt_add(point *o, const point *p, const point *q){
    fe A,B,C,D,E,F,G,H, t1,t2;
    fe_sub(t1, p->Y, p->X); fe_sub(t2, q->Y, q->X); fe_mul(A, t1, t2);
    fe_add(t1, p->Y, p->X); fe_add(t2, q->Y, q->X); fe_mul(B, t1, t2);
    fe_mul(t1, p->T, q->T); fe_mul(C, t1, ED_D2);
    fe_mul(t1, p->Z, q->Z); fe_add(D, t1, t1); fe_carry(D);
    fe_sub(E, B, A); fe_sub(F, D, C); fe_add(G, D, C); fe_add(H, B, A);
    fe_mul(o->X, E, F); fe_mul(o->Y, G, H); fe_mul(o->T, E, H); fe_mul(o->Z, F, G);
}
static void pt_dbl(point *o, const point *p){
    fe A,B,C,D,E,F,G,H, t1;
    fe_sq(A, p->X); fe_sq(B, p->Y); fe_sq(t1, p->Z); fe_add(C, t1, t1); fe_carry(C);
    fe_neg(D, A); fe_carry(D);
    fe_add(t1, p->X, p->Y); fe_sq(E, t1); fe_sub(E, E, A); fe_sub(E, E, B);
    fe_add(G, D, B); fe_sub(F, G, C); fe_sub(H, D, B);
    fe_mul(o->X, E, F); fe_mul(o->Y, G, H); fe_mul(o->T, E, H); fe_mul(o->Z, F, G);
}
static void pt_neg(point *o, const point *p){ fe_neg(o->X, p->X); fe_copy(o->Y, p->Y); fe_copy(o->Z, p->Z); fe_neg(o->T, p->T); }
// decompress: returns 0 on failure
static int pt_decode(point *o, const u8 *s){
    fe y, y2, u, v, x, vx2, one, xx, vi;
    fe_frombytes(y, s);
    int sign = (s[31] >> 7) & 1;
    fe_1(one);
    fe_sq(y2, y);
    fe_mul(v, y2, ED_D); fe_add(v, v, one); fe_carry(v);   // v = d*y^2 + 1
    fe_sub(u, y2, one);                                    // u = y^2 - 1
    fe_inv(vi, v); fe_mul(xx, u, vi);                      // xx = u/v
    static const u8 EP[32] = { 0xfe,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0f };
    fe_pow(x, xx, EP);                                     // x = xx^((p+3)/8)
    fe_sq(vx2, x);
    if (!fe_eq(vx2, xx)){
        fe_mul(x, x, ED_I); fe_sq(vx2, x);
        if (!fe_eq(vx2, xx)) return 0;
    }
    if (fe_lowbit(x) != sign){ fe_neg(x, x); fe_carry(x); }
    fe_copy(o->X, x); fe_copy(o->Y, y); fe_1(o->Z); fe_mul(o->T, x, y);
    return 1;
}
static void pt_encode(u8 *s, const point *p){
    fe zi, x, y;
    fe_inv(zi, p->Z); fe_mul(x, p->X, zi); fe_mul(y, p->Y, zi);
    fe_tobytes(s, y); s[31] |= (u8)(fe_lowbit(x) << 7);
}
static void pt_scalarmult(point *o, const point *p, const u8 *k, int nbits){
    point r, a; pt_identity(&r); a = *p;
    for (int i=0;i<nbits;i++){
        int bit = (k[i/8] >> (i%8)) & 1;
        if (bit){ point t; pt_add(&t, &r, &a); r = t; }
        if (i+1<nbits){ point t; pt_dbl(&t, &a); a = t; }
    }
    *o = r;
}
// verify: sig = R||S, pub = A. Returns 1 valid.
static int tsg_ed25519_verify(const u8 *sig, const u8 *pub, const u8 *msg, int msglen){
    ed_init();
    // S < L check: L = 2^252 + 27742317777372353535851937790883648493
    static const u8 L[32] = { 0xed,0xd3,0xf5,0x5c,0x1a,0x63,0x12,0x58,0xd6,0x9c,0xf7,0xa2,0xde,0xf9,0xde,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10 };
    const u8 *S = sig + 32;
    int ge = 1; // is S >= L?
    { int cmp = 0;
      for (int i=31;i>=0;i--){ if (S[i]!=L[i]){ cmp = S[i]>L[i]? 1 : -1; break; } }
      ge = (cmp>=0); }
    if (ge) return 0;
    point A, R, pB, pH;
    if (!pt_decode(&A, pub)) return 0;
    if (!pt_decode(&R, sig)) return 0;
    // h = SHA512(R || A || M) as a 512-bit scalar
    u8 h[64]; { sha512_ctx c; sha512_init(&c); sha512_update(&c, sig, 32); sha512_update(&c, pub, 32); sha512_update(&c, msg, msglen); sha512_final(&c, h); }
    static const u8 BP[32] = { 0x58,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66 };
    point B; if (!pt_decode(&B, BP)) return 0;
    pt_scalarmult(&pB, &B, S, 253);
    pt_scalarmult(&pH, &A, h, 512);
    point sum; pt_add(&sum, &R, &pH);
    u8 e1[32], e2[32]; pt_encode(e1, &pB); pt_encode(e2, &sum);
    u8 diff = 0; for (int i=0;i<32;i++) diff |= e1[i]^e2[i];
    return diff==0;
}

