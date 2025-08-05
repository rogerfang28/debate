import React, { useEffect, useState } from "react";
import getInfo from "./functions/getInfo.js";
import { PageRenderer } from "./functions/rendering/PageRenderer.jsx";

export default function Renderer() {
  const [data, setData] = useState(null);
  const [loading, setLoading] = useState(false);

  // 🔹 Global reload function for handleEvent.js
  window.reloadPage = async () => {
    try {
      setLoading(true); // show spinner
      const info = await getInfo("/api/data");
      if (info) {
        setData(info);
      } else {
        console.warn("reloadPage: No data returned from getInfo");
      }
    } catch (err) {
      console.error("reloadPage: Failed to load data", err);
    } finally {
      setLoading(false); // hide spinner
    }
  };

  useEffect(() => {
    let intervalId;

    async function fetchData() {
      try {
        setLoading(true);
        const info = await getInfo("/api/data");
        if (info) {
          setData(info);
          if (intervalId) clearInterval(intervalId); // stop retry loop
        } else {
          console.warn("Renderer: No data returned from getInfo");
        }
      } catch (err) {
        console.warn("Renderer: Failed to load data, retrying...", err);
      } finally {
        setLoading(false);
      }
    }

    fetchData(); // first try
    intervalId = setInterval(fetchData, 3000); // retry every 3s until successful

    return () => clearInterval(intervalId);
  }, []);

  // 🔹 Loading spinner (Tailwind styled)
  if (loading && !data) {
    return (
      <div className="flex flex-col items-center justify-center h-screen text-gray-300">
        <div className="animate-spin rounded-full h-12 w-12 border-t-2 border-b-2 border-blue-400"></div>
        <p className="mt-4">Loading page...</p>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gray-900 text-white">
      {loading && (
        <div className="absolute top-4 right-4 bg-gray-800 px-4 py-2 rounded shadow text-sm text-gray-200">
          Loading new page...
        </div>
      )}
      {data ? <PageRenderer page={data} /> : <div className="p-6">Loading...</div>}
    </div>
  );
}
