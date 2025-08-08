import express from "express";
import homePage from "../virtualRenderer/pages/homePage.js";
import roomPage from "../virtualRenderer/pages/roomPage.js";
import profilePage from "../virtualRenderer/pages/profilePage.js";
import publicDebatesPage from "../virtualRenderer/pages/publicDebatesPage.js";

function setCurrentPage(req: any, pageFn: () => any) {
    if (!req.session) {
        console.error("❌ Session is undefined! Cannot set page.");
        return;
    }
    if (typeof pageFn !== "function") {
        console.error("❌ Page function is invalid:", pageFn);
        return;
    }

    try {
        const page = pageFn();
        req.session.currentPage = page;
        console.log(`✅ Current page set to: ${page.pageId || "[unknown ID]"}`);
    } catch (err) {
        console.error("❌ Error creating page object:", err);
    }
}

export default function handleEvent(req: any, actionId: string) {
    switch (actionId) {
        case "goHome":
            console.log("Going to home page");
            setCurrentPage(req, homePage);
            break;
        
        case "goProfile":
            console.log("Going to profile page");
            setCurrentPage(req, profilePage);
            break;
        
        case "goRoom":
            console.log("Going to room page");
            setCurrentPage(req, roomPage);
            break;
        
        case "goPublicDebates":
            console.log("Going to public debates page");
            setCurrentPage(req, publicDebatesPage);
            break;
        
        default:
            console.warn("Unknown actionId:", actionId);
    }
}