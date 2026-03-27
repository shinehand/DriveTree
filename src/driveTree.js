function normalizePath(path) {
  return path.replaceAll("\\", "/").split("/").filter(Boolean);
}

function createFolder(name) {
  return {
    name,
    type: "folder",
    size: 0,
    children: [],
    _childrenByName: new Map(),
  };
}

function insertEntry(root, entryPath, size) {
  const parts = normalizePath(entryPath);

  if (parts.length === 0) {
    return;
  }

  let current = root;
  current.size += size;

  for (let index = 0; index < parts.length - 1; index += 1) {
    const part = parts[index];
    let next = current._childrenByName.get(part);

    if (!next) {
      next = createFolder(part);
      current._childrenByName.set(part, next);
      current.children.push(next);
    }

    next.size += size;
    current = next;
  }

  const fileName = parts.at(-1);
  const existing = current._childrenByName.get(fileName);

  if (existing) {
    existing.size += size;
    return;
  }

  const fileNode = {
    name: fileName,
    type: "file",
    size,
  };

  current._childrenByName.set(fileName, fileNode);
  current.children.push(fileNode);
}

function finalizeTree(node) {
  if (node.type === "file") {
    return node;
  }

  node.children = node.children.map(finalizeTree).sort((left, right) => {
    if (right.size !== left.size) {
      return right.size - left.size;
    }

    if (left.type !== right.type) {
      return left.type === "folder" ? -1 : 1;
    }

    return left.name.localeCompare(right.name);
  });

  delete node._childrenByName;
  return node;
}

function deriveRoot(entries, fallbackName = "Selected Folder") {
  const firstSegments = new Set();

  for (const entry of entries) {
    const parts = normalizePath(entry.path);

    if (parts.length > 1) {
      firstSegments.add(parts[0]);
    }
  }

  if (firstSegments.size !== 1) {
    return {
      rootName: fallbackName,
      stripLeadingSegment: false,
    };
  }

  return {
    rootName: Array.from(firstSegments)[0],
    stripLeadingSegment: true,
  };
}

export function buildTreeFromEntries(entries, fallbackName) {
  const safeEntries = entries
    .filter((entry) => Number.isFinite(entry.size) && entry.size >= 0)
    .map((entry) => ({
      path: entry.path,
      size: entry.size,
    }));

  const { rootName, stripLeadingSegment } = deriveRoot(safeEntries, fallbackName);
  const root = createFolder(rootName);

  for (const entry of safeEntries) {
    const parts = normalizePath(entry.path);
    const trimmed = stripLeadingSegment ? parts.slice(1) : parts;
    insertEntry(root, trimmed.join("/"), entry.size);
  }

  return finalizeTree(root);
}

export function summarizeNode(node) {
  if (node.type === "file") {
    return {
      totalSize: node.size,
      folderCount: 0,
      fileCount: 1,
      largestChild: node,
    };
  }

  let folderCount = 0;
  let fileCount = 0;
  let largestChild = node.children[0] ?? null;

  for (const child of node.children) {
    if (child.type === "folder") {
      folderCount += 1;
      const summary = summarizeNode(child);
      folderCount += summary.folderCount;
      fileCount += summary.fileCount;
    } else {
      fileCount += 1;
    }

    if (!largestChild || child.size > largestChild.size) {
      largestChild = child;
    }
  }

  return {
    totalSize: node.size,
    folderCount,
    fileCount,
    largestChild,
  };
}

export function formatBytes(size) {
  if (size === 0) {
    return "0 B";
  }

  const units = ["B", "KB", "MB", "GB", "TB"];
  const exponent = Math.min(Math.floor(Math.log(size) / Math.log(1024)), units.length - 1);
  const value = size / 1024 ** exponent;

  return `${value >= 10 || exponent === 0 ? value.toFixed(0) : value.toFixed(1)} ${units[exponent]}`;
}

export function layoutTreemap(nodes, bounds) {
  const items = nodes.filter((node) => node.size > 0);

  if (items.length === 0) {
    return [];
  }

  if (items.length === 1) {
    return [
      {
        node: items[0],
        ...bounds,
      },
    ];
  }

  const total = items.reduce((sum, node) => sum + node.size, 0);
  const horizontalSplit = bounds.width >= bounds.height;
  let running = 0;
  let splitIndex = 0;

  while (splitIndex < items.length - 1 && running < total / 2) {
    running += items[splitIndex].size;
    splitIndex += 1;
  }

  const firstGroup = items.slice(0, splitIndex);
  const secondGroup = items.slice(splitIndex);
  const firstTotal = firstGroup.reduce((sum, node) => sum + node.size, 0);
  const ratio = total === 0 ? 0 : firstTotal / total;

  if (horizontalSplit) {
    const firstWidth = Math.round(bounds.width * ratio);
    return [
      ...layoutTreemap(firstGroup, {
        x: bounds.x,
        y: bounds.y,
        width: firstWidth,
        height: bounds.height,
      }),
      ...layoutTreemap(secondGroup, {
        x: bounds.x + firstWidth,
        y: bounds.y,
        width: bounds.width - firstWidth,
        height: bounds.height,
      }),
    ];
  }

  const firstHeight = Math.round(bounds.height * ratio);
  return [
    ...layoutTreemap(firstGroup, {
      x: bounds.x,
      y: bounds.y,
      width: bounds.width,
      height: firstHeight,
    }),
    ...layoutTreemap(secondGroup, {
      x: bounds.x,
      y: bounds.y + firstHeight,
      width: bounds.width,
      height: bounds.height - firstHeight,
    }),
  ];
}
