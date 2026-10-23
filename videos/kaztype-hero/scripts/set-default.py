# Sets a composition variable's default in index.html:  python3 scripts/set-default.py <id> <value>
import html, json, re, sys

path = "index.html"
src = open(path).read()
m = re.search(r'data-composition-variables="([^"]*)"', src)
variables = json.loads(html.unescape(m.group(1)))
for v in variables:
    if v["id"] == sys.argv[1]:
        v["default"] = sys.argv[2]
attr = html.escape(json.dumps(variables, ensure_ascii=False, separators=(",", ":")), quote=True)
open(path, "w").write(src[: m.start(1)] + attr + src[m.end(1) :])
