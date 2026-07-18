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

function parsePosition(frame) {
    const colon = frame.indexOf(":");
    if (colon < 0) return null;
    let info = frame.slice(colon + 1);
    if (info[0] === "/" || info[0] === "@") info = info.slice(8);
    else if (info[0] === "!" || info[0] === "=") info = info.slice(1);
    else return null;

    const match = info.match(/^(\d{2})(\d{2}\.\d{2})([NS]).(\d{3})(\d{2}\.\d{2})([EW])/);
    if (match) {
        let lat = Number(match[1]) + Number(match[2]) / 60;
        let lon = Number(match[4]) + Number(match[5]) / 60;
        if (match[3] === "S") lat *= -1;
        if (match[6] === "W") lon *= -1;
        return Number.isFinite(lat) && Number.isFinite(lon) ? { lat, lon } : null;
    }

    // Compressed APRS: symbol table, four base-91 latitude characters,
    // four base-91 longitude characters, symbol code and compression type.
    // The final csT compression bytes are optional on LoRa APRS beacons.
    const compressed = info.match(/^[\/\\A-Z0-9]([!-{]{4})([!-{]{4})[!-~]/);
    if (!compressed) return null;
    const encodedLat = base91(compressed[1]);
    const encodedLon = base91(compressed[2]);
    if (encodedLat === null || encodedLon === null) return null;
    const lat = 90 - (encodedLat / 380926);
    const lon = -180 + (encodedLon / 190463);
    return Number.isFinite(lat) && Number.isFinite(lon) ? { lat, lon } : null;
}

function callsign(frame) {
    const end = frame.indexOf(">");
    return end > 0 ? frame.slice(0, end) : "Unknown";
}

function markerIcon(direction, source) {
    const markerClass = source === "APRS-IS" ? "is" : (direction === "TX" ? "tx" : "");
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
                const marker = L.marker([position.lat, position.lon], { icon: markerIcon(direction, source) })
                    .bindTooltip(station, { permanent: true, direction: "top", offset: [0, -8], className: "callsign-label" })
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
