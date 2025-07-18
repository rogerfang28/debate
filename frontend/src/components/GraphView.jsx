// Status: Done and functional
// GraphView.jsx should make the graph appear on the screen
import React, { useEffect, useRef, useState } from 'react';
import ForceGraph2D from 'react-force-graph-2d';
import * as d3 from 'd3';
import { socket } from '../lib/socket';

export default function GraphView({ graphData, dimensions, selection, setSelection }) {
  const fgRef = useRef();
  const [shouldZoom, setShouldZoom] = useState(true);

  const safeGraph = {
    nodes: Array.isArray(graphData?.nodes) ? graphData.nodes : [],
    links: Array.isArray(graphData?.links) ? graphData.links : []
  };

  useEffect(() => {
    if (fgRef.current) {
      fgRef.current.d3Force('charge', d3.forceManyBody().strength(0));
    }
  }, [graphData, dimensions]);

  return (
    <div style={{ position: 'fixed', top: 0, left: 0, width: '100vw', height: '100vh' }}>
      <ForceGraph2D
        ref={fgRef}
        graphData={safeGraph}
        nodeLabel="label"
        nodeAutoColorBy="id"
        width={dimensions.width}
        height={dimensions.height}
        nodeRelSize={4}
        d3AlphaDecay={0.05}
        d3VelocityDecay={0.4}
        enableNodeDrag={true}
        onNodeClick={node => setSelection({ type: 'node', item: node })}
        onLinkClick={link => setSelection({ type: 'edge', item: link })}
        onNodeDragEnd={node => {
          socket.emit('update-node-position', {
            id: node.id,
            x: node.x,
            y: node.y
          });
        }}
        onNodeHover={node => {
          if (node) console.log('Hovered node:', node);
        }} // debugging
        onEngineStop={() => {
          if (shouldZoom && fgRef.current) {
            fgRef.current.zoomToFit(400, 40);
            setShouldZoom(false);
          }
        }}
      />
    </div>
  );
}
