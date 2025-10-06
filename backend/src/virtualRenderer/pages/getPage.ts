import type { Request } from "express";
import { loadHomePage, loadProfilePage, loadPublicDebatesPage, loadRoomPage, loadTestPage } from "./loaders/index.ts";

export default async function getPage(req: Request) {
  // 1) If you've been storing a full page object in the session:
  if (req.session?.currentPage) {
    const page = (req.session as any).currentPage;
    return page;
  }

  // 2) Optional: choose by query (?page=room,profile,public,test)
  const which = String(req.query.page ?? "").toLowerCase();
  
  switch (which) {
    case "room": return loadRoomPage();
    case "profile": return loadProfilePage();
    case "public": return loadPublicDebatesPage();
    case "test": return loadTestPage();
    default: 
      const homePage = await loadHomePage();
      return homePage;
  }
}
