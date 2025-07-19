import express from 'express';
import { addNode, deleteNode, addEdge, deleteEdge } from './graph.controller.js';

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

export default router;