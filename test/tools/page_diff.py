#!/usr/bin/env python3
"""page_diff.py -- is the server's page the same on two consecutive requests?

Every click in the debate tool re-fetches the WHOLE page (a ui::Page tree).
When nothing changed, two consecutive responses must be byte-identical: same
components, same order, same ids. If they are not, React moves DOM nodes and
the page twitches. This script asks for the same page several times and
diffs the answers.

How to run (the backend must be up on port 8080):

    python test/tools/page_diff.py                 # debate 56, as a visitor
    python test/tools/page_diff.py --debate 12     # another debate
    python test/tools/page_diff.py --home          # the home page (Home tab)
    python test/tools/page_diff.py --home --click homeTab_explore
    python test/tools/page_diff.py --cookie "session_token=..."   # signed in
    python test/tools/page_diff.py --times 5 --show 80

It signs in as a fixed visitor (the same way the browser does: every message
carries a well-formed visitor name, and a request without a session becomes
that visitor), opens the page with the click the UI sends, then sends N empty
"refresh" messages -- exactly what the frontend's reloadPage() and the 2 s
AI-job poll send -- and compares the responses.

The protobuf Python bindings are generated on the fly from protos/ with protoc
(found on PATH, in $PROTOC, or in the repo's vcpkg tree), so the diff is
printed as readable JSON. Exit status: 0 identical, 1 different, 2 error.
"""

import argparse
import difflib
import glob
import http.cookiejar
import importlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
PROTOS = os.path.join(REPO, "protos")

# "visitor-" + 8 lowercase hex digits, the shape AnonymousUser.h accepts.
VISITOR = "visitor-0d1ff0d1"


def find_protoc():
    if os.environ.get("PROTOC"):
        return os.environ["PROTOC"]
    on_path = shutil.which("protoc")
    if on_path:
        return on_path
    # The repo's vcpkg tree; a worktree sits under <repo>/.claude/worktrees/.
    for root in (REPO, os.path.join(REPO, "..", "..", "..")):
        hits = glob.glob(os.path.join(root, "third_party", "vcpkg", "installed",
                                      "*", "tools", "protobuf", "protoc*"))
        if hits:
            return hits[0]
    sys.exit("page_diff: protoc not found (set PROTOC)")


def load_bindings():
    out = tempfile.mkdtemp(prefix="page_diff_pb_")
    subprocess.check_call([find_protoc(), "-I", PROTOS, "--python_out", out,
                           os.path.join(PROTOS, "layout.proto"),
                           os.path.join(PROTOS, "client_message.proto")])
    sys.path.insert(0, out)
    return (importlib.import_module("layout_pb2"),
            importlib.import_module("client_message_pb2"))


class Client:
    def __init__(self, base, cookie, cm_pb2):
        self.base = base.rstrip("/")
        self.cm = cm_pb2
        self.jar = http.cookiejar.CookieJar()
        self.opener = urllib.request.build_opener(
            urllib.request.HTTPCookieProcessor(self.jar))
        self.cookie = cookie

    def post(self, component_id="", event_type="", page_id="", values=None):
        msg = self.cm.ClientMessage()
        msg.component_id = component_id
        msg.event_type = event_type
        msg.page_data.page_id = page_id or "login"
        for k, v in (values or {}).items():
            c = msg.page_data.components.add()
            c.id, c.value = k, str(v)
        c = msg.page_data.components.add()
        c.id, c.value = "visitorName", VISITOR
        req = urllib.request.Request(
            self.base + "/clientmessage", data=msg.SerializeToString(),
            headers={"Content-Type": "application/x-protobuf"}, method="POST")
        if self.cookie:
            req.add_header("Cookie", self.cookie)
        with self.opener.open(req, timeout=60) as res:
            return res.read()


def _sort_maps(node):
    """Sort the keys of the css/attributes maps, recursively, in place."""
    if isinstance(node, dict):
        for key in ("css", "attributes"):
            if isinstance(node.get(key), dict):
                node[key] = dict(sorted(node[key].items()))
        for v in node.values():
            _sort_maps(v)
    elif isinstance(node, list):
        for v in node:
            _sort_maps(v)


def to_json(layout_pb2, raw, wire_maps=False):
    from google.protobuf import json_format
    page = layout_pb2.Page()
    page.ParseFromString(raw)
    tree = json_format.MessageToDict(page)
    # No sort_keys: the order of repeated fields (children, nodes, edges) is
    # exactly what is under test. Map fields (css, attributes) carry no order
    # for the page, so they are sorted unless --wire-maps asks to see them in
    # wire order (sorted anyway once the server serialises deterministically;
    # the byte comparison catches it either way).
    if not wire_maps:
        _sort_maps(tree)
    return json.dumps(tree, indent=1, ensure_ascii=False).splitlines()


def sequences(layout_pb2, raw):
    """{parent id: [child ids]} plus GRAPH node/edge id lists, by path."""
    page = layout_pb2.Page()
    page.ParseFromString(raw)
    out = {}

    def walk(comps, path):
        out[path] = [c.id or "<no id>" for c in comps]
        for i, c in enumerate(comps):
            here = "%s/%s" % (path, c.id or "#%d" % i)
            if c.nodes:
                out[here + ":nodes"] = [n.id for n in c.nodes]
            if c.edges:
                out[here + ":edges"] = [e.id for e in c.edges]
            walk(c.children, here)

    walk(page.components, "")
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--base", default="http://localhost:8080")
    ap.add_argument("--debate", type=int, default=56)
    ap.add_argument("--home", action="store_true", help="stay on the home page")
    ap.add_argument("--click", action="append", default=[],
                    help="a further onClick component id (repeatable)")
    ap.add_argument("--cookie", default="", help="raw Cookie header (signed-in)")
    ap.add_argument("--times", type=int, default=3)
    ap.add_argument("--show", type=int, default=60, help="diff lines to print")
    ap.add_argument("--wire-maps", action="store_true",
                    help="diff css/attributes maps in wire order, not sorted")
    ap.add_argument("--dump", default="", help="write response 1 as JSON here")
    args = ap.parse_args()

    layout_pb2, cm_pb2 = load_bindings()
    cl = Client(args.base, args.cookie, cm_pb2)
    try:
        cl.post()  # sign in as the visitor (or use the cookie)
        # The server remembers where this reader is, so start from home.
        cl.post("goHomeButton", "onClick", "debate")
        cl.post("homeTab_home", "onClick", "home")
        if not args.home:
            cl.post("openDebateCard_%d" % args.debate, "onClick", "home")
        for click in args.click:
            cl.post(click, "onClick", "home" if args.home else "debate")
        pages = [cl.post() for _ in range(max(2, args.times))]
    except Exception as e:  # noqa: BLE001
        print("page_diff: request failed: %s" % e)
        return 2

    first = to_json(layout_pb2, pages[0], args.wire_maps)
    if args.dump:
        with open(args.dump, "w", encoding="utf-8") as f:
            f.write("\n".join(first))
    status = 0
    for i, raw in enumerate(pages[1:], start=2):
        same_bytes = raw == pages[0]
        other = to_json(layout_pb2, raw, args.wire_maps)
        diff = list(difflib.unified_diff(first, other, "response 1",
                                         "response %d" % i, lineterm="", n=2))
        print("response 1 vs %d: %d bytes vs %d, bytes %s, JSON %s (%d diff lines)"
              % (i, len(pages[0]), len(raw),
                 "identical" if same_bytes else "differ",
                 "identical" if not diff else "differs", len(diff)))
        if diff:
            status = 1
            a, b = sequences(layout_pb2, pages[0]), sequences(layout_pb2, raw)
            moved = [k for k in a if k in b and a[k] != b[k]
                     and sorted(a[k]) == sorted(b[k])]
            if moved:
                print("   reordered lists (%d):" % len(moved))
                for k in moved:
                    print("     " + (k or "<page>"))
            for line in diff[:args.show]:
                print("   " + line)
            if len(diff) > args.show:
                print("   ... %d more" % (len(diff) - args.show))
        elif not same_bytes:
            status = 1  # same tree, different bytes: map order on the wire
    return status


if __name__ == "__main__":
    sys.exit(main())
