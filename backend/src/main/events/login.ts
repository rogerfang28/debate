import type { Request, Response } from "express";
import { auth } from "../../lib/auth";                 // your betterAuth instance
import { forwardSetCookies } from "../../utils/forwardSetCookies.ts";

export interface LoginParams {
  email: string;
  password: string;
}

export interface LoginResult {
  user?: any;       // shape depends on your BetterAuth config
  session?: any;    // optional; present if you need it
}

/**
 * Server-side login:
 * - Calls BetterAuth to authenticate
 * - Forwards httpOnly Set-Cookie headers to the browser
 * - Returns user/session payload (if you want to use it to build SDUI)
 */
export async function login(_req: Request, res: Response, params: LoginParams): Promise<LoginResult> {
  const { email, password } = params;
  if (!email || !password) {
    res.status(400);
    throw new Error("Missing email or password");
  }

  // Ask BetterAuth for a raw Response so we can read Set-Cookie
  const rsp = await auth.api.signInEmail({
    body: { email, password },
    asResponse: true,
  });

  // Copy Set-Cookie headers to Express response
  forwardSetCookies(rsp as any, res);

  // Read the JSON payload from BetterAuth (often { user, session })
  const data = await (rsp as any).json().catch(() => ({}));
  return {
    user: (data && data.user) ?? undefined,
    session: (data && data.session) ?? undefined,
  };
}
