import express from "express";
import { create, toBinary, fromBinary } from "@bufbuild/protobuf";
import { PageSchema } from "../../../src/gen/page_pb.js";
import testPage from "../virtualRenderer/pages/testPage.js";
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
    const pageData = req.session.currentPage || homePage();

    // Create protobuf message from plain JS object
    const pageMessage = create(PageSchema, pageData);

    // Serialize to binary
    const bytes = toBinary(PageSchema, pageMessage);

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

    console.log("📩 Received Page:", receivedPage);

    res.send("✅ Page received and decoded!");
  } catch (err) {
    console.error("❌ Failed to decode Page protobuf:", err);
    res.status(400).send("Invalid Protobuf payload");
  }
});

export default router;
