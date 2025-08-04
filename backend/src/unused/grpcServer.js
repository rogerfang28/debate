import grpc from '@grpc/grpc-js';
import protoLoader from '@grpc/proto-loader';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Load proto definition
const packageDef = protoLoader.loadSync(
  path.join(__dirname, '../../protos/data.proto'),
  {
    keepCase: true,
    longs: String,
    enums: String,
    defaults: true,
    oneofs: true
  }
);

const proto = grpc.loadPackageDefinition(packageDef).test;

// gRPC method implementation
function sendData(call, callback) {
  console.log("📩 Received from client:", call.request);
  // Just echo back what was sent
  callback(null, call.request);
}

// Create server
const server = new grpc.Server();
server.addService(proto.DataService.service, { sendData });

// Start listening
const PORT = 50051;
server.bindAsync(`0.0.0.0:${PORT}`, grpc.ServerCredentials.createInsecure(), () => {
  console.log(`🚀 gRPC server running on port ${PORT}`);
});
