// src/components/ToolBar.jsx
import React, { useState } from 'react';

export default function ToolBar({ onAddNode }) {
  const [label, setLabel] = useState('');

  const handleAdd = () => {
    if (!label.trim()) return;
    const newNode = {
      id: Date.now().toString(),
      label,
      metadata: {}
    };
    onAddNode(newNode);
    setLabel('');
  };

  return (
    <div style={{ position: 'absolute', top: 10, left: 10, background: 'rgba(255,255,255,0.8)', padding: '8px', borderRadius: '4px' }}>
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
