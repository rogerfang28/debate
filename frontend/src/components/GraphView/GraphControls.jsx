// src/features/graph/components/GraphControls.jsx
import React from "react";
import * as d3 from "d3";
import { socket } from "../../lib/socket.js";
import ControlButton from "./ControlButton.jsx";

export default function GraphControls({
  fgRef,
  safeGraph,
  roomId,
  isSpreadOut,
  setIsSpreadOut,
}) {
  const centerGraph = () => {
    if (!fgRef.current || safeGraph.nodes.length === 0) return;
    safeGraph.nodes.forEach((n) => {
      if (n.x !== undefined && n.y !== undefined) {
        n.x *= 0.1;
        n.y *= 0.1;
        socket.emit("update-node-position", {
          id: n.id,
          x: n.x,
          y: n.y,
          room: roomId,
        });
      }
    });
    fgRef.current.d3ReheatSimulation();
  };

  const gravitate = () => {
    if (!fgRef.current) return;
    fgRef.current.d3Force("center", d3.forceCenter(0, 0).strength(0.1));
    fgRef.current.d3ReheatSimulation();
  };

  const toggleSpread = () => {
    if (!fgRef.current) return;
    if (isSpreadOut) {
      fgRef.current.d3Force("charge", d3.forceManyBody().strength(0));
      fgRef.current.d3Force("collision", null);
    } else {
      fgRef.current.d3Force("charge", d3.forceManyBody().strength(-300));
      fgRef.current.d3Force("collision", d3.forceCollide().radius(30));
    }
    setIsSpreadOut(!isSpreadOut);
    fgRef.current.d3ReheatSimulation();
  };

  const zoomToFit = () => fgRef.current?.zoomToFit(400, 40);

  return (
    <div
      style={{
        position: "absolute",
        top: 64,
        right: 16,
        zIndex: 10,
        display: "flex",
        gap: 8,
      }}
    >
      <ControlButton
        onClick={centerGraph}
        style={{
          background:
            "linear-gradient(135deg, rgba(59,130,246,.9), rgba(99,102,241,.9))",
          borderColor: "rgba(99,102,241,.3)",
        }}
      >
        🎯 Center
      </ControlButton>

      <ControlButton
        onClick={gravitate}
        style={{
          background:
            "linear-gradient(135deg, rgba(16,185,129,.9), rgba(5,150,105,.9))",
          borderColor: "rgba(16,185,129,.3)",
        }}
      >
        🧲 Gravitate
      </ControlButton>

      <ControlButton
        onClick={toggleSpread}
        style={{
          background: isSpreadOut
            ? "linear-gradient(135deg, rgba(239,68,68,.9), rgba(220,38,38,.9))"
            : "linear-gradient(135deg, rgba(168,85,247,.9), rgba(147,51,234,.9))",
          borderColor: isSpreadOut ? "rgba(239,68,68,.3)" : "rgba(168,85,247,.3)",
        }}
      >
        {isSpreadOut ? "📉 Compact" : "📈 Spread"}
      </ControlButton>

      <ControlButton
        onClick={zoomToFit}
        style={{
          background:
            "linear-gradient(135deg, rgba(245,158,11,.9), rgba(217,119,6,.9))",
          borderColor: "rgba(245,158,11,.3)",
        }}
      >
        🔍 Fit
      </ControlButton>
    </div>
  );
}
