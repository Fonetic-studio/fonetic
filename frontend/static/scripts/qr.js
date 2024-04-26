var html5QrCode;
var config;
function initQR(){
    html5QrCode = new Html5Qrcode("reader");
    config = { fps: 60, qrbox: { width: 400, height: 400 } };
}
const qrCodeSuccessCallback = (decodedText, decodedResult) => {
    /* handle success */
    html5QrCode.stop()
    let info = JSON.parse(decodedText)
    switchvehicle(line=info.line, vehicle = info.vehicle)
    
};

function scanforQR(){
    initQR()
    console.log("scanning")
    // If you want to prefer back camera
    console.debug(config)
    html5QrCode.start({ facingMode: "environment" }, config, qrCodeSuccessCallback);

    setTimeout(function(){
        let doc = document.getElementsByTagName("video")[0]
	console.debug(doc)
        console.debug(doc.style.width)
	//doc.style.width = "500px;"
        //doc.setAttribute("style", "width: 500px");
        console.log(doc)
    }, 500);
    //document.getElementsByTagName("video")[0].style.width = "500px;"
    //console.log(document.getElementsByTagName("video")[0])
}
