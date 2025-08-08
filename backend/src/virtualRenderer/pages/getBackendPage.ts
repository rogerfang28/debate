import type { Request } from "express";
import { loadHomePage, loadProfilePage, loadPublicDebatesPage, loadRoomPage, loadTestPage } from "./loaders/index.ts";
import getCurrentPage from "../utils/getCurrentPage.ts";

export default function getBackendPage(req: Request) {
  // 1) First check if there's a page stored in session (using session management utilities)
  const sessionPage = getCurrentPage(req);
  if (sessionPage) return sessionPage;

  // 2) Optional: choose by query (?page=room,profile,public,test)
  const which = String(req.query.page ?? "").toLowerCase();
  switch (which) {
    case "room": return loadRoomPage(req);
    case "profile": return loadProfilePage(req);
    case "public": return loadPublicDebatesPage(req);
    case "test": return loadTestPage(req);
    default: return loadHomePage(req);
  }
}uest } from "express";
import { loadHomePage, loadProfilePage, loadPublicDebatesPage, loadRoomPage, loadTestPage } from "./loaders/index.ts";

export default function getPage(req: Request) {
  // 1) If you’ve been storing a full page object in the session:
  if (req.session?.currentPage) return req.session.currentPage;

  // 2) Optional: choose by query (?page=room,profile,public,test)
  const which = String(req.query.page ?? "").toLowerCase();
  switch (which) {
    case "room": return loadRoomPage(req);
    case "profile": return loadProfilePage(req);
    case "public": return loadPublicDebatesPage(req);
    case "test": return loadTestPage(req);
    default: return loadHomePage(req);
  }
}
