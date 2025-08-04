// frontend/src/functions/getInfo.js
import { PageSchema } from "../../../../protos/page_pb.js";
import { fromBinary } from "@bufbuild/protobuf";

export default async function getInfo() {
  try {
    const res = await fetch("/api/data", {
      method: "GET",
      headers: {
        "Accept": "application/x-protobuf"
      }
    });

    if (!res.ok) throw new Error(`HTTP error! Status: ${res.status}`);

    // Get binary data
    const buffer = new Uint8Array(await res.arrayBuffer());

    // Decode Protobuf using BufBuild helpers
    const page = fromBinary(PageSchema, buffer);

    console.log("Decoded Page from server:", page);

    return page;
  } catch (err) {
    console.error("Error fetching page:", err);
    return null;
  }
}
