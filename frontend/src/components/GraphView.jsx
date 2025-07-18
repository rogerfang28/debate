// Enhanced GraphView with better visual feedback
import React, { useEffect, useRef, useState } from 'react';
import ForceGraph2D from 'react-force-graph-2d';
import * as d3 from 'd3';
import { socket } from '../lib/socket';

export default function GraphView({ graphData, dimensions, selection, setSelection }) {
  const fgRef = useRef();
  const [shouldZoom, setShouldZoom] = useState(true);
  const [hoveredNode, setHoveredNode] = useState(null);

  const safeGraph = {
    nodes: Array.isArray(graphData?.nodes) ? graphData.nodes : [],
    links: Array.isArray(graphData?.links) ? graphData.links : []
  };

  useEffect(() => {
    if (fgRef.current) {
      fgRef.current.d3Force('charge', d3.forceManyBody().strength(-100));
      fgRef.current.d3Force('center', d3.forceCenter());
      fgRef.current.d3Force('collision', d3.forceCollide().radius(20));
    }
  }, [graphData, dimensions]);

  // Custom node rendering with enhanced visuals
  const nodeCanvasObject = (node, ctx, globalScale) => {
    const size = 8;
    const isSelected = selection?.type === 'node' && selection?.item?.id === node.id;
    const isHovered = hoveredNode?.id === node.id;
    
    // Draw node shadow
    if (isSelected || isHovered) {
      ctx.shadowBlur = 20;
      ctx.shadowColor = isSelected ? '#6366f1' : '#f59e0b';
    }
    
    // Draw main node
    ctx.beginPath();
    ctx.arc(node.x, node.y, size + (isSelected ? 3 : 0) + (isHovered ? 2 : 0), 0, 2 * Math.PI);
    ctx.fillStyle = isSelected ? '#6366f1' : (isHovered ? '#f59e0b' : node.color || '#8b5cf6');
    ctx.fill();
    
    // Draw border
    ctx.strokeStyle = isSelected ? '#a5b4fc' : (isHovered ? '#fbbf24' : 'rgba(255,255,255,0.3)');
    ctx.lineWidth = isSelected ? 3 : (isHovered ? 2 : 1);
    ctx.stroke();
    
    // Reset shadow
    ctx.shadowBlur = 0;
    
    // Draw label
    if (isSelected || isHovered || globalScale > 1) {
      const label = node.label || node.id;
      const fontSize = Math.max(10, 12 / globalScale);
      ctx.font = `${fontSize}px Inter, system-ui, sans-serif`;
      ctx.fillStyle = '#e5e7eb';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      
      // Draw label background
      const textWidth = ctx.measureText(label).width;
      const padding = 4;
      ctx.fillStyle = 'rgba(31, 41, 55, 0.8)';
      ctx.fillRect(
        node.x - textWidth / 2 - padding,
        node.y + size + 8 - fontSize / 2 - padding,
        textWidth + padding * 2,
        fontSize + padding * 2
      );
      
      // Draw label text
      ctx.fillStyle = '#e5e7eb';
      ctx.fillText(label, node.x, node.y + size + 8);
    }
  };

  // Custom link rendering
  const linkCanvasObject = (link, ctx) => {
    const isSelected = selection?.type === 'edge' && 
      selection?.item?.source?.id === link.source.id && 
      selection?.item?.target?.id === link.target.id;
    
    ctx.strokeStyle = isSelected ? '#6366f1' : '#6b7280';
    ctx.lineWidth = isSelected ? 3 : 1;
    ctx.globalAlpha = isSelected ? 1 : 0.7;
    
    ctx.beginPath();
    ctx.moveTo(link.source.x, link.source.y);
    ctx.lineTo(link.target.x, link.target.y);
    ctx.stroke();
    
    ctx.globalAlpha = 1;
  };
  
  return (
    <div className="graph-container graph-with-nav">
      <div className="graph-overlay">
        <div className="graph-info">
          <span className="graph-stats">
            {safeGraph.nodes.length} nodes • {safeGraph.links.length} connections
          </span>
          {selection?.type && (
            <span className="selection-indicator animate-fade-in">
              {selection.type === 'node' ? '🔘' : '🔗'} {selection.type} selected
            </span>
          )}
        </div>
      </div>
      
      <ForceGraph2D
        ref={fgRef}
        graphData={safeGraph}
        width={dimensions.width}
        height={dimensions.height}
        nodeCanvasObject={nodeCanvasObject}
        linkCanvasObject={linkCanvasObject}
        nodeRelSize={8}
        d3AlphaDecay={0.02}
        d3VelocityDecay={0.3}
        enableNodeDrag={true}
        enableZoomInteraction={true}
        enablePanInteraction={true}
        onNodeClick={node => setSelection({ type: 'node', item: node })}
        onLinkClick={link => setSelection({ type: 'edge', item: link })}
        onNodeHover={node => setHoveredNode(node)}
        onNodeDragEnd={node => {
          socket.emit('update-node-position', {
            id: node.id,
            x: node.x,
            y: node.y
          });
        }}
        onEngineStop={() => {
          if (shouldZoom && fgRef.current) {
            fgRef.current.zoomToFit(400, 50);
            setShouldZoom(false);
          }
        }}
        cooldownTicks={100}
        onEngineStart={() => {
          // Optional: Show loading state
        }}
      />
    </div>
  );
}
