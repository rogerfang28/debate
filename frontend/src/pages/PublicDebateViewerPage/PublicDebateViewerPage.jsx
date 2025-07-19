// src/pages/PublicDebateViewerPage/PublicDebateViewerPage.jsx
import React, { useState, useEffect, useRef } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import ForceGraph2D from 'react-force-graph-2d';
import { transformGraph } from '../../functions/graphFunctions/transformGraph.js';
import { API_BASE } from '../../lib/config';
import './PublicDebateViewerPage.css';

export default function PublicDebateViewerPage() {
  const { roomId } = useParams();
  const navigate = useNavigate();
  const graphRef = useRef();
  
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [room, setRoom] = useState(null);
  const [graphData, setGraphData] = useState({ nodes: [], links: [] });

  useEffect(() => {
    fetchPublicRoomData();
  }, [roomId]);

  const fetchPublicRoomData = async () => {
    setLoading(true);
    try {
      const response = await fetch(`${API_BASE}/api/rooms/public/${roomId}/graph`);
      if (response.ok) {
        const data = await response.json();
        console.log('🔍 Raw data from API:', data);
        console.log('📊 Nodes received:', data.nodes?.length || 0);
        console.log('🔗 Edges received:', data.edges?.length || 0);
        console.log('⚡ Challenges received:', data.challenges?.length || 0);
        
        setRoom(data.room);
        
        // Transform the data for the graph
        const transformedData = transformGraph(data);
        console.log('🎨 Transformed graph data:', transformedData);
        console.log('📈 Final nodes count:', transformedData.nodes?.length || 0);
        console.log('🔀 Final links count:', transformedData.links?.length || 0);
        
        setGraphData(transformedData);
        console.log('💾 GraphData has been set!', transformedData);
        setError(null);
      } else if (response.status === 404) {
        setError('Debate not found');
      } else if (response.status === 403) {
        setError('This debate is not public');
      } else {
        setError('Failed to load debate');
      }
    } catch (err) {
      console.error('Error fetching public room data:', err);
      setError('Failed to connect to server');
    } finally {
      setLoading(false);
    }
  };

  const goBack = () => {
    navigate('/public-debates');
  };

  const goHome = () => {
    navigate('/');
  };

  // Graph styling - copied from GraphView but simplified for read-only
  const nodeCanvasObject = (node, ctx, globalScale) => {
    if (!ctx || typeof ctx.measureText !== 'function') {
      console.warn('Invalid canvas context provided to nodeCanvasObject');
      return;
    }

    const size = 4; // Same size as GraphView
    
    // Simple purple color for all nodes (no interaction states)
    const nodeColor = '#8b5cf6'; // Default purple like GraphView
    
    // Draw main node
    ctx.beginPath();
    ctx.arc(node.x, node.y, size, 0, 2 * Math.PI);
    ctx.fillStyle = nodeColor;
    ctx.fill();
    
    // Draw border
    ctx.strokeStyle = 'rgba(255,255,255,0.3)';
    ctx.lineWidth = 1;
    ctx.stroke();
    
    // Draw node label (same as GraphView)
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

  const linkCanvasObject = (link, ctx, globalScale) => {
    if (!ctx || typeof ctx.strokeStyle === 'undefined') {
      console.warn('Invalid canvas context provided to linkCanvasObject');
      return;
    }

    // Simple purple color for all edges (same as GraphView default)
    const lineColor = '#8b5cf6'; // Default purple
    
    // Set line style (same as GraphView)
    ctx.lineWidth = Math.max(1, 1 / globalScale);
    ctx.strokeStyle = lineColor;
    
    // Draw the link
    ctx.beginPath();
    ctx.moveTo(link.source.x, link.source.y);
    ctx.lineTo(link.target.x, link.target.y);
    ctx.stroke();
  };

  if (loading) {
    return (
      <div className="public-viewer-page">
        <div className="loading-state">
          <div className="loading-spinner"></div>
          <p>Loading debate...</p>
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="public-viewer-page">
        <div className="error-state">
          <h2>❌ {error}</h2>
          <div className="error-actions">
            <button onClick={goBack} className="action-button">
              ← Back to Public Debates
            </button>
            <button onClick={goHome} className="action-button">
              🏠 Home
            </button>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="public-viewer-page">
      <div className="viewer-header">
        <button onClick={goBack} className="back-button">
          ← Back to Public Debates
        </button>
        
        <div className="debate-info">
          <h1>{room?.name}</h1>
          {room?.description && (
            <p className="debate-description">{room.description}</p>
          )}
          <span className="debate-meta">
            📅 Created on {new Date(room?.createdAt).toLocaleDateString()}
          </span>
        </div>

        <div className="viewer-badge">
          👁️ Viewing Mode
        </div>
      </div>

      <div className="graph-container">
        {graphData.nodes.length === 0 ? (
          <div className="empty-graph">
            <h3>No content yet</h3>
            <p>This debate hasn't started yet. Check back later!</p>
          </div>
        ) : (
          <ForceGraph2D
            ref={graphRef}
            graphData={graphData}
            nodeCanvasObject={nodeCanvasObject}
            linkCanvasObject={linkCanvasObject}
            nodePointerAreaPaint={nodeCanvasObject}
            backgroundColor="rgba(15, 23, 42, 0.95)"
            linkDirectionalParticles={0}
            cooldownTicks={100}
            onEngineStop={() => graphRef.current?.zoomToFit(400)}
            enableZoomInteraction={true}
            enablePanInteraction={true}
            enableNodeDrag={false}
            nodeRelSize={6}
            linkWidth={1}
          />
        )}
      </div>

      <div className="viewer-info">
        <div className="info-card">
          <h3>🔍 How to Explore</h3>
          <ul>
            <li>Zoom in/out with your mouse wheel</li>
            <li>Pan around by clicking and dragging</li>
            <li>View the connections between ideas</li>
            <li>This is a read-only view - no editing allowed</li>
          </ul>
        </div>
        
        <div className="info-card">
          <h3>💡 About This Debate</h3>
          <p>
            This is a public debate that you can explore but not participate in. 
            Each node represents an idea or argument, and edges show how they connect.
          </p>
        </div>
      </div>
    </div>
  );
}
