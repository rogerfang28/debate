import React, { useEffect, useState } from "react";
import getInfo from "./functions/getInfo.js";
import { PageRenderer } from "./functions/rendering/PageRenderer.jsx";

export default function Renderer() {
  const [data, setData] = useState(null);

  useEffect(() => {
    let intervalId;

    async function fetchData() {
      try {
        const info = await getInfo("/api/data");
        if (info) {
          setData(info);
          if (intervalId) clearInterval(intervalId); // stop retrying once connected
        } else {
          console.warn("Renderer: No data returned from getInfo");
        }
      } catch (err) {
        console.warn("Renderer: Failed to load data, retrying...", err);
      }
    }

    fetchData(); // initial try
    intervalId = setInterval(fetchData, 3000); // retry every 3 sec

    return () => clearInterval(intervalId); // cleanup on unmount
  }, []);

  return data ? <PageRenderer page={data} /> : <div>Loading...</div>;
}
