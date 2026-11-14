// Mint a REAL unlock code for testing (signs with the production key
// from the hidden key file). Usage: node mint-test-code.mjs <sku>
// The code is printed to stdout and never stored.
import crypto from "node:crypto";
import fs from "node:fs";
import worker from "./worker.js";
const { ed25519Sign, encodeUnlockCode } = worker;
const keys = JSON.parse(fs.readFileSync(process.env.HOME + "/workspace/goals/the-survival-game/hidden_files/whales-only-keys.json", "utf8"));
const jwk = typeof keys.private_jwk === "string" ? JSON.parse(keys.private_jwk) : keys.private_jwk;
const seed = Buffer.from(jwk.d, "base64url");
const sku = parseInt(process.argv[2], 10);
const digest = crypto.createHash("sha256").update("pi_whales2_selftest_" + sku, "utf8").digest();
const payload = Buffer.concat([Buffer.from([1, sku]), digest.subarray(0, 8)]);
const { signature } = await ed25519Sign(new Uint8Array(seed), new Uint8Array(payload));
console.log(encodeUnlockCode(new Uint8Array(payload), new Uint8Array(signature)));
