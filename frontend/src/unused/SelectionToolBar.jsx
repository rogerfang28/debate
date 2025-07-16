// Status: ditched for ToolBar.jsx, incomplete/broken
// The toolbar that pops up when you select a node or edge
export default function SelectionToolbar({ selection, onAddNode, onAddEdge, onClear }) {
  if (!selection) return null;
  
  const { type } = selection;

  return (
    <div style={{ position: 'fixed', top: '40%', left: '40%', background: 'white', padding: 16, border: '1px solid #ccc', zIndex: 100 }}>
      {type === 'node' && (
        <>
          <h3>Node Options</h3>
          <button onClick={() => onAddNode(selection.item)}>Add Connected Node</button>
          <button onClick={() => onClear()}>Cancel</button>
        </>
      )}
      {type === 'edge' && (
        <>
          <h3>Edge Options</h3>
          <button onClick={() => onAddEdge(selection.item)}>Edit Edge</button>
          <button onClick={() => onClear()}>Cancel</button>
        </>
      )}
    </div>
  );
}
