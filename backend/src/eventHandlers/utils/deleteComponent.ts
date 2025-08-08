import getPage from "./getPage.ts";
import setPage from "./setPage.ts";
import { create } from "@bufbuild/protobuf";
import { PageSchema } from "../../../../src/gen/page_pb.js";

export default function deleteComponent(req: any, componentId: string) {
    try {
        let currentPage = getPage(req);
        
        if (!currentPage) {
            console.error("❌ No current page found");
            return false;
        }

        // Filter out the component with the specified ID
        const updatedComponents = currentPage.components.filter(
            (component: any) => component.id !== componentId
        );

        // Check if component was actually found and removed
        if (updatedComponents.length === currentPage.components.length) {
            console.warn(`⚠️ Component with ID '${componentId}' not found`);
            return false;
        }

        // Create a new page object without the deleted component
        const updatedPage = create(PageSchema, {
            pageId: currentPage.pageId,
            title: currentPage.title,
            components: updatedComponents
        });

        // Set the updated page as a function (as expected by setPage)
        setPage(req, () => updatedPage);
        
        console.log(`✅ Component '${componentId}' deleted successfully. Remaining components: ${updatedPage.components.length}`);
        return true;
        
    } catch (err) {
        console.error("❌ Error deleting component from page:", err);
        return false;
    }
}