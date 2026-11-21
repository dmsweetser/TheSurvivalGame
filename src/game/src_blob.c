// THE SURVIVAL GAME — game/src_blob.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).

// ---------------------------------------------------------------------------
// THE SOURCE ROOM (v1.26): read access to the embedded stock source.
// build.py generates ../src_embed.h from every .c file under src/
// before compiling (see gen_src_embed in build.py for the format
// contract — the native host's srcbuild.c regenerates the same format
// when a player rebuilds). This file is the only reader of the blob.
// ---------------------------------------------------------------------------
#include "../src_embed.h"

static int src_count(void){ return SRC_NFILES; }
static const char *src_path(int i){
    return (i>=0 && i<SRC_NFILES)? SRC_FILES[i].path : "";
}
static const char *src_text(int i, int *len){
    if (i<0 || i>=SRC_NFILES){ if (len) *len=0; return ""; }
    if (len) *len = (int)SRC_FILES[i].len;
    return (const char*)SRC_BLOB + SRC_FILES[i].off;
}
static int src_find(const char *path){
    for (int i=0;i<SRC_NFILES;i++) if (scmp(SRC_FILES[i].path, path)==0) return i;
    return -1;
}
