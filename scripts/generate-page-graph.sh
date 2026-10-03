#!/bin/bash

set -e

python3 - <<'PY'
from pathlib import Path
import re
import html
import json

root = Path(".")
pages = []

# Find C++ classes whose names end with Page.
for file in root.rglob("*"):
    if file.suffix not in (".h", ".hpp"):
        continue

    # Ignore build/dependency directories.
    if any(part in {"build", ".git", "cmake-build-debug"} for part in file.parts):
        continue

    try:
        text = file.read_text(errors="ignore")
    except Exception:
        continue

    # class FooPage / struct FooPage
    matches = re.findall(
        r'\b(?:class|struct)\s+([A-Za-z_][A-Za-z0-9_]*Page)\b',
        text
    )

    for page_name in matches:
        fields = []

        # Extract the class body.
        pattern = (
            r'\b(?:class|struct)\s+'
            + re.escape(page_name)
            + r'\b[^\\{]*\{(.*?)\};'
        )

        match = re.search(pattern, text, re.S)

        if match:
            body = match.group(1)

            # Remove comments.
            body = re.sub(r'//.*', '', body)
            body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)

            # Find simple member declarations.
            for line in body.splitlines():
                line = line.strip()

                if not line:
                    continue

                if line.endswith(":"):
                    continue

                # Ignore methods.
                if "(" in line:
                    continue

                # Ignore access modifiers.
                if line in ("public:", "private:", "protected:"):
                    continue

                # Match declarations such as:
                # uint32_t pageId;
                # PageId nextPageId;
                # char data[8192];
                m = re.match(
                    r'((?:const\s+)?[\w:<>]+(?:\s*[*&])?)\s+'
                    r'([A-Za-z_][A-Za-z0-9_]*)'
                    r'(?:\s*\[[^\]]+\])?\s*;',
                    line
                )

                if m:
                    fields.append({
                        "type": m.group(1),
                        "name": m.group(2)
                    })

        pages.append({
            "name": page_name,
            "file": str(file),
            "fields": fields
        })

# Remove duplicates.
unique = {}
for page in pages:
    unique[page["name"]] = page

pages = sorted(unique.values(), key=lambda x: x["name"])

pages_json = json.dumps(pages)

html_output = f"""<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>UniDB Page Visualizer</title>

<style>
* {{
    box-sizing: border-box;
}}

body {{
    margin: 0;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    background: #f5f5f5;
    color: #222;
}}

header {{
    background: white;
    padding: 18px 24px;
    border-bottom: 1px solid #ddd;
}}

h1 {{
    margin: 0;
    font-size: 22px;
}}

.subtitle {{
    margin-top: 5px;
    color: #666;
}}

.controls {{
    padding: 15px 24px;
    background: white;
    border-bottom: 1px solid #ddd;
}}

input {{
    width: 350px;
    max-width: 90%;
    padding: 9px 12px;
    border: 1px solid #bbb;
    border-radius: 7px;
    font-size: 14px;
}}

#pages {{
    padding: 25px;
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(350px, 1fr));
    gap: 22px;
}}

.page {{
    background: white;
    border: 1px solid #ccc;
    border-radius: 12px;
    overflow: hidden;
}}

.page-header {{
    padding: 14px 16px;
    border-bottom: 1px solid #ddd;
}}

.page-name {{
    font-size: 18px;
    font-weight: 700;
}}

.page-file {{
    margin-top: 4px;
    color: #777;
    font-size: 11px;
    word-break: break-all;
}}

.page-body {{
    padding: 0;
}}

.field {{
    display: grid;
    grid-template-columns: 1fr 1.2fr;
    padding: 9px 15px;
    border-bottom: 1px solid #eee;
    font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
    font-size: 12px;
}}

.field:last-child {{
    border-bottom: none;
}}

.type {{
    color: #555;
}}

.name {{
    font-weight: 600;
}}

.empty {{
    padding: 15px;
    color: #888;
    font-size: 13px;
}}

.count {{
    color: #777;
    margin-left: 10px;
}}
</style>
</head>

<body>

<header>
    <h1>UniDB Page Visualizer</h1>
    <div class="subtitle">
        Physical and logical page structures discovered from the C++ source.
        <span class="count" id="count"></span>
    </div>
</header>

<div class="controls">
    <input
        id="search"
        type="search"
        placeholder="Search pages or fields..."
    >
</div>

<div id="pages"></div>

<script>
const pages = {pages_json};

const container = document.getElementById("pages");
const search = document.getElementById("search");
const count = document.getElementById("count");

function render() {{
    const query = search.value.toLowerCase().trim();

    const filtered = pages.filter(page => {{
        if (!query) return true;

        if (page.name.toLowerCase().includes(query))
            return true;

        if (page.file.toLowerCase().includes(query))
            return true;

        return page.fields.some(field =>
            field.name.toLowerCase().includes(query) ||
            field.type.toLowerCase().includes(query)
        );
    }});

    count.textContent =
        `${{filtered.length}} of ${{pages.length}} pages`;

    container.innerHTML = "";

    for (const page of filtered) {{
        const card = document.createElement("div");
        card.className = "page";

        const header = document.createElement("div");
        header.className = "page-header";

        const name = document.createElement("div");
        name.className = "page-name";
        name.textContent = page.name;

        const file = document.createElement("div");
        file.className = "page-file";
        file.textContent = page.file;

        header.appendChild(name);
        header.appendChild(file);

        const body = document.createElement("div");
        body.className = "page-body";

        if (page.fields.length === 0) {{
            const empty = document.createElement("div");
            empty.className = "empty";
            empty.textContent =
                "No simple fields detected. Inspect class manually.";
            body.appendChild(empty);
        }} else {{
            for (const field of page.fields) {{
                const row = document.createElement("div");
                row.className = "field";

                const type = document.createElement("div");
                type.className = "type";
                type.textContent = field.type;

                const fieldName = document.createElement("div");
                fieldName.className = "name";
                fieldName.textContent = field.name;

                row.appendChild(type);
                row.appendChild(fieldName);

                body.appendChild(row);
            }}
        }}

        card.appendChild(header);
        card.appendChild(body);
        container.appendChild(card);
    }}
}}

search.addEventListener("input", render);

render();
</script>

</body>
</html>
"""

Path("page-visualization.html").write_text(html_output)

print(f"Found {len(pages)} page classes.")
print("Generated: page-visualization.html")
PY

open page-visualization.html
