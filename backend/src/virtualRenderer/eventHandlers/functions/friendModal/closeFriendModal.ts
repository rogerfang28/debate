import deleteComponent from "../../../utils/deleteComponent.ts";

export default async function closeCreateRoomModal(req: any) {
    try {
        // Delete the create room modal component
        const success = await deleteComponent(req, "friendModal");
        
        return success;
        
    } catch (err) {
        console.error("❌ Error closing create room modal:", err);
        return false;
    }
}
