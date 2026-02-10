import http.server
import socketserver
import os

PORT = 8080
DIRECTORY = "www"

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_POST(self):
        if self.path == '/login':
            self.send_response(302)
            self.send_header('Location', '/menu.html')
            self.end_headers()
        elif self.path == '/book':
            self.send_response(302)
            self.send_header('Location', '/confirm.html')
            self.end_headers()
        elif self.path == '/cancel':
            self.send_response(302)
            self.send_header('Location', '/cancel_confirm.html')
            self.end_headers()
        else:
            self.send_error(404, "File not found")

    def do_GET(self):
        # Handle custom routes without .html extension
        if self.path == '/':
            self.path = '/index.html'
        elif self.path == '/menu':
            self.path = '/menu.html'
        elif self.path == '/flights':
            self.path = '/flights.html'
        elif self.path == '/book':
            self.path = '/book.html'
        elif self.path == '/cancel':
            self.path = '/cancel.html'
        elif self.path == '/cancel_confirm':
            self.path = '/cancel_confirm.html'
        elif self.path == '/confirm':
            self.path = '/confirm.html'
        elif self.path == '/logout':
            self.send_response(302)
            self.send_header('Location', '/')
            self.end_headers()
            return

        return super().do_GET()

os.makedirs(DIRECTORY, exist_ok=True)
with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
    print(f"Server started at http://localhost:{PORT}")
    print("Serving files from", DIRECTORY)
    httpd.serve_forever()
