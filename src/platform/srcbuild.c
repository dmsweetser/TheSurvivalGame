// THE SURVIVAL GAME — platform/srcbuild.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
//
// HOST COMPILE (v1.26 Item 1e) — native only. The Source terminal's
// BUILD action stages the player's current source tree (edited files
// over stock) into <save-home>/tsg-build/src/, regenerates
// src_embed.h in build.py's exact format, compiles the WEB build with
// build.py's exact wasm command line on a worker thread, then
// assembles the page in C: gzip (stored blocks) + base64, spliced
// into the stock page template in place of its single long base64
// run. The result lands at <save-home>/tsg-build/page.html (atomic
// temp + rename) and the server serves it instead of stock. A failed
// compile NEVER touches the live page. The stock binary always boots
// stock code — the custom build is a served web page only.
// No webbuild.h is staged: the compile is -DPLAT_WASM, and the
// server's ../webbuild.h include sits in the native-only section of
// platform/server.c, so the wasm compile never looks for it.
#if defined(PLAT_WIN32) || defined(PLAT_LINUX) || defined(PLAT_MAC)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#if defined(PLAT_WIN32)
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#endif

static int sb_state = 0;              // 0 idle, 1 building, 2 done-ok, 3 failed
static char sb_log[16384];
static char sb_compiler[1200];
static int sb_compiler_zig = 1;       // 1: "<cc>" cc -target ...; 0: clang form
static int sb_found = -1;             // -1 undiscovered, 0 none, 1 found
static char sb_root[1200];            // <save-home>/tsg-build

static void sb_log_add(const char *t){
    int cur = (int)strlen(sb_log), add = (int)strlen(t);
    if (cur+add > (int)sizeof(sb_log)-1) add = (int)sizeof(sb_log)-1-cur;
    if (add>0){ memcpy(sb_log+cur, t, add); sb_log[cur+add]=0; }
}
static void sb_root_init(void){
    if (sb_root[0]) return;
    char home[900]; plat_save_home_dir(home, (int)sizeof(home));
    snprintf(sb_root, sizeof(sb_root), "%s/tsg-build", home);
}
static int sb_file_exists(const char *p){
    FILE *f = fopen(p, "rb");
    if (f){ fclose(f); return 1; }
    return 0;
}
// --- compiler discovery ---------------------------------------------------
static void sb_discover(void){
    if (sb_found>=0) return;
    sb_found = 0;
    const char *env = getenv("TSG_ZIG");
    if (env && env[0] && sb_file_exists(env)){
        snprintf(sb_compiler, sizeof(sb_compiler), "%s", env);
        sb_compiler_zig = 1; sb_found = 1; return;
    }
#if !defined(PLAT_WIN32)
    // zig on PATH
    { const char *path = getenv("PATH");
      if (path){
          char tmp[1400]; snprintf(tmp, sizeof(tmp), "%s", path);
          for (char *tok = strtok(tmp, ":"); tok; tok = strtok(0, ":")){
              char cand[1500]; snprintf(cand, sizeof(cand), "%s/zig", tok);
              if (access(cand, X_OK)==0){
                  snprintf(sb_compiler, sizeof(sb_compiler), "%s", cand);
                  sb_compiler_zig = 1; sb_found = 1; return;
              }
          }
      } }
    // common install spots: ~/workspace/tools/zig-*/zig (this dev
    // machine's layout), then beside nothing in particular
    { const char *home = getenv("HOME");
      if (home){
          char dir[1200]; snprintf(dir, sizeof(dir), "%s/workspace/tools", home);
          DIR *d = opendir(dir);
          if (d){
              struct dirent *e;
              while ((e = readdir(d))){
                  if (strncmp(e->d_name, "zig-", 4) && strncmp(e->d_name, "zig", 3)) continue;
                  char cand[1500]; snprintf(cand, sizeof(cand), "%s/%s/zig", dir, e->d_name);
                  if (access(cand, X_OK)==0){
                      snprintf(sb_compiler, sizeof(sb_compiler), "%s", cand);
                      sb_compiler_zig = 1; sb_found = 1;
                      closedir(d); return;
                  }
              }
              closedir(d);
          }
      } }
    // clang on PATH (target wasm32)
    { const char *path = getenv("PATH");
      if (path){
          char tmp[1400]; snprintf(tmp, sizeof(tmp), "%s", path);
          for (char *tok = strtok(tmp, ":"); tok; tok = strtok(0, ":")){
              char cand[1500]; snprintf(cand, sizeof(cand), "%s/clang", tok);
              if (access(cand, X_OK)==0){
                  snprintf(sb_compiler, sizeof(sb_compiler), "%s", cand);
                  sb_compiler_zig = 0; sb_found = 1; return;
              }
          }
      } }
#else
    { char exe[1200]; GetModuleFileNameA(0, exe, (DWORD)sizeof(exe));
      char *sl = strrchr(exe, '\\'); if (sl) *sl=0;
      const char *names[2] = { "zig.exe", "zig\\zig.exe" };
      for (int i=0;i<2;i++){
          char cand[1500]; snprintf(cand, sizeof(cand), "%s\\%s", exe, names[i]);
          if (sb_file_exists(cand)){
              snprintf(sb_compiler, sizeof(sb_compiler), "%s", cand);
              sb_compiler_zig = 1; sb_found = 1; return;
          }
      } }
    { const char *path = getenv("PATH");
      if (path){
          char tmp[1400]; snprintf(tmp, sizeof(tmp), "%s", path);
          for (char *tok = strtok(tmp, ";"); tok; tok = strtok(0, ";")){
              char cand[1500]; snprintf(cand, sizeof(cand), "%s\\zig.exe", tok);
              if (sb_file_exists(cand)){
                  snprintf(sb_compiler, sizeof(sb_compiler), "%s", cand);
                  sb_compiler_zig = 1; sb_found = 1; return;
              }
          }
      } }
#endif
}
int plat_srcbuild_available(void){ sb_discover(); return sb_found==1; }

// --- staging ---------------------------------------------------------------
void plat_srcbuild_begin(void){
    sb_root_init();
    char src[1300]; snprintf(src, sizeof(src), "%s/src", sb_root);
    tsg_rmrf(src);
    tsg_mkdirs(src);
}
void plat_srcbuild_file(const char *path, const char *data, int len){
    sb_root_init();
    char full[1600]; snprintf(full, sizeof(full), "%s/src/%s", sb_root, path);
    // create the parent folders
    char dir[1600]; snprintf(dir, sizeof(dir), "%s", full);
    { char *sl = strrchr(dir, '/'); if (sl) *sl=0; }
    tsg_mkdirs(dir);
    FILE *f = fopen(full, "wb");
    if (f){ fwrite(data, 1, (size_t)len, f); fclose(f); }
}
int plat_srcbuild_active(void){
    sb_root_init();
    char p[1300]; snprintf(p, sizeof(p), "%s/page.html", sb_root);
    return sb_file_exists(p);
}
void plat_srcbuild_clear(void){
    sb_root_init();
    char p[1300]; snprintf(p, sizeof(p), "%s/page.html", sb_root);
    remove(p);
}
unsigned char *plat_srcbuild_page(int *len){
    sb_root_init();
    char p[1300]; snprintf(p, sizeof(p), "%s/page.html", sb_root);
    FILE *f = fopen(p, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char *buf = (unsigned char*)malloc((size_t)n+1);
    if (!buf){ fclose(f); return 0; }
    if (n>0 && fread(buf, 1, (size_t)n, f)!=(size_t)n){ free(buf); fclose(f); return 0; }
    fclose(f);
    buf[n]=0;
    if (len) *len = (int)n;
    return buf;
}
int plat_srcbuild_state(void){ return sb_state; }
int plat_srcbuild_poll(void){
    if (sb_state==1) return 0;
    if (sb_state==3) return -1;
    return 1;
}
int plat_srcbuild_log(char *buf, int cap){
    int n = (int)strlen(sb_log);
    if (n>cap-1) n = cap-1;
    memcpy(buf, sb_log, n); buf[n]=0;
    return n;
}

// --- src_embed.h regeneration (build.py's format, mechanically) -----------
#define SB_MAXFILES 512
static char sb_files[SB_MAXFILES][160];
static int sb_nfiles = 0;
static int sb_cmpstr(const void *a, const void *b){ return strcmp((const char*)a, (const char*)b); }
#if defined(PLAT_WIN32)
// Win32 twin of the POSIX scan below: same contract — relative paths
// recorded with '/' separators, dot-entries skipped, only .c files
// collected (the caller qsorts, so traversal order does not matter).
static void sb_scan(const char *base, const char *rel){
    char pat[1700];
    if (rel[0]) snprintf(pat, sizeof(pat), "%s/%s/*", base, rel);
    else snprintf(pat, sizeof(pat), "%s/*", base);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pat, &fd);
    if (h==INVALID_HANDLE_VALUE) return;
    do {
        const char *name = fd.cFileName;
        if (name[0]=='.') continue;
        char r2[200]; snprintf(r2, sizeof(r2), "%s%s%s", rel, rel[0]? "/":"", name);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
            sb_scan(base, r2);
        } else {
            int L = (int)strlen(name);
            if (L>2 && !strcmp(name+L-2, ".c") && sb_nfiles<SB_MAXFILES){
                snprintf(sb_files[sb_nfiles], sizeof(sb_files[0]), "%s", r2);
                sb_nfiles++;
            }
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}
#else
static void sb_scan(const char *base, const char *rel){
    char full[1600]; snprintf(full, sizeof(full), "%s/%s", base, rel);
    DIR *d = opendir(full);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d))){
        if (e->d_name[0]=='.') continue;
        char r2[200]; snprintf(r2, sizeof(r2), "%s%s%s", rel, rel[0]? "/":"", e->d_name);
        char f2[1700]; snprintf(f2, sizeof(f2), "%s/%s", base, r2);
        struct stat st;
        if (stat(f2, &st)!=0) continue;
        if (S_ISDIR(st.st_mode)) sb_scan(base, r2);
        else {
            int L = (int)strlen(e->d_name);
            if (L>2 && !strcmp(e->d_name+L-2, ".c") && sb_nfiles<SB_MAXFILES){
                snprintf(sb_files[sb_nfiles], sizeof(sb_files[0]), "%s", r2);
                sb_nfiles++;
            }
        }
    }
    closedir(d);
}
#endif
static int sb_regen_embed(void){
    sb_nfiles = 0;
    char src[1300]; snprintf(src, sizeof(src), "%s/src", sb_root);
    sb_scan(src, "");
    qsort(sb_files, (size_t)sb_nfiles, sizeof(sb_files[0]), sb_cmpstr);
    // read every file, then write the header exactly as build.py does
    char out[1500]; snprintf(out, sizeof(out), "%s/src/src_embed.h", sb_root);
    FILE *o = fopen(out, "wb");
    if (!o) return -1;
    fprintf(o, "// generated by build.py \xe2\x80\x94 do not edit\n");
    fprintf(o, "// THE SOURCE ROOM: the game's own source, embedded so the\n");
    fprintf(o, "// in-game terminal can browse/edit it (and the native host\n");
    fprintf(o, "// can rebuild it). Regenerated by build.py every build;\n");
    fprintf(o, "// srcbuild.c regenerates the same format for staged builds.\n");
    fprintf(o, "static const unsigned char SRC_BLOB[] = {");
    long offs[SB_MAXFILES]; long lens[SB_MAXFILES];
    long total = 0;
    for (int i=0;i<sb_nfiles;i++){
        char full[1600]; snprintf(full, sizeof(full), "%s/%s", src, sb_files[i]);
        FILE *f = fopen(full, "rb");
        if (!f){ fclose(o); return -1; }
        fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
        unsigned char *data = (unsigned char*)malloc((size_t)n+1);
        if (n>0 && fread(data, 1, (size_t)n, f)!=(size_t)n){ free(data); fclose(f); fclose(o); return -1; }
        fclose(f);
        offs[i]=total; lens[i]=n; total+=n;
        for (long k=0;k<n;k++) fprintf(o, "%s%u", (total-n+k)>0||k>0? ",":"", (unsigned)data[k]);
        free(data);
    }
    fprintf(o, "};\n");
    fprintf(o, "typedef struct { const char *path; unsigned off, len; } SrcFileRec;\n");
    fprintf(o, "static const SrcFileRec SRC_FILES[] = {\n");
    for (int i=0;i<sb_nfiles;i++)
        fprintf(o, "    { \"%s\", %ld, %ld },\n", sb_files[i], offs[i], lens[i]);
    fprintf(o, "};\n");
    fprintf(o, "static const int SRC_NFILES = %d;\n", sb_nfiles);
    fclose(o);
    return 0;
}

// --- gzip (stored blocks) + base64 ------------------------------------------
static unsigned long sb_crc32(const unsigned char *data, long n){
    unsigned long crc = 0xFFFFFFFFul;
    for (long i=0;i<n;i++){
        crc ^= data[i];
        for (int k=0;k<8;k++) crc = (crc>>1) ^ (0xEDB88320ul & (unsigned long)(-(long)(crc&1)));
    }
    return crc ^ 0xFFFFFFFFul;
}
static long sb_gzip_stored(const unsigned char *in, long n, unsigned char *out){
    long o=0;
    out[o++]=0x1f; out[o++]=0x8b; out[o++]=8; out[o++]=0;
    out[o++]=0; out[o++]=0; out[o++]=0; out[o++]=0;   // mtime 0, like build.py
    out[o++]=0; out[o++]=3;
    long pos=0;
    for (;;){
        long chunk = n-pos; if (chunk>65535) chunk=65535;
        int fin = (pos+chunk>=n)? 1:0;
        out[o++]=(unsigned char)fin;
        out[o++]=(unsigned char)(chunk&255); out[o++]=(unsigned char)((chunk>>8)&255);
        unsigned inv = (unsigned)~chunk & 0xFFFFu;
        out[o++]=(unsigned char)(inv&255); out[o++]=(unsigned char)((inv>>8)&255);
        memcpy(out+o, in+pos, (size_t)chunk); o+=chunk; pos+=chunk;
        if (fin) break;
    }
    unsigned long crc = sb_crc32(in, n);
    out[o++]=(unsigned char)(crc&255); out[o++]=(unsigned char)((crc>>8)&255);
    out[o++]=(unsigned char)((crc>>16)&255); out[o++]=(unsigned char)((crc>>24)&255);
    out[o++]=(unsigned char)(n&255); out[o++]=(unsigned char)((n>>8)&255);
    out[o++]=(unsigned char)((n>>16)&255); out[o++]=(unsigned char)((n>>24)&255);
    return o;
}
static const char SB_B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static long sb_b64(const unsigned char *in, long n, char *out){
    long o=0;
    for (long i=0;i<n;i+=3){
        unsigned a=in[i], b=(i+1<n)? in[i+1]:0, c=(i+2<n)? in[i+2]:0;
        out[o++]=SB_B64[a>>2];
        out[o++]=SB_B64[((a&3)<<4)|(b>>4)];
        out[o++]=(i+1<n)? SB_B64[((b&15)<<2)|(c>>6)] : '=';
        out[o++]=(i+2<n)? SB_B64[c&63] : '=';
    }
    out[o]=0;
    return o;
}
static int sb_is_b64(unsigned char c){
    return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='+'||c=='/'||c=='=';
}
// assemble: splice the new base64 run into the stock page template
static int sb_assemble(const unsigned char *wasm, long wasm_n){
    long gzcap = wasm_n + (wasm_n/65535+2)*5 + 64;
    unsigned char *gz = (unsigned char*)malloc((size_t)gzcap);
    char *b64 = (char*)malloc((size_t)(wasm_n*2+16));
    if (!gz || !b64){ free(gz); free(b64); return -1; }
    long gz_n = sb_gzip_stored(wasm, wasm_n, gz);
    long b64_n = sb_b64(gz, gz_n, b64);
    free(gz);
    int stock_n=0; const unsigned char *stock = tsg_stock_page(&stock_n);
    if (!stock || stock_n<=0){ free(b64); return -1; }
    // the stock page's single long base64 run: find the longest one
    long best_s=-1, best_n=0;
    for (long i=0;i<stock_n;){
        if (sb_is_b64(stock[i])){
            long j=i; while (j<stock_n && sb_is_b64(stock[j])) j++;
            if (j-i>best_n){ best_n=j-i; best_s=i; }
            i=j;
        } else i++;
    }
    if (best_s<0 || best_n<1000){ free(b64); return -1; }
    long new_n = stock_n - best_n + b64_n;
    unsigned char *page = (unsigned char*)malloc((size_t)new_n);
    if (!page){ free(b64); return -1; }
    memcpy(page, stock, (size_t)best_s);
    memcpy(page+best_s, b64, (size_t)b64_n);
    memcpy(page+best_s+b64_n, stock+best_s+best_n, (size_t)(stock_n-best_s-best_n));
    free(b64);
    char tmp[1400], fin[1400];
    snprintf(tmp, sizeof(tmp), "%s/page.html.tmp", sb_root);
    snprintf(fin, sizeof(fin), "%s/page.html", sb_root);
    FILE *f = fopen(tmp, "wb");
    if (!f){ free(page); return -1; }
    fwrite(page, 1, (size_t)new_n, f);
    fclose(f);
    free(page);
    remove(fin);
    if (rename(tmp, fin)!=0) return -1;
    return 0;
}

// --- the compile itself ------------------------------------------------------
static void sb_slurp_log(void){
    char p[1400]; snprintf(p, sizeof(p), "%s/build.log", sb_root);
    FILE *f = fopen(p, "rb");
    if (!f) return;
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    if (n>12000){ fseek(f, n-12000, SEEK_SET); n = 12000; }   // tail
    int cur = (int)strlen(sb_log);
    if (cur + n > (int)sizeof(sb_log)-1) n = (int)sizeof(sb_log)-1-cur;
    if (n>0){ size_t rd = fread(sb_log+cur, 1, (size_t)n, f); sb_log[cur+(int)rd]=0; }
    fclose(f);
}
static void sb_worker(void){
    sb_log[0]=0;
    if (sb_regen_embed()!=0){ sb_log_add("STAGING FAILED: could not regenerate src_embed.h\n"); sb_state=3; return; }
    char cmd[4200], wasmpath[1400], srcmain[1400], logpath[1400];
    snprintf(wasmpath, sizeof(wasmpath), "%s/game.wasm", sb_root);
    snprintf(srcmain, sizeof(srcmain), "%s/src/survival.c", sb_root);
    snprintf(logpath, sizeof(logpath), "%s/build.log", sb_root);
    if (sb_compiler_zig)
        snprintf(cmd, sizeof(cmd),
            "\"%s\" cc -target wasm32-freestanding -DPLAT_WASM -O2 -std=c99 -nostdlib "
            "-Wl,--no-entry -Wl,--export=frame -Wl,--export-memory -Wl,--initial-memory=16777216 "
            "-o \"%s\" \"%s\" > \"%s\" 2>&1",
            sb_compiler, wasmpath, srcmain, logpath);
    else
        snprintf(cmd, sizeof(cmd),
            "\"%s\" -target wasm32-freestanding -DPLAT_WASM -O2 -std=c99 -nostdlib "
            "-Wl,--no-entry -Wl,--export=frame -Wl,--export-memory -Wl,--initial-memory=16777216 "
            "-o \"%s\" \"%s\" > \"%s\" 2>&1",
            sb_compiler, wasmpath, srcmain, logpath);
    sb_log_add("BUILDING WITH: "); sb_log_add(sb_compiler); sb_log_add("\n");
    int rc = system(cmd);
    sb_slurp_log();
    if (rc!=0){
        sb_log_add("\nBUILD FAILED - the last good page stays served.\n");
        sb_state=3; return;
    }
    FILE *f = fopen(wasmpath, "rb");
    if (!f){ sb_log_add("\nBUILD FAILED: no wasm produced.\n"); sb_state=3; return; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char *wasm = (unsigned char*)malloc((size_t)n);
    if (!wasm || fread(wasm, 1, (size_t)n, f)!=(size_t)n){ free(wasm); fclose(f); sb_state=3; return; }
    fclose(f);
    if (sb_assemble(wasm, n)!=0){
        free(wasm);
        sb_log_add("\nBUILD FAILED at page assembly - the last good page stays served.\n");
        sb_state=3; return;
    }
    free(wasm);
    sb_log_add("\nBUILD OK - your page is live. This window still runs stock code;\nonly the served web page changed.\n");
    sb_state=2;
}
#if defined(PLAT_WIN32)
static DWORD WINAPI sb_thread(LPVOID u){ (void)u; sb_worker(); return 0; }
#else
static void *sb_thread(void *u){ (void)u; sb_worker(); return 0; }
#endif
void plat_srcbuild_run_async(void){
    sb_root_init();
    sb_discover();
    if (sb_found!=1){ sb_state=3; sb_log[0]=0; sb_log_add("NO COMPILER FOUND on this host.\n"); return; }
    if (sb_state==1) return;
    sb_state=1;
#if defined(PLAT_WIN32)
    { HANDLE h = CreateThread(0, 0, sb_thread, 0, 0, 0); if (h) CloseHandle(h); else sb_state=3; }
#else
    { pthread_t th; if (pthread_create(&th, 0, sb_thread, 0)==0) pthread_detach(th); else sb_state=3; }
#endif
}

#endif // native only
