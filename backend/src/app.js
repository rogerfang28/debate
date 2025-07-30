import dotenv from 'dotenv';
import express from 'express';
import mongoose from 'mongoose';
import http from 'http';
import { Server } from 'socket.io';
import { connectDB } from './lib/db.js';
import cors from 'cors';
import { NodeModel } from './models/Node.js';
import { EdgeModel } from './models/Edge.js';
import registerSockets from './socket/index.js'; // * new import
// import { registerSocketHandlers } from './socket/socketHandler.js'; old import, replaced with new import below
import authRoutes from "./routes/auth.routes.js";
import roomRoutes from "./routes/room.routes.js";
import friendRoutes from "./routes/friend.routes.js";
import { authenticateToken } from './middleware/auth.middleware.js';
// import importExportRouter from './routes/importExport.routes.js';

// app.use('/api/rooms', importExportRouter);   // http://…/api/rooms/:roomId/export/nodes

if (process.env.NODE_ENV !== 'production') {
  dotenv.config(); // ✅ Only use .env in development
}
await connectDB();

const app = express();
const server = http.createServer(app);

const allowedOrigins = [
  'http://localhost:5173',
  'https://debate-frontend.onrender.com'
];

const io = new Server(server, { 
  cors: { 
    origin: allowedOrigins,
    methods: ["GET", "POST"],
    credentials: true
  } 
});

app.use(
  cors({
    origin: function (origin, callback) {
      if (!origin || allowedOrigins.includes(origin)) {
        callback(null, true);
      } else {
        callback(new Error('Not allowed by CORS'));
      }
    },
    credentials: true
  })
);

app.use(express.json());
app.use("/api/auth", authRoutes);
app.use("/api/rooms", roomRoutes);
app.use("/api/friends", friendRoutes);

// // registerSocketHandlers(io);
registerSockets(io);

const PORT = process.env.PORT;
server.listen(PORT, () => console.log(`Backend running on port ${PORT}`));
