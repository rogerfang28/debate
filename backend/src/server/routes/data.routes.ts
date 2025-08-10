import express from "express";
import { create, toBinary } from "@bufbuild/protobuf";
import { PageSchema } from "../../../../src/gen/page_pb.js";
import getPage from "../../virtualRenderer/pages/getPage.ts";

const router = express.Router();

router.get("/", async (req, res) => {
  try {
    const pageData = await getPage(req); // <— all selection logic lives here
    
    const pageMessage = create(PageSchema, pageData);
    const bytes = toBinary(PageSchema, pageMessage);

    res.type("application/x-protobuf").send(Buffer.from(bytes));
  } catch (err) {
    console.error("❌ Failed to encode Page protobuf:", err);
    res.status(500).send("Failed to encode protobuf");
  }
});

export default router;
