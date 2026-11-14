# Whales Only — license-code issuer

A tiny Cloudflare Worker for **The Survival Game**. Players buy purely cosmetic
Whales Only perks through Stripe Payment Links. After payment, Stripe redirects
the buyer to this Worker's `/done` page, which verifies the checkout session
with Stripe's API and shows a signed unlock code. The game verifies the code
**offline** with Ed25519 — no accounts, no server check, no secrets in the game.

Files:

- `worker.js` — the Worker (no dependencies). Ed25519 is implemented in pure
  JS with BigInt; signing follows RFC 8032.
- `wrangler.toml` — Worker name/config (`whales-only-issuer`).
- `test-worker.mjs` — Node test: signs payloads for all 7 items with the
  Worker's implementation, verifies them with Node's crypto, and writes
  `test-vectors.json` (throwaway test keypair).

## Deployment (owner steps)

### 1. Create a free Cloudflare account

Sign up at <https://dash.cloudflare.com/sign-up> — the free tier is plenty.
No custom domain is needed; the Worker gets a `*.workers.dev` address.

### 2. Deploy the Worker

**Dashboard route (no tools to install):**

1. In the Cloudflare dashboard, go to **Workers & Pages → Create → Worker**.
2. Name it `whales-only-issuer` and deploy the placeholder.
3. Choose **Edit code**, replace everything with the contents of `worker.js`,
   and deploy again.

**Wrangler route (command line):**

```sh
npm install -g wrangler
wrangler login
# from this directory (it reads wrangler.toml):
wrangler deploy
```

### 3. Set the two secrets

**Never commit these values anywhere, and never paste them into the game.**

`STRIPE_SECRET_KEY` — a Stripe **restricted** key with *only* Checkout
Sessions **read** permission:

1. Stripe Dashboard → **Developers → API keys → Create restricted key**.
2. Name it e.g. "Whales Only issuer".
3. Set every permission to **None** except **Checkout Sessions → Read**.
4. Create the key and copy its value (`rk_live_...`).

`ED25519_PRIVATE_JWK` — the signing key for unlock codes. **Muse supplies
this value separately.** It is a JWK JSON string generated for this purpose;
keep it wherever you keep passwords, and give it only to this Worker's secret
store.

Set both secrets:

- **Dashboard:** Worker → **Settings → Variables and Secrets → Add** →
  type **Secret**, name `STRIPE_SECRET_KEY`, paste the value; repeat for
  `ED25519_PRIVATE_JWK`.
- **Or wrangler:** `wrangler secret put STRIPE_SECRET_KEY` and
  `wrangler secret put ED25519_PRIVATE_JWK` (you'll be prompted to paste).

### 4. Check it, then tell Muse the URL

Visit `https://<your-worker-url>/health` — it should answer `ok`.

Then **give Muse the final workers.dev URL**. The Stripe Payment Links must
send buyers to:

```
https://<your-worker-url>/done?session_id={CHECKOUT_SESSION_ID}
```

(`{CHECKOUT_SESSION_ID}` is a literal placeholder Stripe substitutes itself.)
Muse will set that as the after-payment redirect on every Whales Only
payment link. Until that redirect is set, buyers pay but never see a code —
so this is the one step that closes the loop.

## How a purchase flows

1. In the game: Bug Mall → **Whales Only** counter → buy → the item's Stripe
   Payment Link opens in the browser.
2. Buyer pays on Stripe's hosted checkout.
3. Stripe redirects to `/done?session_id=cs_...` on this Worker.
4. The Worker asks Stripe (server-to-server, restricted key) whether that
   session is paid, and which item it was.
5. If paid, it builds a 10-byte payload —
   `[1, sku, first 8 bytes of SHA-256(payment intent id)]` — signs it with
   Ed25519, and displays the payload+signature as a grouped Crockford base32
   code.
6. The buyer pastes/types the code at the Whales Only counter; the game checks
   the signature against the public key baked into it and unlocks the perk in
   the save data. The code doubles as the buyer's receipt: if the save data is
   ever wiped, re-entering the code restores the perk.

## Local test

```sh
node test-worker.mjs
```

Regenerates `test-vectors.json` with a fresh throwaway test keypair and
verifies every signature against Node's own Ed25519 implementation.
