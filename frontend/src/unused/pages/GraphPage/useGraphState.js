import { useState } from 'react';

export function useGraphState() {
  const [dimensions, setDimensions] = useState({
    width: window.innerWidth,
    height: window.innerHeight
  });

  const [data, setData] = useState({ nodes: [], links: [] });
  const [selection, setSelection] = useState({ type: null, item: null });
  const [form, setForm] = useState(null);

  return {
    dimensions,
    setDimensions,
    data,
    setData,
    selection,
    setSelection,
    form,
    setForm
  };
}
