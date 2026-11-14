// Generates Whales Only test codes signed with the PRODUCTION key,
// using the issuer's own signer/encoder (worker.js) so the game's
// decoder + verifier are tested against the real production format.
// Reads the private JWK from the goal's hidden_files (never copies it).
import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import worker from "../whales-only/worker.js";

const { ed25519Sign, encodeUnlockCode } = worker;
const here = path.dirname(fileURLToPath(import.meta.url));
const keys = JSON.parse(fs.readFileSync(
  path.join(here, "../../goals/the-survival-game/hidden_files/whales-only-keys.json"), "utf8"));
const jwk = keys.private_jwk || keys.jwk || keys;
const seed = Buffer.from(jwk.d, "base64url");
if (seed.length !== 32) throw new Error("unexpected JWK shape in whales-only-keys.json");

const codes = [];
for (let sku = 0; sku < 15; sku++) {
  const nonce = crypto.createHash("sha256").update("pi_test_game_" + sku, "utf8").digest().subarray(0, 8);
  const payload = Buffer.concat([Buffer.from([1, sku]), nonce]);
  const { signature, publicKey } = await ed25519Sign(new Uint8Array(seed), new Uint8Array(payload));
  codes.push(encodeUnlockCode(new Uint8Array(payload), signature));
  if (sku === 0) console.log("pubkey from signer:", Buffer.from(publicKey).toString("hex"));
}
// tampered: flip one character in the middle of code 0
const t = codes[0].split("");
t[20] = t[20] === "A" ? "B" : "A";
// wrong version: version byte 2, validly signed
const nonce2 = crypto.createHash("sha256").update("pi_test_game_v2", "utf8").digest().subarray(0, 8);
const payload2 = Buffer.concat([Buffer.from([2, 0]), nonce2]);
const sig2 = await ed25519Sign(new Uint8Array(seed), new Uint8Array(payload2));
fs.writeFileSync(path.join(here, "whale-codes.json"), JSON.stringify({
  codes, tampered: t.join(""), wrongVersion: encodeUnlockCode(new Uint8Array(payload2), sig2.signature),
}, null, 2));
console.log("wrote whale-codes.json:", codes.length, "codes; code0 len", codes[0].length);
