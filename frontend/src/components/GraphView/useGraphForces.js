// src/features/graph/hooks/useGraphForces.js
import { useEffect } from "react";
import * as d3 from "d3";

/**
 * React hook that (re)configures the d3‑forces each render. Isolated so the
 * physics logic lives outside of React component bodies.
 */
export default function useGraphForces(fgRef, draggedNodeId, safeGraph) {
  useEffect(() => {
    if (!fgRef.current) return;

    // Neutral charge unless we toggle "Spread" mode elsewhere.
    fgRef.current.d3Force("charge", d3.forceManyBody().strength(0));

    // When a node is dragged we pin it (fx/fy) so it doesn't spring back.
    if (draggedNodeId) {
      fgRef.current.d3ReheatSimulation();
      const n = safeGraph.nodes.find((x) => x.id === draggedNodeId);
      if (n) {
        n.fx = n.x;
        n.fy = n.y;
      }
    } else {
      safeGraph.nodes.forEach((n) => {
        delete n.fx;
        delete n.fy;
      });
    }
  }, [draggedNodeId, safeGraph.nodes]);
}
