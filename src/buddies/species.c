// THE SURVIVAL GAME — buddies/species.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 840-914 in the Nov 2026 fine-grained refactor; code unchanged.

// ---------------------------------------------------------------------------
// Banzai Buddies — species data and derived stats
// ---------------------------------------------------------------------------
#define BUDN 6
static const char *BUD_NAME[BUDN] = { "PUPPY", "KITTY", "MOOSE", "PLATYPUS", "PET ROCK", "KITCHEN SINK" };
static const char *BUD_MOVE2[BUDN] = { "POUNCE", "RAKE", "CHARGE", "SPUR DIVE", "ROLL OUT", "SPLASH DOWN" };
static const char *BUD_SPEC[BUDN] = { "BARK", "HAIRBALL", "STOMP", "VENOM STING", "ROCK STEADY", "OVERFLOW" };
static const char *BUD_MOVE3[BUDN] = { "FETCH", "FLYING CLAWS", "ANTLER SWEEP", "BILL SLAM", "SEDIMENT SLAM", "DISPOSAL" };
static const char *BUD_MOVE4[BUDN] = { "PUPPY EYES", "AMBUSH", "TIMBER!", "SIXTH SENSE", "AVALANCHE", "EVERYTHING" };
static const char *BUD_BASIC[BUDN] = { "CHOMP", "SCRATCH", "HEADBUTT", "BILL POKE", "BONK", "CLANG" };
// Buddy energy (Daniel): specials are not free anymore. The basic attack
// costs nothing; each taught move has a price, and the pool refills on
// its own as you go. Sleeping (Banzai World) refills it outright.
#define BUD_ENERGY_MAX 100.0f
#define BUD_EN_MOVE2 20.0f
#define BUD_EN_SPEC  45.0f
#define BUD_EN_MOVE3 30.0f
#define BUD_EN_MOVE4 55.0f
#define BUD_EN_REGEN 5.0f
static const int BUD_PRICE[BUDN] = { 100, 150, 300, 200, 60, 175 };
static const float BUD_HP0[BUDN] = { 95, 75, 230, 120, 420, 160 };
static const float BUD_DMG0[BUDN] = { 7, 5, 15, 9, 6, 10 };
static const float BUD_CD0[BUDN] = { 0.55f, 0.32f, 0.9f, 0.6f, 1.1f, 0.7f };
static const float BUD_SPD[BUDN] = { 185, 195, 165, 175, 118, 148 };   // the rock is in no hurry
static const float BUD_DEF[BUDN] = { 1.0f, 1.0f, 0.9f, 0.9f, 0.45f, 0.8f };   // damage taken multiplier
// the name list: obscure Elves (and a few Maiar) of the Elder Days,
// out of The Silmarillion — diacritics folded to plain letters
#define BUD_NAMES_N 60
// elven names from The Silmarillion, The Hobbit and The Lord of the
// Rings, ASCII-folded and alphabetized (Daniel, Nov 2). Half-elves
// Earendil, Elrond and Elwing make the cut; the old list's Maiar
// (Salmar, Osse and friends) do not — they were never elves.
static const char *BUD_ELF[BUD_NAMES_N] = {
    "AEGNOR", "AMRAS", "AMROD", "ANGROD", "AREDHEL", "ARGON", "ARWEN", "BELEG",
    "CARANTHIR", "CELEBORN", "CELEBRIAN", "CELEGORM", "CIRDAN", "CURUFIN", "DAERON", "DENETHOR",
    "EARENDIL", "ECTHELION", "EGALMOTH", "ELENWE", "ELLADAN", "ELROHIR", "ELROND", "ELWING",
    "EOL", "ERESTOR", "FEANOR", "FINARFIN", "FINDUILAS", "FINGOLFIN", "FINROD", "FINWE",
    "GALADRIEL", "GALION", "GILDOR", "GILGALAD", "GLORFINDEL", "GWINDOR", "HALDIR", "IDRIL",
    "INGWE", "LEGOLAS", "LINDIR", "LUTHIEN", "MABLUNG", "MAEDHROS", "MAEGLIN", "MAGLOR",
    "MIRIEL", "NERDANEL", "NIMLOTH", "ORODRETH", "OROPHIN", "PENGOLODH", "RUMIL", "SAEROS",
    "THINGOL", "THRANDUIL", "TURGON", "VORONWE"
};
// the original 20-name list, kept only so saves written before the
// expansion can map their stored index to the same name (the bnv
// save token marks new-format saves; the Maiar names that are gone
// fall back to the species default)
static const char *BUD_ELF_LEGACY[20] = {
    "AEGNOR", "AMROD", "AMRAS", "ANGROD", "CARANTHIR", "EOL", "MAEGLIN", "VORONWE",
    "GWINDOR", "MABLUNG", "BELEG", "FINDUILAS", "NERDANEL", "SALMAR", "ILMARE",
    "EONWE", "TILION", "ARIEN", "OSSE", "UINEN"
};
static const int BUD_DEF_NAME[BUDN] = { 7, 43, 57, 37, 24, 12 };   // BELEG, LUTHIEN, THRANDUIL, GWINDOR, EOL, CIRDAN
// buddy coats: 0 is the species' own color; 1-8 are the standard set;
// 9 is the psychedelic neon rainbow (cycles with time)
#define BUD_COLORS_N 10
static const char *BUD_COLOR_NAME[BUD_COLORS_N] = {
    "NATURAL", "RED", "ORANGE", "YELLOW", "GREEN", "BLUE", "PURPLE", "PINK", "BROWN", "NEON RAINBOW"
};
static u32 bud_hsv(float h){   // h in turns; full saturation, full neon
    h -= (int)h; if (h<0) h+=1.0f;
    float x = h*6.0f; int s = (int)x; float f = x - s;
    int up = (int)(f*255.0f), dn = 255-up;
    switch (s%6){
        case 0: return COL(255, up, 0); case 1: return COL(dn, 255, 0);
        case 2: return COL(0, 255, up); case 3: return COL(0, dn, 255);
        case 4: return COL(up, 0, 255); default: return COL(255, 0, dn);
    }
}
static u32 bud_coat(Buddy *b, u32 natural){
    static const u32 STD[8] = { COL(216,64,54), COL(240,140,40), COL(244,208,64), COL(84,190,84),
                                COL(72,130,230), COL(150,90,210), COL(244,140,190), COL(140,95,60) };
    if (b->color==9) return bud_hsv(G.anim_t*0.22f + (float)(b - G.bud)*0.37f);
    if (b->color>=1 && b->color<=8) return STD[b->color-1];
    return natural;
}
