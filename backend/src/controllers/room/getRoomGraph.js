import { RoomModel } from '../../models/Room.js';
import { NodeModel } from '../../models/Node.js';
import { EdgeModel } from '../../models/Edge.js';
import { ChallengeModel } from '../../models/Challenge.js';
import mongoose from 'mongoose';

/**
 * Get room's graph data (nodes and edges)
 */
export async function getRoomGraph(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (!room.hasMember(userId)) {
      return res.status(403).json({ message: 'Access denied - not a room member' });
    }

    const [nodes, edges, challenges] = await Promise.all([
      NodeModel.find({ room: roomId }).populate('createdBy', 'email'),
      EdgeModel.find({ room: roomId }).populate('createdBy', 'email'),
      ChallengeModel.find({ room: roomId })
        .populate('challenger', 'email username')
        .populate('responder', 'email username')
    ]);

    res.json({
      nodes,
      edges,
      challenges,
      room: {
        id: room._id,
        name: room.name,
        description: room.description
      }
    });
  } catch (error) {
    console.error('Get room graph error:', error);
    res.status(500).json({ message: 'Failed to fetch room graph' });
  }
}
