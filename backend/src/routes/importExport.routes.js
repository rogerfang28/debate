import express from 'express';
import { exportNodes }  from '../controllers/export/exportNodes.js';
import { exportEdges }  from '../controllers/export/exportEdges.js';
import multer from 'multer';               // will be used in Stage 3 (import)

const router  = express.Router({ mergeParams: true });

/* ---------- EXPORT ROUTES ---------- */
router.get('/:roomId/export/nodes',  exportNodes);
router.get('/:roomId/export/edges',  exportEdges);

/* ---------- IMPORT ROUTES (place-holders) ---------- */
/* Stage 3 will add:
   router.post('/:roomId/import/nodes', upload.single('file'), importNodes);
   router.post('/:roomId/import/edges', upload.single('file'), importEdges);
*/

export default router;
