// VR logic on backend
function getUISpecForPath(path, query) {
  if (path.startsWith("/graph/")) {
    const roomId = path.split("/")[2];
    return makeGraphScreen(roomId); // returns JSON spec
  }
  if (path.startsWith("/profile/")) {
    const username = path.split("/")[2];
    return makeProfileScreen(username);
  }
  return makeHomeScreen();
}

wsServer.on("connection", (socket) => {
  socket.on("message", (raw) => {
    const event = JSON.parse(raw);
    if (event.type === "urlChanged") {
      const spec = getUISpecForPath(event.path, event.query);
      socket.send(JSON.stringify(spec));
    }
    if (event.type === "click") {
      // Handle click → maybe send new UI spec
    }
  });
});
