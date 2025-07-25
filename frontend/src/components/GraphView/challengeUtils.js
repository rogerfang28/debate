// src/features/graph/utils/challengeUtils.js
/**
 * Returns meta about a node/edge's challenge status relative to the current
 * user. Keeps this logic out of rendering code so it's easy to unit‑test.
 */
export const getChallengeStatus = (item, currentUserId) => {
  const pending = (item.challenges || []).filter((c) => c.status === "pending");
  const isChallenger = pending.some((c) => c.challenger?._id === currentUserId);
  const isResponder = pending.some((c) => c.responder?._id === currentUserId);

  return {
    pending,
    isChallenger,
    isResponder,
    hasPending: pending.length > 0,
  };
};
