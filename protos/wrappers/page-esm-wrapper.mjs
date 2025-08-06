// page-esm-wrapper.mjs
import { createRequire } from 'module';
const require = createRequire(import.meta.url);
const pkg = require('../unused/page_pb.js');

export const { PageSchema, ComponentType } = pkg;
