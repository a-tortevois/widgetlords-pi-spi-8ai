const WSS_HOSTNAME = window.location.hostname;
const WSS_PORT = 8888;

const ws = (() => {
    let ws = null;
    let timeout = null;

    this.connect = () => {
        ws = new WebSocket(`ws://${WSS_HOSTNAME}:${WSS_PORT}`);

        ws.onopen = () => {
            log('WebSocket opened');
            window.dispatchEvent(new Event('WSConnected'));
        };

        ws.onmessage = (event) => {
            log(`WebSocket receive: ${event.data}`);
            this._updateContextData(event.data);
            this._setTimeout();
        };

        ws.onclose = () => {
            log('WebSocket connection closed');
            this._clearTimeout();
            updateConnexionStatus();
            ws = null;
            try {
                // TODO add a timeout before try again.
                // this.connect();
            } catch (err) {
                log(`WebSocket connect error: ${err.message}`);
            }
        };

        ws.onerror = (event) => {
            log(`WebSocket error observed: ${event}`);
        }
    }

    this.send = (msg) => {
        try {
            ws.send(JSON.stringify(msg));
            log(`WebSocket send: ${JSON.stringify(msg)}`);
        } catch (err) {
            log(`WebSocket send error: ${err.message}`);
        }
    }

    this.isConnected = () => {
        return (ws && ws.readyState === WebSocket.OPEN);
    }

    // Private
    this._clearTimeout = () => {
        if (timeout !== null) {
            window.clearTimeout(timeout);
            timeout = null;
        }
    }

    this._setTimeout = () => {
        this._clearTimeout();
        if (this.isConnected()) {
            timeout = window.setTimeout(() => {
                this.send({topic: 'cmd/ping', payload: {}});
                this._setTimeout();
            }, (10_000));
        }
    }

    this._updateContextData = (json) => {
        try {
            let msg = JSON.parse(json);
            if (msg.hasOwnProperty('timestamp')) {
                contextData.timestamp = msg.timestamp * 1_000;
            }
            if (msg.hasOwnProperty('payload')) {
                for (let [key, item] of Object.entries(msg.payload)) {
                    if (item === undefined) continue;
                    if (!contextData.hasOwnProperty(key)) contextData[key] = {};
                    if (item.i !== undefined && item.v !== undefined) {
                        if ((item.i).length === (item.v).length) {
                            for (let k = 0; k < (item.i).length; k++) {
                                contextData[key][item.i[k]] = item.v[k];
                                this._updateHtml(key, item.i[k], item.v[k]);
                                updateHtmlCallback(key, item.i[k], item.v[k]);
                            }
                        }
                    } else {
                        if (item instanceof Object) {
                            if (!contextData.hasOwnProperty(key)) contextData[key] = {};
                            Object.keys(item).forEach((prop) => {
                                contextData[key][prop] = item[prop];
                            });
                        } else {
                            contextData[key] = item;
                        }
                        this._updateHtml(key, null, item);
                        updateHtmlCallback(key, null, item);
                    }
                }
            }
            setElementValue('context_data', JSON.stringify(contextData));
            // Update the datetime after the context_data to get the good timezone
            this._updateDatetime();
        } catch (err) {
            console.error(`JSON: ${json}, error: ${err.message}`);
        }
    }

    this._updateHtml = (key, id, value) => {
        switch (key) {
            case 'adc':
                setElementValue(ADC[id].name, value);
                break;
            default:
                if (id === null) {
                    setElementValue(key, value);
                }
                break;
        }
    }

    this._updateDatetime = () => {
        // Wed, 15 Apr 2020 17:35:14 +02:00
        let date, hour, timezone;
        let options = {
            timeZone: 'Europe/Paris',
            year: 'numeric',
            month: '2-digit',
            day: '2-digit',
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit',
            hour12: false,
            timeZoneName: 'short'
        };

        try {
            // Get the date string representation from the timestamp with the define timezone
            date = new Date(contextData.timestamp).toLocaleString('fr-FR', options).replace(/,/g, '');
            date = date.split(' ');

            if (date.length === 3) {
                // 0: date dd/mm/yyyy
                // 1: hour hh:mm:ss
                // 2: timezone UTC
                hour = date[1].split(':').map(Number);
                timezone = date[2].replace(/−/g, '-');
            } else if (date.length === 4) {
                // 0: date dd/mm/yyyy
                // 1: 'à'
                // 2: hour hh:mm:ss
                // 3: timezone UTC
                hour = date[2].split(':').map(Number);
                timezone = date[3].replace(/−/g, '-');
            } else {
                // TODO
            }

            // Format timezone
            let matches = [];
            let regex = /UTC([+-])/g;
            if (regex.test(timezone)) {
                regex = /:/g;
                if (regex.test(timezone)) {
                    regex = /UTC([+-])(\d+):(\d+)/g;
                    matches = regex.exec(timezone);
                    timezone = `UTC${matches[1]}${(`0${matches[2]}`).slice(-2)}:${(`0${matches[3]}`).slice(-2)}`;
                } else {
                    regex = /UTC([+-])(\d+)/g;
                    matches = regex.exec(timezone);
                    timezone = `UTC${matches[1]}${(`0${matches[2]}`).slice(-2)}:00`;
                }
            } else {
                timezone = `UTC+00:00`;
            }

            date = date[0].split('/');
            date = new Date(Date.UTC(date[2], (date[1] - 1), date[0], hour[0], hour[1], hour[2]));
        } catch (error) {
            log(`error: ${error}`);
            date = new Date(contextData.timestamp);
            timezone = 'UTC';
        }

        const wday = date.toLocaleString('en-US', {weekday: 'short'});
        const d = (`0${date.getUTCDate()}`).slice(-2);
        const month = date.toLocaleString('en-US', {month: 'short'});
        const h = (`0${date.getUTCHours()}`).slice(-2);
        const m = (`0${date.getUTCMinutes()}`).slice(-2);
        const s = (`0${date.getUTCSeconds()}`).slice(-2);
        const year = date.getUTCFullYear();
        setElementValue('datetime', `${wday}, ${d} ${month} ${year} ${h}:${m}:${s} ${timezone}`);
    }

    return {
        connect: connect,
        send: send,
        isConnected: isConnected,
    }
})();

window.addEventListener('load', ws.connect);

const isHtmlRendered = new Promise((resolve) => {
    function handle_DOMContentLoaded() {
        renderHtml(resolve);
        // resolve(); // should be called by render_html
    }

    window.addEventListener('DOMContentLoaded', handle_DOMContentLoaded); // load ?
});

const isWsConnected = new Promise((resolve) => {
    function isWsConnectedHandler(event) {
        resolve(event);
    }

    window.addEventListener('WSConnected', isWsConnectedHandler);
});

Promise.all([isHtmlRendered, isWsConnected]).then(() => {
    window.dispatchEvent(new Event('AppIsReady'));
});

window.addEventListener('AppIsReady', () => {
    log('App is ready');
    ws.send({topic: 'get/all', payload: {}});
    updateConnexionStatus();
});