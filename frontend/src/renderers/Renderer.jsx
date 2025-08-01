import React, { useEffect, useState } from "react";
import getInfo from "./functions/getInfo.js"; // make sure path is correct
import renderPage from "./functions/renderPage.jsx";

export default function Renderer() {
  const [page, setPage] = useState(null);

  useEffect(() => {
    async function fetchData() {
      try {
        // Get JSON UI from backend via getInfo helper
        const data = await getInfo("/api/data");

        if (!data) {
          console.error("Renderer: No data returned from getInfo");
          return;
        }

        setPage(data);
      } catch (err) {
        console.error("Renderer: Failed to load page", err);
      }
    }
    fetchData();
  }, []);

  // Render JSON-driven UI
  return page ? renderPage(page) : null;
}
