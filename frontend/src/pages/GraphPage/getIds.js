export function getIds(item) {
  return {
    src: item.source.id || item.source,
    tgt: item.target.id || item.target
  };
}
