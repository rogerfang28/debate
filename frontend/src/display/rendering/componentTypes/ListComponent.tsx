import React from "react";
import { BaseComponentProps } from "./TextComponent";
import buildClassName from "../buildClassName";
import handleEvent from "../../events/handleEvent";

interface ListItem {
  icon?: string;
  label?: string;
  style?: any;
  [key: string]: any;
}

const ListComponent: React.FC<BaseComponentProps> = ({ component, className, style }) => {
  const handleItemClick = (e: React.MouseEvent, item: ListItem, idx: number) => {
    console.log(`📝 List "${component.id}" item ${idx} clicked:`, item.label);
    handleEvent(
      e as any,
      { ...component, value: item.label, text: item.label },
      'onClick',
      `${component.id}-item-${idx}` || 'unknown-list-item'
    );
  };

  const items: ListItem[] = component.items || [];

  return (
    <ul
      id={component.id}
      className={className}
      style={{
        ...style,
        display: 'flex',
        flexDirection: 'column' as const,
        gap: '2px',
        borderRadius: 'var(--radius-lg)',
        overflow: 'hidden',
        background: 'var(--surface-card)',
        border: '1px solid var(--border-subtle)',
        padding: '4px',
      }}
    >
      {items.map((item, idx) => {
        const itemClassName = buildClassName(item.style);
        return (
          <li
            key={idx}
            className={`flex items-center gap-3 px-4 py-3 rounded-md cursor-pointer transition-colors ${itemClassName}`}
            style={{
              color: 'var(--text-primary)',
              fontSize: '0.875rem',
            }}
            onMouseEnter={(e) => {
              (e.currentTarget as HTMLElement).style.background = 'var(--surface-hover)';
            }}
            onMouseLeave={(e) => {
              (e.currentTarget as HTMLElement).style.background = 'transparent';
            }}
            onClick={(e) => handleItemClick(e, item, idx)}
          >
            {item.icon && (
              <span
                className={`icon ${item.icon}`}
                style={{ color: 'var(--text-muted)', fontSize: '1rem', flexShrink: 0 }}
              />
            )}
            <span style={{ flex: 1 }}>{item.label}</span>
            <svg
              width="16" height="16" viewBox="0 0 24 24" fill="none"
              stroke="var(--text-muted)" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"
              style={{ opacity: 0.4, flexShrink: 0 }}
            >
              <path d="M9 18l6-6-6-6" />
            </svg>
          </li>
        );
      })}
    </ul>
  );
};

export default ListComponent;
