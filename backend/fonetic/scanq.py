import os
import requests
from time import sleep
from colorzero import Color
from gpiozero import RGBLED

Path = "queue"
Path_in = Path + "/in"
Path_out = Path + "/out"

def Process(file):
    # create filenames
    raw = Path_in + "/" + file
    mp3 = Path_out + "/" + file[0:-4] + ".mp3"

    LED1.color = Color('#ff0000')

    # convert to mp3
    channels = os.getenv("FONETIC_CHANNELS")
    cmd = f"/usr/bin/ffmpeg -hide_banner -loglevel error -y -f s16le -sample_rate 48000 -channels {channels} -i {raw} -b:a 128k -acodec mp3 {mp3}"
    os.system(cmd);

    # remove raw file
    os.unlink(raw)

    LED1.color = Color('#0000ff')

    # send to server
    payload = {
        "key": "hQYDSGC3qMtdpsEwVxAWTazPHFyN2Xb79kRv5gKcjZmJ68LU4rXqtB9ewczkKEZC2rHS83GhLDUFmuyg4s6TnVadvP5YRNJxfWMAkweF5Y6PKjspzhSbx7uBHmUTGJqgRtNy2n8VrDCc4ZdfMv9AQW",
        "method": "transcribe"
    }

    try:
        files = {'audio': open(mp3, 'rb')}
        r = requests.post("https://local.noties.nl/rest/fonetic/api.php", params=payload, files=files, timeout=10)

        if r.status_code != 200:
            print(f"Failed ({r})")
            LED1.color = Color('#ffa500')
            sleep(5)
        else:
            print(f"{file} - {r.text}")
            LED1.color = Color('#00ff00')
            sleep(0.5)
            os.unlink(mp3)

    except:
        LED1.color = Color('#ff0000')
        sleep(5)

#
# Scan the audio queue
#

LED1 = RGBLED(17, 22, 27)
LED1.color = Color('#000000')

for file in os.listdir(Path_in):
    if file[-4:] == ".raw":
        Process(file)
