#!/usr/bin/env python3
"""rvsim-gdb: a bridge between GDB (VS Code) on your computer and the RVisio SoC simulator in the browser.

The simulator runs in the browser, but GDB expects a TCP port. This program listens on 127.0.0.1:3333
(VS Code attaches there) and forwards the bytes to the RVisio server, which hands them to your page. It is
paired with the code the page shows after you click “🐞 GDB server” (analysis System on chip).

Usage (Python 3, nothing to install):
    python3 rvsim-gdb.py CODE
    python3 rvsim-gdb.py CODE --server https://rvisio.com --port 3333

Then press F5 in VS Code (a project from github.com/Department-Of-Electronics/rvisio). Stop: Ctrl+C.
"""

import argparse
import http.client
import json
import socket
import sys
import threading
import time
import urllib.parse

SERVER = "https://rvisio.com"  # when downloaded from RVisio, the page writes its own server address here


def say(text):
    print(time.strftime("%H:%M:%S"), text, flush=True)


class NotPaired(Exception):
    pass


class Server:
    """HTTP to the simulator server; every thread has its own (persistent) connection."""

    def __init__(self, url, token):
        u = urllib.parse.urlsplit(url if "://" in url else "http://" + url)
        self.https = u.scheme == "https"
        self.host = u.hostname
        self.port = u.port or (443 if self.https else 80)
        self.base = u.path.rstrip("/")
        self.token = token.strip().upper()
        self.local = threading.local()

    def _conn(self):
        c = getattr(self.local, "conn", None)
        if c is None:
            cls = http.client.HTTPSConnection if self.https else http.client.HTTPConnection
            c = self.local.conn = cls(self.host, self.port, timeout=30)
        return c

    def call(self, method, path, body=None):
        for attempt in (1, 2):
            c = self._conn()
            try:
                data = json.dumps(body).encode() if body is not None else None
                headers = {"Content-Type": "application/json"} if data is not None else {}
                c.request(method, self.base + path, body=data, headers=headers)
                r = c.getresponse()
                payload = r.read()
                if r.status == 404:
                    raise NotPaired(json.loads(payload or b"{}").get("error", "unknown code"))
                if r.status != 200:
                    raise OSError(f"the server answered {r.status}")
                return json.loads(payload or b"{}")
            except (OSError, http.client.HTTPException):
                c.close()
                self.local.conn = None
                if attempt == 2:
                    raise

    def up(self, data=b"", gdb=True):
        return self.call("POST", "/api/gdb/proxy/up", {"token": self.token, "data": data.hex(), "gdb": gdb})

    def down(self):
        return self.call("GET", "/api/gdb/proxy/down?token=" + urllib.parse.quote(self.token))


class Bridge:
    def __init__(self, server):
        self.server = server
        self.gdb = None          # current connection with GDB
        self.lock = threading.Lock()
        self.stop = threading.Event()

    # bytes from the simulator to GDB; the long poll also tells the server the bridge is alive
    def downlink(self):
        errors = 0
        paused = False
        while not self.stop.is_set():
            try:
                r = self.server.down()
                errors = 0
                if paused:
                    paused = False
                    say("The simulator is waiting for GDB again, the bridge is running")
            except NotPaired:
                if not paused:
                    paused = True
                    say("The GDB server in the simulator is stopped, waiting until you start it again (🐞 GDB server)…")
                self.drop_gdb()
                time.sleep(3)
                continue
            except (OSError, http.client.HTTPException) as e:
                errors += 1
                if errors == 1:
                    say(f"The server is not reachable ({e}), retrying…")
                time.sleep(2)
                continue
            data = bytes.fromhex(r.get("data", ""))
            if data:
                with self.lock:
                    g = self.gdb
                if g:
                    try:
                        g.sendall(data)
                    except OSError:
                        pass

    def drop_gdb(self):
        with self.lock:
            g, self.gdb = self.gdb, None
        if g:
            try:
                g.close()
            except OSError:
                pass

    # bytes from GDB to the simulator, in order
    def serve_gdb(self, conn, addr):
        say(f"GDB connected ({addr[0]}:{addr[1]})")
        try:
            r = self.server.up(b"", gdb=True)
            if r.get("page") is False:
                say("The tab with the SoC board is not open (or was reloaded): open the SoC in the browser, turn on “🐞 GDB server” and start debugging again")
                return
            while not self.stop.is_set():
                data = conn.recv(65536)
                if not data:
                    break
                self.server.up(data, gdb=True)
        except NotPaired:
            say("The simulator is not waiting for GDB (the GDB server is stopped), closing the GDB connection")
        except (OSError, http.client.HTTPException) as e:
            say(f"Connection error: {e}")
        finally:
            with self.lock:
                if self.gdb is conn:
                    self.gdb = None
                replaced = self.gdb is not None   # a new GDB connected in the meantime
            try:
                conn.close()
            except OSError:
                pass
            if not replaced:
                try:
                    self.server.up(b"", gdb=False)
                except Exception:
                    pass
            say("GDB disconnected")


def main():
    for stream in (sys.stdout, sys.stderr):  # terminal without UTF-8: "?" instead of a crash
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(errors="replace")
    ap = argparse.ArgumentParser(description="Bridge GDB (VS Code) <-> RVisio SoC simulator in the browser")
    ap.add_argument("code", help="the code the simulator shows after you click “GDB server”")
    ap.add_argument("--server", default=SERVER, help=f"simulator address (default {SERVER})")
    ap.add_argument("--port", type=int, default=3333, help="port GDB connects to (default 3333)")
    ap.add_argument("--host", default="127.0.0.1", help="address to listen on (default: this computer only)")
    a = ap.parse_args()

    server = Server(a.server, a.code)
    try:
        r = server.up(b"", gdb=False)
    except NotPaired as e:
        sys.exit(f"Error: {e}\n(the code is shown in the simulator: System on chip → 🐞 GDB server)")
    except (OSError, http.client.HTTPException) as e:
        sys.exit(f"Error: the server {a.server} is not reachable ({e})")

    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        listener.bind((a.host, a.port))
    except OSError as e:
        sys.exit(f"Error: port {a.port} is in use ({e}). Try --port 3334 (and the same port in .vscode/launch.json).")
    listener.listen(1)
    listener.settimeout(1.0)

    bridge = Bridge(server)
    threading.Thread(target=bridge.downlink, daemon=True).start()
    say(f"Bridge running: simulator {a.server} (user {r.get('user', '?')}) <-> GDB on {a.host}:{a.port}")
    say("Start debugging in VS Code (F5). Stop: Ctrl+C.")
    try:
        while not bridge.stop.is_set():
            try:
                conn, addr = listener.accept()
            except socket.timeout:
                continue
            conn.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            with bridge.lock:
                old, bridge.gdb = bridge.gdb, conn
            if old:
                try:
                    old.close()
                except OSError:
                    pass
            threading.Thread(target=bridge.serve_gdb, args=(conn, addr), daemon=True).start()
    except KeyboardInterrupt:
        pass
    finally:
        bridge.stop.set()
        listener.close()
        try:
            server.up(b"", gdb=False)
        except Exception:
            pass
        say("Bridge stopped")


if __name__ == "__main__":
    main()
