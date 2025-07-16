// src/components/ToolBar.jsx
// The UI for this ToolBar should be a menu that changes when selecting nodes and edges, you can add node without selecting to get one with no edges, add nodes while selecting another node to make a new node and an edge from that node to the other. Should be able to add a 
import React, { useState } from 'react';

export default function ToolBar({ onAddNode }) {
  const [label, setLabel] = useState('');

  const handleAdd = () => {
    if (!label.trim()) return;
    const newNode = {
      id: Date.now().toString(),
      text: label,
      metadata: {}
    };
    onAddNode(newNode);
    setLabel('');
  };

  return (
    <div style={{ position: 'absolute', top: 10, left: 10, background: 'rgba(71, 71, 71, 0)', padding: '8px', borderRadius: '4px' }}>
      <input
        type="text"
        value={label}
        onChange={e => setLabel(e.target.value)}
        placeholder="New node label"
      />
      <button onClick={handleAdd} style={{ marginLeft: 8 }}>
        Add Node
      </button>
    </div>
  );
}
