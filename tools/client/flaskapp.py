from flask import Flask, send_from_directory
from pathlib import Path

ROOT_DIR = Path(__file__).absolute().parent
REACT_BUILD_DIR = ROOT_DIR.joinpath('build')

app = Flask(__name__, static_folder=REACT_BUILD_DIR)

@app.route("/")
def serve_react():
    return send_from_directory(REACT_BUILD_DIR, "index.html")

@app.route("/<path:path>")
def serve_static(path):
    return send_from_directory(REACT_BUILD_DIR, path)


if __name__ == "__main__":
    app.run(debug=True)
