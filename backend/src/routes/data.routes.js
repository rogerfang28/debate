import express from "express";

const router = express.Router();

router.get("/", (req, res) => {
  res.json({
    status: "success",
    timestamp: new Date().toISOString(),
    payload: {
      title: "Welcome to Debate",
      description: "This data came from the backend.",
      items: [
        { id: 1, name: "Topic A" },
        { id: 2, name: "Topic B" }
      ]
    }
  });
});

export default router;
