import express from "express";
import fs from "fs";
import path from "path";
import { fileURLToPath } from "url";
import jwt from "jsonwebtoken";
import mongoose from "mongoose";
import User from "../models/User.js";
import Room from "../models/Room.js";
import Node from "../models/Node.js";
import Edge from "../models/Edge.js";

const router = express.Router();

// Resolve __dirname in ESM
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Helper function to load and parse JSON pages
const loadPageTemplate = (pageName) => {
  try {
    const pagePath = path.join(__dirname, `../virtualRenderer/pages/${pageName}.json`);
    return JSON.parse(fs.readFileSync(pagePath, "utf8"));
  } catch (error) {
    console.error(`Failed to load page template: ${pageName}`, error);
    return null;
  }
};

// Helper function to get user from token
const getUserFromToken = async (token) => {
  if (!token) return null;
  try {
    const decoded = jwt.verify(token, process.env.JWT_SECRET || 'default-secret');
    return await User.findById(decoded.userId);
  } catch (error) {
    console.error('Token verification failed:', error);
    return null;
  }
};

// Helper function to validate ObjectId
const isValidObjectId = (id) => {
  return mongoose.Types.ObjectId.isValid(id);
};

// GET /api/data/:page - Dynamic page routing
router.get("/:page", async (req, res) => {
  try {
    const { page } = req.params;
    const token = req.headers.authorization?.replace('Bearer ', '');
    const user = await getUserFromToken(token);

    console.log(`📖 Loading page: ${page} for user: ${user?.username || 'anonymous'}`);

    switch (page) {
      case 'home':
        await handleHomePage(req, res, user);
        break;
      case 'profile':
        await handleProfilePage(req, res, user);
        break;
      case 'graph':
        await handleGraphPage(req, res, user);
        break;
      case 'publicDebates':
        await handlePublicDebatesPage(req, res, user);
        break;
      case 'publicDebateViewer':
        await handlePublicDebateViewerPage(req, res, user);
        break;
      default:
        console.warn(`Unknown page requested: ${page}`);
        res.status(404).json({ error: 'Page not found' });
    }
  } catch (error) {
    console.error('Data route error:', error);
    res.status(500).json({ error: 'Internal server error' });
  }
});

// Legacy route for home page
router.get("/", async (req, res) => {
  const token = req.headers.authorization?.replace('Bearer ', '');
  const user = await getUserFromToken(token);
  await handleHomePage(req, res, user);
});

// Page handler functions
async function handleHomePage(req, res, user) {
  const template = loadPageTemplate('home');
  if (!template) {
    return res.status(500).json({ error: 'Failed to load home page' });
  }

  if (!user) {
    // Return login form if no user
    const loginTemplate = loadPageTemplate('login');
    return res.json(loginTemplate || template);
  }

  // Load user's rooms
  try {
    const rooms = await Room.find({
      $or: [
        { owner: user._id },
        { members: user._id }
      ]
    }).sort({ updatedAt: -1 });

    // Inject rooms data into template
    template.payload.data = {
      user: {
        id: user._id,
        username: user.username,
        email: user.email
      },
      rooms: rooms.map(room => ({
        _id: room._id,
        name: room.name,
        description: room.description,
        isPublic: room.isPublic,
        memberCount: room.members.length,
        createdAt: room.createdAt,
        isOwner: room.owner.toString() === user._id.toString()
      }))
    };

    res.json(template);
  } catch (error) {
    console.error('Failed to load rooms:', error);
    res.json(template);
  }
}

async function handleProfilePage(req, res, user) {
  const template = loadPageTemplate('profile');
  if (!template) {
    return res.status(500).json({ error: 'Failed to load profile page' });
  }

  if (!user) {
    return res.status(401).json({ error: 'Authentication required' });
  }

  // Inject user data
  template.payload.data = {
    user: {
      id: user._id,
      username: user.username,
      email: user.email,
      createdAt: user.createdAt
    }
  };

  res.json(template);
}

async function handleGraphPage(req, res, user) {
  const template = loadPageTemplate('graph');
  if (!template) {
    return res.status(500).json({ error: 'Failed to load graph page' });
  }

  if (!user) {
    return res.status(401).json({ error: 'Authentication required' });
  }

  const { roomId } = req.query;
  
  if (!roomId) {
    return res.status(400).json({ error: 'Room ID required' });
  }

  try {
    // Load room and verify access
    const room = await Room.findById(roomId);
    if (!room) {
      return res.status(404).json({ error: 'Room not found' });
    }

    const hasAccess = room.isPublic || 
                     room.owner.toString() === user._id.toString() ||
                     room.members.includes(user._id);

    if (!hasAccess) {
      return res.status(403).json({ error: 'Access denied' });
    }

    // Load graph data
    const nodes = await Node.find({ room: roomId });
    const edges = await Edge.find({ room: roomId });

    template.payload.data = {
      user: {
        id: user._id,
        username: user.username
      },
      room: {
        _id: room._id,
        name: room.name,
        description: room.description,
        isOwner: room.owner.toString() === user._id.toString()
      },
      graph: {
        nodes: nodes,
        edges: edges
      }
    };

    res.json(template);
  } catch (error) {
    console.error('Failed to load graph data:', error);
    res.status(500).json({ error: 'Failed to load graph data' });
  }
}

async function handlePublicDebatesPage(req, res, user) {
  const template = loadPageTemplate('publicDebates');
  if (!template) {
    return res.status(500).json({ error: 'Failed to load public debates page' });
  }

  try {
    const page = parseInt(req.query.page) || 1;
    const limit = parseInt(req.query.limit) || 12;
    const search = req.query.search || '';
    const skip = (page - 1) * limit;

    const searchQuery = search ? {
      isPublic: true,
      $or: [
        { name: { $regex: search, $options: 'i' } },
        { description: { $regex: search, $options: 'i' } }
      ]
    } : { isPublic: true };

    const [rooms, totalCount] = await Promise.all([
      Room.find(searchQuery)
        .populate('owner', 'username')
        .sort({ updatedAt: -1 })
        .skip(skip)
        .limit(limit),
      Room.countDocuments(searchQuery)
    ]);

    template.payload.data = {
      user: user ? {
        id: user._id,
        username: user.username
      } : null,
      rooms: rooms.map(room => ({
        _id: room._id,
        name: room.name,
        description: room.description,
        memberCount: room.members.length,
        owner: room.owner.username,
        createdAt: room.createdAt
      })),
      pagination: {
        currentPage: page,
        totalPages: Math.ceil(totalCount / limit),
        totalCount,
        hasNext: page < Math.ceil(totalCount / limit),
        hasPrev: page > 1
      },
      searchQuery: search
    };

    res.json(template);
  } catch (error) {
    console.error('Failed to load public debates:', error);
    res.status(500).json({ error: 'Failed to load public debates' });
  }
}

async function handlePublicDebateViewerPage(req, res, user) {
  const template = loadPageTemplate('publicDebateViewer');
  if (!template) {
    return res.status(500).json({ error: 'Failed to load public debate viewer page' });
  }

  const { roomId } = req.query;
  
  if (!roomId) {
    return res.status(400).json({ error: 'Room ID required' });
  }

  try {
    const room = await Room.findById(roomId).populate('owner', 'username');
    
    if (!room || !room.isPublic) {
      return res.status(404).json({ error: 'Public debate not found' });
    }

    // Load graph data for public viewing
    const nodes = await Node.find({ room: roomId });
    const edges = await Edge.find({ room: roomId });

    template.payload.data = {
      user: user ? {
        id: user._id,
        username: user.username
      } : null,
      room: {
        _id: room._id,
        name: room.name,
        description: room.description,
        owner: room.owner.username,
        memberCount: room.members.length,
        createdAt: room.createdAt
      },
      graph: {
        nodes: nodes,
        edges: edges
      }
    };

    res.json(template);
  } catch (error) {
    console.error('Failed to load public debate:', error);
    res.status(500).json({ error: 'Failed to load public debate' });
  }
}

export default router;
