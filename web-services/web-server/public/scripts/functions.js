const DEBUG = false;

const log = (message) => {
    let log = `${new Date().toLocaleString()} ${message}\n`
    if (DEBUG) console.log(log);

    if (isElementExists('log')) {
        document.getElementById('log').value += log;
    }
}

const isElementExists = (id) => {
    let element = document.getElementById(id);
    return (typeof (element) !== undefined && element !== null);
}

const setElementValue = (id, value) => {
    if (isElementExists(id)) {
        let element = document.getElementById(id);
        log(`element_set_value: ${id}; value: ${value}; nodeName: ${element.nodeName}`);
        if (element.nodeName === 'SELECT' || element.nodeName === 'INPUT' || element.nodeName === 'TEXTAREA') {
            element.value = value;
        } else {
            element.innerHTML = value;
        }
    }
}

const getDatetime = () => {
    return `<span id="datetime"></span>`;
}

const getWsStatus = () => {
    return `Web Socket Status: <span id="ws_status" class="led led-${(ws.isConnected()) ? 'green' : 'red'}"></span>`;
}

const getMenu = () => {
    let html = ``;
    html += `<menu>`;
    html += `<a href="./index.html">Home</a>`;
    html += ` \u2014 `;
    html += getDatetime();
    html += ` \u2014 `;
    html += getWsStatus();
    html += `</menu>`;
    return html;
}

const getSwVersion = () => {
    return `<span id="sw_version">${contextData.sw_version}</span>`;
}

const getWsVersion = () => {
    return `<span id="ws_version">${contextData.ws_version}</span>`;
}

const updateConnexionStatus = () => {
    let status = (ws.isConnected()) ? 'green' : 'red';
    if (isElementExists('ws_status')) {
        document.getElementById('ws_status').className = `led led-${status}`;
    }
}