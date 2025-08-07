import express from "express";
import { fromBinary } from "@bufbuild/protobuf";
import { UIEventSchema } from "../../../src/gen/event_pb.js";
import homePage from "../virtualRenderer/pages/homePage.js";
import roomPage from "../virtualRenderer/pages/roomPage.js";
import profilePage from "../virtualRenderer/pages/profilePage.js";
import publicDebatesPage from "../virtualRenderer/pages/publicDebatesPage.js";

const router = express.Router();

function setCurrentPage(req, pageFn) {
  if (!req.session) {
    console.error("❌ Session is undefined! Cannot set page.");
    return;
  }
  if (typeof pageFn !== "function") {
    console.error("❌ Page function is invalid:", pageFn);
    return;
  }

  try {
    const page = pageFn();
    req.session.currentPage = page;
    console.log(`✅ Current page set to: ${page.pageId || "[unknown ID]"}`);
  } catch (err) {
    console.error("❌ Error creating page object:", err);
  }
}

router.post("/", express.raw({ type: "application/octet-stream" }), (req, res) => {
  try {
    // 1️⃣ Decode Protobuf binary into a UIEvent message
    const message = fromBinary(UIEventSchema, new Uint8Array(req.body));

    console.log("📩 Received UI Event:", message);

    // 2️⃣ Extract the actionId from the event
    const actionId = message.data?.actionId;

    console.log("🔍 Parsed actionId:", actionId);

    // 3️⃣ Switch page based on actionId
    switch (actionId) {
      case "goHome":
        setCurrentPage(req, homePage);
        break;
      case "goProfile":
        setCurrentPage(req, profilePage);
        break;
      case "goRoom":
        setCurrentPage(req, roomPage);
        break;
      case "goPublicDebates":
        setCurrentPage(req, publicDebatesPage);
        break;
      default:
        console.warn("ℹ No page switch for actionId:", actionId);
    }

    res.status(200).send("Event processed");
  } catch (error) {
    console.error("❌ Failed to decode event:", error);
    res.status(400).send("Invalid event");
  }
});

export default router;
