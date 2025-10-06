// src/functions/testPing.ts
export async function testPing() {
  console.log("▶️ GET /ping");
  const ctrl = new AbortController();
  const to = setTimeout(() => ctrl.abort("timeout"), 8000);

  try {
    // Use 127.0.0.1 to force IPv4; no headers/body → no preflight
    const res = await fetch("http://127.0.0.1:8080/ping", {
      method: "GET",
      cache: "no-store",
      signal: ctrl.signal
    });
    clearTimeout(to);
    console.log("status:", res.status);
    const text = await res.text();
    console.log("body:", text); // expect "pong"
  } catch (e) {
    clearTimeout(to);
    console.error("❌ ping failed:", e);
  }
}

// Optional: POST test once ping works (kept here for convenience)
export async function testCPPBackend() {
  console.log("▶️ POST /test");
  const ctrl = new AbortController();
  const to = setTimeout(() => ctrl.abort("timeout"), 8000);

  try {
    const res = await fetch("http://127.0.0.1:8080/test", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ message: "Hello from frontend!" }),
      signal: ctrl.signal
    });
    clearTimeout(to);
    console.log("status:", res.status);
    const text = await res.text();
    console.log("body:", text); // expect {"reply":"Hello from C++ server!"}
  } catch (e) {
    clearTimeout(to);
    console.error("❌ POST failed:", e);
  }
}
