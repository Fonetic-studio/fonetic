let MAX_TRAVEL_TIME = 60;
let MAX_MSG_TIME = 1;

const dateoptions = { weekday: 'long',  month: 'long', day: 'numeric', hour: 'numeric', minute: 'numeric', second: 'numeric' };

window.onload = function () {
    signin()
     setInterval(removeOldMsg, 1000); //1000*60*MAX_MSG_TIME); 
}

function switchvehicle(line=false, vehicle=false){
    //sessionStorage.clear()
    console.trace("clearing session storage")
    if (line==false){
        line = prompt("what line are you on?")
    }
    SSset("line", line)
    if (vehicle == false){
        vehicle = prompt("what is the vehicle number?")
    }
    SSset("vehicle", vehicle)
    SSset("travelstart", Date.now()) // to invalidate the input after x time (an hour?)
    joinroom(line, vehicle)
}

function signin(){
    // get vehicle data
    let line;
    let vehicle;
    // first check if it exists
    if (SSexists("line")){
        if (minutesSince(SSget("travelstart")) > MAX_TRAVEL_TIME) {// Too long since last input, sign in again
            switchvehicle() // allows user to set new vehicle data.
        }
        line = SSget("line")
        vehicle = SSget("vehicle")
        console.log(line)
        joinroom(line, vehicle)

    }
    
}

function SSexists(key){
    return JSON.parse(sessionStorage.getItem(key)) != null
}
function SSset(key, value){
    //console.trace("set: " + key + " to value: " + value)
    sessionStorage.setItem(key,JSON.stringify(value))
}

function SSget(key){
    return JSON.parse(sessionStorage.getItem(key))
}

function minutesSince(originalTime){
    let now = Date.now()
    let delta = now-originalTime
    minutes = Math.floor(delta/1000/60)
    return minutes
}

String.prototype.capitalize = function() {
    string = this
    return string.charAt(0).toUpperCase() + string.slice(1);
}

function createTextlog(msg){
    let msgel = document.createElement("div")
    msgel.classList.add("message")

    let timeel = document.createElement("span")
    timeel.classList.add("time")
    let curtime = new Date()
    timeel.textContent = curtime.toLocaleString(dateoptions)
    //timeel.textContent = new Date().toLocaleString("nl-NL", dateoptions)
    msgel.dataset.timestamp = Date.now()
    let textel = document.createElement("span")
    textel.classList.add("text")
    textel.textContent = msg

    msgel.append(timeel)
    msgel.append(textel)
    return msgel
}

function removeOldMsg(){
    // get messages
    let msgs = document.querySelectorAll(".message")

    for (x in Array.from(msgs)){
	let msg = msgs[x]
        let ts = msg.dataset.timestamp
	let timesince = minutesSince(ts)
	if (timesince >= MAX_MSG_TIME){
	    console.log("removing element for being old!")
	    document.querySelector('[data-timestamp="' + ts + '"]').remove()
	}
    }
}
