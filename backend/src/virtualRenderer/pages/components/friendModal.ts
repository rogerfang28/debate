import { ComponentType } from "../../../../../src/gen/page_pb.ts";

export default function createFriendModal(){
    return {
        id: "friendModal",
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
                                text: "Add Friend",
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
                                events: { onClick: "closeFriendModal" }
                            }
                        ]
                    },

                    // Modal Form
                    {
                        id: "addFriendForm",
                        type: ComponentType.CONTAINER,
                        style: { 
                            customClass: "space-y-4"
                        },
                        children: [
                            {
                                id: "friendEmailInput",
                                type: ComponentType.INPUT,
                                text: "Enter friend's email",
                                name: "friendEmail",
                                style: { 
                                    customClass: "w-full p-3 bg-gray-700 border border-gray-600 rounded-lg text-white placeholder-gray-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
                                },
                                events: { onChange: "updateFriendEmail" }
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
                                        id: "btnAddFriend",
                                        type: ComponentType.BUTTON,
                                        text: "Add Friend",
                                        style: { 
                                            customClass: "flex-1 px-6 py-3 bg-blue-600 hover:bg-blue-500 text-white font-medium rounded-lg transition-colors"
                                        },
                                        events: { onClick: "submitAddFriend" }
                                    },
                                    {
                                        id: "btnCancelAdd",
                                        type: ComponentType.BUTTON,
                                        text: "Cancel",
                                        style: { 
                                            customClass: "px-6 py-3 bg-gray-600 hover:bg-gray-500 text-white font-medium rounded-lg transition-colors"
                                        },
                                        events: { onClick: "closeFriendModal" }
                                    }
                                ]
                            }
                        ]
                    }
                ]
            }
        ]
    };
}