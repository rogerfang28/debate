import getPage from "./getPage.ts";
import setPage from "./setPage.ts";
import { create } from "@bufbuild/protobuf";
import { PageSchema } from "../../../../src/gen/page_pb.js";

export default function addComponent(req: any, component: any) {
    try {
        // Get the current page
        let currentPage = getPage(req);
        
        if (!currentPage) {
            console.error("❌ No current page found");
            return false;
        }

        // Create a new page object with the added component
        const updatedPage = create(PageSchema, {
            pageId: currentPage.pageId,
            title: currentPage.title,
            components: [
                ...currentPage.components, // Spread existing components
                component                   // Add the new component
            ]
        });

        // Set the updated page as a function (as expected by setPage)
        setPage(req, () => updatedPage);
        
        console.log(`✅ Component added successfully. Total components: ${updatedPage.components.length}`);
        return true;
        
    } catch (err) {
        console.error("❌ Error adding component to page:", err);
        return false;
    }
}