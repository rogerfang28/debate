import addComponent from "../../utils/addComponent.ts";
import { ComponentType } from "../../../../../src/gen/page_pb.js";

export default function openCreateRoomModal(req: any) {
    try {
        // Create the modal component
        const createRoomModal = {
            id: "createRoomModal",
            type: ComponentType.CONTAINER,
            style: { 
                customClass: "fixed inset-0 bg-black/50 backdrop-blur-sm z-50 flex items-center justify-center"
            },
            children: [
                {
                    id: "modalContent",
                    type: ComponentType.CONTAINER,
                    style: { 
                        customClass: "bg-gray-800 rounded-lg border border-gray-700 p-6 w-full max-w-md mx-4"
                    },
                    children: [
                        // Modal Header
                        {
                            id: "modalHeader",
                            type: ComponentType.CONTAINER,
                            style: { 
                                customClass: "flex justify-between items-center mb-6"
                            },
                            children: [
                                {
                                    id: "modalTitle",
                                    type: ComponentType.TEXT,
                                    text: "Create New Room",
                                    style: { 
                                        customClass: "text-xl font-bold text-white"
                                    }
                                },
                                {
                                    id: "btnCloseModal",
                                    type: ComponentType.BUTTON,
                                    text: "✕",
                                    style: { 
                                        customClass: "text-gray-400 hover:text-white p-2 rounded transition-colors bg-transparent border-none"
                                    },
                                    events: { onClick: "closeCreateRoomModal" }
                                }
                            ]
                        },
                        
                        // Modal Form
                        {
                            id: "createRoomForm",
                            type: ComponentType.CONTAINER,
                            style: { 
                                customClass: "space-y-4"
                            },
                            children: [
                                {
                                    id: "roomNameInput",
                                    type: ComponentType.INPUT,
                                    text: "Enter room name",
                                    name: "roomName",
                                    style: { 
                                        customClass: "w-full p-3 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                                    },
                                    events: { onChange: "updateRoomName" }
                                },
                                {
                                    id: "roomDescInput",
                                    type: ComponentType.INPUT,
                                    text: "Room description (optional)",
                                    name: "roomDescription",
                                    style: { 
                                        customClass: "w-full p-3 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                                    },
                                    events: { onChange: "updateRoomDescription" }
                                },
                                
                                // Form Actions
                                {
                                    id: "modalActions",
                                    type: ComponentType.CONTAINER,
                                    style: { 
                                        customClass: "flex gap-3 pt-4"
                                    },
                                    children: [
                                        {
                                            id: "btnCreateRoom",
                                            type: ComponentType.BUTTON,
                                            text: "Create Room",
                                            style: { 
                                                customClass: "flex-1 px-6 py-3 bg-blue-600 hover:bg-blue-500 text-white font-medium rounded-lg transition-colors"
                                            },
                                            events: { onClick: "submitCreateRoom" }
                                        },
                                        {
                                            id: "btnCancelCreate",
                                            type: ComponentType.BUTTON,
                                            text: "Cancel",
                                            style: { 
                                                customClass: "px-6 py-3 bg-gray-600 hover:bg-gray-500 text-white font-medium rounded-lg transition-colors"
                                            },
                                            events: { onClick: "closeCreateRoomModal" }
                                        }
                                    ]
                                }
                            ]
                        }
                    ]
                }
            ]
        };

        // Add the modal to the current page
        const success = addComponent(req, createRoomModal);
        
        if (success) {
            console.log("✅ Create room modal opened successfully");
        } else {
            console.error("❌ Failed to open create room modal");
        }
        
        return success;
        
    } catch (err) {
        console.error("❌ Error opening create room modal:", err);
        return false;
    }
}