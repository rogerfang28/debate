import express from "express";
import getPage from "../virtualRenderer/pages/getPage.ts";
import setCurrentPage from "../virtualRenderer/utils/setPage.ts";
import openCreateRoomModal from "../virtualRenderer/functions/roomModal/openCreateRoomModal.ts";
import closeCreateRoomModal from "../virtualRenderer/functions/roomModal/closeCreateRoomModal.ts";
import openFriendModal from "../virtualRenderer/functions/friendModal/openFriendModal.ts";
import closeFriendModal from "../virtualRenderer/functions/friendModal/closeFriendModal.ts";
import { loadHomePage, loadProfilePage, loadPublicDebatesPage, loadRoomPage, loadTestPage, loadAuthPage } from "../virtualRenderer/pages/loaders/index.ts";
import goHome from "./events/goHome.ts";
import { loadavg } from "os";
import goLogin from "./events/goLogin.ts";
import goSignup from "./events/goSignup.ts";

export default async function handleEvent(req: any, actionId: string, eventData?: Record<string, any>) {
    if (!req.session) {
        return;
    }
    console.log(`Handling event ${actionId}`)
    console.log(eventData)
    
    switch (actionId) {
        case "goHome":
            goHome(req);
            break;
        
        case "goLogin":
            goLogin(req);
            break;

        case "goSignUp":
            goSignup(req);
            break;
        case "login":
            // handle login
            break;

        case "logout":
            //logout
            goSignup(req);
            break;

        case "signup":
            //signup
            break;
        
        case "goProfile":
            setCurrentPage(req, loadProfilePage);
            break;
        
        case "goRoom":
            setCurrentPage(req, loadRoomPage);
            break;
        
        case "goPublicDebates":
            setCurrentPage(req, loadPublicDebatesPage);
            break;
        
        case "submitCreateRoom":
            if (eventData) {
                // TODO: Implement actual room creation logic
            }
            await closeCreateRoomModal(req);
            break;
            
        case "submitAddFriend":
            if (eventData) {
                // TODO: Implement actual friend adding logic
            }
            await closeFriendModal(req);
            break;
        case "openCreateRoomModal":
            await openCreateRoomModal(req);
            break;
        
        case "closeCreateRoomModal":
            await closeCreateRoomModal(req);
            break;
        case "openFriendsModal":
            await openFriendModal(req);
            break;
        case "closeFriendModal":
            await closeFriendModal(req);
            break;
        
        case "joinRoom":
            if (eventData && eventData.joinRoomForm) {
                const inviteCode = eventData.joinRoomForm;
                
                try {
                    // Import the existing room joining function
                    const { joinRoomByCode } = await import("../old_server/controllers/room/joinRoomByCode.js");
                    
                    // Create a mock response object to capture the result
                    let joinResult: any = null;
                    let joinError: any = null;
                    
                    const mockReq = {
                        params: { inviteCode },
                        user: { userId: "687ac6a3b292e5fa181ed2f1" } // TODO: Get from actual auth session
                    };
                    
                    const mockRes = {
                        status: (code: number) => ({
                            json: (data: any) => {
                                joinError = { code, ...data };
                                return mockRes;
                            }
                        }),
                        json: (data: any) => {
                            joinResult = data;
                            return mockRes;
                        }
                    };
                    
                    // Call the existing function
                    await joinRoomByCode(mockReq, mockRes);
                    
                    if (joinResult) {
                        // Redirect to the room page
                        await setCurrentPage(req, "room");
                        await loadRoomPage(req, joinResult.room._id);
                    } else if (joinError) {
                        // TODO: Show error message in UI
                    }
                    
                } catch (error) {
                    // TODO: Show error message in UI
                }
            } else {
                // No invite code provided
            }
            break;
            
        case "submitCreateRoom":
            if (eventData && eventData.roomNameInput) {
                const roomName = eventData.roomNameInput;
                const roomDescription = eventData.roomDescInput || "";
                
                try {
                    // Import the existing room creation function
                    const { createRoom } = await import("../old_server/controllers/room/createRoom.js");
                    
                    // Create a mock response object to capture the result
                    let createResult: any = null;
                    let createError: any = null;
                    
                    const mockReq = {
                        body: { 
                            name: roomName,
                            description: roomDescription,
                            isPublic: false,
                            maxMembers: 50
                        },
                        user: { userId: "687ac6a3b292e5fa181ed2f1" } // TODO: Get from actual auth session
                    };
                    
                    const mockRes = {
                        status: (code: number) => ({
                            json: (data: any) => {
                                createError = { code, ...data };
                                return mockRes;
                            }
                        }),
                        json: (data: any) => {
                            createResult = data;
                            return mockRes;
                        }
                    };
                    
                    // Call the existing function
                    await createRoom(mockReq, mockRes);
                    
                    if (createResult) {
                        // Close the modal
                        await closeCreateRoomModal(req);
                        
                        // Redirect to the new room page
                        await setCurrentPage(req, "room");
                        await loadRoomPage(req, createResult.room._id);
                    } else if (createError) {
                        // TODO: Show error message in UI
                    }
                    
                } catch (error) {
                    // TODO: Show error message in UI
                }
            } else {
                // No room name provided
            }
            break;
            
        case "enterRoom":
            // Try to extract roomId from different possible locations
            let roomId = eventData?.roomId;
            
            // If no roomId found, try fallback for testing
            if (!roomId) {
                roomId = "room1"; // Fallback for testing the button
            }
            
            if (roomId) {
                try {
                    // Load the room page and set it as current (correct order)
                    const roomPage = await loadRoomPage(req, roomId);
                    await setCurrentPage(req, roomPage);
                } catch (error) {
                    // Error entering room
                }
            } else {
                // No room ID could be determined
            }
            break;
            
        default:
            // Unknown action
    }
}