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

  <form action="/goData_page">
  <input type="submit" value="Data Table">
  </form> 

</div>

</body>
</html>
)=====";

const char script[] PROGMEM = R"=====(
<script>
function download_csv(csv, filename) {
    var csvFile;
    var downloadLink;

    // CSV FILE
    csvFile = new Blob([csv], {type: "text/csv"});

    // Download link
    downloadLink = document.createElement("a");

    // File name
    downloadLink.download = filename;

    // We have to create a link to the file
    downloadLink.href = window.URL.createObjectURL(csvFile);

    // Make sure that the link is not displayed
    downloadLink.style.display = "none";

    // Add the link to your DOM
    document.body.appendChild(downloadLink);

    // Lanzamos
    downloadLink.click();
}

function export_table_to_csv(html, filename) {
  var csv = [];
  var rows = document.querySelectorAll("table tr");
  
    for (var i = 0; i < rows.length; i++) {
    var row = [], cols = rows[i].querySelectorAll("td, th");
    
        for (var j = 0; j < cols.length; j++) 
            row.push(cols[j].innerText);
        
    csv.push(row.join(","));    
  }

    // Download CSV
    download_csv(csv.join("\n"), filename);
}

document.querySelector("button").addEventListener("click", function () {
    var html = document.querySelector("table").outerHTML;
  export_table_to_csv(html, "table.csv");
});
</script>
)=====";

/* SCRIPT PARA ATUALIZAR INFO NO BROWSER
 * <div class="table">
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
*/
