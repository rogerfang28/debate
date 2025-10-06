import express from "express";
import { fromBinary } from "@bufbuild/protobuf";
import { UIEventSchema } from "../../../src/gen/js/event_pb.js";
import handleEvent from "../main/eventHandler.ts"

const router = express.Router();

router.post("/", express.raw({ type: "application/octet-stream" }), (req, res) => {
  try {
    // 1️⃣ Decode Protobuf binary into a UIEvent message
    const message = fromBinary(UIEventSchema, new Uint8Array(req.body));

    // 2️⃣ Extract the actionId - try new field first, fallback to legacy
    const actionId = message.actionId || message.data?.actionId;
    
    // 3️⃣ Convert event_data array to convenient object for handlers
    const eventData: Record<string, any> = {};
    
    if (message.eventData) {
      message.eventData.forEach(item => {
        const key = item.key;
        const value = item.value;
        
        // Handle the new oneOf format
        if (value?.value?.case === "textValue") {
          eventData[key] = value.value.value;
        } else if (value?.value?.case === "numberValue") {
          eventData[key] = Number(value.value.value);
        } else if (value?.value?.case === "booleanValue") {
          eventData[key] = value.value.value;
        } else if (value?.value?.case === "decimalValue") {
          eventData[key] = value.value.value;
        } else if (value?.textValue !== undefined) {
          // Fallback to old format
          eventData[key] = value.textValue;
        } else if (value?.numberValue !== undefined) {
          eventData[key] = Number(value.numberValue);
        } else if (value?.booleanValue !== undefined) {
          eventData[key] = value.booleanValue;
        } else if (value?.decimalValue !== undefined) {
          eventData[key] = value.decimalValue;
        } else if (value?.listValue !== undefined) {
          eventData[key] = value.listValue.values;
        }
      });
    }

    // 4️⃣ Handle the event using the event handler
    if (actionId) {
      // Pass both the request and the parsed event data
      handleEvent(req, actionId, eventData);
    }

    res.status(200).send("Event processed");
  } catch (error) {
    res.status(400).send("Invalid event");
  }
});

export default router;