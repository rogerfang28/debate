import dotenv from 'dotenv';
import express from 'express';
import http from 'http';
import { Server } from 'socket.io';
import graphRoutes from './routes/graph.routes.js';
import graphStore from './graph.store.js';
import cors from 'cors';

dotenv.config();

const app = express();
const server = http.createServer(app);
const io = new Server(server, { cors: { origin: '*' } });

app.use(cors({ origin: 'http://localhost:5173' }));
app.use(express.json());
app.use('/api/graph', graphRoutes);

// Serve current graph state
app.get('/api/graph', (req, res) => res.json(graphStore));

// --- Socket.io logic ---
io.on('connection', socket => {
  console.log('Client connected:', socket.id);

  socket.onAny((event, ...args) => {
    console.log(`Received socket event: ${event}`, args);
  });

  // ✅ Update node position
  socket.on('update-node-position', ({ id, x, y }) => {
    if (graphStore.nodes[id]) {
      graphStore.nodes[id].x = x;
      graphStore.nodes[id].y = y;
      console.log(`Updated position for ${id}: (${x}, ${y})`);
      io.emit('graph-updated', graphStore);
    }
  });

  // ✅ Add new node
  socket.on('add-node', node => {
    if (typeof node.label !== 'string') {
      console.warn('Rejected node: label must be string', node);
      return;
    }

    graphStore.nodes[node.id] = {
      label: node.label,
      metadata: node.metadata || {},
      x: typeof node.x === 'number' ? node.x : 0,
      y: typeof node.y === 'number' ? node.y : 0
    };

    console.log('Added node with ID:', node.id);
    io.emit('graph-updated', graphStore);
  });

  // ✅ Delete node
  socket.on('delete-node', ({ id }) => {
    if (graphStore.nodes[id]) {
      delete graphStore.nodes[id];
      graphStore.edges = graphStore.edges.filter(e => e.source !== id && e.target !== id);
      console.log('Deleted node:', id);
      io.emit('graph-updated', graphStore);
    }
  });

  // ✅ Add edge
  socket.on('add-edge', edge => {
    const { source, target } = edge;
    if (graphStore.nodes[source] && graphStore.nodes[target]) {
      graphStore.edges.push({
        source,
        target,
        relation: edge.relation || null,
        metadata: edge.metadata || {}
      });
      console.log(`Added edge: ${source} -> ${target}`);
      io.emit('graph-updated', graphStore);
    } else {
      console.warn('Rejected edge: source or target not found', edge);
    }
  });

  // ✅ Delete edge
  socket.on('delete-edge', criteria => {
    const { source, target, relation } = criteria;

    graphStore.edges = graphStore.edges.filter(e =>
      !(e.source === source && e.target === target && e.relation === relation)
    );

    console.log(`Deleted edge: ${source} -> ${target}`);
    io.emit('graph-updated', graphStore);
  });

  socket.on('disconnect', () => {
    console.log('Client disconnected:', socket.id);
  });
});

const PORT = process.env.PORT;
server.listen(PORT, () => console.log(`Backend running on port ${PORT}`));
