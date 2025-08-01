// Renderer.jsx
import React, { useEffect } from "react";
import getInfo from "./functions/getInfo";
import sendData from "./functions/sendData";
import renderPage from "./functions/renderPage";

export default function Renderer() {
  useEffect(() => {
    async function fetchData() {
      try {
        // Get JSON UI definition from backend
        const data = await getInfo();

        if (!data || typeof data !== "object") {
          console.error("Renderer: No valid data from backend", data);
          return;
        }

        // Pass JSON to renderer
        renderPage(data);
      } catch (error) {
        console.error("Renderer: Failed to fetch and render page", error);
      }
    }

    fetchData();
  }, []);

  return null; // All rendering handled by renderPage
}
