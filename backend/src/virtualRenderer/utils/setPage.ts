export default function setCurrentPage(req: any, pageFn: () => any) {
    if (typeof pageFn !== "function") {
        console.error("❌ Page function is invalid:", pageFn);
        return;
    }
    try {
        const page = pageFn();
        req.session.currentPage = page;
        // console.log(`✅ Current page set to: ${JSON.stringify(page)}`);
    } catch (err) {
        console.error("❌ Error creating page object:", err);
    }
}