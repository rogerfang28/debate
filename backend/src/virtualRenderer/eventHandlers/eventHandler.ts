import express from "express";
import getPage from "../pages/getPage.ts";
import setCurrentPage from "../utils/setPage.ts";
import openCreateRoomModal from "./functions/roomModal/openCreateRoomModal.ts";
import closeCreateRoomModal from "./functions/roomModal/closeCreateRoomModal.ts";
import openFriendModal from "./functions/friendModal/openFriendModal.ts";
import closeFriendModal from "./functions/friendModal/closeFriendModal.ts";
import { loadHomePage, loadProfilePage, loadPublicDebatesPage, loadRoomPage, loadTestPage } from "../pages/loaders/index.ts";

export default async function handleEvent(req: any, actionId: string, eventData?: Record<string, any>) {
    if (!req.session) {
        console.error("❌ Session is undefined! Cannot set page.");
        return;
    }
    
    console.log(`🎯 Handling action: ${actionId}`, eventData ? `with data: ${JSON.stringify(eventData)}` : '');
    
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
        
        case "submitCreateRoom":
            console.log("Submitting create room form");
            if (eventData) {
                console.log("Room data:", eventData);
                // Here you can access eventData.roomName, eventData.roomDescription, etc.
                // TODO: Implement actual room creation logic
            }
            await closeCreateRoomModal(req);
            break;
            
        case "submitAddFriend":
            console.log("Submitting add friend form");
            if (eventData) {
                console.log("Friend data:", eventData);
                // Here you can access eventData.friendEmail, etc.
                // TODO: Implement actual friend adding logic
            }
            await closeFriendModal(req);
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