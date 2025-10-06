import addComponent from "../../utils/addComponent.ts";
import { ComponentType } from "../../../../../src/gen/js/page_pb.js";
import createFriendModal from "../../pages/components/friendModal.ts";

export default async function openFriendModal(req: any) {
    try {
        // Create the modal component
        const friendModal = createFriendModal();
        // Add the modal to the current page
        const success = await addComponent(req, friendModal);

        return success;
        
    } catch (err) {
        console.error("❌ Error opening create friend modal:", err);
        return false;
    }
}