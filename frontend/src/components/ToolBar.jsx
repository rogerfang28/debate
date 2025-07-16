// Status: incomplete
// src/components/ToolBar.jsx
// The UI for this ToolBar should be a menu that changes when selecting nodes and edges, you can add node without selecting to get one with no edges, add nodes while selecting another node to make a new node and an edge from that node to the other. Should be able to add a 
// src/components/ToolBar.jsx
// src/components/ToolBar.jsx
import React from 'react';

export default function ToolBar({
  selection,
  onAddNode,
  onAddConnectedNode,
  onChallenge,
  onDelete,
  onClearSelection
}) {
  const type = selection?.type;

  return (
    <div style={{
      position: 'fixed',
      bottom: 16,
      left: '50%',
      transform: 'translateX(-50%)',
      backgroundColor: 'rgba(70, 70, 70, 0.9)',
      padding: '8px 16px',
      borderRadius: 8,
      boxShadow: '0 2px 8px rgba(0,0,0,0.2)',
      display: 'flex',
      gap: 8,
      zIndex: 100
    }}>
      {!type && <button onClick={onAddNode}>Add Node</button>}

      {type === 'node' && (
        <>
          <button onClick={() => onAddConnectedNode(selection.item)}>Add Connected Node</button>
          <button onClick={() => onChallenge(selection.item)}>Challenge Node</button>
          <button onClick={() => onDelete(selection.item)}>Delete Node</button>
        </>
      )}

      {type === 'edge' && (
        <>
          <button onClick={() => onChallenge(selection.item)}>Challenge Edge</button>
          <button onClick={() => onDelete(selection.item)}>Delete Edge</button>
        </>
      )}

      {type && <button onClick={onClearSelection}>Close</button>}
    </div>
  );
}
