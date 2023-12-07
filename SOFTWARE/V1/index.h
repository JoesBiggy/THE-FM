const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <script
      src="https://code.jquery.com/jquery-3.7.1.min.js"
      integrity="sha256-/JqT3SQfawRcv/BIHPThkBvs0OEvtFFmqPF/lYI/Cxo="
      crossorigin="anonymous"
    ></script>
    <title>Controller</title>
  </head>
  <body>
    <h2 class="text-center" onclick="websocket.send('connected')">Controller</h2>
    <p id="statustext" onclick="reboot()" class="text-center">-</p>
    <div class="info">
      <h4>Motor-Overview</h4>
      <table class="ttable">
        <thead>
          <th class="tth">Variable</th>
          <th class="tth">Value</th>
        </thead>
        <tbody>
          <tr>
            <td class="ttd">Status</td>
            <td class="ttd" id="motor_state">-</td>
          </tr>
          <tr>
            <td class="ttd">Speed</td>
            <td class="ttd" id="motor_speed">-</td>
          </tr>
          <tr>
            <td class="ttd" >StepperSpeed</td>
            <td class="ttd" id="motor_speed_stepper">-</td>
          </tr>
          <tr>
            <td class="ttd">Stufe</td>
            <td class="ttd" id="motor_stufe">-</td>
          </tr>
          <tr>
            <td class="ttd">Modus</td>
            <td class="ttd" id="playmode">-</td>
          </tr>
          
          
        </tbody>
      </table>
    </div>
    <hr />
    <div class="controls">
      <div class="directions"></div>
      <div class="speed">
        <h5>Speed</h5>
        <table>
          <tr>
            <td>
              <button
                onclick="websocket.send('addspeed')"
                class="bigbutton"
                id="addspeed"
              >
                +1
              </button>
            </td>
            <td>
              <button
                onclick="websocket.send('remspeed')"
                class="bigbutton"
                id="remspeed"
              >
                -1
              </button>
            </td>
            <tr>
            <td>
              <button
                onclick="websocket.send('addspeed5')"
                class="bigbutton"
                id="addspeed"
              >
                +5
              </button>
            </td>
            <td>
              <button
                onclick="websocket.send('remspeed5')"
                class="bigbutton"
                id="remspeed"
              >
                -5
              </button>
            </td>
          </tr>
        </table>
      </div>
      <div class="speed">
        <h5>Motor</h5>
        <table>
          <tr>
            <td>
              <button
                onclick="websocket.send('motor_on')"
                class="bigbutton"
                id="motoron"
              >
                ON
              </button>
            </td>
            <td>
              <button
                onclick="websocket.send('motor_off')"
                class="bigbutton"
                id="motoroff"
              >
                OFF
              </button>
            </td>
          </tr>
          <tr>
            <td colspan="2">
              <table >
                <tr>
                  <td>
                    <button
                      onclick="websocket.send('mode0')"
                      class="bigbutton"
                      id="motoroff">
                      SPEED CTRL.
                    </button>
                  </td>
                  <td>
                    <button
                      onclick="websocket.send('mode1')"
                      class="bigbutton"
                      id="motoroff">
                      START STOP
                    </button>
                  </td>
                  <td>
                    <button
                      onclick="websocket.send('mode2')"
                      class="bigbutton"
                      id="motoroff">
                      WAVE
                    </button>
                  </td>
                  <td>
                    <button
                      onclick="websocket.send('mode3')"
                      class="bigbutton"
                      id="motoroff">
                      EDGING
                    </button>
                  </td>
                </tr>
            </td>
          </tr>
        </table>
      </div>
    </div>
    <div>
      <button
        onclick="websocket.send('motor_stop')"
        class="bigbutton"
        style="background-color: red; height: 100px"
      >
        STOP
      </button>
    </div>
  </body>

  <script>
    function sync_info(text) {
      $("#statustext").html(text);
    }

    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;
    function initWebSocket() {
      console.log("Trying to open a WebSocket connection...");
      websocket = new WebSocket(gateway);
      websocket.onopen = onOpen;
      websocket.onclose = onClose;
      websocket.onmessage = onMessage; // <-- add this line
    }
    function onOpen(event) {
      console.log("Connection opened");
      sync_info("Controller connected!");
      websocket.send('connected');
    }
    function onClose(event) {
      console.log("Connection closed");
      sync_info("Controller disconnected!");
      setTimeout(initWebSocket, 2000);
    }
    function onMessage(event) {
      var datatx = event.data;
      const data = JSON.parse(datatx);
      console.debug("got data!");
      const date = new Date();
      sync_info("SYNCED - " + date.toTimeString());
      if (data.motor_speed != null) {
        $("#motor_speed").html(data.motor_speed);
      }
      if (data.motor_speed_stepper != null) {
        $("#motor_speed_stepper").html(data.motor_speed_stepper);
      }
      if (data.motor_stufe != null) {
        $("#motor_stufe").html(data.motor_stufe);
      }
      if (data.playmode != null) {
        $("#playmode").html(data.playmode);
      }
      if (data.motor_state != null) {
        if (data.motor_state == true) {
          $("#motor_state").html("ON");
        } else {
          $("#motor_state").html("OFF");
        }
      }
    }
    window.addEventListener("load", onLoad);
    function onLoad(event) {
      initWebSocket();
      
    }
    var rebootbutton = 0;
    function reboot()
    {
      if (rebootbutton == 10)
      {
        rebootbutton = 0;
        websocket.send('reboot');
        sync_info("REBOOT!");
      }
      else
      {
        rebootbutton +=1;
      }
    }
  </script>

  <style>
    body {
      background-color: #232323;
      color: white;
      font-family: Arial, Helvetica, sans-serif;
    }
    button {
      border-radius: 16px;
      border: 0px solid;
    }

    .text-center {
      text-align: center;
    }
    table {
      width: 100%;
      border-collapse: collapse;
    }
    .smallbutton {
      width: 25%;
      height: 50px;
    }
    .mediumbutton {
      width: 50%;
      height: 50px;
    }
    .bigbutton {
      width: 100%;
      height: 50px;
    }
    #ttable, #ttd, #tth {
    border: #ffffff 1px solid;
    }
  </style>
</html>
)rawliteral";