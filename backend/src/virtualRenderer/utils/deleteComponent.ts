import getPage from "../pages/getPage.ts";
import setPage from "./setPage.ts";
import { create } from "@bufbuild/protobuf";
import express from "express";
import { Page } from "../../../../src/gen/js/page_pb.js";
import fs from "fs";
import { PageSchema } from "../../../../src/gen/js/page_pb.js";

export default async function deleteComponent(req: any, componentId: string) {
    try {
        let currentPage = await getPage(req);
        
        if (!currentPage) {
            return false;
        }

        // Filter out the component with the specified ID
        const updatedComponents = currentPage.components.filter(
            (component: any) => component.id !== componentId
        );

        // Check if component was actually found and removed
        if (updatedComponents.length === currentPage.components.length) {
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
        
        return true;
        
    } catch (err) {
        return false;
    }
}