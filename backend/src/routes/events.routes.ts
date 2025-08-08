import express from "express";
import { fromBinary } from "@bufbuild/protobuf";
import { UIEventSchema } from "../../../src/gen/event_pb.js";
import handleEvent from "../eventHandlers/eventHandler.ts";

const router = express.Router();

router.post("/", express.raw({ type: "application/octet-stream" }), (req, res) => {
  try {
    // 1️⃣ Decode Protobuf binary into a UIEvent message
    const message = fromBinary(UIEventSchema, new Uint8Array(req.body));

    console.log("📩 Received UI Event:", message);

    // 2️⃣ Extract the actionId from the event
    const actionId = message.data?.actionId;

    console.log("🔍 Parsed actionId:", actionId);

    // 3️⃣ Handle the event using the event handler
    if (actionId) {
      handleEvent(req, actionId);
    } else {
      console.warn("ℹ No actionId found in event");
    }

    res.status(200).send("Event processed");
  } catch (error) {
    console.error("❌ Failed to decode event:", error);
    res.status(400).send("Invalid event");
  }
});

export default router;