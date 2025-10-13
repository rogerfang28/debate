import { fromBinary } from "@bufbuild/protobuf";
import { PageSchema } from "../../../src/gen/ts/layout_pb";

export interface PageData {
  [key: string]: any;
}

/**
 * Fetch a protobuf Page from your C++ server.
 * - Defaults to http(s)://<current-host>:8080/
 * - Validates Content-Type
 * - Supports optional ?id=<pageId>
 * - Keeps no-store to avoid caching during dev
 */
export default async function getPageFromCPP(opts?: {
  pageId?: string;
  endpoint?: string;          // override default root if you want
  withCredentials?: boolean;  // send cookies if needed
}): Promise<PageData | null> {
  try {
    const defaultEndpoint = `${location.protocol}//${location.hostname}:8080/`;
    const endpoint = opts?.endpoint ?? defaultEndpoint;

    const url = new URL(endpoint);
    if (opts?.pageId) url.searchParams.set("id", opts.pageId);

    console.log("▶️ GET", url.toString(), "(expecting protobuf Page)");

    const res = await fetch(url.toString(), {
      method: "GET",
      headers: { Accept: "application/x-protobuf" },
      cache: "no-store",
      credentials: opts?.withCredentials ? "include" : "same-origin",
    });

    if (!res.ok) {
      const errText = await res.text().catch(() => "");
      console.error(`HTTP ${res.status} ${res.statusText}: ${errText}`);
      return null;
    }

    const ct = res.headers.get("content-type") || "";
    if (!ct.includes("application/x-protobuf")) {
      const preview = await res.text().catch(() => "");
      console.error(`Expected protobuf, got "${ct}". Body: ${preview.slice(0, 200)}`);
      return null;
    }

    const bytes = new Uint8Array(await res.arrayBuffer());
    const page = fromBinary(PageSchema, bytes);
    console.log("✅ Received Page (protobuf):", page);
    return page as PageData;
  } catch (err) {
    console.error("getPageFromCPP error:", err);
    return null;
  }
}
