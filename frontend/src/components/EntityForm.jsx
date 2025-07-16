// src/components/EntityForm.jsx
/*
This is a pop-up that appears when adding a node or challenging something.
It asks for a statement for nodes and reasoning for challenges.
*/

import React, { useState } from 'react';

export default function EntityForm({ form, onSubmit, onCancel }) {
  const isNode = form.type === 'node';
  const isChallenge = form.type?.startsWith('challenge');

  const [text, setText] = useState(isNode ? '' : form.item?.relation || '');
  const [reason, setReason] = useState('');

  const handleSubmit = e => {
    e.preventDefault();
    onSubmit({ label: text, reason });
  };

  return (
    <div style={{
      position: 'fixed',
      top: '40%',
      left: '40%',
      transform: 'translate(-40%, -40%)',
      backgroundColor: '#333',
      color: '#fff',
      padding: 20,
      border: '1px solid gray',
      borderRadius: 8,
      zIndex: 200
    }}>
      <form onSubmit={handleSubmit}>
        <h3>
          {isNode ? (form.parent ? 'New Connected Node' : 'New Node') :
           form.type === 'challenge-node' ? 'Challenge Node' :
           'Challenge Edge'}
        </h3>

        {isNode && (
          <input
            required
            type="text"
            placeholder="Node label"
            value={text}
            onChange={e => setText(e.target.value)}
            style={{ width: '100%', marginBottom: 10, padding: 6 }}
          />
        )}

        {isChallenge && (
          <textarea
            required
            placeholder="Reasoning"
            value={reason}
            onChange={e => setReason(e.target.value)}
            style={{ width: '100%', marginBottom: 10, padding: 6, height: 80 }}
          />
        )}

        <div style={{ display: 'flex', justifyContent: 'flex-end', gap: 10 }}>
          <button type="submit">Submit</button>
          <button type="button" onClick={onCancel}>Cancel</button>
        </div>
      </form>
    </div>
  );
}
