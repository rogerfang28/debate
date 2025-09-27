// src/routes/auth.routes.ts
import { Router } from "express";
import { toNodeHandler, fromNodeHeaders } from "better-auth/node";
import { auth } from "../lib/auth";  // your Better Auth config

const router = Router();

// Mount all Better Auth routes at /api/auth/*
router.all("/*", toNodeHandler(auth));

// Optional: Friendly session endpoint
router.get("/me", async (req, res) => {
  const session = await auth.api.getSession({ headers: fromNodeHeaders(req.headers) });
  res.json(session ?? null);
});

export default router;
