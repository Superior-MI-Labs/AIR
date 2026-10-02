#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INDEX="$ROOT/web/index.html"

[[ -f "$INDEX" ]] || {
    echo "ERROR: missing web/index.html" >&2
    exit 1
}

echo "=== AIR WEB PREFLIGHT ==="

python3 - "$ROOT" <<'PY'
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
index = (root / "web" / "index.html").read_text()

refs = re.findall(r'(?:src|href)="(/app/[^"]+)"', index)
if not refs:
    raise SystemExit("no /app assets referenced by web/index.html")

missing = []
for ref in refs:
    path = root / "web" / ref.lstrip("/")
    if not path.is_file():
        missing.append(str(path.relative_to(root)))
if missing:
    raise SystemExit("missing referenced web assets: " + ", ".join(missing))

script_refs = re.findall(r'<script defer src="(/app/[^"]+\.js)"></script>', index)
if not script_refs:
    raise SystemExit("no deferred application scripts found")

names = [pathlib.PurePosixPath(x).name for x in script_refs]
if names[0] != "core-v330.js" or names[1] != "api-v330.js" or names[-1] != "main-v330.js":
    raise SystemExit("invalid Control Room boot order: " + " -> ".join(names))

required_views = {
    "home", "playground", "decision", "models", "machine", "runtime",
    "plan", "execution", "semantics", "diagnostics", "metrics", "setup", "about",
}
registered = set()
for ref in script_refs[2:-1]:
    path = root / "web" / ref.lstrip("/")
    text = path.read_text()
    registered.update(re.findall(r"App\.registerView\('([^']+)'", text))
missing_views = sorted(required_views - registered)
if missing_views:
    raise SystemExit("missing registered Control Room views: " + ", ".join(missing_views))

api = (root / "web" / "app" / "api-v330.js").read_text()
required_endpoints = [
    "/health", "/model", "/runtime", "/machine", "/environment",
    "/events", "/timeline", "/execution-graphs", "/semantics", "/metrics",
]
missing_endpoints = [x for x in required_endpoints if x not in api]
if missing_endpoints:
    raise SystemExit("API client missing canonical endpoints: " + ", ".join(missing_endpoints))

main = (root / "web" / "app" / "main-v330.js").read_text()
for marker in ["uiMode", "fetchStructural", "fetchResearchEvidence", "shouldFetchResearchEvidence"]:
    if marker not in main:
        raise SystemExit("Control Room main missing mode/polling contract: " + marker)

if 'air-web-version" content="3.3.0"' not in index:
    raise SystemExit("web/index.html does not declare Web 3.3.0")

print("web_asset_references=PASS")
print("web_boot_order=PASS")
print("web_required_views=PASS")
print("web_canonical_endpoints=PASS")
print("web_mode_polling_contract=PASS")
PY

if command -v node >/dev/null 2>&1; then
    while IFS= read -r script; do
        node --check "$script"
    done < <(find "$ROOT/web/app" -type f -name '*.js' -print | sort)
    echo "web_javascript_syntax=PASS"
else
    echo "web_javascript_syntax=SKIP_NODE_UNAVAILABLE"
fi

echo "AIR_WEB_PREFLIGHT=PASS"
