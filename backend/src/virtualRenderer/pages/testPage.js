import { PageSchema, ComponentType } from "../../../../src/gen/page_pb.js";
import { create } from "@bufbuild/protobuf";

export default function testPage() {
  return create(PageSchema, {
    pageId: "test",
    title: "Simple Tailwind Test Page",
    components: [
      {
        id: "main",
        type: ComponentType.CONTAINER,
        style: {
          customClass: "min-h-screen bg-gray-900 text-white p-8 flex flex-col items-center gap-6"
        },
        children: [
          {
            id: "title",
            type: ComponentType.TEXT,
            text: "🧪 Test Page",
            style: { customClass: "text-3xl font-bold text-blue-400" }
          },
          {
            id: "description",
            type: ComponentType.TEXT,
            text: "This is a minimal page to check if Tailwind styles apply correctly.",
            style: { customClass: "text-gray-300 text-center" }
          },
          {
            id: "testButton",
            type: ComponentType.BUTTON,
            text: "Click Me",
            style: {
              customClass:
                "px-6 py-3 bg-green-600 hover:bg-green-500 text-white rounded-lg transition"
            },
            events: { onClick: "testClick" }
          }
        ]
      }
    ]
  });
}
