import deleteComponent from "../../utils/deleteComponent.ts";

export default async function closeFriendModal(req: any) {
    try {
        // Delete the friend modal component
        const success = await deleteComponent(req, "friendModal");
        
        return success;
        
    } catch (err) {
        console.error("❌ Error closing friend modal:", err);
        return false;
    }
}
