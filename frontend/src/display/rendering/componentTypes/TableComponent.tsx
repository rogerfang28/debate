import React from "react";
import { BaseComponentProps } from "./TextComponent";

const TableComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  return (
    <table id={component.id} className={className} style={style}>
      <tbody>
        {component.items?.map((row: any, rowIndex: number) => (
          <tr key={rowIndex}>
            {Object.values(row).map((cell: any, cellIndex: number) => (
              <td key={cellIndex}>{cell}</td>
            ))}
          </tr>
        ))}
      </tbody>
    </table>
  );
};

export default TableComponent;
