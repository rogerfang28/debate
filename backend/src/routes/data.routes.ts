import express from "express";
import { create, toBinary } from "@bufbuild/protobuf";
import { PageSchema } from "../../../src/gen/page_pb.js";
import getCurrentPage from "../virtualRenderer/pages/getCurrentPage.ts";
import { authenticateToken } from "../middleware/auth.middleware.js";

const router = express.Router();

// Add authentication middleware to protect the data route
router.use(authenticateToken);

router.get("/", (req, res) => {
  try {
    const pageData = getCurrentPage(req); // <— all selection logic lives here
    const pageMessage = create(PageSchema, pageData);
    const bytes = toBinary(PageSchema, pageMessage);

    res.type("application/x-protobuf").send(Buffer.from(bytes));
  } catch (err) {
    console.error("❌ Failed to encode Page protobuf:", err);
    res.status(500).send("Failed to encode protobuf");
  }
});

export default router;
