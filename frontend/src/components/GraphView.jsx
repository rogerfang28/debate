// GraphView.jsx should make the graph appear on the screen
import React, { useEffect, useRef, useState } from 'react';
import ForceGraph2D from 'react-force-graph-2d';
import * as d3 from 'd3';

export default function GraphView({ graphData, dimensions }) {
  const fgRef = useRef();
  const [shouldZoom, setShouldZoom] = useState(true); // ensure zoom only runs once

  useEffect(() => {
    if (fgRef.current) {
      fgRef.current.d3Force('charge', d3.forceManyBody().strength(0));
    }
  }, [graphData, dimensions]);

  return (
    <div style={{ position: 'fixed', top: 0, left: 0, width: '100vw', height: '100vh' }}>
      <ForceGraph2D
        onNodeClick={node => setSelection({ type: 'node', item: node })}
        onLinkClick={link => setSelection({ type: 'edge', item: link })}
        ref={fgRef}
        graphData={graphData}
        nodeLabel="text"
        nodeAutoColorBy="id"
        width={dimensions.width}
        height={dimensions.height}
        d3AlphaDecay={0.05}
        d3VelocityDecay={0.4}
        onEngineStop={() => {
          if (shouldZoom && fgRef.current) {
            fgRef.current.zoomToFit(400, 40);
            setShouldZoom(false); // prevent infinite zoom calls
          }
        }}
      />
    </div>
  );
}
