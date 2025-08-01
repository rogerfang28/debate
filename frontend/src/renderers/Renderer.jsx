// this will need to use a function that takes in a json format and renders it as a react page
import React, { useEffect } from "react";
import getInfo from './functions/getInfo'
import sendData from './functions/sendData'
import renderPage from './functions/renderPage'

export default function Renderer() {
  useEffect(() => {
    async function fetchData() {
      const data = await getInfo();
      renderPage(data);
    }
    fetchData();
  }, []);

  return null; // nothing to directly render
}