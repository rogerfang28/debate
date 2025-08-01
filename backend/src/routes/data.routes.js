import express from "express";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";

const router = express.Router();

// Resolve __dirname in ESM
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Load JSON file synchronously
const homeDataPath = path.join(__dirname, "../virtualRenderer/pages/home.json");
const homeData = JSON.parse(fs.readFileSync(homeDataPath, "utf8"));

router.get("/", (req, res) => {
  res.json(homeData);
});

export default router;
