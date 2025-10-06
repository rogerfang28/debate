import roomPage from "../../schemas/roomPage.js";

export default async function loadRoomPage(req: any, roomId?: string) {
  // Get the base room page structure
  const page = roomPage();
  
  if (roomId) {
    try {
      // Import the existing room fetching function
      const { getRoom } = await import("../../../../old_server/controllers/room/getRoom.js");
      
      // Create mock request/response to get room data
      let roomData: any = null;
      const mockReq = {
        params: { roomId },
        user: { userId: "687ac6a3b292e5fa181ed2f1" } // TODO: Get from actual auth session
      };
      
      const mockRes = {
        status: (code: number) => ({ json: (data: any) => null }),
        json: (data: any) => { roomData = data; }
      };
      
      await getRoom(mockReq, mockRes);
      
      if (roomData?.room) {
        // Update the room title with the actual room name
        const roomTitle = page.children?.[0]?.children?.[0]?.children?.[1]?.children?.[1];
        if (roomTitle) {
          roomTitle.text = `🗣️ ${roomData.room.name}`;
        }
        
        console.log("✅ Loaded room page for:", roomData.room.name);
      }
      
    } catch (error) {
      console.error("❌ Error loading room data:", error);
    }
  }
  
  return page;
}