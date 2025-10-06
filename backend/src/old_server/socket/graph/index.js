import addNode            from './addNode.js';
import deleteNode         from './deleteNode.js';
import addEdge            from './addEdge.js';
import deleteEdge         from './deleteEdge.js';
import updateNodePosition from './updateNodePosition.js';
import challenge          from './challenge.js';
import respondChallenge   from './respondChallenge.js';

export default function registerGraphEvents(io, socket) {
  socket.on('add-node',             p => addNode(io, socket, p));
  socket.on('delete-node',          p => deleteNode(io, socket, p));
  socket.on('add-edge',             p => addEdge(io, socket, p));
  socket.on('delete-edge',          p => deleteEdge(io, socket, p));
  socket.on('update-node-position', p => updateNodePosition(io, socket, p));
  socket.on('challenge',            p => challenge(io, socket, p));
  socket.on('respond-to-challenge', p => respondChallenge(io, socket, p));
}