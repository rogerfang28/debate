import React from "react";

export default function TableComponent({ component, className, style, events }) {
  return (
    <table className={className} style={style} {...events}>
      <tbody>
        {component.items?.map((row, rowIndex) => (
          <tr key={rowIndex}>
            {Object.values(row).map((cell, cellIndex) => (
              <td key={cellIndex}>{cell}</td>
            ))}
          </tr>
        ))}
      </tbody>
    </table>
  );
}
