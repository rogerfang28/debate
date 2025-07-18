import dotenv from 'dotenv';
import express from 'express';
import mongoose from 'mongoose';
import http from 'http';
import { Server } from 'socket.io';
import { connectDB } from './lib/db.js';
import graphRoutes from './unused/graph.routes.js';
import cors from 'cors';
import { NodeModel } from './models/Node.js';
import { EdgeModel } from './models/Edge.js';
import { registerSocketHandlers } from './lib/socketHandler.js';

dotenv.config();
await connectDB();

const app = express();
const server = http.createServer(app);
const io = new Server(server, { cors: { origin: '*' } });

app.use(cors({ origin: 'http://localhost:5173' }));
app.use(express.json());
app.use('/api/graph', graphRoutes);

app.get('/api/graph', async (req, res) => {
  const nodes = await NodeModel.find();
  const edges = await EdgeModel.find();
  const nodeMap = {};
  for (const n of nodes) {
    nodeMap[n.id] = {
      id: n.id,
      //text: n.label,
      label: n.label,
      metadata: n.metadata,
      x: n.x,
      y: n.y
    };
  }
  res.json({ nodes: nodeMap, edges });
});

registerSocketHandlers(io);

const PORT = process.env.PORT;
server.listen(PORT, () => console.log(`Backend running on port ${PORT}`));
