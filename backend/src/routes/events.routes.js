// routes/events.routes.js
import express from "express";
import { UIEventSchema } from "../../../protos/event_pb.js";
import { fromBinary, toJson } from "@bufbuild/protobuf";

import homePage from "../virtualRenderer/pages/homePage.js";
import roomPage from "../virtualRenderer/pages/roomPage.js";
import profilePage from "../virtualRenderer/pages/profilePage.js";
import publicDebatesPage from "../virtualRenderer/pages/publicDebatesPage.js";

const router = express.Router();

/**
 * Safely set the current page in the session.
 */
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

router.post(
  "/",
  express.raw({ type: "application/octet-stream" }),
  (req, res) => {
    try {
      // 1️⃣ Decode Protobuf binary into a UIEvent message
      const message = fromBinary(UIEventSchema, req.body);

      // 2️⃣ Convert to plain JSON for logging
      const eventObject = toJson(UIEventSchema, message, {
        emitDefaultValues: true,
      });

      console.log("📩 Received UI Event:", eventObject);

      // 3️⃣ Extract the actionId from the event
      const actionId = eventObject.data?.actionId;
      console.log("🔍 Parsed actionId:", actionId);

      // 4️⃣ Switch page based on actionId
      switch (actionId) {
        case "goHome":
          console.log("➡ Switching to homePage");
          setCurrentPage(req, homePage);
          break;
        case "goProfile":
          console.log("➡ Switching to profilePage");
          setCurrentPage(req, profilePage);
          break;
        case "goRoom":
          console.log("➡ Switching to roomPage");
          setCurrentPage(req, roomPage);
          break;
        case "goPublicDebates":
          console.log("➡ Switching to publicDebatesPage");
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
  }
);

export default router;
