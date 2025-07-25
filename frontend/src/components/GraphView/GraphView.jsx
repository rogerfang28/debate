import React, { useRef, useState } from "react";
import { STYLES } from "./constants.js";
import useGraphForces from "./useGraphForces.js";
import GraphControls from "./GraphControls.jsx";
import GraphCanvas from "./GraphCanvas.jsx";
import { socket } from "../../lib/socket.js";

/**
 * GraphView – orchestrator component that composes controls + canvas.
 * Public props intentionally match the original monolithic GraphView so callers
 * don’t have to change.
 */
export default function GraphView({
  graphData,
  dimensions,
  selection,
  setSelection,
  roomId,
  draggedNodeId,
  setDraggedNodeId,
  onChallengeClick,
  currentUserId,
}) {
  const fgRef = useRef();
  const [isSpreadOut, setIsSpreadOut] = useState(false);
  const [hoveredNode, setHoveredNode] = useState(null);
  const [hoveredLink, setHoveredLink] = useState(null);

  const safeGraph = {
    nodes: Array.isArray(graphData?.nodes) ? graphData.nodes : [],
    links: Array.isArray(graphData?.links) ? graphData.links : [],
  };

  useGraphForces(fgRef, draggedNodeId, safeGraph);

  // Persist drag‑end position to server --------------------------------------
  const handleNodeDragEnd = (node) => {
    socket.emit("update-node-position", {
      id: node.id,
      x: node.x,
      y: node.y,
      room: roomId,
    });
    setDraggedNodeId(null);
  };

  return (
    <div style={STYLES.CONTAINER}>
      <GraphControls
        fgRef={fgRef}
        safeGraph={safeGraph}
        roomId={roomId}
        isSpreadOut={isSpreadOut}
        setIsSpreadOut={setIsSpreadOut}
      />

      <GraphCanvas
        fgRef={fgRef}
        safeGraph={safeGraph}
        dimensions={dimensions}
        selection={selection}
        setSelection={setSelection}
        hoverNode={hoveredNode}
        setHoverNode={setHoveredNode}
        hoverLink={hoveredLink}
        setHoverLink={setHoveredLink}
        setDraggedNodeId={setDraggedNodeId}
        onChallengeClick={onChallengeClick}
        currentUserId={currentUserId}
        onNodeDragEnd={handleNodeDragEnd}
      />
    </div>
  );
}
