// Whales Only license-code issuer for The Survival Game.
//
// Dependency-free Cloudflare Worker (service-worker syntax).
//
//   GET /health                    -> plain text "ok"
//   GET /done?session_id=cs_...   -> verifies the Stripe checkout session
//                                     server-side and, if paid, displays a
//                                     signed Ed25519 unlock code that the
//                                     game verifies offline.
//
// Secrets (set as Worker secrets, never committed):
//   STRIPE_SECRET_KEY     - Stripe RESTRICTED key, Checkout Sessions read only.
//   ED25519_PRIVATE_JWK   - JWK JSON string; only its "d" (seed) member is used.
//
// Ed25519 is implemented in pure JS with BigInt (Workers has no Node crypto).
// Signing follows RFC 8032 exactly.

"use strict";

/* ------------------------------------------------------------------ */
/* Catalog: Stripe price id -> [sku index, display name]               */
/* ------------------------------------------------------------------ */

const PRICE_TO_SKU = {
  "price_1UXRHSIyTocie9gtRIJ0iC6O": [0, "Golden Carcass"],
  "price_1UXRHeIyTocie9gtYCob2o0g": [1, "Comically Enormous-er Glasses"],
  "price_1UXRHgIyTocie9gt6hIVyGoV": [2, "Royal Cape"],
  "price_1UXRHhIyTocie9gtvofeDxEL": [3, "Sparkle Trail"],
  "price_1UXRHjIyTocie9gtakqTf81X": [4, "The Forbidden Monocle"],
  "price_1UXRHlIyTocie9gtnwA1TtkC": [5, "Dramatic Rain"],
  "price_1UXRHnIyTocie9gtnwIy3fqO": [6, "WHALE Title"],
  "price_1UXkhSIyTocie9gtYTuFYbLy": [7, "Royal Buddy"],
  "price_1UXkhhIyTocie9gtKAewIWEe": [8, "Parade Duck"],
  "price_1UXkhkIyTocie9gtyqmjfXRy": [9, "Disco Mall"],
  "price_1UXkhnIyTocie9gtM2Peo5kE": [10, "Confetti Squash"],
  "price_1UXkhrIyTocie9gtlwfOwFn5": [11, "Aura of Importance"],
  "price_1UXkhuIyTocie9gtOxuPX8OF": [12, "Space Buddy"],
  "price_1UXkhxIyTocie9gtMF3GDpCL": [13, "Name in Lights"],
  "price_1UXki1IyTocie9gtKHIStoDq": [14, "Kazoo Mode"],
};

const CODE_VERSION = 1;
const GAME_URL = "https://dmsweetser.itch.io/the-survival-game";

/* ------------------------------------------------------------------ */
/* Byte helpers                                                        */
/* ------------------------------------------------------------------ */

function concatBytes() {
  let total = 0;
  for (const a of arguments) total += a.length;
  const out = new Uint8Array(total);
  let off = 0;
  for (const a of arguments) {
    out.set(a, off);
    off += a.length;
  }
  return out;
}

function bytesToIntLE(bytes) {
  let n = 0n;
  for (let i = bytes.length - 1; i >= 0; i--) {
    n = (n << 8n) | BigInt(bytes[i]);
  }
  return n;
}

function intToBytesLE(n, length) {
  const out = new Uint8Array(length);
  let v = n;
  for (let i = 0; i < length; i++) {
    out[i] = Number(v & 0xffn);
    v >>= 8n;
  }
  return out;
}

function base64UrlDecode(str) {
  let s = String(str).replace(/-/g, "+").replace(/_/g, "/");
  while (s.length % 4 !== 0) s += "=";
  const bin = atob(s);
  const out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
  return out;
}

async function sha256(bytes) {
  const digest = await crypto.subtle.digest("SHA-256", bytes);
  return new Uint8Array(digest);
}

async function sha512(bytes) {
  const digest = await crypto.subtle.digest("SHA-512", bytes);
  return new Uint8Array(digest);
}

/* ------------------------------------------------------------------ */
/* Crockford base32 (no padding)                                       */
/* ------------------------------------------------------------------ */

const B32_ALPHABET = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
const B32_REVERSE = (function () {
  const map = Object.create(null);
  for (let i = 0; i < B32_ALPHABET.length; i++) map[B32_ALPHABET[i]] = i;
  return map;
})();

function base32Encode(bytes) {
  let out = "";
  let buffer = 0;
  let bits = 0;
  for (const byte of bytes) {
    buffer = (buffer << 8) | byte;
    bits += 8;
    while (bits >= 5) {
      bits -= 5;
      out += B32_ALPHABET[(buffer >> bits) & 31];
    }
    buffer &= (1 << bits) - 1;
  }
  if (bits > 0) {
    out += B32_ALPHABET[(buffer << (5 - bits)) & 31];
  }
  return out;
}

function base32Decode(str) {
  const bytes = [];
  let buffer = 0;
  let bits = 0;
  for (const rawCh of String(str)) {
    const ch = rawCh.toUpperCase();
    if (ch === "-") continue;
    const value = B32_REVERSE[ch];
    if (value === undefined) throw new Error("invalid base32 character");
    buffer = (buffer << 5) | value;
    bits += 5;
    if (bits >= 8) {
      bits -= 8;
      bytes.push((buffer >> bits) & 0xff);
    }
    buffer &= (1 << bits) - 1;
  }
  return new Uint8Array(bytes);
}

function groupCode(flat) {
  const groups = [];
  for (let i = 0; i < flat.length; i += 5) groups.push(flat.slice(i, i + 5));
  return groups.join("-");
}

function encodeUnlockCode(payload, signature) {
  return groupCode(base32Encode(concatBytes(payload, signature)));
}

/* ------------------------------------------------------------------ */
/* Ed25519 (RFC 8032) in pure JS / BigInt                              */
/*                                                                     */
/* Curve: -x^2 + y^2 = 1 + d x^2 y^2  over GF(2^255 - 19).            */
/* Points are held in extended coordinates (X, Y, Z, T).               */
/* ------------------------------------------------------------------ */

const FIELD_P = (1n << 255n) - 19n;
const GROUP_L = (1n << 252n) + 27742317777372353535851937790883648493n;

function fmod(a) {
  const r = a % FIELD_P;
  return r < 0n ? r + FIELD_P : r;
}

function fpow(base, exp) {
  let result = 1n;
  let b = fmod(base);
  let e = exp;
  while (e > 0n) {
    if (e & 1n) result = (result * b) % FIELD_P;
    b = (b * b) % FIELD_P;
    e >>= 1n;
  }
  return result;
}

function finv(a) {
  return fpow(a, FIELD_P - 2n);
}

const CURVE_D = fmod(-121665n * finv(121666n));
const SQRT_M1 = fpow(2n, (FIELD_P - 1n) / 4n);

function xRecover(y) {
  const yy = fmod(y * y);
  const xx = fmod((yy - 1n) * finv(fmod(CURVE_D * yy + 1n)));
  let x = fpow(xx, (FIELD_P + 3n) / 8n);
  if (fmod(x * x - xx) !== 0n) x = fmod(x * SQRT_M1);
  if (x & 1n) x = FIELD_P - x;
  return x;
}

const BASE_Y = fmod(4n * finv(5n));
const BASE_X = xRecover(BASE_Y);
const BASE_POINT = [BASE_X, BASE_Y, 1n, fmod(BASE_X * BASE_Y)];
const NEUTRAL_POINT = [0n, 1n, 1n, 0n];

function pointAdd(p1, p2) {
  const a = fmod((p1[1] - p1[0]) * (p2[1] - p2[0]));
  const b = fmod((p1[1] + p1[0]) * (p2[1] + p2[0]));
  const c = fmod(2n * CURVE_D * p1[3] * p2[3]);
  const d = fmod(2n * p1[2] * p2[2]);
  const e = fmod(b - a);
  const f = fmod(d - c);
  const g = fmod(d + c);
  const h = fmod(b + a);
  return [fmod(e * f), fmod(g * h), fmod(f * g), fmod(e * h)];
}

function scalarMult(point, scalar) {
  let result = NEUTRAL_POINT;
  let addend = point;
  let s = scalar;
  while (s > 0n) {
    if (s & 1n) result = pointAdd(result, addend);
    addend = pointAdd(addend, addend);
    s >>= 1n;
  }
  return result;
}

function encodePoint(point) {
  const zInv = finv(point[2]);
  const x = fmod(point[0] * zInv);
  const y = fmod(point[1] * zInv);
  const bits = y | ((x & 1n) << 255n);
  return intToBytesLE(bits, 32);
}

function clampHash(h) {
  const out = new Uint8Array(h.slice(0, 32));
  out[0] &= 248;
  out[31] &= 63;
  out[31] |= 64;
  return out;
}

// Returns { publicKey: Uint8Array(32) } derived from a 32-byte seed.
async function ed25519PublicKey(seed) {
  const h = await sha512(seed);
  const a = bytesToIntLE(clampHash(h));
  return encodePoint(scalarMult(BASE_POINT, a));
}

// RFC 8032 signing. Returns { signature: Uint8Array(64), publicKey: Uint8Array(32) }.
async function ed25519Sign(seed, message) {
  if (!seed || seed.length !== 32) throw new Error("seed must be 32 bytes");
  const h = await sha512(seed);
  const clamped = clampHash(h);
  const a = bytesToIntLE(clamped);
  const prefix = h.slice(32, 64);
  const publicKey = encodePoint(scalarMult(BASE_POINT, a));

  const r = bytesToIntLE(await sha512(concatBytes(prefix, message))) % GROUP_L;
  const rEncoded = encodePoint(scalarMult(BASE_POINT, r));
  const k =
    bytesToIntLE(await sha512(concatBytes(rEncoded, publicKey, message))) %
    GROUP_L;
  const s = (r + k * a) % GROUP_L;
  const signature = concatBytes(rEncoded, intToBytesLE(s, 32));
  return { signature: signature, publicKey: publicKey };
}

/* ------------------------------------------------------------------ */
/* HTML pages                                                          */
/* ------------------------------------------------------------------ */

function escapeHtml(s) {
  return String(s)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function htmlResponse(title, bodyHtml, status) {
  const html =
    "<!DOCTYPE html>\n" +
    '<html lang="en"><head><meta charset="utf-8">' +
    '<meta name="viewport" content="width=device-width, initial-scale=1">' +
    "<title>" +
    escapeHtml(title) +
    " — Whales Only</title>" +
    "<style>" +
    "body{background:#14161c;color:#e8e6df;font-family:Georgia,'Times New Roman',serif;" +
    "margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:24px;box-sizing:border-box}" +
    ".card{max-width:640px;width:100%;background:#1d2029;border:1px solid #3a3f4d;border-radius:12px;padding:32px;text-align:center}" +
    "h1{font-size:26px;margin:0 0 12px}p{line-height:1.55;color:#c9c6bb}" +
    ".code{font-family:'Courier New',monospace;font-size:19px;word-break:break-all;background:#0c0e12;" +
    "border:1px solid #4a5064;border-radius:8px;padding:18px;margin:20px 0;color:#ffd968;letter-spacing:1px}" +
    "button,.btn{display:inline-block;background:#ffd968;color:#14161c;border:none;border-radius:8px;" +
    "padding:12px 22px;font-size:16px;font-family:inherit;font-weight:bold;cursor:pointer;text-decoration:none;margin:6px}" +
    ".btn.secondary{background:#2a2e3a;color:#e8e6df;border:1px solid #4a5064}" +
    ".small{font-size:14px;color:#9a978c}" +
    "</style></head><body><main class=\"card\">" +
    bodyHtml +
    "</main></body></html>";
  return new Response(html, {
    status: status || 200,
    headers: { "content-type": "text/html; charset=utf-8" },
  });
}

function errorPage(message) {
  return htmlResponse(
    "Something went wrong",
    "<h1>Something went wrong</h1>" +
      "<p>" +
      escapeHtml(message) +
      "</p>" +
      '<p class="small">If you completed a payment and still see this, keep your Stripe receipt email — it proves your purchase and we can sort your code out from it.</p>' +
      '<a class="btn" href="' +
      GAME_URL +
      '">Back to The Survival Game</a>'
  );
}

function successPage(itemName, code) {
  const body =
    "<h1>Thank you, whale! 🐋</h1>" +
    "<p>Your <strong>" +
    escapeHtml(itemName) +
    "</strong> unlock code:</p>" +
    '<div class="code" id="unlock-code">' +
    escapeHtml(code) +
    "</div>" +
    '<button id="copy-btn" type="button">Copy code</button>' +
    "<p>Open The Survival Game → enter the Bug Mall → Whales Only counter → " +
    "ENTER UNLOCK CODE → paste or type this code. Keep this code: it is also " +
    "your receipt — if you ever wipe the game's save data, re-entering the " +
    "code restores the perk.</p>" +
    '<a class="btn secondary" href="' +
    GAME_URL +
    '">Play The Survival Game</a>' +
    "<script>" +
    "document.getElementById('copy-btn').addEventListener('click',function(){" +
    "var code=document.getElementById('unlock-code').textContent.trim();" +
    "var btn=document.getElementById('copy-btn');" +
    "function done(){btn.textContent='Copied!';}" +
    "if(navigator.clipboard&&navigator.clipboard.writeText){" +
    "navigator.clipboard.writeText(code).then(done,function(){fallback();});" +
    "}else{fallback();}" +
    "function fallback(){var r=document.createRange();" +
    "r.selectNodeContents(document.getElementById('unlock-code'));" +
    "var sel=window.getSelection();sel.removeAllRanges();sel.addRange(r);" +
    "try{document.execCommand('copy');done();}catch(e){btn.textContent='Select the code and copy it';}}" +
    "});" +
    "</script>";
  return htmlResponse("Your unlock code", body);
}

function notPaidPage() {
  return htmlResponse(
    "Payment not completed",
    "<h1>Payment not completed yet</h1>" +
      "<p>Stripe says this checkout isn't paid yet. If you just finished paying, " +
      "wait a few seconds and refresh this page. If the payment didn't go through, " +
      "head back to the game and try the Whales Only counter again — you won't be " +
      "charged twice for a checkout that never completed.</p>" +
      '<a class="btn" href="' +
      GAME_URL +
      '">Back to The Survival Game</a>'
  );
}

/* ------------------------------------------------------------------ */
/* Request handling                                                    */
/* ------------------------------------------------------------------ */

function resolveEnv(env) {
  if (env && (env.STRIPE_SECRET_KEY || env.ED25519_PRIVATE_JWK)) return env;
  // Service-worker format: secrets are exposed as global bindings.
  return {
    STRIPE_SECRET_KEY:
      typeof STRIPE_SECRET_KEY !== "undefined" ? STRIPE_SECRET_KEY : undefined,
    ED25519_PRIVATE_JWK:
      typeof ED25519_PRIVATE_JWK !== "undefined"
        ? ED25519_PRIVATE_JWK
        : undefined,
  };
}

async function handleDone(url, env) {
  const sessionId = url.searchParams.get("session_id");
  if (!sessionId) {
    return errorPage(
      "This page needs a checkout session from Stripe. Please start from the Whales Only counter in the game."
    );
  }
  const secrets = resolveEnv(env);
  if (!secrets.STRIPE_SECRET_KEY || !secrets.ED25519_PRIVATE_JWK) {
    return errorPage(
      "The issuer isn't fully configured yet. Please try again later."
    );
  }

  let session;
  try {
    const apiUrl =
      "https://api.stripe.com/v1/checkout/sessions/" +
      encodeURIComponent(sessionId) +
      "?" +
      encodeURIComponent("expand[]") +
      "=" +
      encodeURIComponent("line_items");
    const resp = await fetch(apiUrl, {
      method: "GET",
      headers: { Authorization: "Bearer " + secrets.STRIPE_SECRET_KEY },
    });
    if (!resp.ok) {
      return errorPage(
        "Stripe couldn't confirm this checkout. If you were charged, keep your Stripe receipt email and we'll make it right."
      );
    }
    session = await resp.json();
  } catch (err) {
    return errorPage(
      "We couldn't reach Stripe to confirm this checkout. Please refresh in a moment; if it keeps failing, keep your Stripe receipt email."
    );
  }

  if (!session || session.payment_status !== "paid") {
    return notPaidPage();
  }

  const lineItems = session.line_items && session.line_items.data;
  const firstItem = lineItems && lineItems[0];
  const priceId = firstItem && firstItem.price && firstItem.price.id;
  const entry = priceId ? PRICE_TO_SKU[priceId] : undefined;
  if (!entry) {
    return errorPage(
      "This purchase doesn't match any Whales Only item. Keep your Stripe receipt email and we'll sort it out."
    );
  }
  const sku = entry[0];
  const itemName = entry[1];

  const paymentIntent = session.payment_intent;
  if (typeof paymentIntent !== "string" || paymentIntent.length === 0) {
    return errorPage(
      "Stripe confirmed the payment but didn't return a payment reference. Keep your Stripe receipt email and we'll sort it out."
    );
  }

  let seed;
  try {
    const jwk = JSON.parse(secrets.ED25519_PRIVATE_JWK);
    seed = base64UrlDecode(jwk.d);
    if (seed.length !== 32) throw new Error("bad seed length");
  } catch (err) {
    return errorPage(
      "The issuer isn't fully configured yet. Please try again later."
    );
  }

  try {
    const nonceFull = await sha256(new TextEncoder().encode(paymentIntent));
    const nonce = nonceFull.slice(0, 8);
    const payload = concatBytes(new Uint8Array([CODE_VERSION, sku]), nonce);
    const signed = await ed25519Sign(seed, payload);
    const code = encodeUnlockCode(payload, signed.signature);
    return successPage(itemName, code);
  } catch (err) {
    return errorPage(
      "Something went wrong while issuing your code. Keep your Stripe receipt email — your payment is safe and we'll make it right."
    );
  }
}

async function handleRequest(request, env) {
  const url = new URL(request.url);
  if (url.pathname === "/health") {
    return new Response("ok", {
      status: 200,
      headers: { "content-type": "text/plain; charset=utf-8" },
    });
  }
  if (url.pathname === "/done") {
    return handleDone(url, env);
  }
  return new Response("not found", {
    status: 404,
    headers: { "content-type": "text/plain; charset=utf-8" },
  });
}

/* ------------------------------------------------------------------ */
/* Entry points                                                        */
/* ------------------------------------------------------------------ */

if (typeof addEventListener !== "undefined") {
  addEventListener("fetch", function (event) {
    event.respondWith(handleRequest(event.request, undefined));
  });
}

if (typeof module !== "undefined" && module.exports) {
  module.exports = {
    PRICE_TO_SKU: PRICE_TO_SKU,
    CODE_VERSION: CODE_VERSION,
    concatBytes: concatBytes,
    bytesToIntLE: bytesToIntLE,
    intToBytesLE: intToBytesLE,
    base64UrlDecode: base64UrlDecode,
    sha256: sha256,
    sha512: sha512,
    base32Encode: base32Encode,
    base32Decode: base32Decode,
    groupCode: groupCode,
    encodeUnlockCode: encodeUnlockCode,
    ed25519PublicKey: ed25519PublicKey,
    ed25519Sign: ed25519Sign,
    handleRequest: handleRequest,
    handleDone: handleDone,
  };
}
