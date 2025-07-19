// Transform graph data from MongoDB format to ForceGraph format
export function transformGraph(json) {
  // Handle new MongoDB format (nodes/edges as arrays)
  if (Array.isArray(json.nodes) && Array.isArray(json.edges)) {
    const challenges = json.challenges || [];
    
    // Create maps for quick challenge lookup
    const nodeChallenges = new Map();
    const edgeChallenges = new Map();
    
    challenges.forEach(challenge => {
      if (challenge.targetType === 'node') {
        if (!nodeChallenges.has(challenge.targetId)) {
          nodeChallenges.set(challenge.targetId, []);
        }
        nodeChallenges.get(challenge.targetId).push(challenge);
      } else if (challenge.targetType === 'edge') {
        if (!edgeChallenges.has(challenge.targetId)) {
          edgeChallenges.set(challenge.targetId, []);
        }
        edgeChallenges.get(challenge.targetId).push(challenge);
      }
    });

    const nodes = json.nodes.map(node => ({
      id: node.id,          // Use the custom id field, not MongoDB _id
      _id: node._id,        // Keep _id for reference if needed
      label: node.label,
      metadata: node.metadata || {},
      x: node.x,
      y: node.y,
      challenges: nodeChallenges.get(node.id) || [],
      hasPendingChallenge: (nodeChallenges.get(node.id) || []).some(c => c.status === 'pending')
    }));

    const links = json.edges.map(edge => ({
      id: edge._id?.toString(), // Add edge ID for challenge targeting
      source: edge.source,
      target: edge.target,
      metadata: edge.metadata || {},
      challenges: edgeChallenges.get(edge._id?.toString()) || [],
      hasPendingChallenge: (edgeChallenges.get(edge._id?.toString()) || []).some(c => c.status === 'pending')
    }));

    return { nodes, links, challenges };
  }

  // Handle old format (nodes as object, edges as array) for backward compatibility
  const challenges = json.challenges || [];
  
  // Create maps for quick challenge lookup
  const nodeChallenges = new Map();
  const edgeChallenges = new Map();
  
  challenges.forEach(challenge => {
    if (challenge.targetType === 'node') {
      if (!nodeChallenges.has(challenge.targetId)) {
        nodeChallenges.set(challenge.targetId, []);
      }
      nodeChallenges.get(challenge.targetId).push(challenge);
    } else if (challenge.targetType === 'edge') {
      if (!edgeChallenges.has(challenge.targetId)) {
        edgeChallenges.set(challenge.targetId, []);
      }
      edgeChallenges.get(challenge.targetId).push(challenge);
    }
  });

  const nodes = Object.entries(json.nodes || {}).map(([id, node]) => ({
    id,
    label: node.label,
    metadata: node.metadata || {},
    x: node.x,
    y: node.y,
    challenges: nodeChallenges.get(id) || [],
    hasPendingChallenge: (nodeChallenges.get(id) || []).some(c => c.status === 'pending')
  }));

  const links = Array.isArray(json.edges)
    ? json.edges.map(edge => ({
        id: edge._id?.toString(), // Add edge ID for challenge targeting
        source: edge.source,
        target: edge.target,
        metadata: edge.metadata || {},
        challenges: edgeChallenges.get(edge._id?.toString()) || [],
        hasPendingChallenge: (edgeChallenges.get(edge._id?.toString()) || []).some(c => c.status === 'pending')
      }))
    : [];

  return { nodes, links, challenges };
}
