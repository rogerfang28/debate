export default function setCurrentPage(req: any, pageFn: () => any) {
    if (typeof pageFn !== "function") {
        console.error("❌ Page function is invalid:", pageFn);
        return;
    }
    try {
        const page = pageFn();
        req.session.currentPage = page;
        
        // Force session save to ensure persistence
        req.session.save((err: any) => {
            if (err) {
                console.error("❌ Session save error:", err);
            }
        });
        
    } catch (err) {
        console.error("❌ Error creating page object:", err);
    }
}