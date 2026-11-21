// THE SURVIVAL GAME — player/speed.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from buddies.c lines 775-791 in the Nov 2026 fine-grained refactor; code unchanged.

static float speed_curve(Player *p){   // the raw burden curve, pre-floor
    float bmul = 1.0f + 0.04f*p->upg[4]; if (bmul>1.40f) bmul=1.40f;
    float cap = 55.0f * (1.0f + 0.50f*p->upg[4]);
    return 148.0f * bmul / (1.0f + p->carried/cap);   // heavy hat = slow dev
}
static float player_speed(Player *p){
    float sp = speed_curve(p);
    // the floor (Daniel, Nov 14): however heavy the hat gets, the
    // crawl never drops below 40 px/s — rather slow, never extra
    // painfully slow. (The old curve sank toward ~28 under a full
    // 240 hat and toward nothing beyond it.) Sprint multiplies
    // after the floor, so it still helps at the bottom.
    if (sp < 40.0f) sp = 40.0f;
    if (p->cur_keys & K_SPRINT) sp *= 1.22f;
    return sp;
}

