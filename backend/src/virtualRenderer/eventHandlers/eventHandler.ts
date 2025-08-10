import express from "express";
import getPage from "../pages/getPage.ts";
import setCurrentPage from "../utils/setPage.ts";
import openCreateRoomModal from "./functions/roomModal/openCreateRoomModal.ts";
import closeCreateRoomModal from "./functions/roomModal/closeCreateRoomModal.ts";
import openFriendModal from "./functions/friendModal/openFriendModal.ts";
import closeFriendModal from "./functions/friendModal/closeFriendModal.ts";
import { loadHomePage, loadProfilePage, loadPublicDebatesPage, loadRoomPage, loadTestPage } from "../pages/loaders/index.ts";

export default async function handleEvent(req: any, actionId: string) {
    if (!req.session) {
        console.error("❌ Session is undefined! Cannot set page.");
        return;
    }
    switch (actionId) {
        case "goHome":
            console.log("Going to home page");
            let homePage = await loadHomePage();
            setCurrentPage(req, homePage);
            break;
        
        case "goProfile":
            console.log("Going to profile page");
            setCurrentPage(req, loadProfilePage);
            break;
        
        case "goRoom":
            console.log("Going to room page");
            setCurrentPage(req, loadRoomPage);
            break;
        
        case "goPublicDebates":
            console.log("Going to public debates page");
            setCurrentPage(req, loadPublicDebatesPage);
            break;
        
        case "openCreateRoomModal":
            console.log("Opening create room modal");
            await openCreateRoomModal(req);
            break;
        
        case "closeCreateRoomModal":
            console.log("Closing create room modal");
            await closeCreateRoomModal(req);
            break;
        case "openFriendsModal":
            console.log("Opening friend modal");
            await openFriendModal(req);
            break;
        case "closeFriendModal":
            console.log("Closing friend modal");
            await closeFriendModal(req);
            break;
        default:
            console.warn("Unknown actionId:", actionId);
    }
}