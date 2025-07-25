import fetch from 'node-fetch';

const token = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOiI2ODY5ZTUzNmY4NWFmMGI1MjNmNTAwY2YiLCJlbWFpbCI6InBhdWxAZ21haWwuY29tIiwiaWF0IjoxNzM3MjQ3MTM0LCJleHAiOjE3MzcyNTA3MzR9.y4L2WLBjeMZ6KeOLcLs4lfxhDZFvBJJrZJKKMcSBP_A';
const roomId = '687b386af85af0b523f500db';

console.log('Testing API endpoints...');

try {
  // Test the test endpoint first
  console.log('\n1. Testing test endpoint...');
  const testResponse = await fetch(`http://localhost:3000/api/rooms/${roomId}/test`, {
    headers: { 'Authorization': `Bearer ${token}` }
  });
  console.log('Test endpoint status:', testResponse.status);
  const testData = await testResponse.text();
  console.log('Test endpoint response:', testData);

  // Test the members endpoint
  console.log('\n2. Testing members endpoint...');
  const membersResponse = await fetch(`http://localhost:3000/api/rooms/${roomId}/members`, {
    headers: { 'Authorization': `Bearer ${token}` }
  });
  console.log('Members endpoint status:', membersResponse.status);
  console.log('Members endpoint headers:', Object.fromEntries(membersResponse.headers.entries()));
  const membersData = await membersResponse.text();
  console.log('Members endpoint response:', membersData);

} catch (error) {
  console.error('Error testing API:', error);
}
