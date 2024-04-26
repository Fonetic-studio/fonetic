from flask import Flask, render_template, request
from flask_socketio import SocketIO, join_room, leave_room, send, emit, disconnect, rooms
from time import sleep, time
import os, subprocess, json


with open(".keys.json") as f:
    keys = json.loads(f.read())
app = Flask(__name__)
app.config['SECRET_KEY'] = keys["flask_secret"]
socketio = SocketIO(app)
socketio.init_app(app, cors_allowed_origins=["https://www.fonetic.app", "https://fonetic.app"])

userdict = {}

@app.route("/")
def main():
    return render_template("index.html")

def gettarget(data):
    """gets the most specific target from data"""
    if data.get("vehicle"):
        return {"target": data["vehicle"], "source": "vehicle"}
    if data.get("line"):
        return {"target": data["line"], "source": "line"}
    if data.get("region"):
        return {"target": data["region"], "source": "region"}
    if data.get("global") == True:
        return {"target": "global", "source": "global"}
    return {"target": False}

@app.route("/announcements", methods=["GET", "POST"]) # currently no auth required
def announcements():
    if request.method == 'POST':
        """ POST example: 
            {
                "line": "12",
                "vehicle": "3012",
                "time": 1708032858,
                "msg": "3 minute delay"
            }
        """
        data = request.json
        print(data)
        targetdata = gettarget(data)
        target = targetdata["target"]
        print(targetdata)

        if targetdata["target"]:
            msg = data["msg"]
            source = targetdata["source"]
            if source in ["vehicle", "line"]:
                emit("notification", msg, to=target, namespace="/")
            elif source in ["region"]:
                emit("alert", msg, to=target, namespace="/")
            else:
                emit("alert", msg, broadcast=True, namespace="/")
            print(f"sent msg: {msg} to {target}")
            return f"msg: {msg} sent."
        
        return "couldn't send message, no target found."

    return "200"

def minutesSince(oldtime):
    return int(time())-oldtime//60

@socketio.on("join")
def on_join(data):
    line = data.get("line", None)
    vehicle = data.get("vehicle")

    print(f"line: {line}")
    if line:
        join_room(line)
        emit("room", {"join": {"line":line}}, to=line)
    if vehicle:
        join_room(vehicle)
        emit("room", {"join": {"vehicle":vehicle}}, to=vehicle)
        #get_region_from_vehicle() = "amsterdam-west" # TO IMPLEMENT
        join_room("amsterdam-west") # for region wide announcements
    else:
        print(data)


@socketio.on("message")
def on_message(data):
    print(data)

@socketio.on("leave")
def on_leave(data):
    line = data["line"]
    vehicle = data["vehicle"]
    leave_room(line)
    leave_room(vehicle)
    send("someone has left the room.", to=line)
    send("someone has left the room.", to=vehicle)

app.run(debug=True, host="0.0.0.0", port=9876)

