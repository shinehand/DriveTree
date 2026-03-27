import { createReadStream, existsSync } from "node:fs";
import { extname, resolve, sep } from "node:path";
import { createServer } from "node:http";

const rootDir = process.cwd();
const port = Number(process.env.PORT || 3000);

const contentTypes = {
  ".css": "text/css; charset=utf-8",
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
};

createServer((request, response) => {
  const requestUrl = new URL(request.url ?? "/", `http://${request.headers.host ?? "localhost"}`);
  const requestPath = requestUrl.pathname === "/" ? "/index.html" : requestUrl.pathname;
  const relativePath = decodeURIComponent(requestPath).replace(/^\/+/, "");
  const filePath = resolve(rootDir, relativePath);

  if (filePath !== rootDir && !filePath.startsWith(`${rootDir}${sep}`)) {
    response.writeHead(403, { "content-type": "text/plain; charset=utf-8" });
    response.end("Forbidden");
    return;
  }

  if (!existsSync(filePath)) {
    response.writeHead(404, { "content-type": "text/plain; charset=utf-8" });
    response.end("Not found");
    return;
  }

  response.writeHead(200, {
    "content-type": contentTypes[extname(filePath)] ?? "application/octet-stream",
  });
  createReadStream(filePath).pipe(response);
}).listen(port, () => {
  console.log(`DriveTree running at http://localhost:${port}`);
});
