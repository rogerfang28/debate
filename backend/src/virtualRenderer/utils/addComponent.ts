import getPage from "../pages/getPage.ts";
import setCurrentPage from "./setPage.ts";
import { create } from "@bufbuild/protobuf";
import express from "express";
import { Page } from "../../../../src/gen/js/page_pb.js";
import fs from "fs";
import { PageSchema } from "../../../../src/gen/js/page_pb.js";

export default async function addComponent(req: any, component: any) {
    try {
        // Get the current page
        let currentPage = await getPage(req);
        
        if (!currentPage) {
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
        
        return true;
        
    } catch (err) {
        return false;
    }
}