// src/lib/virtualRendererClient.js
let subscribers = [];

export function subscribeToVR(callback) {
  subscribers.push(callback);
  return () => { subscribers = subscribers.filter(cb => cb !== callback); };
}

export function sendToVR(event) {
  socket.send(JSON.stringify(event)); // socket to backend VR service
}

// WebSocket connection to backend VR
const socket = new WebSocket("ws://localhost:3000/ui");

socket.onmessage = (msg) => {
  const spec = JSON.parse(msg.data);
  subscribers.forEach(cb => cb(spec));
};
