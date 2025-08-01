import { API_BASE } from '../../lib/config.js'

export default async function sendDataToBackend() {
  try {
    const res = await fetch(`${API_BASE}/data`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ message: "Hello from frontend!" })
    });
    const data = await res.text();
    console.log("Server says:", data);
  } catch (err) {
    console.error("Error sending data:", err);
  }
}
