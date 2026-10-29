"""Local HTTP fixture for the final LVRS runtime batch; never contacts a public host."""
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from threading import Thread
import subprocess
import sys

SVG=b'<svg xmlns="http://www.w3.org/2000/svg" width="8" height="8"><rect width="8" height="8" fill="red"/></svg>'

class Handler(BaseHTTPRequestHandler):
    def log_message(self,*args):
        pass
    def do_GET(self):
        path=self.path.split('?',1)[0]
        if path=='/root/redirect':
            self.send_response(302);self.send_header('Location','../asset.svg?redirected=1');self.end_headers();return
        if path=='/loop':
            self.send_response(302);self.send_header('Location','/loop');self.end_headers();return
        status=503 if path=='/status' else 200
        body=b'' if path=='/empty' else b'x'*(1024*1024+1) if path=='/limit' else SVG
        self.send_response(status);self.send_header('Content-Type','image/svg+xml')
        self.send_header('Content-Length',str(len(body)));self.end_headers()
        try:self.wfile.write(body)
        except (BrokenPipeError,ConnectionResetError):pass

def main():
    program,mode=sys.argv[1:]
    paths={'success':'/asset.svg','redirect':'/root/redirect','empty':'/empty','status':'/status','limit':'/limit','loop':'/loop'}
    server=ThreadingHTTPServer(('127.0.0.1',0),Handler)
    thread=Thread(target=server.serve_forever,daemon=True);thread.start()
    try:
        result=subprocess.run([program,'http_'+mode,f'http://127.0.0.1:{server.server_port}{paths[mode]}'],timeout=12)
        return result.returncode
    finally:
        server.shutdown();server.server_close();thread.join(timeout=2)

if __name__=='__main__':raise SystemExit(main())
