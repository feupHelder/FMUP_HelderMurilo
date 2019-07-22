const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<style>
.card{
    max-width: 400px;
     min-height: 250px;
     background: #02b875;
     padding: 30px;
     box-sizing: border-box;
     color: #FFF;
     margin:20px;
     box-shadow: 0px 2px 18px -4px rgba(0,0,0,0.75);
}
</style>
<body>

<div class="card">
  <h1>Sensor Value:<span id="SensorValue">0</span></h1><br>
  

  <form action="/action_page">
  Voltage:<br>
  <input type="number" name="Voltage" max="5" min="0" step="0.01">
  <br><br>
  <input type="submit" value="Submit">
  </form> 

</div>


<script>

setInterval(function() {
  // Call a function repetatively with 2 Second interval
  getData();
}, 500); //2000mSeconds update rate

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("SensorValue").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "readSensor", true);
  xhttp.send();
}
</script>
</body>
</html>
)=====";
