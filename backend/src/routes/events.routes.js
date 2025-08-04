// routes/events.route.js
import express from 'express';
// Adjust the path to where your event_pb.js is actually located
import { UIEventSchema } from '../../../protos/event_pb.js';
import { create, fromBinary, toJson } from '@bufbuild/protobuf';

const router = express.Router();

// POST /api/events
router.post(
  '/',
  express.raw({ type: 'application/octet-stream' }),
  (req, res) => {
    try {
      // Decode Protobuf binary into a UIEvent message
      const message = fromBinary(UIEventSchema, req.body);

      // Convert to plain JSON for logging
      const eventObject = toJson(UIEventSchema, message, { emitDefaultValues: true });

      console.log('📩 Received UI Event:', eventObject);

      res.status(200).send('Event received');
    } catch (error) {
      console.error('❌ Failed to decode event:', error);
      res.status(400).send('Invalid event');
    }
  }
);

export default router;
