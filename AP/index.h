const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<style>
.card{
    max-width: 400px;
     min-height: 250px;
     background: #02b875;
     padding: 30px;
     box-sizing: border-box;
     color: #FFF;
     margin:20px;
     font-size: 15px;
     box-shadow: 0px 2px 18px -4px rgba(0,0,0,0.75);
}
.table{
      max-width: 400px;
     min-height: 250px;
     background: #02b875;
     padding: 30px;
     box-sizing: border-box;
     color: #FFF;
     margin:20px;
      font-size: 15px;
     box-shadow: 0px 2px 18px -4px rgba(0,0,0,0.75);
}

</style>
</head>
<body>

<div class="card">
  <form action="/action_page">
  Voltage:<br>
  <input type="number" name="Voltage" max="255" min="0" step="0.01">
  <br><br>
  CicleTime:<br>
  <input type="number" name="Cicle Time" step="0.01">
  <br><br>
  StimulusTime:<br>
  <input type="number" name="Stimulus Time" step="0.01">
  <br><br>
  <input type="submit" value="Submit">
  </form> 

</div>

<div class="table">
Data:<span id="Data">0</span>  <font-size= 15px;<br>

</div>
<script>

setInterval(function() {
  
  getData();
}, 1000); //mSeconds update rate

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("Data").innerHTML =
      this.responseText;
    }
  };
  xhttp.open("GET", "readData", true);
  xhttp.send();
}
</script>
</body>
</html>
)=====";
