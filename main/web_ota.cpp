#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <WiFi.h>
#include "matrix_globals.h"
#include "globals.h"
#include "web_ota.h"
#include "clock.h"
#include "config.h"


// ===== INDEX PAGE =====
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<body style='background:#1e1e1e; color:#e0e0e0; display:flex; flex-direction:column; justify-content:center; align-items:center; height:100vh; margin:0;'>
  <h2>LED Matrix Display Web</h2>
  <ul style='list-style:none; padding:0; margin-top:20px;'>
    <li><a href='/update' style='color:#ff9800; text-decoration:none; font-size:18px;'>OTA Update</a></li>
    <li><a href='#' onclick='rebootDevice()' style='position:absolute; bottom:16px; right:16px; color:#00e5ff; text-decoration:none; font-size:18px;'>Restart</a></li>
     <li><a href='javascript:void(0)' onclick='if(confirm("Restart Clock?")) fetch("/reboot");' 
     style='position:absolute; bottom:16px; right:16px; color:#00e5ff; text-decoration:none; font-size:18px;'>
     Restart</a></li>

  </ul>

<div style="position:absolute; top:12px; right:16px; font-size:14px; color:#ff9800;">
  <span id="ssid">---</span> | <span id="rssiValue">--</span> dBm
</div>

<script>

let rainActive = false;
let clicks = 0;
document.getElementById('ssid').onclick = function() {
  clicks ++;
  if(clicks > 4) {
    fetch('/matrix');
    clicks = 0;
    
    // 1. Change to "Matrix" style
    const el = this;
    const oldSSID = el.innerText; // Save current SSID
    el.innerText = "[MATRIX] Follow the white rabbit...";
    el.style.color = "#00ff00";

    // 2. Set a timer to revert the text after 30 seconds
    setTimeout(() => {
      clicks = 0;
      el.innerText = oldSSID;     // Put SSID back
      el.style.color = "#ff9800"; // Put Orange back
      loadSSID();                 // Refresh just in case it changed
    }, 15000); // 30,000ms = 30 seconds
  }
};

function loadSSID() {
  fetch('/ssid')
    .then(r => r.text())
    .then(v => { document.getElementById('ssid').innerText = v; });
}

function updateRSSI() {
  fetch('/rssi')
    .then(r => r.text())
    .then(v => { document.getElementById('rssiValue').innerText = v; });
}

loadSSID();
updateRSSI();
setInterval(updateRSSI, 3000);
</script>
</body>
</html>
)rawliteral";


// ===== OTA PAGE =====
static const char OTA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<body style='background:#1e1e1e; color:#e0e0e0; display:flex; flex-direction:column; justify-content:center; align-items:center; height:100vh; margin:0;'>
  <h2>OTA Update</h2>

  <input type='file' id='firmwareFile' style='margin:10px 0;'><br>
  <button onclick='uploadFirmware()'
    style='padding:10px 20px; background:#ff9800; color:#1e1e1e; border:none; border-radius:5px; cursor:pointer;'>
    Upload & Update
  </button>

  <br><br>
  <progress id='progressBar' value='0' max='100' style='width:300px; height:20px;'></progress>
  <span id='status' style='margin-top:10px;'></span>

<script>
function uploadFirmware() {
  var fileInput = document.getElementById('firmwareFile');
  var file = fileInput.files[0];
  if (!file) { alert("Select file first."); return; }

  var xhr = new XMLHttpRequest();

  xhr.upload.addEventListener('progress', function(e) {
    if (e.lengthComputable) {
      var percent = Math.round((e.loaded / e.total) * 100);
      document.getElementById('progressBar').value = percent;
      document.getElementById('status').innerText = percent + "%";
    }
  });

  xhr.addEventListener('load', function() {
    let t = 8;
    let lbl = document.getElementById('status');
    let timer = setInterval(() => {
      lbl.innerText = "Rebooting... " + t + "s";
      if (--t < 0) { clearInterval(timer); window.location.href = "/"; }
    }, 1000);
  });

  xhr.addEventListener('error', function() {
    document.getElementById('status').innerText = "Upload failed.";
  });

  xhr.open('POST', '/update', true);
  var formData = new FormData();
  formData.append('update', file);
  xhr.send(formData);
}
</script>
</body>
</html>
)rawliteral";

void setupWebServer(AsyncWebServer &server)
{
    server.on("/ssid", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        char buf[33];
        strncpy(buf, WiFi.SSID().c_str(), sizeof(buf));
        buf[sizeof(buf) - 1] = '\0';
        request->send(200, "text/plain", buf);
    });

    server.on("/rssi", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        char buf[12];
        snprintf(buf, sizeof(buf), "%d", WiFi.RSSI());
        request->send(200, "text/plain", buf);
    });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "text/plain", "Rebooting...");
        
        // Give the response time to send before killing the processor
        delay(1000); 
        ESP.restart();
    });

server.on("/matrix", HTTP_GET, [](AsyncWebServerRequest *request) {
  scrollingLayer.stop();  
  currentClockMode = MODE_RAIN;
    initMatrixRain(); // Initialize the rain drops
    
    // Start or restart the 15s timer
    if (rainTimer != NULL) {
        xTimerStart(rainTimer, 0);
    }
    
    request->send(200, "text/plain", "Follow the white rabbit...");
});

server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
{
    request->send(200, "text/html", INDEX_HTML);
});

    server.begin();
    Serial.println("HTTP server started");
}

void setupOTA(AsyncWebServer &server)
{
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "text/html", OTA_HTML);
    });

    server.on("/update", HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            bool ok = !Update.hasError();
            request->send(200, "text/plain", ok ? "OK" : "FAIL");

            if (ok)
            {
                Serial.println("Update Success. Rebooting...");
                delay(500);
                ESP.restart();
            }
        },
        [](AsyncWebServerRequest *request,
           String filename,
           size_t index,
           uint8_t *data,
           size_t len,
           bool final)
        {
            if (len == 0)
                return;

            if (index == 0)
            {
              scrollingLayer.stop();
              logStatusMessage("Updating...");
              vTaskDelay(pdMS_TO_TICKS(500));
              if(adviceTaskHandle) vTaskSuspend(adviceTaskHandle);        
              if(weatherTaskHandle) vTaskSuspend(weatherTaskHandle);
              if(animationTaskHandle) vTaskSuspend(animationTaskHandle); //Stop animation task & other 

                Serial.printf("Update Start: %s\n", filename.c_str());
                

                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }

            if (len) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
             }

            if (final) {
                if (Update.end(true)) {
                    Serial.printf("Update Success: %u bytes\n", index + len);
                } else {
                    Update.printError(Serial);
                    // Resume task if update fails so you can try again without reboot
                    if(animationTaskHandle != NULL) vTaskResume(animationTaskHandle);
                }
              }
        });

}
