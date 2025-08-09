import getPage from "../pages/getPage.ts";
import setCurrentPage from "./setPage.ts";
import { create } from "@bufbuild/protobuf";
import { PageSchema } from "../../../../src/gen/page_pb.js";

export default async function addComponent(req: any, component: any) {
    try {
        console.log("🔄 addComponent: Starting to add component:", component.id);
        
        // Get the current page
        let currentPage = await getPage(req);
        console.log("🔄 addComponent: Current page retrieved:", currentPage?.pageId, "with", currentPage?.components?.length, "components");
        
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

        console.log("🔄 addComponent: Created updated page with", updatedPage.components.length, "components");
        
        // Set the updated page as a function (as expected by setCurrentPage)
        setCurrentPage(req, () => updatedPage);
        
        console.log("🔄 addComponent: Updated page set to session");
        // console.log(updatedPage);
        console.log(`✅ Component added successfully. Total components: ${updatedPage.components.length}`);
        return true;
        
    } catch (err) {
        console.error("❌ Error adding component to page:", err);
        return false;
    }
}