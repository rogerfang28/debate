// src/auth.ts
import { betterAuth } from "better-auth";
import { mongodbAdapter } from "better-auth/adapters/mongodb";
import { client, connectDB } from "./db";

// Ensure Mongo is connected before creating the auth instance
await connectDB();

// Use default DB from your MONGODB_URI (already includes /graphdb)
const db = client.db();

export const auth = betterAuth({
  database: mongodbAdapter(db),

  // ✅ Enable email & password auth
  emailAndPassword: {
    enabled: true,
  },

  // ✅ Put cookie settings under `advanced`
  advanced: {
    // Simple option: prefix all Better-Auth cookies
    cookiePrefix: "dg",

    // Or fully customize names/attributes:
    // cookies: {
    //   session_token: {
    //     name: "dg_session",
    //     attributes: {
    //       // sameSite: "lax",
    //       // secure: process.env.NODE_ENV === "production",
    //       // domain: "yourdomain.com",
    //       // path: "/",
    //     },
    //   },
    //   // session_data: { name: "dg_session_data" },
    //   // dont_remember: { name: "dg_dont_remember" },
    // },
  },
});
