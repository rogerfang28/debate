import getPage from "../pages/getPage.ts";
import setCurrentPage from "./setPage.ts";
import { create } from "@bufbuild/protobuf";
import { PageSchema } from "../../../../src/gen/page_pb.js";

export default async function addComponent(req: any, component: any) {
    try {
        // Get the current page
        let currentPage = await getPage(req);
        // console.log("page: ",currentPage);
        
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

        // Set the updated page as a function (as expected by setCurrentPage)
        setCurrentPage(req, () => updatedPage);
        // console.log(updatedPage);
        console.log(`✅ Component added successfully. Total components: ${updatedPage.components.length}`);
        return true;
        
    } catch (err) {
        console.error("❌ Error adding component to page:", err);
        return false;
    }
}