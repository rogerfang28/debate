// src/components/EntityForm.jsx
/*
This is a pop-up that appears when adding a node or challenging something.
It asks for a statement for nodes and reasoning for challenges.
*/

import React, { useState, useEffect } from 'react';

export default function EntityForm({ form, onSubmit, onCancel, roomId }) {
  console.log('🚨 ENTITYFORM COMPONENT LOADED!!!', { form, roomId });
  console.log('🎬 EntityForm rendered with props:', { form, roomId, formType: form?.type });
  
  const isNode = form.type === 'node';
  const isChallenge = form.type?.startsWith('challenge');

  const [text, setText] = useState(isNode ? '' : form.item?.relation || '');
  const [reason, setReason] = useState('');
  const [responder, setResponder] = useState('');

  const handleSubmit = e => {
    e.preventDefault();
    onSubmit({ label: text, reason, responder });
  };

  return (
    <div style={{
      position: 'fixed',
      top: '0',
      left: '0',
      width: '100vw',
      height: '100vh',
      background: 'rgba(0, 0, 0, 0.7)',
      backdropFilter: 'blur(4px)',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
      zIndex: 200
    }}>
      <div style={{
        background: 'linear-gradient(135deg, #1e293b 0%, #334155 100%)',
        color: '#fff',
        padding: '32px',
        borderRadius: '12px',
        border: '1px solid rgba(255, 255, 255, 0.1)',
        boxShadow: '0 20px 40px rgba(0, 0, 0, 0.3)',
        minWidth: '400px',
        maxWidth: '500px'
      }}>
      <form onSubmit={handleSubmit}>
        <h3 style={{
          marginTop: 0,
          marginBottom: '24px',
          fontSize: '20px',
          fontWeight: '600',
          background: 'linear-gradient(135deg, #60a5fa, #a78bfa)',
          backgroundClip: 'text',
          WebkitBackgroundClip: 'text',
          color: 'transparent'
        }}>
          {isNode ? (form.parent ? '🔗 New Connected Node' : '➕ New Node') :
           form.type === 'challenge-node' ? '⚡ Challenge Node' :
           '⚡ Challenge Edge'}
        </h3>

        {isNode && (
          <input
            required
            type="text"
            placeholder="Enter node statement..."
            value={text}
            onChange={e => setText(e.target.value)}
            style={{ 
              width: '100%', 
              marginBottom: '20px', 
              padding: '12px 16px',
              background: 'rgba(255, 255, 255, 0.1)',
              border: '1px solid rgba(255, 255, 255, 0.2)',
              borderRadius: '8px',
              color: 'white',
              fontSize: '14px',
              outline: 'none',
              transition: 'all 0.2s ease'
            }}
            onFocus={(e) => {
              e.target.style.borderColor = 'rgba(99, 102, 241, 0.5)';
              e.target.style.boxShadow = '0 0 0 3px rgba(99, 102, 241, 0.1)';
            }}
            onBlur={(e) => {
              e.target.style.borderColor = 'rgba(255, 255, 255, 0.2)';
              e.target.style.boxShadow = 'none';
            }}
          />
        )}

        {isChallenge && (
          <>
            <input
              required
              type="text"
              placeholder="Enter username to challenge..."
              value={responder}
              onChange={e => setResponder(e.target.value)}
              style={{ 
                width: '100%', 
                marginBottom: '16px', 
                padding: '12px 16px',
                background: 'rgba(255, 255, 255, 0.1)',
                border: '1px solid rgba(255, 255, 255, 0.2)',
                borderRadius: '8px',
                color: 'white',
                fontSize: '14px',
                outline: 'none',
                transition: 'all 0.2s ease'
              }}
              onFocus={(e) => {
                e.target.style.borderColor = 'rgba(99, 102, 241, 0.5)';
                e.target.style.boxShadow = '0 0 0 3px rgba(99, 102, 241, 0.1)';
              }}
              onBlur={(e) => {
                e.target.style.borderColor = 'rgba(255, 255, 255, 0.2)';
                e.target.style.boxShadow = 'none';
              }}
            />
            <textarea
              required
              placeholder="Enter your reasoning for this challenge..."
              value={reason}
              onChange={e => setReason(e.target.value)}
              style={{ 
                width: '100%', 
                marginBottom: '20px', 
                padding: '12px 16px', 
                height: '100px',
                background: 'rgba(255, 255, 255, 0.1)',
                border: '1px solid rgba(255, 255, 255, 0.2)',
                borderRadius: '8px',
                color: 'white',
                fontSize: '14px',
                outline: 'none',
                resize: 'vertical',
                transition: 'all 0.2s ease',
                fontFamily: 'inherit'
              }}
              onFocus={(e) => {
                e.target.style.borderColor = 'rgba(99, 102, 241, 0.5)';
                e.target.style.boxShadow = '0 0 0 3px rgba(99, 102, 241, 0.1)';
              }}
              onBlur={(e) => {
                e.target.style.borderColor = 'rgba(255, 255, 255, 0.2)';
                e.target.style.boxShadow = 'none';
              }}
            />
          </>
        )}

        <div style={{ display: 'flex', justifyContent: 'flex-end', gap: '12px', marginTop: '24px' }}>
          <button 
            type="submit"
            style={{
              padding: '12px 24px',
              background: 'linear-gradient(135deg, #059669, #10b981)',
              border: 'none',
              borderRadius: '8px',
              color: 'white',
              fontSize: '14px',
              fontWeight: '600',
              cursor: 'pointer',
              transition: 'all 0.2s ease',
              boxShadow: '0 4px 12px rgba(16, 185, 129, 0.3)'
            }}
            onMouseOver={(e) => {
              e.target.style.transform = 'translateY(-1px)';
              e.target.style.boxShadow = '0 6px 16px rgba(16, 185, 129, 0.4)';
            }}
            onMouseOut={(e) => {
              e.target.style.transform = 'translateY(0)';
              e.target.style.boxShadow = '0 4px 12px rgba(16, 185, 129, 0.3)';
            }}
          >
            ✓ Submit
          </button>
          <button 
            type="button" 
            onClick={onCancel}
            style={{
              padding: '12px 24px',
              background: 'rgba(255, 255, 255, 0.1)',
              border: '1px solid rgba(255, 255, 255, 0.2)',
              borderRadius: '8px',
              color: 'white',
              fontSize: '14px',
              fontWeight: '600',
              cursor: 'pointer',
              transition: 'all 0.2s ease'
            }}
            onMouseOver={(e) => {
              e.target.style.background = 'rgba(255, 255, 255, 0.2)';
              e.target.style.transform = 'translateY(-1px)';
            }}
            onMouseOut={(e) => {
              e.target.style.background = 'rgba(255, 255, 255, 0.1)';
              e.target.style.transform = 'translateY(0)';
            }}
          >
            ✕ Cancel
          </button>
        </div>
      </form>
    </div>
    </div>
  );
}
