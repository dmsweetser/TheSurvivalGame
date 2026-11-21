// THE SURVIVAL GAME — platform/posix.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from platform.c lines 1369-1459 in the Nov 2026 fine-grained refactor; code unchanged.


// ===========================================================================
// POSIX shared bits for the Linux and macOS builds: a small software
// synth (the WebAudio voices, re-implemented in C) and BSD-socket UDP.
// ===========================================================================
#if defined(PLAT_LINUX) || defined(PLAT_MAC)
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

double plat_now(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec/1e9;
}
int plat_daynum(void){
    time_t t = time(0); struct tm lt;
    localtime_r(&t, &lt);
    return days_from_civil(lt.tm_year+1900, lt.tm_mon+1, lt.tm_mday);
}
float plat_minutes(void){
    time_t t = time(0); struct tm lt;
    localtime_r(&t, &lt);
    return (float)lt.tm_hour*60.0f + (float)lt.tm_min + (float)lt.tm_sec/60.0f;
}

// --- software synth: square / triangle / noise voices with decay ---
typedef struct { double t0; float freq, dur, gain; int instr, ch; unsigned seed; } Voice;
static Voice g_voices[48];
void plat_note(int ch, int note, int vel, int instr){
    if (vel<=0 || note<0 || note>127) return;
    double now = plat_now();
    for (int i=0;i<48;i++){
        Voice *v=&g_voices[i];
        if (v->t0<=0 || now > v->t0 + v->dur){
            v->t0 = now;
            v->freq = (float)(440.0 * pow(2.0, (note-69)/12.0));
            v->dur = (instr==3)? 0.13f : (ch==1? 0.24f : 0.16f);
            float vv = vel/110.0f; if (vv>1) vv=1;
            v->gain = vv * (ch==0? 0.30f : 0.36f) * (instr==3? 0.7f : 1.0f);
            v->instr = instr; v->ch = ch;
            v->seed = (unsigned)(now*44100.0) + (unsigned)i*777u;
            return;
        }
    }
}
// render n mono float samples at 44100 Hz starting at absolute time `now`
void synth_render(float *out, int n, double now){
    for (int i=0;i<n;i++) out[i]=0;
    for (int vi=0; vi<48; vi++){
        Voice *v=&g_voices[vi]; if (v->t0<=0) continue;
        for (int i=0;i<n;i++){
            double t = now + (double)i/44100.0 - v->t0;
            if (t<0) continue;
            if (t>=v->dur) break;
            float s;
            if (v->instr==3){
                unsigned sd = v->seed + (unsigned)(t*44100.0);
                sd = sd*1664525u + 1013904223u;
                s = ((float)((sd>>9)&0xFFFF)/32768.0f - 1.0f) * (1.0f - (float)(t/v->dur));
            } else {
                double ph = t * v->freq; ph -= (double)(long long)ph;
                if (v->instr==4){   // WHALES ONLY: KAZOO — buzzy square, 29 Hz buzz, a breath of noise
                    float sq = ph<0.5? 1.0f : -1.0f;
                    float am = 0.72f + 0.28f*(float)sin(6.2831853*29.0*t);
                    unsigned sd2 = v->seed + (unsigned)(t*44100.0); sd2 = sd2*1664525u + 1013904223u;
                    s = sq*am + ((float)((sd2>>9)&0xFFFF)/32768.0f - 1.0f)*0.16f;
                }
                else s = (v->instr==1)? (float)(2.0*fabs(2.0*ph-1.0)-1.0) : (ph<0.5? 1.0f : -1.0f);
                s *= (float)exp(-6.0 * t / v->dur);
            }
            out[i] += s * v->gain;
        }
    }
    for (int i=0;i<n;i++){ if (out[i]>1) out[i]=1; else if (out[i]<-1) out[i]=-1; }
}
// (networking lives in the shared embedded-web-server section at the
// end of this file: the app serves the web build and browser guests
// relay straight into the host — there is no separate UDP transport.)

// ---------------------------------------------------------------------------
// THE SOURCE ROOM (v1.26): shared POSIX helpers + the mod overlay.
// The save home mirrors each backend's own save location (linux.c's
// ~/.local/share, mac.c's Library/Application Support); edited source
// files live under <home>/tsg-modsrc/, mirroring the src-relative
// path. tsg_mkdirs/tsg_rmrf are shared with srcbuild.c's staging.
// ---------------------------------------------------------------------------
#include <sys/stat.h>
#include <dirent.h>
void plat_save_home_dir(char *out, int cap){
    const char *home = getenv("HOME");
    if (!home || !home[0]){ if (cap>0) out[0]=0; return; }
#ifdef PLAT_MAC
    snprintf(out, (size_t)cap, "%s/Library/Application Support", home);
#else
    snprintf(out, (size_t)cap, "%s/.local/share", home);
#endif
}
static void tsg_mkdirs(const char *dirpath){
    char tmp[1200]; snprintf(tmp, sizeof(tmp), "%s", dirpath);
    for (char *p = tmp+1; *p; p++){
        if (*p=='/'){ *p=0; mkdir(tmp, 0755); *p='/'; }
    }
    mkdir(tmp, 0755);
}
static void tsg_rmrf(const char *path){
    struct stat st;
    if (stat(path, &st)!=0) return;
    if (S_ISDIR(st.st_mode)){
        DIR *d = opendir(path);
        if (d){
            struct dirent *e;
            while ((e = readdir(d))){
                if (e->d_name[0]=='.' && (!e->d_name[1] || (e->d_name[1]=='.' && !e->d_name[2]))) continue;
                char sub[1200]; snprintf(sub, sizeof(sub), "%s/%s", path, e->d_name);
                tsg_rmrf(sub);
            }
            closedir(d);
        }
        rmdir(path);
    } else remove(path);
}
static void mod_path(char *out, int cap, const char *rel){
    char home[1024]; plat_save_home_dir(home, (int)sizeof(home));
    if (rel) snprintf(out, (size_t)cap, "%s/tsg-modsrc/%s", home, rel);
    else snprintf(out, (size_t)cap, "%s/tsg-modsrc", home);
}
int plat_mod_load(const char *path, char *buf, int cap){
    char fp[1200]; mod_path(fp, (int)sizeof(fp), path);
    FILE *f = fopen(fp, "rb");
    if (!f) return -1;
    int n = (int)fread(buf, 1, (size_t)cap, f); fclose(f);
    return n;
}
void plat_mod_save(const char *path, const char *data, int len){
    char fp[1200]; mod_path(fp, (int)sizeof(fp), path);
    if (len<=0){ remove(fp); return; }   // an empty overlay is no overlay
    char dir[1200]; snprintf(dir, sizeof(dir), "%s", fp);
    char *slash = strrchr(dir, '/'); if (slash) *slash = 0;
    tsg_mkdirs(dir);
    FILE *f = fopen(fp, "wb");
    if (!f) return;
    fwrite(data, 1, (size_t)len, f); fclose(f);
}
void plat_mod_clear(void){
    char dir[1200]; mod_path(dir, (int)sizeof(dir), 0);
    tsg_rmrf(dir);
}
int plat_mod_exists(const char *path){
    char fp[1200]; mod_path(fp, (int)sizeof(fp), path);
    struct stat st; return stat(fp, &st)==0 && S_ISREG(st.st_mode);
}

#endif // PLAT_LINUX || PLAT_MAC
