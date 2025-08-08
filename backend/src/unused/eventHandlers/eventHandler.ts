import express from "express";
import homePage from "../../pages";
import roomPage from "../pages/roomPage.js";
import profilePage from "../pages/profilePage.js";
import publicDebatesPage from "../pages/publicDebatesPage.js";
import setCurrentPage from "../../virtualRenderer/eventHandlers/utils/setPage.ts";
import getCurrentPage from "../../virtualRenderer/eventHandlers/utils/getPage.ts";
import openCreateRoomModal from "../../virtualRenderer/eventHandlers/functions/roomModal/openCreateRoomModal.ts";
import closeCreateRoomModal from "../../virtualRenderer/eventHandlers/utils/closeCreateRoomModal.ts";

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