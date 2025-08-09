import addComponent from "../../../utils/addComponent.ts";
import { ComponentType } from "../../../../../../src/gen/page_pb.js";
import createRoomModal from "../../../pages/components/createRoomModal.ts";

export default async function openCreateRoomModal(req: any) {
    try {
        // Create the modal component
        const roomModal = createRoomModal();
        // Add the modal to the current page
        const success = await addComponent(req, roomModal);

        return success;
        
    } catch (err) {
        console.error("❌ Error opening create room modal:", err);
        return false;
    }
}