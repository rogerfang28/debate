import homePage from "../../schemas/homePage.js";
import getRooms from "./functions/loadRooms.ts";
import roomCard from "../../components/roomDisplay.ts"; // adjust path
import { create } from "@bufbuild/protobuf";
import { PageSchema, ListItemSchema } from "../../../../../../src/gen/page_pb.js";

export default async function loadHomePage(req: any) {
  // Load base page structure
  let page = homePage(); // the problem here is that it is just loading the homePage schema, which overrides the component added

  if (true) { // if (req.user && req.user.userId)
    // Fetch the user's rooms from MongoDB
    const rooms = await getRooms("687ac6a3b292e5fa181ed2f1"); // Hardcoded for now

    // Convert DB rooms into proper protobuf ListItem objects
    const roomItems = rooms.map((room: any) => create(ListItemSchema, {
      label: `🏠 ${room.name || "Untitled Room"}`,
      value: room._id.toString(), // Store the room ID in the value field
      icon: "🏠" // Use house icon
    }));
    
    console.log("Loaded rooms:", roomItems.length);
    
    // Find the roomList component and replace its items
    const mainContainer = page.components.find((comp: any) => comp.id === "main");
    const roomsSection = mainContainer?.children?.find((child: any) => child.id === "roomsSection");
    
    if (roomsSection) {
      const roomList = roomsSection.children?.find((child: any) => child.id === "roomList");
      if (roomList) {
        // Keep it as a LIST component and update the items array
        roomList.items = roomItems; // Replace with actual room items
        console.log("Updated roomList with", roomItems.length, "rooms");
      }
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
