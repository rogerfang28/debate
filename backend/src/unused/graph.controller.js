import graph from './graph.store.js';

export function addNode({ id, label, metadata = {} }) {
  graph.nodes[id] = { label, metadata };
}

export function deleteNode(id) {
  delete graph.nodes[id];
  graph.edges = graph.edges.filter(e => e.source !== id && e.target !== id);
}

export function addEdge({ source, target, relation, metadata = {} }) {
  graph.edges.push({ source, target, relation, metadata });
}

export function deleteEdge(criteria) {
  graph.edges = graph.edges.filter(e =>
    !(e.source === criteria.source && e.target === criteria.target && e.relation === criteria.relation)
  );
}
