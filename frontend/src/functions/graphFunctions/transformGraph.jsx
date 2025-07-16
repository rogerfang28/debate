// basically just loads the graph
export function transformGraph(json) {
  const nodes = Object.entries(json.nodes || {}).map(([id, node]) => ({
    id,
    text: node.label,
    metadata: node.metadata || {},
    x: node.x,
    y: node.y
  }));

  const links = Array.isArray(json.edges)
    ? json.edges.map(edge => ({
        source: edge.source,
        target: edge.target,
        metadata: edge.metadata || {}
      }))
    : [];

  return { nodes, links };
}
