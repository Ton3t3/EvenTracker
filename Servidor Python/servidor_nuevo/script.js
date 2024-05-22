const nodes = {};

function toggleNode(id) {
    const node = document.getElementById(id);
    node.classList.toggle('active');
}

function createNode(id, address) {
    const node = document.createElement('div');
    node.id = id;
    node.className = 'node';
    node.innerHTML = `
        <h3 onclick="toggleNode('${id}')">Node ${address}</h3>
        <div class="sensors">
            <label>Humedad: <span id="humedad-${address}"></span></label>
            <label>Temperatura: <span id="temperature-${address}"></span></label>
            <label>Ruido: <span id="noise-${address}"></span></label>
            <div id="sensor-error-${address}" class="sensor-error"></div>
        </div>
        <div class="buttons-group">
            <div>
                <button onclick="requestSensorData(${address})">Request Sensor Data</button>
                <button onclick="requestPhoto(${address})">Request Photo</button>
            </div>
            <div>
                <button onclick="sendImage(${address}, 1)">Apagar pantalla</button>
                <button onclick="sendImage(${address}, 2)">Send Image 2</button>
                <button onclick="sendImage(${address}, 3)">Send Image 3</button>
                <button onclick="sendImage(${address}, 4)">Send Image 4</button>
                <button onclick="sendImage(${address}, 5)">Send Image 5</button>
                <button onclick="sendImage(${address}, 6)">Send Image 6</button>
                <button onclick="sendImage(${address}, 7)">Send Image 7</button>
            </div>
            <div>
                <button onclick="sendSong(${address}, 1)">Send Song 1</button>
                <button onclick="sendSong(${address}, 2)">Send Song 2</button>
                <button onclick="sendSong(${address}, 3)">Send Song 3</button>
                <button onclick="sendSong(${address}, 4)">Send Song 4</button>
                <button onclick="sendSong(${address}, 5)">Send Song 5</button>
                <button onclick="sendSong(${address}, 6)">Send Song 6</button>
                <button onclick="sendSong(${address}, 7)">Send Song 7</button>
            </div>
        </div>
        <input type="text" placeholder="Node Name" onchange="renameNode(${address}, this.value)">
        <label>Address: ${address}</label>
        <img id="image-${address}" style="display:none;">
    `;
    document.getElementById('nodes').appendChild(node);
    nodes[address] = true;
}

function requestAllSensors() {
    for (let address = 2; address <= 5; address++) {
        requestSensorData(address);
    }
}

function requestSensorData(address) {
    sendMessage({ address: address, content: "sensors" });
}

function requestPhoto(address) {
    sendMessage({ address: address, content: "photo" });
}

function sendImage(address, choice) {
    sendMessage({ address: address, content: "image", choice: choice });
}

function sendSong(address, choice) {
    sendMessage({ address: address, content: "song", choice: choice });
}

function renameNode(address, name) {
    const nodeTitle = document.querySelector(`#node-${address} h3`);
    nodeTitle.innerText = name;
}

function sendMessage(message) {
    const jsonMessage = JSON.stringify(message);
    ws.send(jsonMessage);
    console.log(`Sent message: ${jsonMessage}`);
}

const ws = new WebSocket('ws://localhost:8765');

ws.onopen = function() {
    console.log('Connected to WebSocket server');
};

ws.onmessage = function(event) {
    const message = JSON.parse(event.data);
    const address = message.address;
    if (address && !nodes[address]) {
        createNode(`node-${address}`, address);
    }

    if (message.purpose === 'Error') {
        document.getElementById(`sensor-error-${address}`).innerText = 'Error al enviar.';
    } else if (message.purpose === 'sensors') {
        document.getElementById(`sensor-error-${address}`).innerText = ''; // Clear any previous errors
        document.getElementById(`humedad-${address}`).innerText = message.humedad;
        document.getElementById(`temperature-${address}`).innerText = message.temperature;
        document.getElementById(`noise-${address}`).innerText = message.noise;
    } else if (message.purpose === 'image') {
        const img = document.getElementById(`image-${address}`);
        img.src = message.path;
        img.style.display = 'block';
    }
};

ws.onclose = function() {
    console.log('Disconnected from WebSocket server');
};
