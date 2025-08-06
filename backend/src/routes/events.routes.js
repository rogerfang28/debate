import express from "express";
import { UIEvent } from "../../../protos/event_pb.js"; // Generated with protoc3.15.8
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
      const message = UIEvent.deserializeBinary(new Uint8Array(req.body));

      // 2️⃣ Convert to plain JS object for logging
      // `toObject` is the google-protobuf equivalent of Buf's toJson()
      const eventObject = message.toObject();

      console.log("📩 Received UI Event:", eventObject);

      // 3️⃣ Extract the actionId from the event
      const actionId = eventObject.data?.actionid; 
      // NOTE: google-protobuf lowercases field names in toObject()

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
