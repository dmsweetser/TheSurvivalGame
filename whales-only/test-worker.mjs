// Node test for the Whales Only issuer's crypto (worker.js).
//
// For each of the 7 skus:
//   - builds the unlock payload exactly like the Worker does
//     ([version=1, sku, first 8 bytes of SHA-256(payment_intent id)]),
//   - signs it with the pure-JS BigInt Ed25519 implementation in worker.js,
//   - verifies the signature with node:crypto (an independent implementation),
//   - checks the Crockford base32 code round-trips back to payload||signature.
// Also: the derived public key must match the generated keypair, and a
// tampered payload must fail verification.
//
// Writes test-vectors.json using the throwaway TEST keypair generated here
// (never the production key).

import crypto from "node:crypto";
import fs from "node:fs";
import { fileURLToPath } from "node:url";
import path from "node:path";
import worker from "./worker.js";

const { ed25519Sign, ed25519PublicKey, encodeUnlockCode, base32Decode } = worker;

const here = path.dirname(fileURLToPath(import.meta.url));

function assert(cond, msg) {
  if (!cond) throw new Error("ASSERT FAILED: " + msg);
}

function hex(bytes) {
  return Buffer.from(bytes).toString("hex");
}

// --- throwaway test keypair via node:crypto ---
const { publicKey, privateKey } = crypto.generateKeyPairSync("ed25519");
const privJwk = privateKey.export({ format: "jwk" });
const pubJwk = publicKey.export({ format: "jwk" });
assert(privJwk.kty === "OKP" && privJwk.crv === "Ed25519", "keypair is Ed25519");
const seed = Buffer.from(privJwk.d, "base64url");
const pubRaw = Buffer.from(pubJwk.x, "base64url");
assert(seed.length === 32, "seed is 32 bytes");
assert(pubRaw.length === 32, "public key is 32 bytes");

// Public key object built from raw bytes wrapped in a JWK.
const verifyKey = crypto.createPublicKey({
  key: { kty: "OKP", crv: "Ed25519", x: pubRaw.toString("base64url") },
  format: "jwk",
});

// --- the BigInt implementation must derive the same public key ---
const derivedPub = await ed25519PublicKey(new Uint8Array(seed));
assert(
  Buffer.from(derivedPub).equals(pubRaw),
  "derived public key matches generated keypair"
);
console.log("public key derivation: OK");

// --- sign / verify / round-trip for all 7 skus ---
const vectors = [];
for (let sku = 0; sku < 7; sku++) {
  const paymentIntent = "pi_test_vector_" + sku;
  const digest = crypto.createHash("sha256").update(paymentIntent, "utf8").digest();
  const nonce = digest.subarray(0, 8);
  const payload = Buffer.concat([Buffer.from([1, sku]), nonce]);
  assert(payload.length === 10, "payload is 10 bytes");

  const { signature, publicKey } = await ed25519Sign(
    new Uint8Array(seed),
    new Uint8Array(payload)
  );
  assert(signature.length === 64, "signature is 64 bytes");
  assert(
    Buffer.from(publicKey).equals(pubRaw),
    "sign() returns the matching public key for sku " + sku
  );

  const ok = crypto.verify(null, payload, verifyKey, Buffer.from(signature));
  assert(ok, "node:crypto verifies signature for sku " + sku);

  const code = encodeUnlockCode(new Uint8Array(payload), signature);
  const decoded = Buffer.from(base32Decode(code.replace(/-/g, "")));
  const expected = Buffer.concat([payload, Buffer.from(signature)]);
  assert(decoded.equals(expected), "base32 round-trip for sku " + sku);

  vectors.push({
    sku: sku,
    payload_hex: hex(payload),
    sig_hex: hex(signature),
    pubkey_hex: hex(pubRaw),
    code: code,
  });
  console.log("sku " + sku + ": sign + verify + round-trip OK");
}

// --- tampered payload must fail verification ---
{
  const v = vectors[3];
  const tampered = Buffer.from(v.payload_hex, "hex");
  tampered[9] ^= 0x01;
  const ok = crypto.verify(
    null,
    tampered,
    verifyKey,
    Buffer.from(v.sig_hex, "hex")
  );
  assert(!ok, "tampered payload fails verification");
  console.log("tamper check: OK");
}

fs.writeFileSync(
  path.join(here, "test-vectors.json"),
  JSON.stringify(vectors, null, 2) + "\n"
);
console.log("wrote test-vectors.json with " + vectors.length + " vectors");
console.log("test public key (hex): " + hex(pubRaw));
console.log("ALL TESTS PASSED");
