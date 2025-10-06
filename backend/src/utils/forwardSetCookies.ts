// Helper to forward BetterAuth's Set-Cookie headers to Express
import type { Response as ExpressResponse } from "express";

// Accept a WHATWG/undici Response (from betterAuth.api with asResponse: true)
// and copy its Set-Cookie headers to your Express response.
export function forwardSetCookies(fetchRes: any, res: ExpressResponse) {
  if (!fetchRes?.headers) return;

  // Node/undici supports headers.getSetCookie() for multiple cookies.
  const getSetCookie = fetchRes.headers.getSetCookie?.bind(fetchRes.headers);
  let cookies: string[] | undefined = getSetCookie?.();

  // Fallbacks for various runtimes:
  if (!cookies || cookies.length === 0) {
    const raw = fetchRes.headers.raw?.();
    if (raw && raw["set-cookie"]) {
      cookies = raw["set-cookie"];
    } else {
      const single = fetchRes.headers.get?.("set-cookie");
      if (single) cookies = [single];
    }
  }

  if (cookies && cookies.length) {
    // Multiple Set-Cookie headers are allowed; Express will send them all.
    res.setHeader("Set-Cookie", cookies);
  }
}
