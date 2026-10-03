#!/bin/bash

set -e

OUTPUT="service-graph.html"

python3 - <<'PY'
from pathlib import Path
import re
import html

root = Path(".")
services = {}

# Find directories that look like services.
for p in root.iterdir():
    if not p.is_dir() or p.name.startswith("."):
        continue
    if any((p / d).is_dir() for d in ("actions", "domain", "port", "adapter", "adapters")):
        services[p.name] = p

# Known UniDB service names can be included even if their directory
# currently has a slightly different structure.
known = [
    "DiskManager",
    "FreePageManager",
    "BufferPoolManager",
    "TupleService",
    "CatalogService",
    "FreeSpaceMapService",
]

for name in known:
    p = root / name
    if p.is_dir():
        services[name] = p

service_names = sorted(services)

edges = set()

# Look through C++ files for references to another service.
# We intentionally use service-level references rather than every
# class/function call, keeping the graph architectural.
for source_name, source_dir in services.items():
    for file in source_dir.rglob("*"):
        if file.suffix not in (".h", ".hpp", ".cpp", ".cc", ".cxx"):
            continue

        try:
            text = file.read_text(errors="ignore")
        except Exception:
            continue

        for target_name in service_names:
            if target_name == source_name:
                continue

            if re.search(r"\b" + re.escape(target_name) + r"\b", text):
                edges.add((source_name, target_name))

# Also detect includes such as:
# #include "../../BufferPoolManager/..."
for source_name, source_dir in services.items():
    for file in source_dir.rglob("*"):
        if file.suffix not in (".h", ".hpp", ".cpp", ".cc", ".cxx"):
            continue

        try:
            text = file.read_text(errors="ignore")
        except Exception:
            continue

        for target_name in service_names:
            if target_name == source_name:
                continue

            if target_name in text:
                edges.add((source_name, target_name))

nodes = []
for name in service_names:
    nodes.append({
        "id": name,
        "label": name
    })

# Convert Python data to JS safely.
import json

nodes_json = json.dumps(nodes)
edges_json = json.dumps([
    {"from": a, "to": b}
    for a, b in sorted(edges)
])

html_output = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>UniDB Service Graph</title>

<style>
body {{
    margin: 0;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    background: #f5f5f5;
}}

header {{
    padding: 18px 24px;
    background: white;
    border-bottom: 1px solid #ddd;
}}

h1 {{
    margin: 0;
    font-size: 22px;
}}

p {{
    margin: 6px 0 0;
    color: #666;
}}

#graph {{
    width: 100vw;
    height: calc(100vh - 80px);
}}

.node {{
    cursor: grab;
}}

.node:active {{
    cursor: grabbing;
}}

.edge {{
    stroke: #777;
    stroke-width: 1.8;
    marker-end: url(#arrow);
}}

.edge-label {{
    font-size: 11px;
    fill: #555;
}}

.node-box {{
    fill: white;
    stroke: #333;
    stroke-width: 2;
}}

.node-text {{
    font-size: 14px;
    font-weight: 600;
    text-anchor: middle;
    dominant-baseline: middle;
}}

.legend {{
    position: fixed;
    right: 20px;
    top: 95px;
    background: white;
    padding: 12px 15px;
    border: 1px solid #ddd;
    border-radius: 8px;
    font-size: 13px;
}}
</style>
</head>

<body>

<header>
    <h1>UniDB Service Graph</h1>
    <p>
        Service-level dependencies detected from the C++ source tree.
        Drag nodes to rearrange the graph.
    </p>
</header>

<div class="legend">
    <b>Nodes</b><br>
    Service<br><br>
    <b>Arrow</b><br>
    Service reference / dependency
</div>

<svg id="graph"></svg>

<script>
const nodes = {nodes_json};
const edges = {edges_json};

const svg = document.getElementById("graph");

let width = window.innerWidth;
let height = window.innerHeight - 80;

svg.setAttribute("width", width);
svg.setAttribute("height", height);

const NS = "http://www.w3.org/2000/svg";

const defs = document.createElementNS(NS, "defs");

const marker = document.createElementNS(NS, "marker");
marker.setAttribute("id", "arrow");
marker.setAttribute("markerWidth", "10");
marker.setAttribute("markerHeight", "10");
marker.setAttribute("refX", "9");
marker.setAttribute("refY", "3");
marker.setAttribute("orient", "auto");
marker.setAttribute("markerUnits", "strokeWidth");

const arrow = document.createElementNS(NS, "path");
arrow.setAttribute("d", "M0,0 L0,6 L9,3 z");
arrow.setAttribute("fill", "#777");

marker.appendChild(arrow);
defs.appendChild(marker);
svg.appendChild(defs);

const edgeGroup = document.createElementNS(NS, "g");
const nodeGroup = document.createElementNS(NS, "g");

svg.appendChild(edgeGroup);
svg.appendChild(nodeGroup);

const radius = 65;

nodes.forEach((node, i) => {{
    const angle = (Math.PI * 2 * i) / Math.max(nodes.length, 1);

    node.x = width / 2 + Math.cos(angle) * Math.min(width, height) * 0.30;
    node.y = height / 2 + Math.sin(angle) * Math.min(width, height) * 0.30;
}});

function createEdge(edge) {{
    const line = document.createElementNS(NS, "line");
    line.classList.add("edge");

    edge.line = line;
    edgeGroup.appendChild(line);
}}

function createNode(node) {{
    const group = document.createElementNS(NS, "g");
    group.classList.add("node");

    const box = document.createElementNS(NS, "rect");
    box.classList.add("node-box");

    const text = document.createElementNS(NS, "text");
    text.classList.add("node-text");
    text.textContent = node.label;

    const boxWidth = Math.max(150, node.label.length * 9 + 35);

    box.setAttribute("x", -boxWidth / 2);
    box.setAttribute("y", -25);
    box.setAttribute("width", boxWidth);
    box.setAttribute("height", 50);
    box.setAttribute("rx", 10);

    group.appendChild(box);
    group.appendChild(text);

    node.group = group;

    let dragging = false;
    let offsetX = 0;
    let offsetY = 0;

    group.addEventListener("mousedown", event => {{
        dragging = true;
        offsetX = event.clientX - node.x;
        offsetY = event.clientY - node.y;
    }});

    window.addEventListener("mousemove", event => {{
        if (!dragging) return;

        node.x = event.clientX - offsetX;
        node.y = event.clientY - offsetY;

        render();
    }});

    window.addEventListener("mouseup", () => {{
        dragging = false;
    }});

    nodeGroup.appendChild(group);
}}

edges.forEach(createEdge);
nodes.forEach(createNode);

function render() {{
    edges.forEach(edge => {{
        const from = nodes.find(n => n.id === edge.from);
        const to = nodes.find(n => n.id === edge.to);

        if (!from || !to) return;

        edge.line.setAttribute("x1", from.x);
        edge.line.setAttribute("y1", from.y);
        edge.line.setAttribute("x2", to.x);
        edge.line.setAttribute("y2", to.y);
    }});

    nodes.forEach(node => {{
        node.group.setAttribute(
            "transform",
            `translate(${{node.x}},${{node.y}})`
        );
    }});
}}

render();

window.addEventListener("resize", () => {{
    width = window.innerWidth;
    height = window.innerHeight - 80;

    svg.setAttribute("width", width);
    svg.setAttribute("height", height);

    render();
}});
</script>

</body>
</html>
"""

Path("service-graph.html").write_text(html_output)

print(f"Generated {len(nodes)} services and {len(edges)} dependencies.")
print(f"Output: {OUTPUT}")
PY

echo ""
echo "Opening service graph..."
open service-graph.html
