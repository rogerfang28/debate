// Enhanced ToolBar with modern design
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
    <div className="toolbar-container">
      <div className="toolbar-content">
        {!type && (
          <div className="toolbar-section animate-fade-in">
            <h3 className="toolbar-title">🎯 Quick Actions</h3>
            <button onClick={onAddNode} className="toolbar-button btn-primary">
              ➕ Add Node
            </button>
          </div>
        )}

        {type === 'node' && (
          <div className="toolbar-section animate-fade-in">
            <h3 className="toolbar-title">
              🔘 Node Selected: <span className="selection-name">{selection.item?.label || selection.item?.id}</span>
            </h3>
            <div className="toolbar-buttons">
              <button 
                onClick={() => onAddConnectedNode(selection.item)} 
                className="toolbar-button btn-success"
              >
                🔗 Add Connected Node
              </button>
              <button 
                onClick={() => onChallenge()} 
                className="toolbar-button btn-warning"
              >
                ⚡ Challenge Node
              </button>
              <button 
                onClick={() => onDelete(selection.item)} 
                className="toolbar-button btn-danger"
              >
                🗑️ Delete Node
              </button>
            </div>
          </div>
        )}

        {type === 'edge' && (
          <div className="toolbar-section animate-fade-in">
            <h3 className="toolbar-title">
              🔗 Edge Selected: <span className="selection-name">{selection.item?.source?.id} → {selection.item?.target?.id}</span>
            </h3>
            <div className="toolbar-buttons">
              <button 
                onClick={() => onChallenge()} 
                className="toolbar-button btn-warning"
              >
                ⚡ Challenge Edge
              </button>
              <button 
                onClick={() => onDelete(selection.item)} 
                className="toolbar-button btn-danger"
              >
                🗑️ Delete Edge
              </button>
            </div>
          </div>
        )}

        {type && (
          <button onClick={onClearSelection} className="toolbar-close-btn">
            ❌
          </button>
        )}
      </div>
    </div>
  );
}
