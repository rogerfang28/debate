// actions/handleActions.js
import { socket } from '../../lib/socket';
import * as graph from '../../functions/graphFunctions';
import { getIds } from './getIds.js';

export const handleAddNode = (setForm) => {
  setForm({ type: 'node', parent: null });
};

export const handleAddConnectedNode = (node, setForm) => {
  setForm({ type: 'node', parent: node.id });
};

export const handleChallenge = (selection, setForm) => {
  console.log('🎯 handleChallenge called with:', selection);
  const type = selection.type === 'node' ? 'challenge-node' : 'challenge-edge';
  console.log('📝 Setting form type:', type);
  const formData = { type, item: selection.item };
  console.log('📋 Form data being set:', formData);
  setForm(formData);
  console.log('✅ setForm called successfully');
};

export const handleDelete = (selection, setSelection, roomId) => {
  const { type, item } = selection;
  if (type === 'node') {
    graph.deleteNode(item.id, roomId);
  } else if (type === 'edge') {
    const { src, tgt } = getIds(item);
    graph.deleteEdge(src, tgt, roomId);
  }
  setSelection({ type: null, item: null });
};

export const handleSubmitForm = (form, data, setForm, setSelection, createdBy, roomId) => {
  if (form.type === 'node') {
    const newNode = graph.addNode(data.label, roomId);
    if (form.parent && newNode) {
      // Wait a moment for the node to be created, then add the edge
      setTimeout(() => {
        graph.addEdge(form.parent, newNode.id, data.reason || 'connects to', createdBy, roomId);
      }, 100);
    }
  } else if (form.type === 'challenge-node') {
    graph.challengeNode(form.item.id, data.responder, data.reason, roomId);
  } else if (form.type === 'challenge-edge') {
    // Use the edge ID directly for challenging edges
    graph.challengeEdge(form.item.id, data.responder, data.reason, roomId);
  }

  setForm(null);
  setSelection({ type: null, item: null });
};
