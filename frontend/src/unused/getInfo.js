// import * as services from './protos/data_grpc_web_pb.js';
// import * as messages from './protos/data_pb.js';

// const { DataServiceClient } = services;
// const { MyData } = messages;

// export default function getInfo() {
//   return new Promise((resolve, reject) => {
//     const client = new DataServiceClient('http://localhost:8080');

//     const request = new MyData();
//     request.setUsername('adadd');
//     request.setScore(42);

//     client.sendData(request, {}, (err, response) => {
//       if (err) {
//         console.error('❌ Error:', err.message);
//         reject(err);
//         return;
//       }
//       console.log('✅ Response from server:', response.toObject());
//       resolve(response.toObject());
//     });
//   });
// }
