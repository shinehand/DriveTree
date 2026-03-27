import test from "node:test";
import assert from "node:assert/strict";

import { buildTreeFromEntries, layoutTreemap, summarizeNode } from "../src/driveTree.js";

test("buildTreeFromEntries aggregates folder sizes and collapses the common root", () => {
  const tree = buildTreeFromEntries([
    { path: "Drive/Users/me/video.mov", size: 300 },
    { path: "Drive/Users/me/archive.zip", size: 100 },
    { path: "Drive/System/cache.bin", size: 200 },
  ]);

  assert.equal(tree.name, "Drive");
  assert.equal(tree.size, 600);
  assert.deepEqual(
    tree.children.map((node) => [node.name, node.size]),
    [
      ["Users", 400],
      ["System", 200],
    ],
  );
});

test("summarizeNode counts descendants and largest child", () => {
  const tree = buildTreeFromEntries([
    { path: "Drive/Documents/a.psd", size: 400 },
    { path: "Drive/Documents/b.fig", size: 250 },
    { path: "Drive/Downloads/c.zip", size: 900 },
  ]);

  const summary = summarizeNode(tree);

  assert.equal(summary.folderCount, 2);
  assert.equal(summary.fileCount, 3);
  assert.equal(summary.largestChild?.name, "Downloads");
});

test("layoutTreemap fills the requested bounds without losing any nodes", () => {
  const nodes = [
    { name: "A", size: 5, type: "folder" },
    { name: "B", size: 3, type: "folder" },
    { name: "C", size: 2, type: "file" },
  ];

  const layout = layoutTreemap(nodes, { x: 0, y: 0, width: 1000, height: 500 });

  assert.equal(layout.length, 3);

  const totalArea = layout.reduce((sum, tile) => sum + tile.width * tile.height, 0);
  assert.equal(totalArea, 500_000);
  assert.deepEqual(
    layout.map((tile) => tile.node.name).sort(),
    ["A", "B", "C"],
  );
});
