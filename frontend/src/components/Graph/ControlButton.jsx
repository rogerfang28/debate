// src/features/graph/components/ControlButton.jsx
export default function ControlButton({ onClick, children, style = {} }) {
  return (
    <button
      onClick={onClick}
      style={{
        padding: "10px 14px",
        border: "1px solid",
        borderRadius: 8,
        color: "white",
        cursor: "pointer",
        fontSize: 13,
        fontWeight: 500,
        backdropFilter: "blur(10px)",
        boxShadow: "0 4px 12px rgba(0,0,0,.15)",
        transition: "all .2s ease",
        ...style,
      }}
    >
      {children}
    </button>
  );
}
