// backend/src/virtualRenderer/pages/homePage.js
import { PageSchema, ComponentType } from "../../../../protos/page_pb.js";
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
        style: {
          customClass: "flex justify-between bg-gray-800 p-4 text-white items-center"
        },
        children: [
          {
            id: "brand",
            type: ComponentType.TEXT,
            text: "🗣️ DebateGraph",
            style: { customClass: "font-bold text-xl" }
          },
          {
            id: "navActions",
            type: ComponentType.CONTAINER,
            style: { customClass: "flex gap-2 items-center" },
            children: [
              {
                id: "btnPublicDebates",
                type: ComponentType.BUTTON,
                text: "🌍 Public Debates",
                style: { customClass: "btn btn-tertiary" },
                events: { onClick: "goPublicDebates" } // backend actionId
              },
              {
                id: "btnFriends",
                type: ComponentType.BUTTON,
                text: "👥 Friends",
                style: { customClass: "btn btn-primary relative" },
                events: { onClick: "openFriendsModal" }
              },
              {
                id: "btnProfile",
                type: ComponentType.BUTTON,
                text: "👤 Profile",
                style: { customClass: "btn btn-primary" },
                events: { onClick: "goProfile" }
              },
              {
                id: "btnLogout",
                type: ComponentType.BUTTON,
                text: "🚪 Logout",
                style: { customClass: "btn btn-secondary" },
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
        style: { customClass: "p-6" },
        children: [
          {
            id: "welcomeSection",
            type: ComponentType.CONTAINER,
            style: { customClass: "text-center mb-8" },
            children: [
              {
                id: "welcomeTitle",
                type: ComponentType.TEXT,
                text: "Your Debate Rooms",
                style: { customClass: "text-2xl font-bold mb-2" }
              },
              {
                id: "welcomeDesc",
                type: ComponentType.TEXT,
                text: "Create, join, and manage collaborative debate spaces",
                style: { customClass: "text-gray-300 text-lg" }
              }
            ]
          },

          // Join Room Form/Input
          {
            id: "joinRoomForm",
            type: ComponentType.INPUT,
            text: "Enter invite code",
            style: { customClass: "mb-4" },
            events: { onEnter: "joinRoom" } // handled by frontend as keydown + backend action
          },

          // Create Room Button
          {
            id: "btnCreateRoom",
            type: ComponentType.BUTTON,
            text: "➕ Create Room",
            style: { customClass: "btn btn-primary mb-6" },
            events: { onClick: "openCreateRoomModal" }
          },

          // Rooms List (sample clickable items)
          {
            id: "roomList",
            type: ComponentType.LIST,
            style: { customClass: "room-grid grid grid-cols-2 gap-4" },
            items: [
              { label: "Room A", events: { onClick: "enterRoom" } },
              { label: "Room B", events: { onClick: "enterRoom" } }
            ]
          }
        ]
      }
    ]
  });
}
