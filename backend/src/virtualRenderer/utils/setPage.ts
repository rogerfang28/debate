export default function setCurrentPage(req: any, pageOrFn: any) {
    let page;
    
    if (typeof pageOrFn === "function") {
        // It's a function - call it to get the page
        try {
            page = pageOrFn();
        } catch (err) {
            console.error("❌ Error creating page object:", err);
            return;
        }
    } else if (pageOrFn && typeof pageOrFn === "object") {
        // It's already a page object - use it directly
        page = pageOrFn;
    } else {
        console.error("❌ Page function or object is invalid:", pageOrFn);
        return;
    }
    
    try {
        req.session.currentPage = page;
        
        // Force session save to ensure persistence
        req.session.save((err: any) => {
            if (err) {
                console.error("❌ Session save error:", err);
            }
        });
        
    } catch (err) {
        console.error("❌ Error setting page in session:", err);
    }
}