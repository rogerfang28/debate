import type { Request } from "express";
import { loadHomePage, loadProfilePage, loadPublicDebatesPage, loadRoomPage, loadTestPage } from "./loaders/index.ts";

export default async function getPage(req: Request) {
  console.log("🔍 getPage called, checking session and query...");
  
  // 1) If you've been storing a full page object in the session:
  if (req.session?.currentPage) {
    console.log("✅ Found page in session:", req.session.currentPage.pageId);
    return req.session.currentPage;
  }

  // 2) Optional: choose by query (?page=room,profile,public,test)
  const which = String(req.query.page ?? "").toLowerCase();
  console.log("🎯 Page query parameter:", which || "none (defaulting to home)");
  
  switch (which) {
    case "room": return loadRoomPage();
    case "profile": return loadProfilePage();
    case "public": return loadPublicDebatesPage();
    case "test": return loadTestPage();
    default: 
      console.log("🏠 Loading home page...");
      const homePage = await loadHomePage(req);
      console.log("✅ Home page loaded:", homePage?.pageId);
      return homePage;
  }
}
