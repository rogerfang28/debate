import grpc from '@grpc/grpc-js';
import protoLoader from '@grpc/proto-loader';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const packageDef = protoLoader.loadSync(
  path.join(__dirname, '../../protos/data.proto'),
  { keepCase: true, longs: String, enums: String, defaults: true, oneofs: true }
);

const proto = grpc.loadPackageDefinition(packageDef).test;

const client = new proto.DataService(
  'localhost:50051',
  grpc.credentials.createInsecure()
);

const data = { username: 'adadd', score: 42 };

client.SendData(data, (err, response) => {
  if (err) {
    console.error('❌ Error:', err);
  } else {
    console.log('✅ Response from server:', response);
  }
});
