// event-esm-wrapper.mjs
import { createRequire } from 'module';
const require = createRequire(import.meta.url);

// Load the CommonJS protobuf output
const pkg = require('../unused/event_pb.js');

// Export the specific protobuf classes & enums you need
export const { UIEvent, EventType } = pkg;
