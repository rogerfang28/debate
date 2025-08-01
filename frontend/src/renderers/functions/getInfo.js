import { API_BASE } from '../../lib/config.js'

export default async function getInfo() {
  try {
    const res = await fetch(`${API_BASE}/data`, {
      method: "GET",
      headers: { "Content-Type": "application/json" }
    });
    if (!res.ok) throw new Error(`HTTP error! Status: ${res.status}`);
    const data = await res.json();
    console.log("Data from server:", data);
    return data;
  } catch (err) {
    console.error("Error fetching data:", err);
    return null;
  }
}
