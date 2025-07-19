// Status: Done and functional
// GraphView.jsx should make the graph appear on the screen
import React, { useEffect, useRef, useState } from 'react';
import ForceGraph2D from 'react-force-graph-2d';
import * as d3 from 'd3';
import { socket } from '../lib/socket';

export default function GraphView({ graphData, dimensions, selection, setSelection }) {
  const fgRef = useRef();
  const [isSpreadOut, setIsSpreadOut] = useState(false);
  const [hoveredNode, setHoveredNode] = useState(null);
  const [hoveredLink, setHoveredLink] = useState(null);

  const safeGraph = {
    nodes: Array.isArray(graphData?.nodes) ? graphData.nodes : [],
    links: Array.isArray(graphData?.links) ? graphData.links : []
  };

  useEffect(() => {
    if (fgRef.current) {
      fgRef.current.d3Force('charge', d3.forceManyBody().strength(0));
    }
  }, [graphData, dimensions]);

  // Custom node rendering with enhanced visuals
  const nodeCanvasObject = (node, ctx, globalScale) => {
    const size = 4; // Keep original size
    const isSelected = selection?.type === 'node' && selection?.item?.id === node.id;
    const isHovered = hoveredNode?.id === node.id;
    
    // Draw node shadow for selection/hover
    if (isSelected || isHovered) {
      ctx.shadowBlur = 15;
      ctx.shadowColor = isSelected ? '#6366f1' : '#f59e0b';
    }
    
    // Draw main node
    ctx.beginPath();
    ctx.arc(node.x, node.y, size + (isSelected ? 2 : 0) + (isHovered ? 1 : 0), 0, 2 * Math.PI);
    ctx.fillStyle = isSelected ? '#6366f1' : (isHovered ? '#f59e0b' : node.color || '#8b5cf6');
    ctx.fill();
    
    // Draw border
    ctx.strokeStyle = isSelected ? '#a5b4fc' : (isHovered ? '#fbbf24' : 'rgba(255,255,255,0.3)');
    ctx.lineWidth = isSelected ? 2 : (isHovered ? 1.5 : 1);
    ctx.stroke();
    
    // Reset shadow
    ctx.shadowBlur = 0;
    
    // Draw node label above the node
    const label = node.label || node.id;
    if (label && globalScale > 0.6) { // Only show labels when zoomed in enough
      ctx.font = `${Math.max(10, 12 / globalScale)}px Inter, sans-serif`;
      ctx.fillStyle = 'rgba(255, 255, 255, 0.9)';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'bottom';
      
      // Add text background for better readability
      const textMetrics = ctx.measureText(label);
      const textWidth = textMetrics.width;
      const textHeight = 12 / globalScale;
      const padding = 4 / globalScale;
      
      // Background rectangle
      ctx.fillStyle = 'rgba(0, 0, 0, 0.7)';
      ctx.fillRect(
        node.x - textWidth/2 - padding,
        node.y - size - textHeight - padding * 2,
        textWidth + padding * 2,
        textHeight + padding
      );
      
      // Text
      ctx.fillStyle = 'rgba(255, 255, 255, 0.9)';
      ctx.fillText(label, node.x, node.y - size - padding);
    }
  };

  // Custom link rendering with enhanced visuals (conservative approach)
  const linkCanvasObject = (link, ctx, globalScale) => {
    const isSelected = selection?.type === 'edge' && selection?.item?.id === link.id;
    const isHovered = hoveredLink?.id === link.id;
    
    // Set line style - more conservative than before
    ctx.lineWidth = Math.max(1, (isSelected ? 2 : (isHovered ? 1.5 : 1)) / globalScale);
    ctx.strokeStyle = isSelected ? '#6366f1' : (isHovered ? '#f59e0b' : 'rgba(139, 92, 246, 0.4)');
    
    // Skip shadows to avoid performance issues
    
    // Draw the link
    ctx.beginPath();
    ctx.moveTo(link.source.x, link.source.y);
    ctx.lineTo(link.target.x, link.target.y);
    ctx.stroke();
  };

  return (
    <div style={{ 
      position: 'fixed', 
      top: 0, 
      left: 0, 
      width: '100vw', 
      height: '100vh',
      background: 'linear-gradient(135deg, #0f172a 0%, #1e293b 100%)',
      backgroundImage: `
        radial-gradient(circle at 25% 25%, rgba(99, 102, 241, 0.1) 0%, transparent 50%),
        radial-gradient(circle at 75% 75%, rgba(139, 92, 246, 0.1) 0%, transparent 50%)
      `,
      backgroundSize: '100% 100%, 100% 100%'
    }}>
      {/* Control buttons */}
      <div style={{
        position: 'absolute',
        top: 64,
        right: 16,
        zIndex: 10,
        display: 'flex',
        gap: '8px'
      }}>
        <button 
          onClick={() => {
            if (fgRef.current && safeGraph.nodes.length > 0) {
              // Scale all node positions toward center
              safeGraph.nodes.forEach(node => {
                if (node.x !== undefined && node.y !== undefined) {
                  node.x *= 0.1;
                  node.y *= 0.1;
                  // Also update the node position in the backend
                  socket.emit('update-node-position', {
                    id: node.id,
                    x: node.x,
                    y: node.y
                  });
                }
              });
              // Restart the simulation to apply the new positions
              fgRef.current.d3ReheatSimulation();
            }
          }}
          style={{
            padding: '10px 14px',
            background: 'linear-gradient(135deg, rgba(59, 130, 246, 0.9), rgba(99, 102, 241, 0.9))',
            border: '1px solid rgba(99, 102, 241, 0.3)',
            borderRadius: '8px',
            color: 'white',
            cursor: 'pointer',
            fontSize: '13px',
            fontWeight: '500',
            backdropFilter: 'blur(10px)',
            boxShadow: '0 4px 12px rgba(0, 0, 0, 0.15)',
            transition: 'all 0.2s ease'
          }}
        >
          🎯 Center
        </button>
        <button 
          onClick={() => {
            if (fgRef.current) {
              // Enable centering force to make nodes gravitate to center
              fgRef.current.d3Force('center', d3.forceCenter(0, 0).strength(0.1));
              // Restart the simulation to apply the centering force
              fgRef.current.d3ReheatSimulation();
            }
          }}
          style={{
            padding: '10px 14px',
            background: 'linear-gradient(135deg, rgba(16, 185, 129, 0.9), rgba(5, 150, 105, 0.9))',
            border: '1px solid rgba(16, 185, 129, 0.3)',
            borderRadius: '8px',
            color: 'white',
            cursor: 'pointer',
            fontSize: '13px',
            fontWeight: '500',
            backdropFilter: 'blur(10px)',
            boxShadow: '0 4px 12px rgba(0, 0, 0, 0.15)',
            transition: 'all 0.2s ease'
          }}
        >
          🧲 Gravitate
        </button>
        <button 
          onClick={() => {
            if (fgRef.current) {
              if (isSpreadOut) {
                // Turn off spreading - reset to neutral forces
                fgRef.current.d3Force('charge', d3.forceManyBody().strength(0));
                fgRef.current.d3Force('collision', null);
                setIsSpreadOut(false);
              } else {
                // Turn on spreading - enable repulsive forces
                fgRef.current.d3Force('charge', d3.forceManyBody().strength(-300));
                fgRef.current.d3Force('collision', d3.forceCollide().radius(30));
                setIsSpreadOut(true);
              }
              // Restart the simulation to apply the changes
              fgRef.current.d3ReheatSimulation();
            }
          }}
          style={{
            padding: '10px 14px',
            background: isSpreadOut 
              ? 'linear-gradient(135deg, rgba(239, 68, 68, 0.9), rgba(220, 38, 38, 0.9))'
              : 'linear-gradient(135deg, rgba(168, 85, 247, 0.9), rgba(147, 51, 234, 0.9))',
            border: `1px solid ${isSpreadOut ? 'rgba(239, 68, 68, 0.3)' : 'rgba(168, 85, 247, 0.3)'}`,
            borderRadius: '8px',
            color: 'white',
            cursor: 'pointer',
            fontSize: '13px',
            fontWeight: '500',
            backdropFilter: 'blur(10px)',
            boxShadow: '0 4px 12px rgba(0, 0, 0, 0.15)',
            transition: 'all 0.2s ease'
          }}
        >
          {isSpreadOut ? '📉 Compact' : '📈 Spread'}
        </button>
        <button 
          onClick={() => {
            if (fgRef.current) {
              fgRef.current.zoomToFit(400, 40);
            }
          }}
          style={{
            padding: '10px 14px',
            background: 'linear-gradient(135deg, rgba(245, 158, 11, 0.9), rgba(217, 119, 6, 0.9))',
            border: '1px solid rgba(245, 158, 11, 0.3)',
            borderRadius: '8px',
            color: 'white',
            cursor: 'pointer',
            fontSize: '13px',
            fontWeight: '500',
            backdropFilter: 'blur(10px)',
            boxShadow: '0 4px 12px rgba(0, 0, 0, 0.15)',
            transition: 'all 0.2s ease'
          }}
        >
          🔍 Fit
        </button>
      </div>
      
      <ForceGraph2D
        ref={fgRef}
        graphData={safeGraph}
        nodeCanvasObject={nodeCanvasObject}
        linkCanvasObject={linkCanvasObject}
        width={dimensions.width}
        height={dimensions.height}
        nodeRelSize={4}
        d3AlphaDecay={0.05}
        d3VelocityDecay={0.4}
        enableNodeDrag={true}
        onNodeClick={node => setSelection({ type: 'node', item: node })}
        onLinkClick={link => setSelection({ type: 'edge', item: link })}
        onNodeHover={node => setHoveredNode(node)}
        onLinkHover={link => setHoveredLink(link)}
        onNodeDragEnd={node => {
          socket.emit('update-node-position', {
            id: node.id,
            x: node.x,
            y: node.y
          });
        }}
      />
    </div>
  );
}
