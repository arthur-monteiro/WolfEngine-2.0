from flask import Flask
import subprocess
import os

app = Flask(__name__)

@app.route("/graphictests")
def runGraphicTest():
    os.chdir('../Hello Triangle')
    result = subprocess.run('"../x64/Debug/Hello Triangle.exe" graphictests')

    return str(result.returncode)