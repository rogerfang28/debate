import { NodeModel } from '../../models/Node.js';

export async function exportNodes(req, res, next) {
  try {
    const nodes = await NodeModel
      .find({ room: req.params.roomId })
      .lean();                      // plain JSON, no Mongoose docs

    res.setHeader('Content-Type', 'application/json');
    res.setHeader(
      'Content-Disposition',
      'attachment; filename=nodes.json'
    );
    res.end(JSON.stringify(nodes, null, 2));
  } catch (err) {
    next(err);                     // let your global error handler deal with it
  }
}
