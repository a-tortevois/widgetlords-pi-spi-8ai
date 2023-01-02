import WebSocket, {WebSocketServer} from "ws";
import SocketClient from "./socket-client.js";

const DEBUG = true;
const TCP_PORT = 3000;
const WSS_PORT = 8888;

let socketClient = null;
const wss = new WebSocketServer({port: WSS_PORT});

// WebSocket.Server Event: 'close'
wss.on('close', () => {
    console.warn('WebSocket Server: Close');
    clearInterval(interval);
});

// WebSocket.Server Event: 'connection'
wss.on('connection', (ws, req) => {
    ws.ip = `${req.connection.remoteAddress}:${req.connection.remotePort}`;
    ws.isAlive = true;

    console.log(`WebSocket.Server: New connexion from ${ws.ip}`);

    //  WebSocket Event: 'close'
    ws.on('close', () => {
        console.warn(`WebSocket: Close ${ws.ip}`);
    });

    //  WebSocket Event: 'error'
    ws.on('error', (error) => {
        console.error(`WebSocket: Error on ${ws.ip}: ${error}`);
    });

    //  WebSocket Event: 'message'
    ws.on('message', (data) => {
        if (DEBUG) console.log(`WebSocket message: ${data}`);
        try {
            const msg = JSON.parse(data);
            if (!msg._msgid) {
                msg._msgid = Date.now().toString(16);
            }

            switch (msg.topic) {
                case 'cmd/ping': {
                    sendToAllClients(JSON.stringify({
                        _msgid: msg._msgid,
                        status: 200,
                        payload: {},
                        timestamp: (Date.now() / 1_000).toFixed(0),
                    }));
                    break;
                }

                case 'get/all': {
                    socketClient.write(JSON.stringify(msg));
                    break;
                }
                default: {
                    return;
                }
            }
        } catch (e) {
            console.error(`WebSocket JSON error: ${e.message}`)
        }
    });

    // WebSocket Event: 'open'
    // WebSocket Event: 'ping'

    // WebSocket Event: 'pong'
    ws.on('pong', () => {
        if (DEBUG) console.log(`WebSocket: Pong from ${ws.ip}`);
        ws.isAlive = true;
    });

    // WebSocket Event: 'unexpected-response'
    // WebSocket Event: 'upgrade'
});

// WebSocket.Server Event: 'error'
wss.on('error', (error) => {
    console.error(`WebSocket.Server Error: ${error}`);
});

// WebSocket.Server Event: 'headers'
// WebSocket.Server Event: 'listening'

const interval = setInterval(() => {
    if (DEBUG) console.log('Send Ping');
    wss.clients.forEach(function each(ws) {
        if (ws.isAlive === false) return ws.terminate();
        ws.isAlive = false;
        ws.ping();
    });
}, (3_600_000)); // 1h

const sendToAllClients = (data) => {
    wss.clients.forEach(function each(client) {
        if (client.readyState === WebSocket.OPEN) {
            client.send(data);
        }
    });
}

const socketClientConnection = () => {
    const _socket = new SocketClient({name: 'WebsocketGateway', port: TCP_PORT, onData: sendToAllClients});
    _socket.on('close', () => {
        setTimeout(() => {
            socketClient = socketClientConnection();
        }, 1_000);
    });

    return {
        isConnected: () => {
            return _socket.isConnected()
        },
        write: (data) => {
            _socket.write(data)
        },
    }
}

socketClient = socketClientConnection();