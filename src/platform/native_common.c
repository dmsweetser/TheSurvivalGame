// THE SURVIVAL GAME — platform/native_common.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from platform.c lines 1078-1092 in the Nov 2026 fine-grained refactor; code unchanged.


#if defined(PLAT_WIN32) || defined(PLAT_LINUX) || defined(PLAT_MAC)
// civil date -> days since 1970-01-01 (Hinnant's algorithm, integer
// only) — shared by the native plat_daynum() implementations below
// (v1.25 feat 6: the weekly challenge's calendar).
static int days_from_civil(int y, int m, int d){
    y -= m <= 2;
    int era = (y >= 0 ? y : y-399) / 400;
    int yoe = y - era * 400;                                   // [0, 399]
    int doy = (153*(m + (m > 2 ? -3 : 9)) + 2)/5 + d-1;        // [0, 365]
    int doe = yoe * 365 + yoe/4 - yoe/100 + doy;               // [0, 146096]
    return era * 146097 + doe - 719468;
}
#endif

