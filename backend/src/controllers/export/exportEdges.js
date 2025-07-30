import { EdgeModel } from '../../models/Edge.js';

export async function exportEdges(req, res, next) {
  try {
    const edges = await EdgeModel
      .find({ room: req.params.roomId })
      .lean();

    res.setHeader('Content-Type', 'application/json');
    res.setHeader(
      'Content-Disposition',
      'attachment; filename=edges.json'
    );
    res.end(JSON.stringify(edges, null, 2));
  } catch (err) {
    next(err);
  }
}
