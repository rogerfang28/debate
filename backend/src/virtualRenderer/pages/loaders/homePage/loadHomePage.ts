import homePage from "../../schemas/homePage.js";
import getRooms from "./functions/loadRooms.js";
import roomCard from "../../components/roomDisplay.ts"; // adjust path
import { create } from "@bufbuild/protobuf";
import { PageSchema } from "../../../../../../src/gen/page_pb.js";

export default async function loadHomePage(req: any) {
  // Load base page structure
  let page = homePage(); // the problem here is that it is just loading the homePage schema, which overrides the component added

  if (true) { // if (req.user && req.user.userId)
    // Fetch the user's rooms from MongoDB
    const rooms = await getRooms("687ac6a3b292e5fa181ed2f1"); // Hardcoded for now

    // Convert DB rooms into protobuf Component cards
    const roomComponents = rooms.map((room: any) =>
      roomCard({
        _id: room._id.toString(),
        title: room.name,
        description: room.description || "",
        membersCount: room.members?.length || 0
      })
    );
    // console.log(page);
    // Attach cards to the page
    // console.log("eeeee")
    page.userRoom = roomComponents;
    // console.log(page);
  } else {
    // No user logged in
    page.userRoom = [];
  }

  // Ensure we return a valid PageSchema protobuf
  return page;
}
