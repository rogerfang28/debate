import express from "express";
import { PageSchema as Page, ComponentType } from "../../../src/gen/page_pb.js";
import homePage from "../virtualRenderer/pages/homePage.js";
import roomPage from "../virtualRenderer/pages/roomPage.js";
import profilePage from "../virtualRenderer/pages/profilePage.js";
import publicDebatesPage from "../virtualRenderer/pages/publicDebatesPage.js";

const router = express.Router();

/**
 * GET: Send Home Page protobuf
 */
router.get("/", (req, res) => {
  try {
    // Get JS object from separate file
    const pageData = req.session.currentPage || homePage();

    // Create protobuf message instance
    const pageMessage = new Page();

    // TODO: Set fields on the message (this is where you map JS object → protobuf fields)
    // Example: pageMessage.setTitle(pageData.title);
    // You must do this for all relevant fields in your Page proto

    // Serialize to binary
    const bytes = pageMessage.serializeBinary();

    // Send with correct protobuf Content-Type
    res.setHeader("Content-Type", "application/x-protobuf");
    res.send(Buffer.from(bytes));

  } catch (err) {
    console.error("❌ Failed to encode Page protobuf:", err);
    res.status(500).send("Failed to encode protobuf");
  }
});

/**
 * POST: Receive Page protobuf
 */
router.post("/", express.raw({ type: "application/x-protobuf" }), (req, res) => {
  try {
    // Deserialize from binary
    const receivedPage = Page.deserializeBinary(new Uint8Array(req.body));

    // Now receivedPage is a protobuf message instance
    console.log("📩 Received Page:", receivedPage.toObject()); // Convert to plain JS object

    res.send("✅ Page received and decoded!");
  } catch (err) {
    console.error("❌ Failed to decode Page protobuf:", err);
    res.status(400).send("Invalid Protobuf payload");
  }
});

export default router;
