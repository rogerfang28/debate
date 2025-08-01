import dotenv from 'dotenv';
import express from 'express';
import mongoose from 'mongoose';
import http from 'http';
import { Server } from 'socket.io';
import { connectDB } from './lib/db.js';
import cors from 'cors';
import { NodeModel } from './models/Node.js';
import { EdgeModel } from './models/Edge.js';
import registerSockets from './socket/index.js';
import authRoutes from "./routes/auth.routes.js";
import roomRoutes from "./routes/room.routes.js";
import friendRoutes from "./routes/friend.routes.js";
import { authenticateToken } from './middleware/auth.middleware.js';
import configureCors from './configs/cors.js';
import createSocketServer from './configs/socket.js';
import dataRoutes from './routes/data.routes.js';
// import importExportRouter from './routes/importExport.routes.js';

// app.use('/api/rooms', importExportRouter);   // http://…/api/rooms/:roomId/export/nodes

if (process.env.NODE_ENV !== 'production') {
  dotenv.config(); // ✅ Only use .env in development
}
await connectDB();

const app = express();
const server = http.createServer(app);

const io = createSocketServer(server);
app.use(configureCors());

app.use(express.json());
app.use("/api/auth", authRoutes);
app.use("/api/rooms", roomRoutes);
app.use("/api/friends", friendRoutes);
app.use("/api/data", dataRoutes);

// // registerSocketHandlers(io);
registerSockets(io);

const PORT = process.env.PORT;
server.listen(PORT, () => console.log(`Backend running on port ${PORT}`));
