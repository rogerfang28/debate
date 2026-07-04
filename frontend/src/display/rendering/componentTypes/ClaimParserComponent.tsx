import React, { useState } from "react";
import { BaseComponentProps } from "./TextComponent";

interface ClaimParserComponentProps extends BaseComponentProps {}

const ClaimParserComponent: React.FC<ClaimParserComponentProps> = ({ component, className, style }) => {
  const [mode, setMode] = useState<'url' | 'text'>('url');
  const [inputValue, setInputValue] = useState(component.attributes?.defaultValue || '');
  const [parsedContent, setParsedContent] = useState<string | null>(null);

  const handleParse = () => {
    // PLACEHOLDER: Future integration will call the paragraph parser API here.
    // For now, display the input as preview content.
    setParsedContent(mode === 'url' ? `URL: ${inputValue}` : `Text (${inputValue.length} chars): ${inputValue.substring(0, 500)}`);
    console.log(`[ClaimParser] ${mode} submitted:`, inputValue);
  };

  const handleClear = () => {
    setInputValue('');
    setParsedContent(null);
  };

  return (
    <div
      className={`flex flex-col gap-4 p-5 rounded-xl border fade-in ${className || ''}`}
      style={{
        background: 'var(--surface-card)',
        border: '1px solid var(--border-default)',
        ...style,
        minHeight: '120px',
      }}
    >
      {/* Header */}
      <div className="flex items-center gap-3">
        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="var(--accent-indigo)" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
          <path d="M12 20h9" />
          <path d="M16.5 3.5a2.121 2.121 0 0 1 3 3L7 19l-4 1 1-4L16.5 3.5z" />
        </svg>
        <h3 style={{ color: 'var(--text-primary)', fontWeight: 600, fontSize: '1rem', margin: 0 }}>
          Claim Parser
        </h3>
        <span style={{ color: 'var(--text-muted)', fontSize: '0.75rem' }}>(placeholder)</span>
      </div>

      {/* Mode tabs */}
      <div className="flex gap-1 rounded-lg" style={{ background: 'var(--surface-input)' }}>
        <button
          onClick={() => { setMode('url'); setParsedContent(null); }}
          className={`flex-1 text-center text-sm font-medium px-4 py-2 rounded-md transition-opacity ${mode === 'url' ? 'opacity-100' : 'opacity-50 hover:opacity-75'}`}
          style={{ background: mode === 'url' ? 'var(--surface-elevated)' : 'transparent', color: 'var(--text-secondary)' }}
        >
          🔗 URL
        </button>
        <button
          onClick={() => { setMode('text'); setParsedContent(null); }}
          className={`flex-1 text-center text-sm font-medium px-4 py-2 rounded-md transition-opacity ${mode === 'text' ? 'opacity-100' : 'opacity-50 hover:opacity-75'}`}
          style={{ background: mode === 'text' ? 'var(--surface-elevated)' : 'transparent', color: 'var(--text-secondary)' }}
        >
          📝 Text
        </button>
      </div>

      {/* Input */}
      {mode === 'url' ? (
        <input
          type="url"
          placeholder="Enter article URL..."
          value={inputValue}
          onChange={(e) => setInputValue(e.target.value)}
          style={{
            background: 'var(--surface-input)',
            border: '1px solid var(--border-default)',
            color: 'var(--text-primary)',
            padding: '0.625rem 0.75rem',
            borderRadius: '0.5rem',
            fontSize: '0.875rem',
            outline: 'none',
          }}
          onFocus={(e) => (e.target.style.borderColor = 'var(--border-focus)')}
          onBlur={(e) => (e.target.style.borderColor = 'var(--border-default)')}
        />
      ) : (
        <textarea
          placeholder="Paste article text or claims here..."
          value={inputValue}
          onChange={(e) => setInputValue(e.target.value)}
          rows={4}
          style={{
            background: 'var(--surface-input)',
            border: '1px solid var(--border-default)',
            color: 'var(--text-primary)',
            padding: '0.625rem 0.75rem',
            borderRadius: '0.5rem',
            fontSize: '0.875rem',
            resize: 'vertical',
            outline: 'none',
            fontFamily: 'inherit',
          }}
          onFocus={(e) => (e.target.style.borderColor = 'var(--border-focus)')}
          onBlur={(e) => (e.target.style.borderColor = 'var(--border-default)')}
        />
      )}

      {/* Actions */}
      <div className="flex gap-2">
        <button
          onClick={handleParse}
          disabled={!inputValue.trim()}
          className="text-sm font-medium px-4 py-2 rounded-md transition-opacity"
          style={{
            background: inputValue.trim() ? 'var(--accent-indigo)' : 'var(--surface-elevated)',
            color: inputValue.trim() ? '#fff' : 'var(--text-muted)',
            border: '1px solid var(--border-default)',
          }}
        >
          Extract &amp; Render
        </button>
        <button
          onClick={handleClear}
          className="text-sm px-3 py-2 rounded-md transition-opacity"
          style={{
            background: 'var(--surface-elevated)',
            color: 'var(--text-secondary)',
            border: '1px solid var(--border-default)',
          }}
        >
          Clear
        </button>
      </div>

      {/* Preview / result area */}
      {parsedContent && (
        <div
          className="rounded-lg p-3 border"
          style={{ background: 'var(--surface-elevated)', border: '1px solid var(--border-subtle)' }}
        >
          <p style={{ color: 'var(--text-secondary)', fontSize: '0.8rem' }}>{parsedContent}</p>
          <p style={{ color: 'var(--text-muted)', fontSize: '0.7rem', marginTop: '0.5rem', fontStyle: 'italic' }}>
            Parsing output will be rendered here after API integration.
          </p>
        </div>
      )}

      {/* Empty state */}
      {!parsedContent && (
        <div className="flex items-center justify-center" style={{ minHeight: '40px' }}>
          <p style={{ color: 'var(--text-muted)', fontSize: '0.8rem' }}>
            Enter a URL or paste text, then click Extract.
          </p>
        </div>
      )}
    </div>
  );
};

export default ClaimParserComponent;
