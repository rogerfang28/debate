import express from "express";
import { PageSchema } from "../../../protos/page_pb.js";
import { toBinary, fromBinary } from "@bufbuild/protobuf";
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
    // Get page object from separate file
    const page = homePage();

    // Encode to protobuf
    const bytes = toBinary(PageSchema, page);

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
    const receivedPage = fromBinary(PageSchema, new Uint8Array(req.body));
    console.log("📩 Received Page:", JSON.stringify(receivedPage, null, 2));
    res.send("✅ Page received and decoded!");
  } catch (err) {
    console.error("❌ Failed to decode Page protobuf:", err);
    res.status(400).send("Invalid Protobuf payload");
  }
});

export default router;
