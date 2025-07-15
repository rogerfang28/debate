const express = require('express');
const { addNode, deleteNode, addEdge, deleteEdge } = require('../controllers/graph.controllers');
const router = express.Router();

router.post('/node', (req, res) => {
  addNode(req.body);
  res.json({ success: true });
});

router.delete('/node/:id', (req, res) => {
  deleteNode(req.params.id);
  res.json({ success: true });
});

router.post('/edge', (req, res) => {
  addEdge(req.body);
  res.json({ success: true });
});

router.delete('/edge', (req, res) => {
  deleteEdge(req.body);
  res.json({ success: true });
});

module.exports = router;
