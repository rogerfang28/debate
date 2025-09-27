import roomCard from "../pages/components/roomDisplay.ts";
import { PageSchema, ComponentType, ComponentSchema } from "../../../../../../../src/gen//page_pb.js";
import { create, toBinary } from "@bufbuild/protobuf";

// Test if roomCard can be encoded properly
function testRoomCard() {
  try {
    // Create a room card
    const card = roomCard({
      _id: "test123",
      title: "Test Room",
      description: "Test Description",
      membersCount: 5
    });
    
    // Try to create a simple page with the room card
    const testPage = create(PageSchema, {
      pageId: "test",
      title: "Test Page",
      components: [
        create(ComponentSchema, {
          id: "main",
          type: ComponentType.CONTAINER,
          children: [card] // Insert the room card
        })
      ]
    });
    
    // Try to encode it to binary (this is where the error occurs)
    const binary = toBinary(PageSchema, testPage);
    
  } catch (error) {
    // Error in roomCard test
  }
}

export default testRoomCard;
