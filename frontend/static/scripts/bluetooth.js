let btdev;
let codechar;

function scanDevices(){
  localStorage.setItem("totp", "000000")
  console.log(navigator.bluetooth.getDevices())
  navigator.bluetooth.requestDevice({
    filters: [{
        	"namePrefix":"fonetic"
	   }],
    optionalServices: ['b1193402-4ee7-40b6-ba03-c2bda2d05347'] // Required to access service later.
  })
  .then(device => {
	console.log("Found it!")
	createTextlog("found device")
	btdev = device
	console.log(device)
	localStorage.setItem("linedata", device.name.split("-").slice(1))
	readCode()
	//switchvehicle(line, vehnum)
	})
  .catch(error => {
	console.error(error)
	createTextlog(error)
  });
}


function readCode(){
  btdev.gatt.connect().then(server => {
    console.log('Getting totp Service...');
    return server.getPrimaryService('b1193402-4ee7-40b6-ba03-c2bda2d05347');
  })
  .then(service => {
    console.log('Getting totp Characteristic...');
    return service.getCharacteristic('f431cc85-d318-4221-b74e-922ce5961b15');
  })
  .then(characteristic => {
    codechar = characteristic;
    console.log(codechar)
    codechar.addEventListener('characteristicvaluechanged',
        handlecodeChanged);
    codechar.startNotifications()
    console.log("Added event handler");
  })
}

function handlecodeChanged(event) {
  let bf = event.target.value
  let str = new TextDecoder().decode(bf)
  let totpcode = JSON.parse(str)
  console.log('> code is ' + totpcode);
  localStorage.setItem("totp", totpcode)
  let [line,vehnum] = localStorage.getItem("linedata").split(",")
  console.log(line, vehnum)
  switchvehicle(line,vehnum)
}
