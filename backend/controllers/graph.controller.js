const graph = require('./graph.store');

function addNode({ id, label, metadata = {} }) {
  graph.nodes[id] = { label, metadata };
}

function deleteNode(id) {
  delete graph.nodes[id];
  graph.edges = graph.edges.filter(e => e.source !== id && e.target !== id);
}

function addEdge({ source, target, relation, metadata = {} }) {
  graph.edges.push({ source, target, relation, metadata });
}

function deleteEdge(criteria) {
  graph.edges = graph.edges.filter(e =>
    !(e.source === criteria.source && e.target === criteria.target && e.relation === criteria.relation)
  );
}

module.exports = { addNode, deleteNode, addEdge, deleteEdge };
