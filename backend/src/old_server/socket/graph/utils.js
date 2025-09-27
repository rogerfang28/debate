import { NodeModel } from '../../models/Node.js';
import { EdgeModel } from '../../models/Edge.js';
import { ChallengeModel } from '../../models/Challenge.js';

export async function emitFullGraph(io) {
  const nodes = await NodeModel.find();
  const edges = await EdgeModel.find();
  const challenges = await ChallengeModel.find();

  const nodeMap = {};
  for (const n of nodes) {
    nodeMap[n.id] = {
      id: n.id,
      label: n.label,
      metadata: n.metadata,
      x: n.x,
      y: n.y
    };
  }

  io.emit('graph-updated', { nodes: nodeMap, edges, challenges });
}

export async function emitRoomGraph(io, roomId) {
  try {
    const [nodes, edges, challenges] = await Promise.all([
      NodeModel.find({ room: roomId }).populate('createdBy', 'email'),
      EdgeModel.find({ room: roomId }).populate('createdBy', 'email'),
      ChallengeModel.find({ room: roomId })
        .populate('challenger', 'email username')
        .populate('responder', 'email username')
    ]);

    // Keep nodes as array to match the room API format
    const nodesWithCreator = nodes.map(node => ({
      _id: node._id,
      id: node.id,
      label: node.label,
      metadata: node.metadata,
      x: node.x,
      y: node.y,
      room: node.room,
      createdBy: node.createdBy?.email,
      createdAt: node.createdAt
    }));

    // Add creator info to edges
    const edgesWithCreator = edges.map(edge => ({
      ...edge.toObject(),
      createdBy: edge.createdBy?.email
    }));

    // Add creator info to challenges
    const challengesWithCreator = challenges.map(challenge => ({
      ...challenge.toObject(),
      challengerEmail: challenge.challenger?.email,
      challengerUsername: challenge.challenger?.username,
      responderEmail: challenge.responder?.email,
      responderUsername: challenge.responder?.username
    }));

    // Emit to all users in the room using the same event name as frontend expects
    io.to(roomId).emit('room-graph-updated', { 
      nodes: nodesWithCreator, 
      edges: edgesWithCreator, 
      challenges: challengesWithCreator,
      roomId 
    });

    console.log(`📊 Emitted room graph update for room: ${roomId} (${nodes.length} nodes, ${edges.length} edges)`);
  } catch (error) {
    console.error('❌ Failed to emit room graph:', error);
  }
}
