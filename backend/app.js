const express = require('express');
const graphRoutes = require('./graph.routes');
const graphStore = require('./graph.store');
const app = express();

app.use(express.json());
app.use('/api/graph', graphRoutes);

app.get('/api/graph', (req, res) => res.json(graphStore));

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => console.log(`Server listening on port ${PORT}`));
