import React, { useEffect, useState } from "react";
import getInfo from "./functions/getInfo.js";
import { PageRenderer } from "./functions/renderPage.jsx";

export default function Renderer() {
  const [data, setData] = useState(null); // changed from "page"

  useEffect(() => {
    async function fetchData() {
      try {
        const info = await getInfo("/api/data");
        if (!info) {
          console.error("Renderer: No data returned from getInfo");
          return;
        }
        setData(info);
      } catch (err) {
        console.error("Renderer: Failed to load data", err);
      }
    }
    fetchData();
  }, []);

  // Use PageRenderer component instead of calling renderPage as a function
  return data ? <PageRenderer page={data} /> : <div>Loading...</div>;
}
