// src/components/GraphView.jsx
import React, { useEffect, useRef } from 'react';
import ForceGraph2D from 'react-force-graph-2d';

export default function GraphView({ graphData, dimensions }) {
  const fgRef = useRef();

  useEffect(() => {
    fgRef.current?.zoomToFit(400, 40);
  }, [graphData, dimensions]);

  return (
    <div style={{ width: dimensions.width, height: dimensions.height }}>
      <ForceGraph2D
        ref={fgRef}
        graphData={graphData}
        nodeLabel="text"
        nodeAutoColorBy="id"
        width={dimensions.width}
        height={dimensions.height}
        d3AlphaDecay={0.05}
        d3VelocityDecay={0.4}
        d3Force="charge"
        d3ForceFn={d3.forceManyBody().strength(-100)}
      />
    </div>
  );
}
