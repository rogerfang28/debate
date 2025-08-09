import deleteComponent from "../../../utils/deleteComponent.ts";

export default async function closeCreateRoomModal(req: any) {
    try {
        console.log("🔄 closeCreateRoomModal: Starting to close modal");
        
        // Delete the create room modal component
        const success = await deleteComponent(req, "createRoomModal");
        
        if (success) {
            console.log("✅ Create room modal closed successfully");
        } else {
            console.error("❌ Failed to close create room modal - modal not found");
        }
        
        return success;
        
    } catch (err) {
        console.error("❌ Error closing create room modal:", err);
        return false;
    }
}
