const map = L.map("map", { zoomControl: true }).setView([53.2, -1.7], 6);
L.tileLayer("https://tile.openstreetmap.org/{z}/{x}/{y}.png", {
    maxZoom: 19,
    attribution: "&copy; OpenStreetMap contributors",
}).addTo(map);

const markerLayer = L.featureGroup().addTo(map);
let timer;
let hasFittedStations = false;

function base91(value) {
    let decoded = 0;
    for (const character of value) {
        const digit = character.charCodeAt(0) - 33;
        if (digit < 0 || digit > 90) return null;
        decoded = (decoded * 91) + digit;
    }
    return decoded;
}

// Mic-E (APRS 1.0.1 ch.10): latitude + flags are in the destination address, the rest in the
// information field. Verified against the aprslib decoder (see Advanced-APRS-Go-server mice_test.go).
function parseMicE(frame, info) {
    const gt = frame.indexOf(">"), colon = frame.indexOf(":");
    if (gt < 0 || colon < gt || info.length < 9) return null;
    const dest = frame.slice(gt + 1, colon).split(",")[0].split("-")[0];
    if (dest.length < 6) return null;
    const digit = [], high = [];
    for (let i = 0; i < 6; i++) {
        const c = dest[i];
        if (c >= "0" && c <= "9") { digit.push(c.charCodeAt(0) - 48); high.push(false); }
        else if (c >= "A" && c <= "J" && i < 3) { digit.push(c.charCodeAt(0) - 65); high.push(true); }
        else if (c === "K" && i < 3) { digit.push(0); high.push(true); }
        else if (c === "L") { digit.push(0); high.push(false); }
        else if (c >= "P" && c <= "Y") { digit.push(c.charCodeAt(0) - 80); high.push(true); }
        else if (c === "Z") { digit.push(0); high.push(true); }
        else return null;
    }
    const latDeg = digit[0] * 10 + digit[1];
    const latMin = digit[2] * 10 + digit[3] + (digit[4] * 10 + digit[5]) / 100;
    if (latDeg > 90 || latMin >= 60) return null;
    let lat = latDeg + latMin / 60;
    if (!high[3]) lat = -lat;                 // 0-9/L = South, P-Z = North
    const lonOffset = high[4] ? 100 : 0;      // P-Z = +100 degrees
    const west = high[5];                     // P-Z = West
    for (let i = 1; i <= 8; i++) if (info.charCodeAt(i) < 28) return null;
    let d = info.charCodeAt(1) - 28 + lonOffset;
    if (d >= 190) d -= 190; else if (d >= 180) d -= 80;
    let m = info.charCodeAt(2) - 28;
    if (m >= 60) m -= 60;
    const h = info.charCodeAt(3) - 28;
    if (d > 180 || m < 0 || m >= 60 || h < 0 || h > 99) return null;
    let lon = d + (m + h / 100) / 60;
    if (west) lon = -lon;
    const code = info.charCodeAt(7);
    if (code < 33 || code > 126) return null;
    return { lat, lon, sym: info[8] + info[7] };
}

function parsePosition(frame) {
    const colon = frame.indexOf(":");
    if (colon < 0) return null;
    let info = frame.slice(colon + 1);
    if (info[0] === "`" || info[0] === "'") return parseMicE(frame, info);
    if (info[0] === "/" || info[0] === "@") info = info.slice(8);
    else if (info[0] === "!" || info[0] === "=") info = info.slice(1);
    else return null;

    const match = info.match(/^(\d{2})(\d{2}\.\d{2})([NS])(.)(\d{3})(\d{2}\.\d{2})([EW])(.)/);
    if (match) {
        let lat = Number(match[1]) + Number(match[2]) / 60;
        let lon = Number(match[5]) + Number(match[6]) / 60;
        if (match[3] === "S") lat *= -1;
        if (match[7] === "W") lon *= -1;
        return Number.isFinite(lat) && Number.isFinite(lon) ? { lat, lon, sym: match[4] + match[8] } : null;
    }

    // Compressed APRS: symbol table (/ \ A-Z 0-9, or a-j = overlay digits), four base-91
    // latitude characters, four base-91 longitude characters, symbol code and compression type.
    // The final csT compression bytes are optional on LoRa APRS beacons.
    const compressed = info.match(/^([\/\\A-Za-j0-9])([!-{]{4})([!-{]{4})([!-~])/);
    if (!compressed) return null;
    const encodedLat = base91(compressed[2]);
    const encodedLon = base91(compressed[3]);
    if (encodedLat === null || encodedLon === null) return null;
    const lat = 90 - (encodedLat / 380926);
    const lon = -180 + (encodedLon / 190463);
    return Number.isFinite(lat) && Number.isFinite(lon) ? { lat, lon, sym: compressed[1] + compressed[4] } : null;
}

function callsign(frame) {
    const end = frame.indexOf(">");
    return end > 0 ? frame.slice(0, end) : "Unknown";
}

function markerIcon(direction, source, sym) {
    const markerClass = source === "APRS-IS" ? "is" : (direction === "TX" ? "tx" : "");
    // Official APRS symbol (aprs.fi set, loaded from aprsnet.uk - no flash cost on the device) with a
    // small badge for the packet direction/source. Falls back to the plain dot when offline.
    if (window.AprsSymbols && sym && window.AprsSymbols.parse(sym)) {
        return L.divIcon({
            className: "leaflet-div-icon",
            html: `<div class="sym-marker">${window.AprsSymbols.html(sym, { size: 30 })}<span class="sym-badge ${markerClass}"></span></div>`,
            iconSize: [30, 30],
            iconAnchor: [15, 15],
        });
    }
    return L.divIcon({
        className: "leaflet-div-icon",
        html: `<div class="map-marker ${markerClass}"></div>`,
        iconSize: [16, 16],
        iconAnchor: [8, 8],
    });
}

function addText(parent, className, value) {
    const element = document.createElement("span");
    element.className = className;
    element.textContent = value;
    parent.appendChild(element);
}

function render(packets) {
    const mode = document.getElementById("sourceFilter").value;
    packets = packets.filter((packet) => {
        const source = packet.source || "RF";
        if (mode === "rf") return source === "RF";
        if (mode === "is") return source === "APRS-IS";
        return source === "RF" || source === "APRS-IS";
    });
    document.getElementById("activityTitle").textContent =
        mode === "rf" ? "RF Packet Activity" : (mode === "is" ? "APRS-IS Activity" : "All Packet Activity");
    markerLayer.clearLayers();
    const list = document.getElementById("packets");
    list.replaceChildren();
    let positioned = 0;
    const mappedCallsigns = new Set();

    [...packets].reverse().forEach((packet) => {
        const source = packet.source || "RF";
        const direction = source === "APRS-IS" ? "IS" : (packet.direction === "TX" ? "TX" : "RX");
        const position = parsePosition(packet.packet || "");
        const station = callsign(packet.packet || "");
        const row = document.createElement("div");
        row.className = "packet";
        addText(row, "call", station);
        addText(row, `dir ${direction === "TX" ? "tx" : (direction === "IS" ? "is" : "")}`, direction);
        const signal = direction === "RX" ? ` · ${packet.RSSI} dBm / ${packet.SNR} dB` : "";
        addText(row, "meta", `${packet.packetTime || "--:--:--"}${signal}`);
        addText(row, "frame", packet.packet || "");

        if (position) {
            positioned++;
            if (!mappedCallsigns.has(station)) {
                mappedCallsigns.add(station);
                const marker = L.marker([position.lat, position.lon], { icon: markerIcon(direction, source, position.sym) })
                    .bindTooltip(station, { permanent: true, direction: "top", offset: [0, -14], className: "callsign-label" })
                    .bindPopup(`<strong>${station}</strong><br>${direction} · ${packet.packetTime || ""}`);
                marker.addTo(markerLayer);
                row.addEventListener("click", () => {
                    map.setView(marker.getLatLng(), Math.max(map.getZoom(), 11));
                    marker.openPopup();
                });
            }
        }
        list.appendChild(row);
    });

    document.getElementById("packetCount").textContent = `${packets.length} packet${packets.length === 1 ? "" : "s"}`;
    const withoutPosition = packets.length - positioned;
    const emptyState = document.getElementById("mapEmpty");
    emptyState.hidden = mappedCallsigns.size > 0;
    if (mappedCallsigns.size > 0 && !hasFittedStations) {
        fitStations();
        hasFittedStations = true;
    }
    const sourceLabel = mode === "rf" ? "RF" : (mode === "is" ? "APRS-IS" : "RF/APRS-IS");
    document.getElementById("notice").textContent = packets.length
        ? `${mappedCallsigns.size} callsign${mappedCallsigns.size === 1 ? "" : "s"} mapped · ${withoutPosition} packet${withoutPosition === 1 ? "" : "s"} without a supported APRS position`
        : `No ${sourceLabel} packets have been recorded since the last reboot.`;
}

function fitStations() {
    const markers = markerLayer.getLayers();
    if (markers.length === 1) {
        map.setView(markers[0].getLatLng(), 11);
    } else if (markers.length > 1) {
        map.fitBounds(markerLayer.getBounds().pad(0.18), { maxZoom: 12 });
    }
}

async function refresh() {
    const status = document.getElementById("onlineStatus");
    try {
        const response = await fetch("/received-packets.json?includeAprsIs=1", { cache: "no-store" });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const packets = await response.json();
        render(Array.isArray(packets) ? packets : []);
        status.textContent = "Online";
        status.style.color = "";
    } catch (error) {
        status.textContent = "Disconnected";
        status.style.color = "#b42318";
        document.getElementById("notice").textContent = "Unable to refresh packet data.";
        console.error(error);
    }
}

function schedule() {
    clearInterval(timer);
    timer = setInterval(refresh, Number(document.getElementById("refreshRate").value));
}

document.getElementById("refresh").addEventListener("click", refresh);
document.getElementById("refreshRate").addEventListener("change", schedule);
document.getElementById("sourceFilter").addEventListener("change", () => {
    hasFittedStations = false;
    refresh();
});
document.getElementById("fit").addEventListener("click", () => {
    fitStations();
});

refresh();
schedule();
