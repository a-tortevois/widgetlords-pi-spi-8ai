'use strict';

import {Socket} from 'node:net';
import {EventEmitter} from 'node:events';

const DEBUG = false;

class SocketClient extends EventEmitter {
    constructor(options) {
        super();
        this.name = options.hasOwnProperty('name') ? `SocketClient ${options.name}` : 'SocketClient';
        this.host = options.hasOwnProperty('host') ? options.hostname : '127.0.0.1';
        this.port = options.hasOwnProperty('port') ? options.port : null;
        this.onData = options.hasOwnProperty('onData') ? options.onData : null;
        this.socket = new Socket();
        this._isConnected = false;

        if (!this.port) {
            throw new Error(`Unable to create ${this.name}, port is undefined`);
        }

        this.socket.on('close', () => {
            console.warn(`${this.name}: connection closed`);
            this._isConnected = false;
            // the important line that enables you to reopen a connection
            this.socket.removeAllListeners();
            this.emit('close');
        });

        this.socket.on('connect', () => {
            console.log(`${this.name} connected to ${this.host}:${this.port}`);
            this._isConnected = true;
        });

        this.socket.on('data', (data) => {
            data = data.toString().replace(/(\n)|(\r)/g, '');
            if (DEBUG) console.log(`${this.name} received: ${data}`);
            if (this.onData) {
                this.onData(data);
            }
        });

        this.socket.on('error', (error) => {
            console.error(`${this.name}: ${error}`);
        });

        // Try to start the connection
        this.socket.connect(this.port, this.host, () => {
            if (DEBUG) console.log(`${this.name}: connection request`);
        });
    }

    isConnected() {
        return this._isConnected;
    }

    write(data) {
        if (this.socket && this._isConnected) {
            if (DEBUG) console.log(`${this.name} write: ${data}`);
            this.socket.write(data);
        } else {
            console.error(`${this.name}: unable to write ${data}`);
        }
    }
}

export default SocketClient;
