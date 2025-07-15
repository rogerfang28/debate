// app.js
const express = require('express');
const http = require('http');
const { Server } = require('socket.io');

const graphRoutes = require('./routes/graph.routes');
const graphStore = require('./graph.store');

const app = express();
const server = http.createServer(app);
const io = new Server(server, { cors: { origin: '*' } });

app.use(express.json());
app.use('/api/graph', graphRoutes);

// Send full graph on request
app.get('/api/graph', (req, res) => res.json(graphStore));

// Handle socket connections
io.on('connection', socket => {
  console.log('Client connected:', socket.id);

  socket.on('add-node', node => {
    graphStore.nodes[node.id] = { label: node.label, metadata: node.metadata };
    io.emit('graph-updated', graphStore);
  });

  socket.on('delete-node', ({ id }) => {
    delete graphStore.nodes[id];
    graphStore.edges = graphStore.edges.filter(e => e.source !== id && e.target !== id);
    io.emit('graph-updated', graphStore);
  });

  socket.on('add-edge', edge => {
    graphStore.edges.push(edge);
    io.emit('graph-updated', graphStore);
  });

  socket.on('delete-edge', criteria => {
    graphStore.edges = graphStore.edges.filter(e =>
      !(e.source === criteria.source && e.target === criteria.target && e.relation === criteria.relation)
    );
    io.emit('graph-updated', graphStore);
  });

  socket.on('disconnect', () => console.log('Client disconnected:', socket.id));
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => console.log(`Backend running on port ${PORT}`));
