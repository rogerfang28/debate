import { useEffect } from 'react';

export function useWindowResize(setDimensions) {
  useEffect(() => {
    const resize = () => setDimensions({
      width: window.innerWidth,
      height: window.innerHeight
    });
    window.addEventListener('resize', resize);
    return () => window.removeEventListener('resize', resize);
  }, [setDimensions]);
}
