import { PageSchema } from "../../../../src/gen/page_pb.js";
import { fromBinary } from "@bufbuild/protobuf";

// TypeScript interface for API response
interface PageData {
  [key: string]: any; // Generic page structure from protobuf
}

export default async function getInfo(endpoint?: string): Promise<PageData | null> {
  try {
    const url = endpoint || "/api/data";
    const res = await fetch(url, {
      method: "GET",
      headers: {
        "Accept": "application/x-protobuf"
      },
      credentials: "include" // 🔹 send cookies/session ID
    });

    if (!res.ok) throw new Error(`HTTP error! Status: ${res.status}`);

    // Get binary data
    const buffer = new Uint8Array(await res.arrayBuffer());

    // Decode Protobuf using BufBuild helpers
    const page = fromBinary(PageSchema, buffer);

    console.log("Decoded Page from server:", page);

    return page as PageData;
  } catch (err: unknown) {
    console.error("Error fetching page:", err);
    return null;
  }
}
