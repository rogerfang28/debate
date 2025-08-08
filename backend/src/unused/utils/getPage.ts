import setPage from "./setPage.ts";
import homePage from "../../../virtualRenderer/pages/homePage.js";

export default function getCurrentPage(req: any) {
    try {
        let page = req.session.currentPage;
        
        // If no page exists in session, initialize with homePage
        if (!page) {
            console.log("⚠️ No current page found in session, initializing with homePage");
            setPage(req, homePage);
            page = req.session.currentPage;
        }
        
        if (!page) {
            console.error("❌ Failed to initialize page in session");
            return null;
        }
        
        console.log(`Retrieved current page: ${page.pageId || "[unknown ID]"}`);
        return page;
    } catch (err) {
        console.error("❌ Error getting page:", err);
        return null;
    }
}