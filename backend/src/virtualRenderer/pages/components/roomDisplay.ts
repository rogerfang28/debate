import { ComponentType, ComponentSchema } from "../../../../../src/gen/ts/page_pb.ts";
import { create } from "@bufbuild/protobuf";

interface RoomCardProps {
  _id: string;            // MongoDB ObjectId as a string
  title: string;
  description: string;
  membersCount: number;
}

export default function roomCard({ _id, title, description, membersCount }: RoomCardProps) {
  return create(ComponentSchema, {
    id: `room-${_id}`, // Unique UI ID
    type: ComponentType.CARD,
    style: {
      customClass: "p-4 border border-gray-300 rounded-lg shadow-sm hover:shadow-md transition-all bg-white flex flex-col gap-3"
    },
    children: [
      create(ComponentSchema, {
        id: `room-title-${_id}`,
        type: ComponentType.TEXT,
        text: title,
        style: { customClass: "text-lg font-semibold" }
      }),
      create(ComponentSchema, {
        id: `room-desc-${_id}`,
        type: ComponentType.TEXT,
        text: description,
        style: { customClass: "text-sm text-gray-600" }
      }),
      create(ComponentSchema, {
        id: `room-members-${_id}`,
        type: ComponentType.TEXT,
        text: `👥 ${membersCount} members`,
        style: { customClass: "text-sm text-gray-500" }
      }),
      create(ComponentSchema, {
        id: `enter-btn-${_id}`,
        type: ComponentType.BUTTON,
        text: "Enter Room",
        events: { 
          onClick: JSON.stringify({
            actionId: "enterRoom",
            roomId: _id.toString()
          })
        },
        style: { customClass: "px-4 py-2 bg-blue-500 text-white rounded hover:bg-blue-600 transition-all self-start" }
      })
    ]
  });
}
