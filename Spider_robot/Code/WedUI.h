#ifndef wed_h
#define wed_h

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
 #include "stdlib.h"
 #include "wiring.h"
#endif

char SetupPage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Spider Robot Setup</title>
  <style>
    body {
      margin: 30px;
      font-family: sans-serif;
      background-color: #f8f8f8;
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .spider-layout {
      display: grid;
      grid-template-areas:
        "left canvas right";
      gap: 10px;
      align-items: center;
    }

    .left-column, .right-column {
      display: flex;
      flex-direction: column;
      gap: 20px;
    }

    .left-column {
      grid-area: left;
    }

    .right-column {
      grid-area: right;
    }

    .group {
      border: 1px solid #ccc;
      border-radius: 8px;
      padding: 2px;
      background-color: #fff;
      width: 126px;
    }

    .group h3 {
      margin: 0 0 10px 0;
      text-align: center;
    }

    .pair {
      margin-bottom: 10px;
      text-align: center;
    }

    .pair label {
      display: block;
      font-weight: bold;
    }

    .pair button {
      margin: 2px;
      width: 56px;
    }

    .canvas-group {
      grid-area: canvas;
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    canvas {
      border: 2px solid #333;
      background-color: #e0e0e0;
      margin-bottom: 10px;
    }

    .canvas-buttons button {
      margin: 5px;
      width: 60px;
    }

    #output {
      position: absolute;
      bottom: 20px;
      width: 90%;
      height: 100px;
    }
    .status {
      margin-top: 15px;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 2px;
    }
    .led {
      width: 15px;
      height: 15px;
      border-radius: 50%;
      background-color: red;
      border: 1px solid #000;
    }
  </style>
</head>
<body onload="javascript:init()">

<div class="spider-layout">
  <div class="left-column">
    <!-- Fourth Leg -->
    <div class="group fourth">
      <h3>Third Leg</h3>
      <div class="pair"><label>Coxa</label><button onclick="logMessage('Up22')">Up</button><button onclick="logMessage('Down22')">Down</button></div>
      <div class="pair"><label>Femur</label><button onclick="logMessage('Up20')">Up</button><button onclick="logMessage('Down20')">Down</button></div>
      <div class="pair"><label>Tibia</label><button onclick="logMessage('Up21')">Up</button><button onclick="logMessage('Down21')">Down</button></div>
    </div>

    <!-- Third Leg -->
    <div class="group third">
      <h3>Fourth Leg</h3>
      <div class="pair"><label>Coxa</label><button onclick="logMessage('Up32')">Up</button><button onclick="logMessage('Down32')">Down</button></div>
      <div class="pair"><label>Femur</label><button onclick="logMessage('Up30')">Up</button><button onclick="logMessage('Down30')">Down</button></div>
      <div class="pair"><label>Tibia</label><button onclick="logMessage('Up31')">Up</button><button onclick="logMessage('Down31')">Down</button></div>
    </div>
  </div>

  <!-- Central Canvas with Buttons -->
  <div class="canvas-group">
    <canvas id="spiderCanvas" width="50" height="200"></canvas>
    <div class="status">
      <div class="led" id="statusLed"></div>
      <span id="statusText">Offline</span>
    </div><br>
    <div class="canvas-buttons">
      <button onclick="logMessage('Save')">Save</button><br>
      <button onclick="changePage()">Control</button>
    </div>
  </div>

  <div class="right-column">
    <!-- First Leg -->
    <div class="group first">
      <h3>First Leg</h3>
      <div class="pair"><label>Coxa</label><button onclick="logMessage('Up02')">Up</button><button onclick="logMessage('Down02')">Down</button></div>
      <div class="pair"><label>Femur</label><button onclick="logMessage('Up00')">Up</button><button onclick="logMessage('Down00')">Down</button></div>
      <div class="pair"><label>Tibia</label><button onclick="logMessage('Up01')">Up</button><button onclick="logMessage('Down01')">Down</button></div>
    </div>

    <!-- Second Leg -->
    <div class="group second">
      <h3>Second Leg</h3>
      <div class="pair"><label>Coxa</label><button onclick="logMessage('Up12')">Up</button><button onclick="logMessage('Down12')">Down</button></div>
      <div class="pair"><label>Femur</label><button onclick="logMessage('Up10')">Up</button><button onclick="logMessage('Down10')">Down</button></div>
      <div class="pair"><label>Tibia</label><button onclick="logMessage('Up11')">Up</button><button onclick="logMessage('Down11')">Down</button></div>
    </div>
  </div>
</div>

<script>
  let statusTimeout;
  
  function init() {
    socket = new WebSocket('ws://' + window.location.hostname + ':81/');
    socket.onmessage = function(event) {
      var dataTxtBuffer = event.data;
      if (dataTxtBuffer.indexOf("Checked") == 0) {
        setOnline();
        clearTimeout(statusTimeout);
        statusTimeout = setTimeout(setOffline, 2000); // 2 seconds to go offline
      }
    }
  }
  
  function logMessage(msg) {
    socket.send(msg);
  }
  
  // Draw direction arrow in canvas
  const canvas = document.getElementById("spiderCanvas");
  const ctx = canvas.getContext("2d");

  ctx.beginPath();
  ctx.moveTo(25, 20);
  ctx.lineTo(15, 30);
  ctx.moveTo(25, 20);
  ctx.lineTo(35, 30);
  ctx.moveTo(25, 180);
  ctx.lineTo(25, 18);
  ctx.strokeStyle = "#000";
  ctx.lineWidth = 4;
  ctx.stroke();

  function setOnline() {
    document.getElementById("statusLed").style.backgroundColor = "green";
    document.getElementById("statusText").textContent = "Online";
  }

  function setOffline() {
    document.getElementById("statusLed").style.backgroundColor = "red";
    document.getElementById("statusText").textContent = "Offline";
  }

  function changePage(){
    location.replace("/");
  }

  setOnline();
</script>

</body>
</html>
)=====";

char ControlPage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Robot Motion Control</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      margin: 30px;
    }

    .controls {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 15px;
    }

    .button-group {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      justify-content: center;
    }

    .control-button {
      padding: 10px 20px;
      font-size: 4vw;
      width: 25vw;
      aspect-ratio: 1 / 1;
      border-radius: 6px;
      border: 1px solid #333;
      background-color: green;
      color: white;
      cursor: pointer;
      transition: background-color 0.3s;
    }

    .control-button.active {
      background-color: red;
    }

    .slider-group {
      margin-top: 15px;
    }

    .status {
      margin-top: 20px;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 10px;
    }

    .led {
      width: 16px;
      height: 16px;
      border-radius: 50%;
      background-color: red;
      border: 1px solid #000;
    }
  </style>
</head>
<body onload="javascript:init()">
  <div class="controls">
    <div class="status">
      <div class="led" id="statusLed"></div>
      <span id="statusText">Offline</span>
    </div>
    
    <div class="slider-group">
      <label for="speedSlider">Speed: </label><span id="speedValue">6</span><br>
      <input type="range" id="speedSlider" min="1" max="10" value="6" oninput="updateSlider(this.value)">
    </div>
    
    <div class="button-group" id="commandButtons">
      <button class="control-button" onclick="handleButtonPress(this, 'Wave')">Hand wave</button>
      <button class="control-button" onclick="handleButtonPress(this, 'Forward')">Forward</button>
      <button class="control-button" onclick="handleButtonPress(this, 'Shake')">Hand shake</button>
    </div>
    <div class="button-group" id="commandButtons">
      <button class="control-button" onclick="handleButtonPress(this, 'Left')">Turn Left</button>
      <button class="control-button active" onclick="handleButtonPress(this, 'Stand')">Stand</button>
      <button class="control-button" onclick="handleButtonPress(this, 'Right')">Turn Right</button>
    </div>
    <div class="button-group" id="commandButtons">
      <button class="control-button" onclick="handleButtonPress(this, 'Stomp')">Stomp</button>
      <button class="control-button" onclick="handleButtonPress(this, 'Back')">Back</button>
      <button class="control-button" onclick="handleButtonPress(this, 'Sit')">Sit</button>
    </div>
    <div class="button-group" id="commandButtons">
      <button onclick="changePage()">Setup</button>
    </div>
  </div>

  <script>
    let statusTimeout;
  
    function init() {
      socket = new WebSocket('ws://' + window.location.hostname + ':81/');
      socket.onmessage = function(event) {
        var dataTxtBuffer = event.data;
        if (dataTxtBuffer.indexOf("Checked") == 0) {
          setOnline();
          clearTimeout(statusTimeout);
          statusTimeout = setTimeout(setOffline, 2000); // 2 seconds to go offline
        }
        else if (dataTxtBuffer.indexOf("Speed") == 0) {
          const match = dataTxtBuffer.match(/^Speed(\d{1,2})$/i);
          if (match) {
            const speed = parseInt(match[1]);
            if (speed >= 1 && speed <= 10) {
              document.getElementById("speedValue").textContent = val;
            }
          }
        }
      }
    }

    function handleButtonPress(button, command) {
      // Remove 'active' from all buttons
      const allButtons = document.querySelectorAll(".control-button");
      allButtons.forEach(btn => btn.classList.remove("active"));

      // Add 'active' to the clicked one
      button.classList.add("active");

      socket.send(command);
    }

    function updateSlider(val) {
      document.getElementById("speedValue").textContent = val;
      socket.send("Speed" + val);
    }

    function setOnline() {
      document.getElementById("statusLed").style.backgroundColor = "green";
      document.getElementById("statusText").textContent = "Online";
      console.log("Status: Online");
    }

    function setOffline() {
      document.getElementById("statusLed").style.backgroundColor = "red";
      document.getElementById("statusText").textContent = "Offline";
      console.log("Status: Offline");
    }
    
    function changePage(){
      location.replace("/setup");
    }

    setOnline();
  </script>
</body>
</html>
)=====";
#endif
