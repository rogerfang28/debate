// src/features/graph/utils/renderUtils.js
import { COLORS, STYLES } from "./constants.js";
import { getChallengeStatus } from "./challengeUtils.js";

// -----------------------------------------------------------------------------
// Colour + style helpers
// -----------------------------------------------------------------------------
export const getNodeColor = (node, isSelected, isHovered, status) => {
  if (status.hasPending) {
    return status.isChallenger
      ? COLORS.CHALLENGER
      : status.isResponder
      ? COLORS.RESPONDER
      : COLORS.DEFAULT_NODE;
  }
  if (isSelected) return COLORS.SELECTED;
  if (isHovered) return COLORS.HOVERED;
  return node.color || COLORS.DEFAULT_NODE;
};

export const getBorderStyle = (isSelected, isHovered, status) => {
  if (isSelected) return { color: COLORS.BORDER.SELECTED, width: 2 };
  if (isHovered) return { color: COLORS.BORDER.HOVERED, width: 1.5 };
  if (status.hasPending) {
    return {
      color: status.isChallenger ? COLORS.BORDER.CHALLENGER : COLORS.BORDER.RESPONDER,
      width: 2,
    };
  }
  return { color: COLORS.BORDER.DEFAULT, width: 1 };
};

export const getShadowColor = (isSelected, isHovered, status) => {
  if (isSelected) return COLORS.SHADOW.SELECTED;
  if (isHovered) return COLORS.SHADOW.HOVERED;
  if (status.hasPending) {
    return status.isChallenger ? COLORS.SHADOW.CHALLENGER : COLORS.SHADOW.RESPONDER;
  }
  return null;
};

// -----------------------------------------------------------------------------
// Canvas render helpers (factory functions)
// -----------------------------------------------------------------------------

const renderNodeLabel = (ctx, node, globalScale) => {
  const fontSize = Math.max(10, 12 / globalScale);
  ctx.font = `${fontSize}px Inter, sans-serif`;

  const metrics = ctx.measureText(node.label);
  const textWidth = metrics.width;
  const textHeight = fontSize;
  const pad = 4;
  const textX = node.x;
  const textY = node.y - STYLES.NODE_SIZE - textHeight / 2 - pad - 2;

  ctx.fillStyle = STYLES.LABEL_BACKGROUND;
  ctx.fillRect(
    textX - textWidth / 2 - pad,
    textY - textHeight / 2 - pad / 2,
    textWidth + pad * 2,
    textHeight + pad,
  );

  ctx.fillStyle = STYLES.LABEL_TEXT;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(node.label, textX, textY);
};

export const createNodeRenderer = (selection, hoveredNode, currentUserId) => {
  return (node, ctx, globalScale) => {
    const isSelected = selection?.type === "node" && selection?.item?.id === node.id;
    const isHovered = hoveredNode?.id === node.id;
    const status = getChallengeStatus(node, currentUserId);

    const shadowColor = getShadowColor(isSelected, isHovered, status);
    if (shadowColor) {
      ctx.shadowBlur = STYLES.SHADOW_BLUR;
      ctx.shadowColor = shadowColor;
    }

    const nodeColor = getNodeColor(node, isSelected, isHovered, status);
    const sizeOffset =
      (isSelected ? 2 : 0) + (isHovered ? 1 : 0) + (status.hasPending ? 1 : 0);

    ctx.beginPath();
    ctx.arc(node.x, node.y, STYLES.NODE_SIZE + sizeOffset, 0, Math.PI * 2);
    ctx.fillStyle = nodeColor;
    ctx.fill();

    const border = getBorderStyle(isSelected, isHovered, status);
    ctx.strokeStyle = border.color;
    ctx.lineWidth = border.width;
    ctx.stroke();

    ctx.shadowBlur = 0;

    if (node.label) renderNodeLabel(ctx, node, globalScale);
  };
};

export const createLinkRenderer = (selection, hoveredLink, currentUserId) => {
  return (link, ctx, globalScale) => {
    const isSelected = selection?.type === "edge" && selection?.item?.id === link.id;
    const isHovered = hoveredLink?.id === link.id;
    const status = getChallengeStatus(link, currentUserId);

    const color = getNodeColor(link, isSelected, isHovered, status);
    const base = status.hasPending ? 2 : 1;
    const width = Math.max(1, (isSelected ? 2 : isHovered ? 1.5 : base) / globalScale);

    ctx.strokeStyle = color;
    ctx.lineWidth = width;
    ctx.beginPath();
    ctx.moveTo(link.source.x, link.source.y);
    ctx.lineTo(link.target.x, link.target.y);
    ctx.stroke();
  };
};
