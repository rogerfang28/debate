import React from 'react';
import ForceGraph2D from 'react-force-graph-2d';

import { getChallengeStatus } from './challengeUtils.js';
import {
  createNodeRenderer,
  createLinkRenderer
} from './renderUtils.js';

/**
 * <GraphCanvas /> – low‑level drawing component.
 * Handles only ForceGraph2D rendering + event plumbing.
 */
export default function GraphCanvas({
  fgRef,
  safeGraph,
  dimensions,
  selection,
  setSelection,
  hoverNode,
  setHoverNode,
  hoverLink,
  setHoverLink,
  setDraggedNodeId,
  onChallengeClick,
  currentUserId,
  onNodeDragEnd        // comes from GraphView so it can emit the socket event
}) {
  // ----------------------------- click / hover / drag handlers --------------
  const handleNodeClick = node => {
    const { pending, isResponder } = getChallengeStatus(node, currentUserId);
    if (pending.length && isResponder && onChallengeClick) {
      onChallengeClick(pending, 'node', node.id);
    } else {
      setSelection({ type: 'node', item: node });
    }
  };

  const handleLinkClick = link => {
    const { pending, isResponder } = getChallengeStatus(link, currentUserId);
    if (pending.length && isResponder && onChallengeClick) {
      onChallengeClick(pending, 'edge', link.id);
    } else {
      setSelection({ type: 'edge', item: link });
    }
  };

  const handleDragStart = node => setDraggedNodeId(node.id);
  const handleDragEnd   = node => {
    setDraggedNodeId(null);
    onNodeDragEnd?.(node);   // parent persists position via socket
  };

  // ------------------------------------- render -----------------------------
  return (
    <ForceGraph2D
      ref={fgRef}
      graphData={safeGraph}
      nodeCanvasObject={createNodeRenderer(selection, hoverNode, currentUserId)}
      linkCanvasObject={createLinkRenderer(selection, hoverLink, currentUserId)}
      width={dimensions.width}
      height={dimensions.height}
      nodeRelSize={4}
      d3AlphaDecay={0.05}
      d3VelocityDecay={0.4}
      enableNodeDrag
      onNodeClick={handleNodeClick}
      onLinkClick={handleLinkClick}
      onNodeHover={setHoverNode}
      onLinkHover={setHoverLink}
      onNodeDragStart={handleDragStart}
      onNodeDragEnd={handleDragEnd}
    />
  );
}
