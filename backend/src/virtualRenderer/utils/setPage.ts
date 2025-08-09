export default function setCurrentPage(req: any, pageFn: () => any) {
    if (typeof pageFn !== "function") {
        console.error("❌ Page function is invalid:", pageFn);
        return;
    }
    try {
        const page = pageFn();
        console.log("🔄 setCurrentPage: Session ID:", req.sessionID);
        console.log("🔄 setCurrentPage: Session before setting:", Object.keys(req.session || {}));
        console.log("🔄 setCurrentPage: Setting page with", page.components?.length, "components");
        
        req.session.currentPage = page;
        
        // Force session save to ensure persistence
        req.session.save((err: any) => {
            if (err) {
                console.error("❌ Session save error:", err);
            } else {
                console.log("✅ Session saved successfully");
            }
        });
        
        console.log("🔄 setCurrentPage: Session after setting:", Object.keys(req.session || {}));
        console.log("🔄 setCurrentPage: Session currentPage exists after setting:", !!req.session.currentPage);
        console.log(`✅ Current page set to: ${page.pageId || "[unknown ID]"}`);
    } catch (err) {
        console.error("❌ Error creating page object:", err);
    }
}