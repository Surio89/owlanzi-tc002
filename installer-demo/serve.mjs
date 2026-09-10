// SPDX-License-Identifier: GPL-3.0-or-later
// Local static preview only. There are no device, network-proxy or ADB routes.
import {createServer} from 'node:http';
import {readFile} from 'node:fs/promises';
const port=Number(process.env.OWLANZI_PREVIEW_PORT||8091);
if(!Number.isInteger(port)||port<1024||port>65535)throw new Error('Invalid preview port');
const page=new URL('./dist/index.html',import.meta.url);
const server=createServer(async(req,res)=>{
 if(req.method!=='GET'&&req.method!=='HEAD'){res.writeHead(405,{'Allow':'GET, HEAD'});return res.end();}
 if(req.url==='/favicon.ico'){res.writeHead(204);return res.end();}
 if(req.url!=='/'&&req.url!=='/index.html'){res.writeHead(404,{'Content-Type':'text/plain; charset=utf-8'});return res.end('Not found');}
 try{const html=await readFile(page);res.writeHead(200,{'Content-Type':'text/html; charset=utf-8','Cache-Control':'no-store','X-Content-Type-Options':'nosniff','Referrer-Policy':'no-referrer'});res.end(req.method==='HEAD'?undefined:html);}catch{res.writeHead(500);res.end('Preview file unavailable');}
});
server.listen(port,'127.0.0.1',()=>console.log(`Owlanzi installer demo: http://127.0.0.1:${port}`));
for(const signal of ['SIGINT','SIGTERM'])process.on(signal,()=>server.close(()=>process.exit(0)));
