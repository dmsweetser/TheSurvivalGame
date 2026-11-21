// THE SURVIVAL GAME — platform/server.c
// Unity-build piece: #included by survival.c in a fixed order (see the map there).
// Split from platform.c lines 1791-2215 in the Nov 2026 fine-grained refactor; code unchanged.


// ===========================================================================
// PLATFORM (all native builds): the embedded web server + game networking.
//
// This is what makes co-op zero-setup: the native app serves the web
// build of the game over HTTP on port 8379 and relays browser guests
// straight into the host's simulation over WebSocket. The host player
// just runs the app; friends open http://<this machine>:8379 in any
// browser and they are in — no installs, no mode to pick, no separate
// server to run. plat_net_* below is backed by two thread-safe queues
// between the game thread and the server thread.
// ===========================================================================
#if defined(PLAT_WIN32) || defined(PLAT_LINUX) || defined(PLAT_MAC)

// the web build, generated into webbuild.h by build.py (may be absent
// when compiling survival.c standalone — then a placeholder is served)
#if defined(__has_include)
#  if __has_include("../webbuild.h")
#    include "../webbuild.h"
#    define TSG_HAVE_WEB 1
#  endif
#endif
#ifndef TSG_HAVE_WEB
static const unsigned char TSG_WEB_HTML[] =
    "<html><body><h1>THE SURVIVAL GAME</h1>"
    "<p>The web build is not embedded in this binary.</p></body></html>";
static const unsigned TSG_WEB_HTML_LEN = sizeof(TSG_WEB_HTML)-1;
#endif

#define WEB_PORT 8379

// --- socket / thread / mutex shims ---
#ifdef PLAT_WIN32
typedef SOCKET sock_t;
#define SOCK_BAD INVALID_SOCKET
#define sock_close(s) closesocket(s)
#define SEND_FLAGS 0
static CRITICAL_SECTION g_qmtx; static int g_qmtx_ok=0;
static void qlock(void){ if (!g_qmtx_ok){ InitializeCriticalSection(&g_qmtx); g_qmtx_ok=1; } EnterCriticalSection(&g_qmtx); }
static void qunlock(void){ LeaveCriticalSection(&g_qmtx); }
#else
typedef int sock_t;
#define SOCK_BAD (-1)
#define sock_close(s) close(s)
#define SEND_FLAGS MSG_NOSIGNAL
static pthread_mutex_t g_qmtx = PTHREAD_MUTEX_INITIALIZER;
static void qlock(void){ pthread_mutex_lock(&g_qmtx); }
static void qunlock(void){ pthread_mutex_unlock(&g_qmtx); }
#endif
static void server_loop(void);
#ifdef PLAT_WIN32
static DWORD WINAPI server_thread_main(LPVOID p){ (void)p; server_loop(); return 0; }
static void start_server_thread(void){ HANDLE h=CreateThread(0,0,server_thread_main,0,0,0); if (h) CloseHandle(h); }
#else
static void *server_thread_main(void *p){ (void)p; server_loop(); return 0; }
static void start_server_thread(void){ pthread_t t; if (pthread_create(&t,0,server_thread_main,0)==0) pthread_detach(t); }
#endif

// --- SHA-1 + base64 (for the WebSocket handshake) ---
typedef struct { unsigned h[5]; unsigned long long len; unsigned char buf[64]; int n; } Sha1;
static unsigned rol32(unsigned v, int b){ return (v<<b) | (v>>(32-b)); }
static void sha1_block(Sha1 *s, const unsigned char *p){
    unsigned w[80];
    for (int i=0;i<16;i++) w[i]=((unsigned)p[i*4]<<24)|((unsigned)p[i*4+1]<<16)|((unsigned)p[i*4+2]<<8)|(unsigned)p[i*4+3];
    for (int i=16;i<80;i++) w[i]=rol32(w[i-3]^w[i-8]^w[i-14]^w[i-16],1);
    unsigned a=s->h[0], b=s->h[1], c=s->h[2], d=s->h[3], e=s->h[4];
    for (int i=0;i<80;i++){
        unsigned f,k;
        if (i<20){ f=(b&c)|((~b)&d); k=0x5A827999u; }
        else if (i<40){ f=b^c^d; k=0x6ED9EBA1u; }
        else if (i<60){ f=(b&c)|(b&d)|(c&d); k=0x8F1BBCDCu; }
        else { f=b^c^d; k=0xCA62C1D6u; }
        unsigned t=rol32(a,5)+f+e+k+w[i]; e=d; d=c; c=rol32(b,30); b=a; a=t;
    }
    s->h[0]+=a; s->h[1]+=b; s->h[2]+=c; s->h[3]+=d; s->h[4]+=e;
}
static void sha1_init(Sha1 *s){
    s->h[0]=0x67452301u; s->h[1]=0xEFCDAB89u; s->h[2]=0x98BADCFEu; s->h[3]=0x10325476u; s->h[4]=0xC3D2E1F0u;
    s->len=0; s->n=0;
}
static void sha1_update(Sha1 *s, const unsigned char *p, int n){
    s->len += (unsigned)n;
    while (n>0){
        int take = 64 - s->n; if (take>n) take=n;
        memcpy(s->buf + s->n, p, (size_t)take); s->n += take; p += take; n -= take;
        if (s->n==64){ sha1_block(s, s->buf); s->n=0; }
    }
}
static void sha1_final(Sha1 *s, unsigned char out[20]){
    unsigned long long bits = s->len * 8;
    s->buf[s->n++] = 0x80;
    if (s->n > 56){ while (s->n<64) s->buf[s->n++]=0; sha1_block(s, s->buf); s->n=0; }
    while (s->n<56) s->buf[s->n++]=0;
    for (int i=0;i<8;i++) s->buf[56+i] = (unsigned char)(bits >> (56-8*i));
    sha1_block(s, s->buf);
    for (int i=0;i<5;i++){
        out[i*4]=(unsigned char)(s->h[i]>>24); out[i*4+1]=(unsigned char)(s->h[i]>>16);
        out[i*4+2]=(unsigned char)(s->h[i]>>8); out[i*4+3]=(unsigned char)s->h[i];
    }
}
static void b64_encode(const unsigned char *in, int n, char *out){
    static const char T[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int o=0;
    for (int i=0;i<n;i+=3){
        int rem = n-i; unsigned v = (unsigned)in[i]<<16;
        if (rem>1) v |= (unsigned)in[i+1]<<8;
        if (rem>2) v |= in[i+2];
        out[o++] = T[(v>>18)&63]; out[o++] = T[(v>>12)&63];
        out[o++] = rem>1? T[(v>>6)&63] : '=';
        out[o++] = rem>2? T[v&63] : '=';
    }
    out[o]=0;
}

// --- packet queues between the game thread and the server thread ---
#define WQ_DEPTH 96
#define WQ_PKT 8192
typedef struct { int src; int len; int raw; unsigned char data[WQ_PKT]; } WPkt;
static WPkt g_inq[WQ_DEPTH];  static int g_inq_h=0, g_inq_t=0;
static WPkt g_outq[WQ_DEPTH]; static int g_outq_h=0, g_outq_t=0;
static int q_push(WPkt *q, int *h, int *t, int src, const unsigned char *d, int len, int raw){
    qlock();
    int nt = (*t+1) % WQ_DEPTH;
    if (nt == *h){ qunlock(); return 0; }              // full: drop
    WPkt *p = &q[*t];
    if (len > WQ_PKT) len = WQ_PKT;
    p->src = src; p->len = len; p->raw = raw;
    if (len > 0) memcpy(p->data, d, (size_t)len);
    *t = nt;
    qunlock();
    return 1;
}
static int q_pop(WPkt *q, int *h, int *t, WPkt *out){
    qlock();
    if (*h == *t){ qunlock(); return 0; }
    *out = q[*h];
    *h = (*h+1) % WQ_DEPTH;
    qunlock();
    return 1;
}

// --- connections ---
#define MAXCONN 8
typedef struct {
    sock_t fd; int used; int upgraded; int token;
    unsigned char rbuf[8192]; int rlen;
    unsigned char msg[WQ_PKT]; int mlen;
    // THE SOURCE (v1.26): a /src/put body can dwarf rbuf — it streams
    // into a malloc'd buffer until big_len reaches big_want
    unsigned char *big; int big_len, big_cap, big_want, big_clen;
    char big_path[200];
} Conn;
static Conn g_conns[MAXCONN];
static sock_t g_listen = SOCK_BAD;
static char g_join_addr[64] = "";
static unsigned char g_page[sizeof(TSG_WEB_HTML) + 4096];   // sized from the embedded page itself (+ room for the hosted-flag injection) — a fixed 300000 cap silently overflowed when the web build grew past it
static int g_page_len = 0;

static int send_all(sock_t fd, const unsigned char *d, int len){
    int sent = 0;
    while (sent < len){
        int n = (int)send(fd, (const char*)d + sent, len - sent, SEND_FLAGS);
        if (n <= 0) return 0;
        sent += n;
    }
    return 1;
}
static int ws_frame_build(unsigned char *out, const unsigned char *d, int len){
    int h = 0;
    out[h++] = 0x82;                                   // FIN + binary
    if (len < 126){ out[h++] = (unsigned char)len; }
    else if (len < 65536){ out[h++] = 126; out[h++] = (unsigned char)(len>>8); out[h++] = (unsigned char)len; }
    else { out[h++] = 127; for (int i=7;i>=0;i--) out[h++] = (unsigned char)((unsigned long long)len >> (i*8)); }
    if (len > 0) memcpy(out + h, d, (size_t)len);
    return h + len;
}
static void conn_close(int i){
    Conn *c = &g_conns[i];
    if (!c->used) return;
    if (c->big){ free(c->big); c->big = 0; }
    if (c->upgraded){
        unsigned char l = 'L';
        q_push(g_inq, &g_inq_h, &g_inq_t, c->token, &l, 1, 0);   // tell the game
    }
    sock_close(c->fd);
    c->used = 0; c->upgraded = 0; c->rlen = 0; c->mlen = 0;
}
static void conn_src_put_done(int i);   // below
static void conn_http(int i, char *req, int hdr_len){
    Conn *c = &g_conns[i];
    if (strstr(req, "pgrade: websocket") || strstr(req, "pgrade:WebSocket")){
        // WebSocket upgrade: answer the handshake, become a game peer
        const char *kp = strstr(req, "Sec-WebSocket-Key:");
        char key[128]; key[0]=0;
        if (kp){
            kp += strlen("Sec-WebSocket-Key:");
            while (*kp==' ') kp++;
            int n=0; while (kp[n] && kp[n]!='\r' && kp[n]!='\n' && n<120){ key[n]=kp[n]; n++; }
            key[n]=0;
        }
        char combo[256];
        snprintf(combo, sizeof(combo), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", key);
        Sha1 s; sha1_init(&s); sha1_update(&s, (const unsigned char*)combo, (int)strlen(combo));
        unsigned char digest[20]; sha1_final(&s, digest);
        char accept[64]; b64_encode(digest, 20, accept);
        char resp[512];
        int rn = snprintf(resp, sizeof(resp),
            "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
            "Connection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", accept);
        if (send_all(c->fd, (const unsigned char*)resp, rn)){
            c->upgraded = 1; c->token = i+1; c->rlen = 0; c->mlen = 0;
#ifdef PLAT_WIN32
            { unsigned long nd = 1; setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&nd, sizeof(nd)); }
#else
            { int nd = 1; setsockopt(c->fd, IPPROTO_TCP, TCP_NODELAY, &nd, sizeof(nd)); }
#endif
            return;
        }
        conn_close(i);
        return;
    }
    // THE SOURCE (v1.26): the host-compile test surface + page route
    { char method[8]; char path[512];
      method[0]=0; path[0]=0;
      sscanf(req, "%7s %511s", method, path);
      if (!strcmp(method, "POST") && !strncmp(path, "/src/put?path=", 14)){
          const char *rel = path + 14;
          int clen = 0;
          const char *cl = strstr(req, "Content-Length:");
          if (cl) clen = atoi(cl + 15);
          if (strstr(rel, "..") || rel[0]==0 || clen < 0 || clen > 600000){
              const char *bad = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
              send_all(c->fd, (const unsigned char*)bad, (int)strlen(bad));
              conn_close(i); return;
          }
          if (c->rlen >= hdr_len + clen){
              plat_srcbuild_file(rel, (const char*)c->rbuf + hdr_len, clen);
              const char *ok = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n";
              send_all(c->fd, (const unsigned char*)ok, (int)strlen(ok));
              conn_close(i); return;
          }
          c->big_cap = hdr_len + clen; c->big_want = c->big_cap; c->big_clen = clen;
          c->big = (unsigned char*)malloc((size_t)c->big_cap + 1);
          if (!c->big){ conn_close(i); return; }
          memcpy(c->big, c->rbuf, (size_t)c->rlen);
          c->big_len = c->rlen; c->rlen = 0;
          snprintf(c->big_path, sizeof(c->big_path), "%s", rel);
          return;   // conn_read keeps collecting into big
      }
      if (!strcmp(method, "POST") && !strcmp(path, "/src/build")){
          plat_srcbuild_run_async();
          const char *ok = "HTTP/1.1 200 OK\r\nContent-Length: 9\r\n\r\nBUILDING\n";
          send_all(c->fd, (const unsigned char*)ok, (int)strlen(ok));
          conn_close(i); return;
      }
      if (!strcmp(method, "GET") && !strcmp(path, "/src/status")){
          static char body[16400];
          int st = plat_srcbuild_state();
          const char *word = st==1? "building" : st==2? "done-ok" : st==3? "done-failed" : "idle";
          int bn = snprintf(body, sizeof(body), "%s\n", word);
          plat_srcbuild_log(body + bn, (int)sizeof(body) - bn - 1);
          char hdr[160];
          int hn = snprintf(hdr, sizeof(hdr),
              "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\n"
              "Content-Length: %d\r\nConnection: close\r\n\r\n", (int)strlen(body));
          send_all(c->fd, (const unsigned char*)hdr, hn);
          send_all(c->fd, (const unsigned char*)body, (int)strlen(body));
          conn_close(i); return;
      }
    }
    // plain HTTP: serve the game page — the player's custom build
    // first when one is live (v1.26), else the stock page (both carry
    // the hosted flag: the custom page was spliced from the stock one)
    { int cplen = 0; unsigned char *cp = plat_srcbuild_page(&cplen);
      if (cp){
          char hdr[256];
          int hn = snprintf(hdr, sizeof(hdr),
              "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
              "Content-Length: %d\r\nConnection: close\r\n\r\n", cplen);
          send_all(c->fd, (const unsigned char*)hdr, hn);
          send_all(c->fd, cp, cplen);
          free(cp);
          sock_close(c->fd);
          c->used = 0; c->rlen = 0;
          return;
      } }
    if (g_page_len > 0){
        char hdr[256];
        int hn = snprintf(hdr, sizeof(hdr),
            "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\nConnection: close\r\n\r\n", g_page_len);
        send_all(c->fd, (const unsigned char*)hdr, hn);
        send_all(c->fd, g_page, g_page_len);
    } else {
        const char *nf = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send_all(c->fd, (const unsigned char*)nf, (int)strlen(nf));
    }
    sock_close(c->fd);
    c->used = 0; c->rlen = 0;
}
static void conn_src_put_done(int i){
    Conn *c = &g_conns[i];
    plat_srcbuild_file(c->big_path, (const char*)c->big + (c->big_want - c->big_clen), c->big_clen);
    const char *ok = "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n";
    send_all(c->fd, (const unsigned char*)ok, (int)strlen(ok));
    conn_close(i);
}
static void conn_read(int i){
    Conn *c = &g_conns[i];
    if (!c->upgraded && c->big){
        int space = c->big_cap - c->big_len;
        if (space <= 0){ conn_close(i); return; }
        int n = (int)recv(c->fd, (char*)c->big + c->big_len, space, 0);
        if (n <= 0){ conn_close(i); return; }
        c->big_len += n;
        if (c->big_len >= c->big_want) conn_src_put_done(i);
        return;
    }
    int space = (int)sizeof(c->rbuf) - c->rlen;
    if (space <= 0){ conn_close(i); return; }
    int n = (int)recv(c->fd, (char*)c->rbuf + c->rlen, space, 0);
    if (n <= 0){ conn_close(i); return; }
    c->rlen += n;
    if (!c->upgraded){
        // waiting for a full HTTP request header
        if (c->rlen >= 4){
            char tmp[4200];
            int m = c->rlen < 4100? c->rlen : 4100;
            memcpy(tmp, c->rbuf, (size_t)m); tmp[m]=0;
            char *he = strstr(tmp, "\r\n\r\n");
            if (he) conn_http(i, tmp, (int)(he - tmp) + 4);
            else if (c->rlen > 4096) conn_close(i);
        }
        return;
    }
    // WebSocket frames
    for (;;){
        if (c->rlen < 2) return;
        unsigned b0 = c->rbuf[0], b1 = c->rbuf[1];
        int fin = (b0 & 0x80)!=0, opcode = b0 & 0x0F;
        int masked = (b1 & 0x80)!=0;
        unsigned long long len = b1 & 0x7F;
        int hdr = 2;
        if (len == 126){
            if (c->rlen < 4) return;
            len = ((unsigned)c->rbuf[2]<<8) | c->rbuf[3]; hdr = 4;
        } else if (len == 127){
            if (c->rlen < 10) return;
            len = 0; for (int k=0;k<8;k++) len = (len<<8) | c->rbuf[2+k];
            hdr = 10;
        }
        if (len > WQ_PKT){ conn_close(i); return; }
        if (masked) hdr += 4;
        if (c->rlen < hdr + (int)len) return;          // incomplete: wait
        unsigned char *pl = c->rbuf + hdr;
        if (masked){
            const unsigned char *mk = c->rbuf + hdr - 4;
            for (unsigned long long k=0;k<len;k++) pl[k] ^= mk[k&3];
        }
        if (opcode == 8){ conn_close(i); return; }     // close
        else if (opcode == 9){                          // ping -> pong
            unsigned char fr[16];
            int h = 0; fr[h++] = 0x8A;
            if (len < 126){ fr[h++] = (unsigned char)len; if (len > 0) memcpy(fr+h, pl, (size_t)len); h += (int)len; }
            q_push(g_outq, &g_outq_h, &g_outq_t, c->token, fr, h, 1);
        } else if (opcode == 0 || opcode == 1 || opcode == 2){
            if (c->mlen + (int)len <= WQ_PKT){
                if (len > 0) memcpy(c->msg + c->mlen, pl, (size_t)len);
                c->mlen += (int)len;
            }
            if (fin){
                if (c->mlen > 0) q_push(g_inq, &g_inq_h, &g_inq_t, c->token, c->msg, c->mlen, 0);
                c->mlen = 0;
            }
        }
        int consumed = hdr + (int)len;
        memmove(c->rbuf, c->rbuf + consumed, (size_t)(c->rlen - consumed));
        c->rlen -= consumed;
    }
}
static void drain_outq(void){
    WPkt p;
    static unsigned char frame[WQ_PKT + 16];
    while (q_pop(g_outq, &g_outq_h, &g_outq_t, &p)){
        if (p.raw){                                    // pre-framed (pong)
            if (p.src >= 1 && p.src <= MAXCONN){
                Conn *c = &g_conns[p.src-1];
                if (c->used && c->upgraded) send_all(c->fd, p.data, p.len);
            }
            continue;
        }
        int fl = ws_frame_build(frame, p.data, p.len);
        if (p.src < 0){                                // broadcast
            for (int i=0;i<MAXCONN;i++)
                if (g_conns[i].used && g_conns[i].upgraded)
                    send_all(g_conns[i].fd, frame, fl);
        } else if (p.src >= 1 && p.src <= MAXCONN){
            Conn *c = &g_conns[p.src-1];
            if (c->used && c->upgraded) send_all(c->fd, frame, fl);
        }
    }
}
static void server_loop(void){
    while (g_listen != SOCK_BAD){
        fd_set rf; FD_ZERO(&rf); FD_SET(g_listen, &rf);
        sock_t mx = g_listen;
        for (int i=0;i<MAXCONN;i++) if (g_conns[i].used){
            FD_SET(g_conns[i].fd, &rf);
            if (g_conns[i].fd > mx) mx = g_conns[i].fd;
        }
        struct timeval tv; tv.tv_sec = 0; tv.tv_usec = 5000;
        int r = select((int)(mx+1), &rf, 0, 0, &tv);
        if (r > 0){
            if (FD_ISSET(g_listen, &rf)){
                sock_t fd = accept(g_listen, 0, 0);
                if (fd != SOCK_BAD){
                    int slot = -1;
                    for (int i=0;i<MAXCONN;i++) if (!g_conns[i].used){ slot = i; break; }
                    if (slot < 0){ sock_close(fd); }
                    else {
                        Conn *c = &g_conns[slot];
                        memset(c, 0, sizeof(*c));
                        c->fd = fd; c->used = 1;
                        struct timeval to; to.tv_sec = 3; to.tv_usec = 0;
                        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&to, sizeof(to));
                    }
                }
            }
            for (int i=0;i<MAXCONN;i++)
                if (g_conns[i].used && FD_ISSET(g_conns[i].fd, &rf)) conn_read(i);
        }
        drain_outq();
    }
}
void web_server_start(void){
    static int started = 0;
    if (started) return;
    started = 1;
#ifdef PLAT_WIN32
    { WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa); }
#endif
    // build the page to serve: the web build + the hosted flag, so a
    // browser that loads it joins this machine's game automatically
    {
        const char *inject = "<script>window.TSG_HOSTED=1;</script>";
        const char *body = 0;
        for (unsigned i=0; i + 7 < TSG_WEB_HTML_LEN; i++)
            if (memcmp(TSG_WEB_HTML + i, "</body>", 7) == 0){ body = (const char*)TSG_WEB_HTML + i; break; }
        if (body && TSG_WEB_HTML_LEN + strlen(inject) <= sizeof(g_page)){
            unsigned pre = (unsigned)(body - (const char*)TSG_WEB_HTML);
            memcpy(g_page, TSG_WEB_HTML, pre);
            memcpy(g_page + pre, inject, strlen(inject));
            memcpy(g_page + pre + strlen(inject), body, TSG_WEB_HTML_LEN - pre);
            g_page_len = (int)(TSG_WEB_HTML_LEN + strlen(inject));
        } else {
            memcpy(g_page, TSG_WEB_HTML, TSG_WEB_HTML_LEN);
            g_page_len = (int)TSG_WEB_HTML_LEN;
        }
    }
    // find this machine's LAN address (UDP connect trick: no traffic)
    {
        sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
        int got = 0;
        if (s != SOCK_BAD){
            struct sockaddr_in dst; memset(&dst, 0, sizeof(dst));
            dst.sin_family = AF_INET; dst.sin_port = htons(53);
            dst.sin_addr.s_addr = inet_addr("8.8.8.8");
            if (connect(s, (struct sockaddr*)&dst, sizeof(dst)) == 0){
                struct sockaddr_in me; socklen_t ml = (socklen_t)sizeof(me);
                if (getsockname(s, (struct sockaddr*)&me, &ml) == 0 && me.sin_addr.s_addr){
                    snprintf(g_join_addr, sizeof(g_join_addr), "%s:%d", inet_ntoa(me.sin_addr), WEB_PORT);
                    got = 1;
                }
            }
            sock_close(s);
        }
        if (!got) snprintf(g_join_addr, sizeof(g_join_addr), "127.0.0.1:%d", WEB_PORT);
    }
    g_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (g_listen == SOCK_BAD){ g_join_addr[0]=0; return; }
    { int one = 1; setsockopt(g_listen, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(one)); }
    struct sockaddr_in a; memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET; a.sin_addr.s_addr = INADDR_ANY; a.sin_port = htons(WEB_PORT);
    if (bind(g_listen, (struct sockaddr*)&a, sizeof(a)) != 0 || listen(g_listen, 8) != 0){
        sock_close(g_listen); g_listen = SOCK_BAD; g_join_addr[0]=0; return;
    }
    start_server_thread();
}
const char *plat_join_addr(void){ return g_join_addr; }
// the stock page exactly as served (hosted flag already injected) —
// srcbuild.c splices custom builds into this template
const unsigned char *tsg_stock_page(int *len){
    if (len) *len = g_page_len;
    return g_page;
}

// --- plat_net_* on top of the queues ---
static int g_net_mode = 0;
int plat_net_start(int mode, const char *addr){
    (void)addr;
    if (mode == 1){ g_net_mode = 1; return 0; }        // hosting (server already runs)
    if (mode == 0){ g_net_mode = 0; return 0; }
    return -1;                                         // native builds don't join as clients
}
void plat_net_stop(void){ g_net_mode = 0; }
int plat_net_send(const u8 *d, int len, int dest){
    if (g_net_mode != 1) return -1;
    return q_push(g_outq, &g_outq_h, &g_outq_t, dest, (const unsigned char*)d, len, 0)? 0 : -1;
}
int plat_net_recv(u8 *buf, int cap, int *src){
    WPkt p;
    if (!q_pop(g_inq, &g_inq_h, &g_inq_t, &p)) return 0;
    int n = p.len < cap? p.len : cap;
    memcpy(buf, p.data, (size_t)n);
    *src = p.src;
    return n;
}
#endif // native embedded web server
