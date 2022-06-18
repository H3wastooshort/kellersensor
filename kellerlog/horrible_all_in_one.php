<!DOCTYPE html>
<head>
 <meta charset="utf-8">
 <meta http-equiv="X-UA-Compatible" content="IE=edge">
 <meta name="viewport" content="width=device-width, initial-scale=1">

 <meta http-equiv="refresh" content="60">

 <title>Kellersensor Graph</title>

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
	array_push($data, $col);
}
fclose($file);

print(json_encode($data));
?>;

//c++ style map()
function mapfloat(x, in_min, in_max, out_min, out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

var canvas = document.getElementById('graph');
var ctx = canvas.getContext('2d');

canvas.width = data.length;
canvas.height = document.documentElement.clientHeight;
	
ctx.fillStyle = 'black';
ctx.fillRect(0,0,canvas.width,canvas.height);

ctx.lineWidth = 1;

var oldDay = 0;
var lastTime = 9999999999999999;
ctx.fillStyle = 'lime';
ctx.font = '16px monospace';

for (var x = 1; x < data.length; x+=1) { //draw grid
    let date = new Date(); //Draw date lines
    date.setTime(data[x][0] * 1000);
    if (date.getDay() != oldDay) {
        ctx.fillStyle = 'grey';
        ctx.fillRect(x,0,1,canvas.height);
		oldDay = date.getDay();
    }
	
	if (data[x][0] - lastTime > 43200) {//Draw line for data gap
		ctx.fillStyle = 'orange';
		ctx.fillRect(x,0,1,canvas.height);
		//ctx.fillText("GAP IN DATA", x+5, 100);
	}
	lastTime = data[x][0];
}

function drawGraph(index,min,max,color) {
	ctx.beginPath();
	ctx.strokeStyle = color;
	for (var x = 1; x < data.length; x+=1) {	
		let y = mapfloat(data[x][index], min, max, canvas.height, 0); //Draw data point
		    if (x == 1) {
				ctx.moveTo(x,y);
			}
			else {
				ctx.lineTo(x,y);
			}
	}
	ctx.stroke();
}

var scale_label_dist = 3;
function drawScale(min,max,scale_step,unit,scale_color) {
	ctx.fillStyle = scale_color; //Draw scale lines
	for (var i = min; i <= max; i+=scale_step) {
		let y = mapfloat(i, min, max, canvas.height, 0);
		ctx.fillRect(0,y, canvas.width,1);
		ctx.fillText(i + unit, scale_label_dist, y-10);
	}
	scale_label_dist += 50;
}

drawScale(-15,30,5,"°C","#a00");
drawScale(0,100,10,"%","#0a0");
drawScale(0,1023,128,"","#00a");

drawGraph(1,-15,30,"#f00");
drawGraph(2,0,100,"#0f0");
drawGraph(3,0,1023,"#00f");


// Stale data warning
var dataDate = new Date();
dataDate.setTime(data[data.length-1][0] * 1000);
var nowDate = new Date();

var diffHours = (nowDate - dataDate) / 3600 / 1000;

if (diffHours > 1) {

    ctx.fillStyle = 'magenta';
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
    text += "\nTemperature: ";
    text += data[x][1];
    text += "°C\nHumidity: ";
    text += data[x][2];
    text += "%\nWater: ";
    text += data[x][3];
    alert(text);   
});
</script>
</body>
