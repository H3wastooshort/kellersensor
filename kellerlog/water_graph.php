<!DOCTYPE html>
<head>
 <meta charset="utf-8">
 <meta http-equiv="X-UA-Compatible" content="IE=edge">
 <meta name="viewport" content="width=device-width, initial-scale=1">

 <meta http-equiv="refresh" content="60">

 <title>Water Graph</title>

<style>body {
  background: black;
  color: lime;
  scrollbar-width: thin;
}

#graph {
  height: 100%;
  width: max-content;
  position: absolute;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  image-rendering: optimizeSpeed;
  image-rendering: -moz-crisp-edges;
  image-rendering: -webkit-optimize-contrast;
  image-rendering: optimize-contrast;
  -ms-interpolation-mode: nearest-neighbor;
}</style>
</head>
<body>
<canvas id="graph"></canvas>

<script>
const data = <?php
$file = fopen("../keller_log.csv", "r");
$data = array();
while (($col = fgetcsv($file, 100, ",")) !== FALSE) {
	array_push($data, array($col[0],$col[3]));
}
fclose($file);

print(json_encode($data));
?>;

//c++ style map()
function mapfloat(x, in_min, in_max, out_min, out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

const canvas = document.getElementById('graph');
const ctx = canvas.getContext('2d');

canvas.width = data.length;
canvas.height = document.documentElement.clientHeight;
	
ctx.fillStyle = 'black';
ctx.fillRect(0,0,canvas.width,canvas.height);

ctx.lineWidth = 1;
ctx.strokeStyle = 'lime';

var oldDay = 0;
var lastTime = 9999999999999999;
ctx.fillStyle = 'lime';
ctx.font = '16px monospace';

ctx.fillStyle = 'magenta'; //Draw scale lines
for (var i = 0; i <= 1023; i+=128) {
	let y = mapfloat(i, 0, 1023, canvas.height, 0);
    ctx.fillRect(0,y, canvas.width,1);
    ctx.fillText(i, 3, y-10);
}

ctx.beginPath();

for (var x = 1; x < data.length; x+=1) {
    let date = new Date(); //Draw date lines
    date.setTime(data[x][0] * 1000);
    if (date.getDay() != oldDay) {
        ctx.fillStyle = 'cyan';
        ctx.fillRect(x,0,1,canvas.height);
        //ctx.fillText(date.toDateString(), x+5, canvas.height-50);
        ctx.fillStyle = 'lime';
		oldDay = date.getDay();
    }
	
	if (data[x][0] - lastTime > 43200) {//Draw line for data gap
		ctx.fillStyle = 'orange';
		ctx.fillRect(x,0,1,canvas.height);
		//ctx.fillText("GAP IN DATA", x+5, 100);
		ctx.fillStyle = 'lime';
	}
	lastTime = data[x][0];
	
	let y = mapfloat(data[x][1], 0, 1023, canvas.height, 0); //Draw data point
	
    if (x == 1) {
        ctx.moveTo(x,y);
    }
    else {
        ctx.lineTo(x,y);
    }
}
ctx.stroke();


// Stale data warning
var dataDate = new Date();
dataDate.setTime(data[data.length-1][0] * 1000);
var nowDate = new Date();

var diffHours = (nowDate - dataDate) / 3600 / 1000;

if (diffHours > 1) {

    ctx.fillStyle = 'red';
    ctx.textAlign = 'right';
    ctx.font = '50px monospace';
    var text = "Data stale by ";
    text += Math.floor(diffHours);
    text += " hours";
    
    ctx.fillRect(canvas.width - 1, 0, 1, canvas.height);
    ctx.fillText(text, canvas.width, canvas.height * 0.75);
}


canvas.addEventListener("mousedown", function(e) { //Datapoint pop-up box
    //let x = e.clientX - canvas.getBoundingClientRect().left + 1;
	let x = e.offsetX;
    let date = new Date();
    date.setTime(data[x][0] * 1000);
    let text = "Unix Timestamp: ";
    text += data[x][0];
    text += "\nDate: ";
    text += date.toString();
    text += "\nWater: ";
    text += data[x][1];
    alert(text);   
});
</script>
</body>
