import { fromBinary } from "@bufbuild/protobuf";
import { PageSchema } from "../../../src/gen/ts/layout_pb";

export interface PageData {
  [key: string]: any;
}

const API_BASE = import.meta.env.VITE_API_URL?.trim() || "";

/**
 * Fetch a protobuf Page from the server.
 */
export default async function getPageFromCPP(opts?: {
  pageId?: string;
  username?: string;
  endpoint?: string;
  withCredentials?: boolean;
}): Promise<PageData | null> {
  try {
    const defaultEndpoint = "/api";
    const endpoint = opts?.endpoint ?? defaultEndpoint;
    const fullUrl = API_BASE
      ? `${API_BASE.replace(/\/+$/, '')}${endpoint.replace(/^\/api/, '')}`
      : endpoint;

    const url = new URL(fullUrl, window.location.origin);
    if (opts?.pageId) url.searchParams.set("id", opts.pageId);
    if (opts?.username) url.searchParams.set("username", opts.username);

    console.log("▶️ GET", url.toString(), "(expecting protobuf Page)");

    const res = await fetch(url.toString(), {
      method: "GET",
      headers: { Accept: "application/x-protobuf" },
      cache: "no-store",
      credentials: opts?.withCredentials === false ? "omit" : "include",
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
