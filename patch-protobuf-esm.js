import fs from "fs";

const files = [
  "backend/src/protos/page_pb.js",
  "frontend/src/protos/page_pb.js"
];

for (const file of files) {
  let code = fs.readFileSync(file, "utf8");
  code = code.replace(
    /from "protobufjs\/minimal"/g,
    'from "protobufjs/minimal.js"'
  );
  fs.writeFileSync(file, code);
  console.log(`✅ Patched imports in ${file}`);
}
