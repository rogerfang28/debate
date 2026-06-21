import React from "react";
import { BaseComponentProps } from "./TextComponent";

const TableComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const items = component.items || [];

  if (items.length === 0) return null;

  // Extract headers from first row keys
  const headers = Object.keys(items[0]);

  return (
    <div
      id={component.id}
      className={className}
      style={{
        ...style,
        overflow: 'auto',
        borderRadius: 'var(--radius-lg)',
        border: '1px solid var(--border-subtle)',
        background: 'var(--surface-card)',
      }}
    >
      <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: '0.875rem' }}>
        <thead>
          <tr style={{ borderBottom: '1px solid var(--border-subtle)' }}>
            {headers.map((header, i) => (
              <th
                key={i}
                style={{
                  padding: '0.75rem 1rem',
                  textAlign: 'left',
                  color: 'var(--text-secondary)',
                  fontWeight: 600,
                  fontSize: '0.75rem',
                  textTransform: 'uppercase',
                  letterSpacing: '0.05em',
                  background: 'var(--surface-elevated)',
                }}
              >
                {header}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {items.map((row, rowIndex) => (
            <tr
              key={rowIndex}
              style={{
                borderBottom: rowIndex < items.length - 1 ? '1px solid var(--border-subtle)' : 'none',
                transition: 'background var(--transition-fast)',
              }}
              onMouseEnter={(e) => {
                (e.currentTarget as HTMLElement).style.background = 'var(--surface-hover)';
              }}
              onMouseLeave={(e) => {
                (e.currentTarget as HTMLElement).style.background = 'transparent';
              }}
            >
              {Object.values(row).map((cell: any, cellIndex) => (
                <td
                  key={cellIndex}
                  style={{
                    padding: '0.75rem 1rem',
                    color: 'var(--text-primary)',
                  }}
                >
                  {String(cell)}
                </td>
              ))}
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
};

export default TableComponent;
