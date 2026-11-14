// THE SURVIVAL GAME — LAN host server (zero dependencies, plain Node).
// Serves the game page and relays packets between the host browser and
// client browsers on your local network.
//
//   node server.js           (then open http://localhost:8379 and HOST a game;
//                             friends open http://<your-lan-ip>:8379 and JOIN)
//
// Relay framing: client -> relay: raw packet.
//                relay  -> host:  [clientId, ...packet]
//                host   -> relay: [0xFF, ...packet] = broadcast to clients
//                                 [clientId, ...packet] = one client
//                relay  -> client: raw packet.
// When a client disconnects, the host gets a synthetic [clientId, 'L'].
"use strict";
const http = require("http");
const fs = require("fs");
const path = require("path");
const crypto = require("crypto");

const PORT = 8379;
const page = fs.readFileSync(path.join(__dirname, "index.html"));

const server = http.createServer((req, res) => {
  if (req.url === "/" || req.url.startsWith("/index")) {
    res.writeHead(200, { "Content-Type": "text/html; charset=utf-8" });
    res.end(page);
  } else { res.writeHead(404); res.end("not found"); }
});

// ---- minimal WebSocket implementation (RFC 6455) ----
function acceptKey(key) {
  return crypto.createHash("sha1")
    .update(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").digest("base64");
}
function encodeFrame(data, opcode = 2) {
  const len = data.length;
  let head;
  if (len < 126) { head = Buffer.alloc(2); head[1] = len; }
  else if (len < 65536) { head = Buffer.alloc(4); head[1] = 126; head.writeUInt16BE(len, 2); }
  else { head = Buffer.alloc(10); head[1] = 127; head.writeBigUInt64BE(BigInt(len), 2); }
  head[0] = 0x80 | opcode;
  return Buffer.concat([head, data]);
}
function attachWS(socket, role) {
  const conn = { socket, role, id: 0, buf: Buffer.alloc(0), dead: false };
  socket.on("data", (chunk) => {
    conn.buf = Buffer.concat([conn.buf, chunk]);
    while (true) {
      if (conn.buf.length < 2) return;
      const b0 = conn.buf[0], b1 = conn.buf[1];
      const opcode = b0 & 0x0f, masked = (b1 & 0x80) !== 0;
      let len = b1 & 0x7f, off = 2;
      if (len === 126) { if (conn.buf.length < 4) return; len = conn.buf.readUInt16BE(2); off = 4; }
      else if (len === 127) { if (conn.buf.length < 10) return; len = Number(conn.buf.readBigUInt64BE(2)); off = 10; }
      const maskLen = masked ? 4 : 0;
      if (conn.buf.length < off + maskLen + len) return;
      let payload = conn.buf.subarray(off + maskLen, off + maskLen + len);
      if (masked) {
        const mask = conn.buf.subarray(off, off + 4);
        const out = Buffer.alloc(len);
        for (let i = 0; i < len; i++) out[i] = payload[i] ^ mask[i % 4];
        payload = out;
      }
      conn.buf = conn.buf.subarray(off + maskLen + len);
      if (opcode === 8) { try { socket.end(); } catch (e) {} onClose(conn); return; }
      if (opcode === 9) { try { socket.write(encodeFrame(payload, 10)); } catch (e) {} continue; }
      if (opcode === 1 || opcode === 2) onPacket(conn, payload);
    }
  });
  socket.on("close", () => onClose(conn));
  socket.on("error", () => onClose(conn));
  return conn;
}

let host = null;
const clients = new Map(); // id -> conn
let nextId = 1;

function onPacket(conn, payload) {
  if (conn.role === "client") {
    if (host && !host.dead) {
      const framed = Buffer.concat([Buffer.from([conn.id]), payload]);
      try { host.socket.write(encodeFrame(framed)); } catch (e) {}
    }
  } else if (conn.role === "host") {
    if (payload.length < 1) return;
    const dest = payload[0], rest = payload.subarray(1);
    if (dest === 0xFF) {
      for (const c of clients.values()) { try { c.socket.write(encodeFrame(rest)); } catch (e) {} }
    } else {
      const c = clients.get(dest);
      if (c) { try { c.socket.write(encodeFrame(rest)); } catch (e) {} }
    }
  }
}
function onClose(conn) {
  if (conn.dead) return; conn.dead = true;
  if (conn.role === "host" && host === conn) host = null;
  if (conn.role === "client") {
    clients.delete(conn.id);
    if (host && !host.dead) {
      const bye = Buffer.from([conn.id, 0x4C]); // 'L'
      try { host.socket.write(encodeFrame(bye)); } catch (e) {}
    }
  }
}

server.on("upgrade", (req, socket) => {
  const key = req.headers["sec-websocket-key"];
  if (!key) { socket.destroy(); return; }
  const role = req.url.includes("role=host") ? "host" : "client";
  socket.write(
    "HTTP/1.1 101 Switching Protocols\r\n" +
    "Upgrade: websocket\r\nConnection: Upgrade\r\n" +
    "Sec-WebSocket-Accept: " + acceptKey(key) + "\r\n\r\n");
  const conn = attachWS(socket, role);
  if (role === "host") { host = conn; console.log("host connected"); }
  else { conn.id = nextId++; if (conn.id > 250) nextId = 1; clients.set(conn.id, conn); console.log("client", conn.id, "connected"); }
});

server.listen(PORT, "0.0.0.0", () => {
  console.log("THE SURVIVAL GAME server on http://localhost:" + PORT);
  console.log("LAN players: open http://<this-machine-ip>:" + PORT);
});
