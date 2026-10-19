const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Vector Rover Command Center</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        
        :root { 
            --byui-blue: #006EB6; 
            --black: #000000; 
            --white: #FFFFFF; 
            --light-gray: #f5f5f5; 
        }
        
        body { background-color: var(--byui-blue); color: var(--black); font-family: sans-serif; display: flex; flex-direction: column; align-items: center; min-height: 100vh; }
        
        .header { background: var(--white); width: 100%; padding: 15px; display: flex; align-items: center; justify-content: center; position: relative; border-bottom: 4px solid var(--black); margin-bottom: 10px;}
        .header-logo { position: absolute; left: 15px; height: 50px; }
        .header-text { text-align: center; }
        .header-text h1 { color: var(--byui-blue); font-size: 1.5em; }
        .header-text p { font-size: 0.9em; color: #666; }
        
        .container { background: var(--white); padding: 20px; border-radius: 12px; max-width: 600px; width: 95%; box-shadow: 0 4px 15px rgba(0,0,0,0.2); margin-bottom: 20px; }
        
        .video-box { width: 100%; background: var(--black); border: 3px solid var(--byui-blue); border-radius: 8px; aspect-ratio: 4/3; overflow: hidden; margin-bottom: 20px;}
        img#stream { width: 100%; height: 100%; object-fit: cover; }
        
        .vector-panel { background: var(--light-gray); padding: 20px; border-radius: 8px; border-left: 4px solid var(--byui-blue); display: flex; flex-direction: column; align-items: center; gap: 15px; margin-bottom: 15px; position: relative;}
        
        .info-btn { position: absolute; top: 15px; right: 15px; background: var(--byui-blue); color: white; border: none; border-radius: 50%; width: 25px; height: 25px; font-weight: bold; cursor: pointer; display: flex; align-items: center; justify-content: center;}
        .modal { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.5); align-items: center; justify-content: center; z-index: 100;}
        .modal-content { background: white; padding: 20px; border-radius: 8px; max-width: 300px; text-align: left; border-left: 4px solid var(--byui-blue); }
        .modal-content h3 { margin-bottom: 10px; color: var(--byui-blue); }
        .modal-content p { font-size: 0.9em; line-height: 1.4; margin-bottom: 15px; }
        .close-btn { background: var(--byui-blue); color: white; border: none; padding: 8px 15px; border-radius: 4px; cursor: pointer; float: right;}

        .vector-panel h3 { color: var(--black); font-size: 1.2em; text-align: center; }
        .vector-panel .subtitle { font-size: 0.85em; color: #dc2626; margin-bottom: 5px; font-weight: bold;}
        
        .input-row { display: flex; justify-content: space-between; width: 100%; max-width: 250px; align-items: center; }
        .input-row label { font-weight: bold; font-size: 1.1em; }
        .input-row input { width: 90px; padding: 8px; text-align: center; font-size: 1.1em; border: 2px solid var(--byui-blue); border-radius: 4px; font-family: monospace;}
        
        .vector-display { font-family: monospace; font-size: 1.6em; color: var(--byui-blue); font-weight: bold; margin: 10px 0; background: var(--white); padding: 10px 20px; border-radius: 8px; border: 1px solid #ccc; width: 100%; text-align: center;}
        
        .btn-submit { background: var(--byui-blue); color: var(--white); padding: 15px; font-size: 1.2em; border: none; border-radius: 8px; cursor: pointer; font-weight: bold; width: 100%; max-width: 250px; text-transform: uppercase; }
        .btn-submit:active { transform: scale(0.95); }
        
        .btn-stop { background: #dc2626; color: var(--white); padding: 15px; font-size: 1.2em; border: 2px solid var(--black); border-radius: 8px; cursor: pointer; font-weight: bold; width: 100%; max-width: 250px; margin-top: 5px; text-transform: uppercase;}
        .btn-stop:active { background: #b91c1c; transform: scale(0.95); }

        .status { text-align: center; font-size: 0.9em; color: #666; font-weight: bold; margin-top: 10px;}
    </style>
</head>
<body>

    <div class="header">
        <img class="header-logo" src="data:image/jpeg;base64,[PASTE_YOUR_FULL_STRING_HERE]" alt="BYU-I Logo">
        
        <div class="header-text">
            <h1>Vector Rover Command Center</h1>
            <p>Unit 1 SDL Project</p>
        </div>
    </div>

    <div class="container">
        <div class="video-box">
            <img id="stream" src="" alt="Live Video">
        </div>

        <div class="vector-panel">
            <button class="info-btn" id="info-btn">i</button>
            
            <h3>Enter Values (-100 to 100)</h3>
            <p class="subtitle">Multiples of 10 only</p>
            
            <div class="input-row">
                <label>X Value:</label>
                <!-- Removed type="number" step="10" to allow pure JS validation control -->
                <input type="text" id="vec-x" value="0">
            </div>
            
            <div class="input-row">
                <label>Y Value:</label>
                <input type="text" id="vec-y" value="0">
            </div>
            
            <div class="vector-display">
                v = <span id="disp-x">0</span>î <span id="disp-y">+ 0</span>ĵ
            </div>

            <button id="btn-submit" class="btn-submit">Submit Vector</button>
            <button id="btn-stop" class="btn-stop">Emergency Stop</button>
        </div>
        
        <div class="status" id="ws-status">Connecting to Car...</div>
    </div>

    <!-- Instructions Modal -->
    <div class="modal" id="info-modal">
        <div class="modal-content">
            <h3>How to Drive</h3>
            <p>Enter a 2D coordinate vector using multiples of 10.</p>
            <p><b>Direction:</b> The rover will spin in place to face the calculated angle of your vector.</p>
            <p><b>Distance:</b> The rover will drive straight forward for a duration based on the magnitude of your vector.</p>
            <button class="close-btn" id="close-btn">Close</button>
        </div>
    </div>

    <script>
        window.onload = function() {
            var streamUrl = window.location.protocol + '//' + window.location.hostname + ':81/stream';
            document.getElementById('stream').src = streamUrl;
        };

        var ws;
        var statusText = document.getElementById('ws-status');

        function connectWebSocket() {
            ws = new WebSocket('ws://' + window.location.hostname + ':82/');
            
            ws.onopen = function() {
                statusText.innerText = "Car Connected & Ready!";
                statusText.style.color = "green";
            };
            
            ws.onclose = function() {
                statusText.innerText = "Connection Lost. Reconnecting...";
                statusText.style.color = "red";
                setTimeout(connectWebSocket, 2000);
            };

            ws.onmessage = function(event) {
                if (typeof event.data === "string" && event.data.startsWith("ERROR:")) {
                    alert(event.data);
                }
            };
        }
        connectWebSocket();

        function sendCommand(cmd) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(cmd);
            }
        }

        // UI Elements
        const inputX = document.getElementById('vec-x');
        const inputY = document.getElementById('vec-y');
        const dispX = document.getElementById('disp-x');
        const dispY = document.getElementById('disp-y');
        const btnSubmit = document.getElementById('btn-submit');
        const btnStop = document.getElementById('btn-stop');
        
        // Modal Elements
        const infoBtn = document.getElementById('info-btn');
        const infoModal = document.getElementById('info-modal');
        const closeBtn = document.getElementById('close-btn');

        // Modal Logic
        infoBtn.onclick = function() { infoModal.style.display = "flex"; }
        closeBtn.onclick = function() { infoModal.style.display = "none"; }
        window.onclick = function(event) { if (event.target == infoModal) { infoModal.style.display = "none"; } }

        // STRICT INPUT VALIDATION: Allow only numbers, +, -, and .
        function restrictKeystrokes(e) {
            const allowedKeys = ['0','1','2','3','4','5','6','7','8','9','+','-','.','Backspace','ArrowLeft','ArrowRight','Tab','Delete'];
            // Allow control keys (Ctrl+A, Ctrl+C, etc) but block everything else (like 'e' or special chars)
            if (!allowedKeys.includes(e.key) && !e.ctrlKey && !e.metaKey) {
                e.preventDefault();
            }
        }

        inputX.addEventListener('keydown', restrictKeystrokes);
        inputY.addEventListener('keydown', restrictKeystrokes);

        // Enforce multiples of 10 and boundaries when user finishes typing
        function validateAndSnap(inputElement) {
            let val = parseFloat(inputElement.value);
            
            // Default to 0 if input is completely invalid or empty
            if (isNaN(val)) val = 0;
            
            // Snap to nearest multiple of 10
            val = Math.round(val / 10) * 10;
            
            // Clamp between -100 and 100
            if (val > 100) val = 100;
            if (val < -100) val = -100;
            
            // Reassign the cleaned value (defaults to positive naturally without a sign)
            inputElement.value = val;
            updateDisplay();
        }

        // Trigger snap validation when the user clicks out of the input box
        inputX.addEventListener('change', () => validateAndSnap(inputX));
        inputY.addEventListener('change', () => validateAndSnap(inputY));

        // Vector formatting
        function updateDisplay() {
            // Read raw values while typing for live feedback
            let xRaw = parseFloat(inputX.value);
            let yRaw = parseFloat(inputY.value);
            
            let xVal = isNaN(xRaw) ? 0 : xRaw;
            let yVal = isNaN(yRaw) ? 0 : yRaw;

            dispX.innerText = xVal;
            
            if (yVal >= 0) {
                dispY.innerText = "+ " + yVal;
            } else {
                dispY.innerText = "- " + Math.abs(yVal);
            }
        }

        inputX.addEventListener('input', updateDisplay);
        inputY.addEventListener('input', updateDisplay);

        btnSubmit.addEventListener('click', () => {
            // Force a final validation in case they click submit while still focused on the box
            validateAndSnap(inputX);
            validateAndSnap(inputY);
            
            let x = parseInt(inputX.value) || 0;
            let y = parseInt(inputY.value) || 0;
            sendCommand('vec:' + x + ',' + y);
        });

        btnStop.addEventListener('click', () => {
            sendCommand('stop');
            inputX.value = 0;
            inputY.value = 0;
            updateDisplay();
        });

    </script>
</body>
</html>
)rawliteral";