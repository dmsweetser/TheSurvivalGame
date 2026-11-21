// THE SURVIVAL GAME — mall/whales.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from mall.c lines 1-156 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// WHALES ONLY — the mall's fifth counter (in the old Gallery slot).
// Seven purely cosmetic perks, $0.67 each, sold through Stripe payment
// links. Tapping BUY opens the player's browser; after paying, the
// payment page hands back a signed unlock code (Ed25519 — verified
// offline against the public key below; the signing key never ships).
// Codes are the player's receipt: entitlements live in the save data
// (Daniel's warning, printed in the shop), and re-entering a code after
// a wipe restores the perk. NOTHING here touches gameplay numbers.
// ---------------------------------------------------------------------------
#define WHALE_N 15
static const char *WHALE_NAME[WHALE_N] = {
    "GOLDEN CARCASS", "COMICALLY ENORMOUS GLASSES", "ROYAL CAPE",
    "SPARKLE TRAIL", "THE FORBIDDEN MONOCLE", "DRAMATIC RAIN", "WHALE TITLE",
    "ROYAL BUDDY", "PARADE DUCK", "DISCO MALL", "CONFETTI SQUASH",
    "AURA OF IMPORTANCE", "SPACE BUDDY", "NAME IN LIGHTS", "KAZOO MODE" };
static const char *WHALE_BLURB[WHALE_N] = {
    "EVERY CARCASS YOU CARRY, CAST IN GOLD. SAME WEIGHT. MORE GLORY.",
    "THE NERD GLASSES, BUT MORE. ENORMOUS-ER, EVEN. PURE VANITY.",
    "A ROYAL CAPE THAT BILLOWS DRAMATICALLY. ROYALTY NOT INCLUDED.",
    "YOU LEAVE A TRAIL OF SPARKLES. THE BUGS ARE NOT IMPRESSED.",
    "A GOLD MONOCLE FOR YOUR BUDDY. IT HAS SEEN THINGS. TINY THINGS.",
    "IT RAINS ON YOU. ONLY YOU. DRAMATICALLY. EVERYONE ELSE IS DRY.",
    "THE WORD WHALE, FLOATING BY YOUR NAME. EARNED. PAID FOR. YOURS.",
    "A TINY GOLDEN CROWN FOR YOUR BUDDY. ROYALTY, AT LAST.",
    "A DUCK FOLLOWS YOU. IT DOES NOTHING. IT IS NEVER LATE.",
    "THE BUG MALL FLOOR, BUT DISCO. THE KEEPERS PRETEND NOT TO NOTICE.",
    "SQUASHED BUGS BURST INTO CONFETTI. CLEANUP IS SOMEONE ELSE'S PROBLEM.",
    "A GOLDEN RING OF GLOW BENEATH YOU. YOU LOOK IMPORTANT. YOU ARE.",
    "A BUBBLE HELMET FOR YOUR BUDDY. THE MUFFLE IS ADORABLE.",
    "YOUR NAME IN MARQUEE BULBS ON THE TITLE SCREEN. TOP BILLING.",
    "THE WHOLE BEETHOVEN LADDER, ON KAZOO. HE WOULD HAVE WANTED THIS." };
// Stripe payment links — one per perk; filled in when the links exist.
static const char *WHALE_LINK[WHALE_N] = {
    "https://buy.stripe.com/bJebJ215lelo3QPe7OdUY01",   /* 0 Golden Carcass */
    "https://buy.stripe.com/4gMbJ26pFfpsbjhaVCdUY02",   /* 1 Comically Enormous Glasses */
    "https://buy.stripe.com/7sY6oIbJZ0uycnlbZGdUY03",   /* 2 Royal Cape */
    "https://buy.stripe.com/eVqdRa29p5OSfzx8NudUY04",   /* 3 Sparkle Trail */
    "https://buy.stripe.com/fZufZi4hxfpsafdbZGdUY05",   /* 4 The Forbidden Monocle */
    "https://buy.stripe.com/00wbJ2bJZfpsdrp1l2dUY06",   /* 5 Dramatic Rain */
    "https://buy.stripe.com/00wfZiaFV0uygDB7JqdUY07",  /* 6 WHALE Title */
    "https://buy.stripe.com/14A14o01hcdgfzx2p6dUY08",  /* 7 Royal Buddy */
    "https://buy.stripe.com/dRmbJ29BR6SWafd8NudUY09",  /* 8 Parade Duck */
    "https://buy.stripe.com/8x2cN6dS7elo3QPfbSdUY0a",  /* 9 Disco Mall */
    "https://buy.stripe.com/dRmeVe3dt6SW4UTbZGdUY0b",  /* 10 Confetti Squash */
    "https://buy.stripe.com/28E14o4hx5OS9b97JqdUY0c",  /* 11 Aura of Importance */
    "https://buy.stripe.com/eVqcN6eWb3GK3QPd3KdUY0d",  /* 12 Space Buddy */
    "https://buy.stripe.com/9B65kE4hxa58bjh2p6dUY0e",  /* 13 Name in Lights */
    "https://buy.stripe.com/7sYdRaaFV4KO2ML1l2dUY0f" }; /* 14 Kazoo Mode */
// The issuer's Ed25519 public key (the private key lives server-side).
static const u8 WHALE_PUB[32] = {
    0xc6,0x92,0x89,0x6e,0x1b,0x3d,0x00,0x7a,0xc8,0x95,0x8d,0x45,0x46,0x57,0x98,0xd7,
    0x46,0x06,0xa9,0xbc,0xa1,0xa2,0xb0,0x95,0xec,0xe4,0xc4,0x1f,0x29,0x7b,0x33,0xe4 };
// code-entry panel state (per player; only the local player types)
static int  whale_code_mode[MAXP];
static int  whale_bs_hold[MAXP];   // frames Backspace has been held (hold-to-delete)
static char whale_code_buf[MAXP][168];
static int  whale_code_len[MAXP];

static int b32_val(int c){
    if (c>='a' && c<='z') c -= 32;
    if (c>='0' && c<='9') return c-'0';
    if (c=='O' || c=='o') return 0;
    if (c=='I' || c=='L' || c=='i' || c=='l') return 1;
    if (c>='A' && c<='H') return c-'A'+10;
    if (c=='J') return 18;
    if (c=='K') return 19;
    if (c=='M') return 20;
    if (c=='N') return 21;
    if (c>='P' && c<='T') return c-'P'+22;
    if (c>='V' && c<='Z') return c-'V'+27;
    return -1;
}
// Decode + verify one cleaned run of exactly 119 significant chars.
static int whale_try_sig(const char *s){
    u8 raw[74]; int nb=0; u32 acc=0; int accn=0;
    for (int i=0;i<119;i++){
        int v = b32_val((u8)s[i]);
        if (v<0) return -1;
        acc = (acc<<5) | (u32)v; accn += 5;
        if (accn>=8){ accn -= 8; if (nb<74) raw[nb++] = (u8)(acc>>accn); }
    }
    if (nb!=74 || raw[0]!=1) return -1;
    int sku = raw[1];
    if (sku>=WHALE_N) return -1;
    if (!tsg_ed25519_verify(raw+10, WHALE_PUB, raw, 10)) return -1;
    return sku;
}
// Decode + verify a pasted unlock code. Returns the sku (0..6) or -1.
// Format: Crockford base32 of [version=1, sku, 8-byte nonce] || signature.
// Copy accidents happen (Daniel's first real code arrived with 143 of
// 142 chars showing and would not redeem): hyphens, spaces and look-alike
// letters are forgiven, and ONE stray extra character anywhere in the
// paste is forgiven too — the game tries dropping each character in
// turn. A code with a character CHANGED still fails every try, so the
// signature's protection is untouched.
static int whale_redeem(const char *code){
    char sig[200]; int n=0;
    for (const char *s=code; *s && n<199; s++){
        char c=*s;
        if (c=='-' || c==' ' || c=='\n' || c=='\r' || c=='\t') continue;
        if (b32_val((u8)c)<0) continue;   // stray punctuation is skipped, not fatal
        sig[n++]=c;
    }
    sig[n]=0;
    if (n==119) return whale_try_sig(sig);
    if (n==120){
        for (int k=0;k<120;k++){
            char t[120]; int m=0;
            for (int i=0;i<120;i++) if (i!=k) t[m++]=sig[i];
            t[119]=0;
            int sku = whale_try_sig(t);
            if (sku>=0) return sku;
        }
        return -1;
    }
    if (n>120){ // several extra at a copy boundary: try both clean ends
        char t[120];
        for (int i=0;i<119;i++) t[i]=sig[i];
        t[119]=0;
        int sku = whale_try_sig(t);
        if (sku>=0) return sku;
        for (int i=0;i<119;i++) t[i]=sig[n-119+i];
        t[119]=0;
        return whale_try_sig(t);
    }
    return -1;
}
static void whale_grant(int sku){
    G.whale_own |= (1<<sku); G.whale_on |= (1<<sku);
    for (int i=0;i<MAXP;i++) if (G.p[i].connected && G.p[i].is_local){
        G.p[i].whale_own=G.whale_own; G.p[i].whale_on=G.whale_on; }
    do_save();
}
// background pickup: the web build returns from the payment page with
// ?whale=<code>, and native players can drop the code into a
// whale-code.txt beside their saves. Checked a couple of times a second.
static float whale_poll_t = 0;
static void whale_poll(float dt){
    whale_poll_t -= dt;
    if (whale_poll_t>0) return;
    whale_poll_t = 0.4f;
    char buf[200];
    if (plat_whale_poll(buf, (int)sizeof(buf))<=0) return;
    int sku = whale_redeem(buf);
    if (sku>=0){ whale_grant(sku); store_note("UNLOCKED - CHECK THE WHALES ONLY COUNTER!"); sfx(11); }
    else store_note("A CODE ARRIVED BUT DID NOT CHECK OUT");
}


