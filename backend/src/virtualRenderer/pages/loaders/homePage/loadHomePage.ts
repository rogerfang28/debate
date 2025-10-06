import homePage from "../../schemas/homePage.js";
import getRooms from "./functions/loadRooms.js";
import roomCard from "../../components/roomDisplay.js";
import { create } from "@bufbuild/protobuf";
import express from "express";
import setCurrentPage from "../../../utils/setPage.js";
import getPage from "../../getPage.js";
import fs from "fs";
import { PageSchema, ListItemSchema, ComponentType, ComponentSchema } from "../../../../../../src/gen/js/page_pb.js";

export default async function loadHomePage() {
  // Load base page structure
  let page = homePage(); // start from schema

  if (true) { // if (req.user && req.user.userId)
    // Fetch the user's rooms from MongoDB
    const rooms = await getRooms("687ac6a3b292e5fa181ed2f1"); // Hardcoded for now

    // Create roomCard components using the working function
    const roomCards = rooms.map((room: any) => roomCard({
      _id: room._id.toString(),
      title: room.name || "Untitled Room", 
      description: room.description || "No description available",
      membersCount: room.members ? room.members.length : 0
    }));
    
    // Find the roomsSection and completely rebuild it with protobuf components
    const mainContainer = page.components.find((comp: any) => comp.id === "main");
    const roomsSectionIndex = mainContainer?.children?.findIndex((child: any) => child.id === "roomsSection");
    
    if (roomsSectionIndex !== -1) {
      // Rebuild the entire roomsSection with proper protobuf components
      mainContainer.children[roomsSectionIndex] = create(ComponentSchema, {
        id: "roomsSection",
        type: ComponentType.CONTAINER,
        children: [
          create(ComponentSchema, {
            id: "roomsTitle",
            type: ComponentType.TEXT,
            text: "Your Rooms",
            style: { customClass: "text-2xl font-bold mb-6" }
          }),
          create(ComponentSchema, {
            id: "roomList",
            type: ComponentType.CONTAINER,
            style: { customClass: "grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4" },
            children: roomCards
          })
        ]
      });
    }
  } else {
    // No user logged in - clear the room list
    const mainContainer = page.components.find((comp: any) => comp.id === "main");
    const roomsSection = mainContainer?.children?.find((child: any) => child.id === "roomsSection");
    
    if (roomsSection) {
      const roomList = roomsSection.children?.find((child: any) => child.id === "roomList");
      if (roomList) {
        // Keep it as a LIST and clear the items
        roomList.items = [];
      }
    }
  }

  // Ensure we return a valid PageSchema protobuf
  return page;
}
