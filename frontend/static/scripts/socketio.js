var socket = io();
socket.on('connect', function() {
    console.info("connected to server. trying to join!")
    //socket.emit('join', {data: 'I\'m connected!'});
    joinroom()
});

socket.on("message", function(msg){
    console.info("received message")
    console.log(msg)
})
socket.on("room", function(msg){
    console.log(msg)
    console.log(Object.keys(msg))
    if ("join" in msg){
        let jointype = Object.keys(msg["join"])[0]
        let joinval = msg["join"][jointype]
        console.log(jointype)
        console.log("Joined",jointype,joinval)
        document.getElementById(jointype).textContent = jointype.capitalize() + " " + joinval 
    }
})
socket.on("notification", function(msg){
    console.info(msg)
    Toastify({
        text:msg,
        ariaLive:"assertive",
        position: "right"
    }).showToast()
    let msgel = createTextlog(msg)
    document.getElementsByClassName("messages")[0].append(msgel)
})

socket.on("alert", function(msg){
    console.warn(msg)
    Toastify({
        text: msg,
        ariaLive:"assertive",
        position: "center",
        style: {
            background: "black",
            color:"yellow"
          },
    }).showToast()
    let msgel = createTextlog(msg)
    msgel.classList.add("alert")
    document.getElementsByClassName("messages")[0].append(msgel)
})

socket.on("error", function(msg){
    console.error("something went wrong.")
    console.error(msg)
})

socket.on("disconnect", function(msg){
    console.error(msg)
    //alert("Disconnected from the server.")
})

function joinroom(line=false, vehicle=false){
    let joindict = {}
    if (line){joindict["line"] = line}
    if (vehicle){joindict["vehicle"] = vehicle}
    /*
    let totp = localStorage.getItem("totp")
    console.warn(totp)
    if (totp == "000000"){
	console.log("totp not filled yet, waiting")
	setTimeout(function() {
	    joinroom(line, vehicle);
    	}, 5000)
    } else {
        console.log("trying to join..")
	joindict["totpkey"] = totp
    }
    */
    if (Object.keys(joindict).length > 0){
	console.log(socket)
	console.log(joindict)
        socket.emit("join", joindict)
	//alert("socket")
        //console.info("Joined rooms. " + joindict)
    }
}
