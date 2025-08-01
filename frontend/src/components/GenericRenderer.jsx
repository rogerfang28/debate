// src/components/GenericRenderer.jsx
import React, { useEffect, useState } from "react";
import { useLocation } from "react-router-dom";
import { sendToVR, subscribeToVR } from "../lib/virtualRendererClient";

export default function GenericRenderer() {
  const location = useLocation(); // gives pathname, search, hash
  const [uiSpec, setUiSpec] = useState(null);

  // Tell VR whenever the URL changes
  useEffect(() => {
    sendToVR({
      type: "urlChanged",
      path: location.pathname,
      query: location.search
    });
  }, [location]);

  // Subscribe to UI spec updates from VR
  useEffect(() => {
    const unsubscribe = subscribeToVR((newSpec) => {
      setUiSpec(newSpec);
    });
    return unsubscribe;
  }, []);

  if (!uiSpec) return <div>Loading...</div>;

  return renderNode(uiSpec);
}

function renderNode(node) {
  // This is your UI spec renderer
  switch (node.type) {
    case "Text":
      return <p>{node.text}</p>;
    case "Button":
      return <button onClick={() => sendToVR({ type: "click", id: node.id })}>
        {node.label}
      </button>;
    // Add more types like Graph, List, Form, etc.
    default:
      return <div>Unknown node type: {node.type}</div>;
  }
}
