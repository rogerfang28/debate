// app.ts
import dotenv from "dotenv";
import express, { Express } from "express";
import mongoose from "mongoose";
import http from "http";
import { Server } from "socket.io";
import { connectDB } from "./server/lib/db.js";
import cors from "cors";
import { NodeModel } from "./server/models/Node.js";
import { EdgeModel } from "./server/models/Edge.js";
import registerSockets from "./server/socket/index.js";
import authRoutes from "./server/routes/auth.routes.js";
import roomRoutes from "./server/routes/room.routes.js";
import friendRoutes from "./server/routes/friend.routes.js";
import { authenticateToken } from "./server/middleware/auth.middleware.js";
import configureCors from "./server/configs/cors.js";
import createSocketServer from "./server/configs/socket.js";
import dataRoutes from "./server/routes/data.routes.js";
import eventRoutes from "./server/routes/events.routes.js";
import session from "express-session";

if (process.env.NODE_ENV !== "production") {
  dotenv.config(); // ✅ Only use .env in development
}

await connectDB();

const app: Express = express();
const server = http.createServer(app);

const io = createSocketServer(server);

// ✅ CORS must allow credentials for sessions to work
app.use(
  cors({
    origin: "http://localhost:5173", // your frontend
    credentials: true
  })
);

// ✅ Session middleware must come BEFORE routes
app.use(
  session({
    secret: process.env.SESSION_SECRET || "super-secret-key",
    resave: false,
    saveUninitialized: true,
    cookie: {
      secure: false, // true if HTTPS
      httpOnly: true,
      sameSite: "lax"
    }
  })
);

app.use(express.json());

// Your routes (AFTER session middleware)
app.use("/api/auth", authRoutes);
app.use("/api/rooms", roomRoutes);
app.use("/api/friends", friendRoutes);
app.use("/api/data", dataRoutes);
app.use("/api/events", eventRoutes);

// WebSockets
registerSockets(io);

const PORT = Number(process.env.PORT) || 3000;
server.listen(PORT, '0.0.0.0', () => console.log(`Backend running on port ${PORT}`));
