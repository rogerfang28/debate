// backend/src/virtualRenderer/pages/homePage.js
import { PageSchema, ComponentType } from "../../../../../src/gen/page_pb.js";
import { create } from "@bufbuild/protobuf";

export default function homePage() {
  return create(PageSchema, {
    pageId: "home",
    title: "DebateGraph Home",
    components: [
      // Navigation Bar
      {
        id: "navbar",
        type: ComponentType.CONTAINER,
        style: { customClass: "flex justify-between items-center p-4 bg-gray-800 text-white" },
        children: [
          {
            id: "brand",
            type: ComponentType.TEXT,
            text: "🗣️ DebateGraph",
            style: { customClass: "text-xl font-bold" }
          },
          {
            id: "navActions",
            type: ComponentType.CONTAINER,
            style: { customClass: "flex gap-4" },
            children: [
              {
                id: "btnPublicDebates",
                type: ComponentType.BUTTON,
                text: "🌍 Public Debates",
                style: { customClass: "px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded transition-colors" },
                events: { onClick: "goPublicDebates" }
              },
              {
                id: "btnFriends",
                type: ComponentType.BUTTON,
                text: "👥 Friends",
                style: { customClass: "px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded transition-colors" },
                events: { onClick: "openFriendsModal" }
              },
              {
                id: "btnProfile",
                type: ComponentType.BUTTON,
                text: "👤 Profile",
                style: { customClass: "px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded transition-colors" },
                events: { onClick: "goProfile" }
              },
              {
                id: "btnLogout",
                type: ComponentType.BUTTON,
                text: "🚪 Logout",
                style: { customClass: "px-4 py-2 bg-red-600 hover:bg-red-700 text-white rounded transition-colors" },
                events: { onClick: "logout" }
              }
            ]
          }
        ]
      },

      // Main Content Container
      {
        id: "main",
        type: ComponentType.CONTAINER,
        style: { customClass: "p-8 max-w-4xl mx-auto" },
        children: [
          {
            id: "welcomeSection",
            type: ComponentType.CONTAINER,
            style: { customClass: "text-center mb-12" },
            children: [
              {
                id: "welcomeTitle",
                type: ComponentType.TEXT,
                text: "Your Debate Rooms",
                style: { customClass: "text-4xl font-bold mb-4" }
              },
              {
                id: "welcomeDesc",
                type: ComponentType.TEXT,
                text: "Create, join, and manage collaborative debate spaces",
                style: { customClass: "text-gray-600 text-lg" }
              }
            ]
          },

          // Action Section
          {
            id: "actionSection",
            type: ComponentType.CONTAINER,
            style: { customClass: "mb-12 max-w-md mx-auto" },
            children: [
              // Join Room Form/Input
              {
                id: "joinRoomForm",
                type: ComponentType.INPUT,
                text: "Enter invite code",
                style: { customClass: "w-full p-3 border border-gray-300 rounded mb-4" },
                events: { onEnter: "joinRoom" }
              },

              // Create Room Button
              {
                id: "btnCreateRoom",
                type: ComponentType.BUTTON,
                text: "Create New Room",
                style: { customClass: "w-full px-6 py-3 bg-blue-600 hover:bg-blue-700 text-white rounded font-medium transition-colors" },
                events: { onClick: "openCreateRoomModal" }
              }
            ]
          },

          // Rooms List
          {
            id: "roomsSection",
            type: ComponentType.CONTAINER,
            children: [
              {
                id: "roomsTitle",
                type: ComponentType.TEXT,
                text: "Your Rooms",
                style: { customClass: "text-2xl font-bold mb-6" }
              },
              {
                id: "roomList",
                type: ComponentType.LIST,
                style: { customClass: "grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4" },
                items: [
                  // { 
                  //   label: "🎯 Strategy Debate Room", 
                  //   events: { onClick: "enterRoom" },
                  //   style: { customClass: "p-4 border border-gray-300 rounded hover:bg-blue-50 hover:border-blue-400 cursor-pointer transition-all" }
                  // },
                  // { 
                  //   label: "🌍 Climate Change Discussion", 
                  //   events: { onClick: "enterRoom" },
                  //   style: { customClass: "p-4 border border-gray-300 rounded hover:bg-green-50 hover:border-green-400 cursor-pointer transition-all" }
                  // },
                  // { 
                  //   label: "💡 Innovation Lab", 
                  //   events: { onClick: "enterRoom" },
                  //   style: { customClass: "p-4 border border-gray-300 rounded hover:bg-purple-50 hover:border-purple-400 cursor-pointer transition-all" }
                  // }
                ]
              }
            ]
          }
        ]
      }
    ]
  });
}
