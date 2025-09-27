// src/functions/getPageFromCPP.ts
import { fromBinary } from "@bufbuild/protobuf";
import { PageSchema } from "../../../src/gen/ts/page_pb";

/**
 * GET a Page (protobuf) from your C++ server.
 * - Sends Accept: application/x-protobuf
 * - Expects the response body to be a serialized PageSchema
 */
export async function testCPPBackend(pageId?: string) {
  const url = new URL("http://127.0.0.1:8080/");
  if (pageId) url.searchParams.set("id", pageId); // optional ?id=<pageId>

  console.log("▶️ GET", url.toString(), "(expecting protobuf Page)");

  const res = await fetch(url.toString(), {
    method: "GET",
    headers: { Accept: "application/x-protobuf" },
    cache: "no-store",
  });

  if (!res.ok) {
    const errText = await res.text().catch(() => "");
    throw new Error(`HTTP ${res.status} ${res.statusText}: ${errText}`);
  }

  const ct = res.headers.get("content-type") || "";
  if (!ct.includes("application/x-protobuf")) {
    const preview = await res.text().catch(() => "");
    throw new Error(`Expected protobuf, got "${ct}". Body: ${preview.slice(0, 200)}`);
  }

  const bytes = new Uint8Array(await res.arrayBuffer());
  const page = fromBinary(PageSchema, bytes);
  console.log("✅ Received Page (protobuf):", page);
  return page;
}
