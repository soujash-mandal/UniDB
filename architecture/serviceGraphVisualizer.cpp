#include "serviceGraph.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Architecture {

namespace {

constexpr int LEVEL_GAP_X = 260;
constexpr int NODE_GAP_Y = 90;

struct Position {
  int x;
  int y;
};

std::vector<ServiceNode> getNodes() {
  std::set<ServiceNode> uniqueNodes(SERVICE_NODES.begin(), SERVICE_NODES.end());

  for (const auto &[consumer, provider] : SERVICE_EDGES) {
    uniqueNodes.insert(consumer);
    uniqueNodes.insert(provider);
  }

  return {uniqueNodes.begin(), uniqueNodes.end()};
}

std::map<ServiceNode, int> calculateLevels() {
  const auto nodes = getNodes();

  std::map<ServiceNode, std::vector<ServiceNode>> dependencies;

  for (const auto &node : nodes) {
    dependencies[node] = {};
  }

  for (const auto &[consumer, provider] : SERVICE_EDGES) {
    dependencies[consumer].push_back(provider);
  }

  std::map<ServiceNode, int> levels;
  std::set<ServiceNode> visiting;

  std::function<int(const ServiceNode &)> visit =
      [&](const ServiceNode &node) -> int {
    if (levels.contains(node)) {
      return levels[node];
    }

    if (visiting.contains(node)) {
      return 0;
    }

    visiting.insert(node);

    int level = 0;

    for (const auto &dependency : dependencies[node]) {
      level = std::max(level, visit(dependency) + 1);
    }

    visiting.erase(node);
    levels[node] = level;

    return level;
  };

  for (const auto &node : nodes) {
    visit(node);
  }

  return levels;
}

std::map<ServiceNode, Position> calculatePositions() {
  const auto nodes = getNodes();
  const auto levels = calculateLevels();

  std::map<int, std::vector<ServiceNode>> nodesByLevel;

  for (const auto &node : nodes) {
    nodesByLevel[levels.at(node)].push_back(node);
  }

  std::map<ServiceNode, Position> positions;

  for (auto &[level, levelNodes] : nodesByLevel) {
    std::sort(levelNodes.begin(), levelNodes.end());

    const int count = static_cast<int>(levelNodes.size());

    for (int i = 0; i < count; ++i) {
      const int y = static_cast<int>((i - (count - 1) / 2.0) * NODE_GAP_Y);

      positions[levelNodes[i]] = {level * LEVEL_GAP_X, y};
    }
  }

  return positions;
}

std::string escapeJson(const std::string &value) {
  std::string result;

  for (const char character : value) {
    if (character == '"') {
      result += "\\\"";
    } else if (character == '\\') {
      result += "\\\\";
    } else {
      result += character;
    }
  }

  return result;
}

std::string createNodesJson(const std::map<ServiceNode, Position> &positions) {
  std::string json = "[";

  bool first = true;

  for (const auto &[node, position] : positions) {
    if (!first) {
      json += ",";
    }

    first = false;

    json += "{";
    json += "\"id\":\"";
    json += escapeJson(node);
    json += "\",";
    json += "\"label\":\"";
    json += escapeJson(node);
    json += "\",";
    json += "\"x\":";
    json += std::to_string(position.x);
    json += ",";
    json += "\"y\":";
    json += std::to_string(position.y);
    json += "}";
  }

  json += "]";

  return json;
}

std::string createEdgesJson() {
  std::string json = "[";

  for (std::size_t i = 0; i < SERVICE_EDGES.size(); ++i) {
    const auto &[consumer, provider] = SERVICE_EDGES[i];

    if (i > 0) {
      json += ",";
    }

    json += "{";
    json += "\"id\":";
    json += std::to_string(i);
    json += ",";

    json += "\"from\":\"";
    json += escapeJson(consumer);
    json += "\",";

    json += "\"to\":\"";
    json += escapeJson(provider);
    json += "\"";

    json += "}";
  }

  json += "]";

  return json;
}

void replace(std::string &text, const std::string &placeholder,
             const std::string &value) {
  const std::size_t position = text.find(placeholder);

  if (position == std::string::npos) {
    return;
  }

  text.replace(position, placeholder.size(), value);
}

std::string createHtml() {
  const auto positions = calculatePositions();

  const std::string nodesJson = createNodesJson(positions);

  const std::string edgesJson = createEdgesJson();

  std::string html = R"HTML(
<!DOCTYPE html>
<html>

<head>

<meta charset="utf-8">

<title>UniDB Service Graph</title>

<script src="https://unpkg.com/vis-network@9.1.9/standalone/umd/vis-network.min.js"></script>

<style>

html,
body {
    margin: 0;
    height: 100%;
    font-family: system-ui, sans-serif;
}

#network {
    width: 100%;
    height: 100vh;
}

#panel {
    position: absolute;
    top: 12px;
    left: 12px;
    z-index: 10;

    background: white;

    padding: 12px 16px;

    border: 1px solid #ddd;
    border-radius: 8px;

    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.12);

    font-size: 13px;
}

button {
    margin-top: 8px;
    padding: 5px 10px;
    font: inherit;
    cursor: pointer;
}

#focus {
    margin-top: 7px;
    color: #555;
}

</style>

</head>

<body>

<div id="panel">

<strong>UniDB Service Graph</strong>

<div>
    {{NODE_COUNT}} nodes,
    {{EDGE_COUNT}} edges
</div>

<button id="reset">
    Show all
</button>

<div id="focus"></div>

</div>

<div id="network"></div>

<script>

const nodes = new vis.DataSet(
    {{NODES}}
);

const edges = new vis.DataSet(
    {{EDGES}}
);

const network = new vis.Network(
    document.getElementById("network"),

    {
        nodes: nodes,
        edges: edges
    },

    {
        nodes: {
            shape: "box",
            margin: 10,

            font: {
                size: 14,
                color: "#ffffff",
                face: "system-ui"
            },

            color: {
                background: "#4a90d9",
                border: "#4a90d9",

                highlight: {
                    background: "#4a90d9",
                    border: "#4a90d9"
                }
            },

            borderWidth: 0,

            shapeProperties: {
                borderRadius: 5
            }
        },

        edges: {
            arrows: {
                to: {
                    enabled: true,
                    scaleFactor: 0.8
                }
            },

            color: {
                color: "#b8bec5"
            },

            smooth: false
        },

        physics: false,

        interaction: {
            dragNodes: true,
            hover: true
        },

        layout: {
            hierarchical: false
        }
    }
);

network.fit();

const dependencies = {};

edges.get().forEach(function(edge) {

    if (!dependencies[edge.from]) {
        dependencies[edge.from] = [];
    }

    dependencies[edge.from].push(edge.to);

});

function dependencyClosure(start) {

    const keep = new Set();
    const stack = [start];

    while (stack.length > 0) {

        const current = stack.pop();

        if (keep.has(current)) {
            continue;
        }

        keep.add(current);

        const next =
            dependencies[current] || [];

        next.forEach(function(dependency) {
            stack.push(dependency);
        });
    }

    return keep;
}

function showDependencies(start) {

    const keep =
        dependencyClosure(start);

    nodes.update(
        nodes.get().map(function(node) {
            return {
                id: node.id,
                hidden: !keep.has(node.id)
            };
        })
    );

    edges.update(
        edges.get().map(function(edge) {
            return {
                id: edge.id,
                hidden:
                    !keep.has(edge.from) ||
                    !keep.has(edge.to)
            };
        })
    );

    document.getElementById("focus").textContent =
        start +
        " + " +
        (keep.size - 1) +
        " dependencies";

    network.fit();
}

function showAll() {

    nodes.update(
        nodes.get().map(function(node) {
            return {
                id: node.id,
                hidden: false
            };
        })
    );

    edges.update(
        edges.get().map(function(edge) {
            return {
                id: edge.id,
                hidden: false
            };
        })
    );

    document.getElementById("focus").textContent = "";

    network.fit();
}

network.on("click", function(event) {

    if (event.nodes.length === 0) {
        return;
    }

    showDependencies(event.nodes[0]);

});

document
    .getElementById("reset")
    .addEventListener("click", showAll);

</script>

</body>

</html>
)HTML";

  replace(html, "{{NODE_COUNT}}", std::to_string(SERVICE_NODES.size()));

  replace(html, "{{EDGE_COUNT}}", std::to_string(SERVICE_EDGES.size()));

  replace(html, "{{NODES}}", nodesJson);

  replace(html, "{{EDGES}}", edgesJson);

  return html;
}

} // namespace

} // namespace Architecture

int main() {

  const std::string outputDirectory = "architecture/.graph";

  const std::string outputPath = outputDirectory + "/serviceGraph.html";

  const std::string command = "mkdir -p " + outputDirectory;

  if (std::system(command.c_str()) != 0) {
    std::cerr << "Failed to create directory: " << outputDirectory << '\n';

    return 1;
  }

  std::ofstream output(outputPath);

  if (!output) {
    std::cerr << "Failed to create graph: " << outputPath << '\n';

    return 1;
  }

  output << Architecture::createHtml();

  if (!output) {
    std::cerr << "Failed to write graph: " << outputPath << '\n';

    return 1;
  }

  std::cout << "Service graph generated: " << outputPath << '\n';

  return 0;
}
