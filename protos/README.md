# Protobuf generation

## The command

From the **repo root** (not from `protos/`):

```bash
npx buf generate
```

That regenerates every `.proto` in this directory into both targets at once.
Configuration lives in [`buf.gen.yaml`](../buf.gen.yaml) at the repo root.

## First-time setup

`buf` and the TypeScript plugin are root devDependencies, so the root
`node_modules` must exist. Installing only `frontend/` is not enough:

```bash
npm install
```

If `npx buf generate` reports that `protoc-gen-es` cannot be found, this is why.

The C++ plugin is a **remote** plugin (`buf.build/protocolbuffers/cpp`), so
generation needs network access the first time it runs.

## What it writes

| Output | Consumed by |
|---|---|
| `src/gen/cpp/*.pb.{cc,h}` | Backend. Both `backend/CMakeLists.txt` and `test/CMakeLists.txt` glob `src/gen/cpp/*.pb.cc`, so new protos are picked up with no CMake change. |
| `src/gen/ts/*_pb.ts` | Frontend. |

Both directories are committed. Regenerate and commit the output in the same
change as the `.proto` edit, or the two drift apart — see below.

## Use `buf`, never bare `protoc`

Running `protoc --cpp_out=...` by hand produces **different output** for the same
`.proto`. It omits the `json_name` annotations that buf bakes into the serialized
descriptors, which changes JSON field naming at runtime. The result compiles and
looks fine in review — the diff appears to be comment churn — so this is easy to
do by accident and hard to catch afterwards.

If a regeneration diff touches files whose `.proto` you did not edit, that is the
signal something generated them with the wrong tool.

## When to regenerate

Any time a `.proto` changes. Generated output is committed, so a `.proto` edit
without a matching regeneration leaves the build compiling against a schema that
no longer matches the source of truth.

This has already happened once: `debate_event`, `layout`, `rendering_info`,
`test_scenario` and `user` were edited without regenerating, and the stale
generated code sat in `main` until someone ran `buf` and found ~1,800 lines of
drift.

## Verifying a regeneration is clean

`git diff` should touch only the protos you edited (plus their dependents). Note
that generated files are checked out with CRLF line endings while buf emits LF,
so a byte-level `diff` between a fresh generation and the working tree reports
every line as changed. That is a line-ending artifact, not a real difference —
`git diff` normalizes it. Use `git diff`, or `diff --strip-trailing-cr`, to see
what actually changed.
