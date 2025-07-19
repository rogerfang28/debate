// Status: Done and functional
// GraphView.jsx should make the graph appear on the screen
import React, { useEffect, useRef, useState } from 'react';
import ForceGraph2D from 'react-force-graph-2d';
import * as d3 from 'd3';
import { socket } from '../lib/socket';

export default function GraphView({ graphData, dimensions, selection, setSelection, roomId, draggedNodeId, setDraggedNodeId }) {
  const fgRef = useRef();
  const [isSpreadOut, setIsSpreadOut] = useState(false);
  const [hoveredNode, setHoveredNode] = useState(null);
  const [hoveredLink, setHoveredLink] = useState(null);
  // UNUSED: Grid system - disabled due to positioning issues
  // const [gridTransform, setGridTransform] = useState({ k: 1, x: 0, y: 0 });

  const safeGraph = {
    nodes: Array.isArray(graphData?.nodes) ? graphData.nodes : [],
    links: Array.isArray(graphData?.links) ? graphData.links : []
  };

  useEffect(() => {
    if (fgRef.current) {
      fgRef.current.d3Force('charge', d3.forceManyBody().strength(0));
      
      // Disable forces during drag to prevent snapping
      if (draggedNodeId) {
        // Stop the simulation for the dragged node
        fgRef.current.d3ReheatSimulation();
        const node = safeGraph.nodes.find(n => n.id === draggedNodeId);
        if (node) {
          node.fx = node.x;
          node.fy = node.y;
        }
      } else {
        // Re-enable forces when not dragging
        safeGraph.nodes.forEach(node => {
          delete node.fx;
          delete node.fy;
        });
      }
    }
  }, [graphData, dimensions, draggedNodeId, safeGraph.nodes]);

  // Custom node rendering with enhanced visuals
  const nodeCanvasObject = (node, ctx, globalScale) => {
    const size = 4; // Keep original size
    const isSelected = selection?.type === 'node' && selection?.item?.id === node.id;
    const isHovered = hoveredNode?.id === node.id;
    
    // Debug logging for first node to check data structure
    if (safeGraph.nodes.length > 0 && node === safeGraph.nodes[0]) {
      console.log('Node data structure:', node);
    }
    
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
    
    // Draw node label
    if (node.label) {
      const label = node.label;
      const fontSize = Math.max(10, 12 / globalScale); // Responsive font size
      ctx.font = `${fontSize}px Inter, sans-serif`;
      ctx.fillStyle = 'rgba(255, 255, 255, 0.9)';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'middle';
      
      // Draw text background for better readability
      const textMetrics = ctx.measureText(label);
      const textWidth = textMetrics.width;
      const textHeight = fontSize;
      const padding = 4;
      
      // Position text above the node
      const textX = node.x;
      const textY = node.y - size - textHeight/2 - padding - 2;
      
      // Draw background rectangle
      ctx.fillStyle = 'rgba(0, 0, 0, 0.7)';
      ctx.fillRect(
        textX - textWidth/2 - padding, 
        textY - textHeight/2 - padding/2, 
        textWidth + padding * 2, 
        textHeight + padding
      );
      
      // Draw the text
      ctx.fillStyle = 'rgba(255, 255, 255, 0.95)';
      ctx.fillText(label, textX, textY);
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
      `
    }}>
      
      {/* UNUSED: Grid overlay - disabled due to CSS background positioning issues
      <div
        style={{
          position: 'absolute',
          top: 0,
          left: 0,
          width: '100%',
          height: '100%',
          pointerEvents: 'none',
          backgroundImage: `
            linear-gradient(90deg, rgba(255,255,255,0.1) 1px, transparent 1px),
            linear-gradient(rgba(255,255,255,0.1) 1px, transparent 1px)
          `,
          backgroundSize: `${50 * gridTransform.k}px ${50 * gridTransform.k}px`,
          backgroundPosition: `${-gridTransform.x}px ${-gridTransform.y}px`,
          zIndex: 1
        }}
      />
      */}
      
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
                    y: node.y,
                    room: roomId  // Include room ID for proper backend routing
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
        onNodeDragStart={node => {
          setDraggedNodeId(node.id);
        }}
        onNodeDragEnd={node => {
          console.log(`🔄 Frontend: Sending position update for node ${node.id} at (${node.x}, ${node.y}) in room ${roomId}`);
          socket.emit('update-node-position', {
            id: node.id,
            x: node.x,
            y: node.y,
            room: roomId  // Include room ID for proper backend routing
          });
          setDraggedNodeId(null);
        }}
        // UNUSED: Grid zoom handler - disabled with grid system
        /*
        onZoom={transform => {
          // Update grid transform to match graph zoom/pan
          setGridTransform({
            x: transform.x,
            y: transform.y,
            k: transform.k
          });
        }}
        */
      />
    </div>
  );
}
