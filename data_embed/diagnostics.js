function metric(label,value){return `<div class="col-6 col-md-3"><div class="metric"><small>${label}</small><strong>${value}</strong></div></div>`}
function td(row,value){const cell=document.createElement("td");cell.textContent=value;row.appendChild(cell)}
async function refresh(){
  try{
    const response=await fetch("/diagnostics.json",{cache:"no-store"});
    if(!response.ok)throw new Error(`HTTP ${response.status}`);
    const data=await response.json(),r=data.radio;
    document.getElementById("metrics").innerHTML=[
      metric("Valid RF packets",r.rxValid),metric("Duplicates suppressed",r.rxDuplicates),
      metric("CRC failures",r.rxCrcErrors),metric("Other radio errors",r.rxOtherErrors),
      metric("TX successful",r.txSuccess),metric("TX errors",r.txErrors),
      metric("Free heap",`${data.freeHeap} bytes`),metric("Wi-Fi RSSI",`${data.wifiRssi} dBm`)
    ].join("");
    const scan=data.profileScanner;
    document.getElementById("scanState").textContent=scan.enabled?(scan.secondaryActive?"Secondary active":"Enabled · primary active"):"Disabled";
    document.getElementById("scanState").className=scan.enabled?"warn":"safe";
    document.getElementById("scanDetail").textContent=scan.enabled
      ? `${scan.frequency} Hz · SF${scan.spreadingFactor} · CR 4/${scan.codingRate} · ${scan.bandwidth} Hz · ${scan.dwell}s every ${scan.interval}s`
      :"Primary UK profile remains active continuously.";
    const body=document.getElementById("stations");body.replaceChildren();
    data.heardStations.forEach(s=>{const row=document.createElement("tr");td(row,s.callsign);td(row,s.packets);td(row,s.lastHeard);
      td(row,`${s.lastRssi} / ${s.avgRssi.toFixed(1)} / ${s.minRssi}…${s.maxRssi} dBm`);
      td(row,`${s.lastSnr.toFixed(1)} / ${s.avgSnr.toFixed(1)} / ${s.minSnr.toFixed(1)}…${s.maxSnr.toFixed(1)} dB`);
      td(row,`${s.lastFreqError} Hz`);body.appendChild(row)});
    document.getElementById("updated").textContent=`Updated ${new Date().toLocaleTimeString()} · uptime ${data.uptimeSeconds}s`;
  }catch(error){document.getElementById("updated").textContent="Disconnected";console.error(error)}
}
refresh();setInterval(refresh,10000);
