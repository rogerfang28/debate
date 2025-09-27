import type { Request, Response } from "express";
import { auth } from "../../lib/auth";                 // your betterAuth instance
import { forwardSetCookies } from "../../utils/forwardSetCookies.ts";

export interface SignupParams {
  email: string;
  password: string;
  name?: string;
}

export interface SignupResult {
  user?: any;
  session?: any;
}

/**
 * Server-side signup:
 * - Calls BetterAuth to create the user
 * - Forwards httpOnly Set-Cookie headers to the browser (logged in after signup)
 * - Returns user/session for your SDUI
 */
export async function signup(_req: Request, res: Response, params: SignupParams): Promise<SignupResult> {
  const { email, password, name } = params;
  if (!email || !password) {
    res.status(400);
    throw new Error("Missing email or password");
  }

  const rsp = await auth.api.signUpEmail({
    body: { email, password, name },
    asResponse: true,
  });

  forwardSetCookies(rsp as any, res);

  const data = await (rsp as any).json().catch(() => ({}));
  return {
    user: (data && data.user) ?? undefined,
    session: (data && data.session) ?? undefined,
  };
}
