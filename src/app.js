import {
  buildTreeFromEntries,
  formatBytes,
  layoutTreemap,
  summarizeNode,
} from "./driveTree.js";
import { sampleEntries } from "./sampleData.js";

const folderInput = document.querySelector("#folderInput");
const sampleButton = document.querySelector("#sampleButton");
const summary = document.querySelector("#summary");
const breadcrumbs = document.querySelector("#breadcrumbs");
const treemap = document.querySelector("#treemap");
const detailsBody = document.querySelector("#detailsBody");
const tableDescription = document.querySelector("#tableDescription");

const state = {
  root: buildTreeFromEntries(sampleEntries, "Sample Drive"),
  path: [],
};

function getCurrentNode() {
  return state.path.reduce((current, index) => current.children[index], state.root);
}

function getNodePath() {
  const nodes = [state.root];
  let current = state.root;

  for (const index of state.path) {
    current = current.children[index];
    nodes.push(current);
  }

  return nodes;
}

function renderSummary(node) {
  const info = summarizeNode(node);
  const stats = [
    ["현재 용량", formatBytes(info.totalSize)],
    ["하위 폴더", `${info.folderCount.toLocaleString("ko-KR")}개`],
    ["파일 수", `${info.fileCount.toLocaleString("ko-KR")}개`],
    [
      "가장 큰 항목",
      info.largestChild
        ? `${info.largestChild.name} · ${formatBytes(info.largestChild.size)}`
        : "없음",
    ],
  ];

  summary.innerHTML = stats
    .map(
      ([label, value]) => `
        <article class="stat">
          <span class="stat-label">${label}</span>
          <strong class="stat-value">${value}</strong>
        </article>
      `,
    )
    .join("");
}

function renderBreadcrumbs() {
  const nodes = getNodePath();

  breadcrumbs.innerHTML = "";

  nodes.forEach((node, index) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `crumb${index === nodes.length - 1 ? " active" : ""}`;
    button.textContent = node.name;
    button.addEventListener("click", () => {
      state.path = state.path.slice(0, index);
      render();
    });
    breadcrumbs.append(button);
  });
}

function renderDetails(node) {
  const children = node.children ?? [];
  tableDescription.textContent = `${children.length.toLocaleString("ko-KR")}개 항목`;
  detailsBody.innerHTML = children
    .map((child) => {
      const ratio = node.size === 0 ? 0 : (child.size / node.size) * 100;
      return `
        <tr>
          <td>${child.name}</td>
          <td>${child.type === "folder" ? "폴더" : "파일"}</td>
          <td>${formatBytes(child.size)}</td>
          <td>${ratio.toFixed(1)}%</td>
        </tr>
      `;
    })
    .join("");
}

function renderTreemap(node) {
  treemap.innerHTML = "";
  const children = node.children ?? [];

  if (children.length === 0) {
    treemap.innerHTML = `
      <div class="empty-state">
        <div>
          <strong>표시할 하위 항목이 없습니다.</strong>
          <p>다른 폴더를 선택하거나 상위 경로로 이동해 보세요.</p>
        </div>
      </div>
    `;
    return;
  }

  const layout = layoutTreemap(children, {
    x: 0,
    y: 0,
    width: treemap.clientWidth,
    height: treemap.clientHeight,
  });

  layout.forEach((tile) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `tile ${tile.node.type}`;
    button.style.left = `${tile.x}px`;
    button.style.top = `${tile.y}px`;
    button.style.width = `${tile.width}px`;
    button.style.height = `${tile.height}px`;
    button.title = `${tile.node.name} (${formatBytes(tile.node.size)})`;

    const ratio = node.size === 0 ? 0 : (tile.node.size / node.size) * 100;
    const compact = tile.width < 110 || tile.height < 80;

    button.innerHTML = compact
      ? `<span class="tile-name">${tile.node.name}</span>`
      : `
          <span class="tile-name">${tile.node.name}</span>
          <span class="tile-meta">
            ${tile.node.type === "folder" ? "폴더" : "파일"} · ${formatBytes(tile.node.size)} · ${ratio.toFixed(1)}%
          </span>
        `;

    if (tile.node.type === "folder") {
      button.addEventListener("click", () => {
        const childIndex = node.children.indexOf(tile.node);
        state.path = [...state.path, childIndex];
        render();
      });
    } else {
      button.disabled = true;
    }

    treemap.append(button);
  });
}

function render() {
  const current = getCurrentNode();
  renderSummary(current);
  renderBreadcrumbs();
  renderTreemap(current);
  renderDetails(current);
}

folderInput.addEventListener("change", () => {
  const entries = Array.from(folderInput.files ?? []).map((file) => ({
    path: file.webkitRelativePath || file.name,
    size: file.size,
  }));

  state.root = buildTreeFromEntries(entries, "Selected Folder");
  state.path = [];
  render();
});

sampleButton.addEventListener("click", () => {
  state.root = buildTreeFromEntries(sampleEntries, "Sample Drive");
  state.path = [];
  render();
});

window.addEventListener("resize", () => renderTreemap(getCurrentNode()));

render();
