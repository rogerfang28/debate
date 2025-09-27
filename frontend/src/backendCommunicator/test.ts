export default async function test() {
  const url = new URL("http://127.0.0.1:8080/");

  console.log("▶️ GET", url.toString(), "(expecting text)");

  const res = await fetch(url.toString(), {
    method: "GET",
    headers: { Accept: "text/plain" },
    cache: "no-store",
  });

  if (!res.ok) {
    const errText = await res.text().catch(() => "");
    throw new Error(`HTTP ${res.status} ${res.statusText}: ${errText}`);
  }

  console.log(res);
  return;
}
