import express from "express";
import homePage from "../pages/schemas/homePage.js";
import roomPage from "../pages/schemas/roomPage.js";
import profilePage from "../pages/schemas/profilePage.js";
import publicDebatesPage from "../pages/schemas/publicDebatesPage.js";
import setCurrentPage from "./utils/setPage.ts";
import getCurrentPage from "./utils/getPage.ts";
import openCreateRoomModal from "./functions/roomModal/openCreateRoomModal.ts";
import closeCreateRoomModal from "./utils/closeCreateRoomModal.ts";

export default function handleEvent(req: any, actionId: string) {
    if (!req.session) {
        console.error("❌ Session is undefined! Cannot set page.");
        return;
    }
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
        
        case "openCreateRoomModal":
            console.log("Opening create room modal");
            openCreateRoomModal(req);
            break;
        
        case "closeCreateRoomModal":
            console.log("Closing create room modal");
            closeCreateRoomModal(req);
            break;
        
        default:
            console.warn("Unknown actionId:", actionId);
    }
}