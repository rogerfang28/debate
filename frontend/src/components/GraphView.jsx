// Status: Done and functional
// GraphView.jsx should make the graph appear on the screen
import React, { useEffect, useRef, useState } from 'react';
import ForceGraph2D from 'react-force-graph-2d';
import * as d3 from 'd3';
import { socket } from '../lib/socket';

export default function GraphView({ graphData, dimensions, selection, setSelection, roomId, draggedNodeId, setDraggedNodeId, onChallengeClick, currentUserId }) {
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
    
    // Determine challenge status for current user
    const pendingChallenges = (node.challenges || []).filter(c => c.status === 'pending');
    const isChallenger = pendingChallenges.some(c => c.challenger?._id === currentUserId);
    const isResponder = pendingChallenges.some(c => c.responder?._id === currentUserId);
    const hasPendingChallenge = pendingChallenges.length > 0;
    
    // Debug logging for first node to check data structure (commented out to reduce spam)
    // if (safeGraph.nodes.length > 0 && node === safeGraph.nodes[0]) {
    //   console.log('Node data structure:', node);
    //   console.log('Current user ID:', currentUserId);
    //   console.log('Pending challenges:', pendingChallenges);
    // }
    
    // Draw node shadow for selection/hover/challenge
    if (isSelected || isHovered || hasPendingChallenge) {
      ctx.shadowBlur = 15;
      if (isSelected) {
        ctx.shadowColor = '#6366f1';
      } else if (isHovered) {
        ctx.shadowColor = '#f59e0b';
      } else if (isChallenger) {
        ctx.shadowColor = '#eab308'; // Yellow for challenger
      } else if (isResponder) {
        ctx.shadowColor = '#ef4444'; // Red for responder
      }
    }
    
    // Determine node color - different colors for challenger vs responder
    let nodeColor = '#8b5cf6'; // Default purple
    if (hasPendingChallenge) {
      if (isChallenger) {
        nodeColor = '#eab308'; // Yellow for challenger (you created this challenge)
      } else if (isResponder) {
        nodeColor = '#ef4444'; // Red for responder (you need to respond)
      }
    } else if (isSelected) {
      nodeColor = '#6366f1'; // Blue for selected  
    } else if (isHovered) {
      nodeColor = '#f59e0b'; // Orange for hovered
    } else if (node.color) {
      nodeColor = node.color; // Custom color if set
    }
    
    // Draw main node
    ctx.beginPath();
    ctx.arc(node.x, node.y, size + (isSelected ? 2 : 0) + (isHovered ? 1 : 0) + (hasPendingChallenge ? 1 : 0), 0, 2 * Math.PI);
    ctx.fillStyle = nodeColor;
    ctx.fill();
    
    // Draw border with challenge indication
    let borderColor = 'rgba(255,255,255,0.3)';
    let borderWidth = 1;
    
    if (isSelected) {
      borderColor = '#a5b4fc';
      borderWidth = 2;
    } else if (isHovered) {
      borderColor = '#fbbf24';
      borderWidth = 1.5;
    } else if (hasPendingChallenge) {
      if (isChallenger) {
        borderColor = '#fde047'; // Yellow border for challenger
      } else if (isResponder) {
        borderColor = '#fca5a5'; // Red border for responder
      }
      borderWidth = 2;
    }
    
    ctx.strokeStyle = borderColor;
    ctx.lineWidth = borderWidth;
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
    
    // Determine challenge status for current user
    const pendingChallenges = (link.challenges || []).filter(c => c.status === 'pending');
    const isChallenger = pendingChallenges.some(c => c.challenger?._id === currentUserId);
    const isResponder = pendingChallenges.some(c => c.responder?._id === currentUserId);
    const hasPendingChallenge = pendingChallenges.length > 0;
    
    // Determine line color - same colors as nodes, default purple like nodes
    let lineColor = '#8b5cf6'; // Default purple (same as nodes)
    if (hasPendingChallenge) {
      if (isChallenger) {
        lineColor = '#eab308'; // Yellow for challenger
      } else if (isResponder) {
        lineColor = '#ef4444'; // Red for responder
      }
    } else if (isSelected) {
      lineColor = '#6366f1'; // Blue for selected
    } else if (isHovered) {
      lineColor = '#f59e0b'; // Orange for hovered
    }
    
    // Set line style - thicker for challenged edges
    const baseWidth = hasPendingChallenge ? 2 : 1;
    ctx.lineWidth = Math.max(1, (isSelected ? 2 : (isHovered ? 1.5 : baseWidth)) / globalScale);
    ctx.strokeStyle = lineColor;
    
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
        onNodeClick={node => {
          // Check if node has pending challenges and if current user can respond
          const pendingChallenges = (node.challenges || []).filter(c => c.status === 'pending');
          const isResponder = pendingChallenges.some(c => c.responder?._id === currentUserId);
          
          if (pendingChallenges.length > 0 && isResponder && onChallengeClick) {
            onChallengeClick(pendingChallenges, 'node', node.id);
          } else {
            setSelection({ type: 'node', item: node });
          }
        }}
        onLinkClick={link => {
          // Check if edge has pending challenges and if current user can respond
          const pendingChallenges = (link.challenges || []).filter(c => c.status === 'pending');
          const isResponder = pendingChallenges.some(c => c.responder?._id === currentUserId);
          
          if (pendingChallenges.length > 0 && isResponder && onChallengeClick) {
            onChallengeClick(pendingChallenges, 'edge', link.id);
          } else {
            setSelection({ type: 'edge', item: link });
          }
        }}
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
