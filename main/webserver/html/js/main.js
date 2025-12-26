document.addEventListener("DOMContentLoaded", () => {
  document.getElementById("current-year").textContent =
    new Date().getFullYear();

  const statusOutput = document.getElementById("status-output");
  const wifiOutput = document.getElementById("wifi-output");
  const statusLoader = document.getElementById("status-loader");
  const wifiLoader = document.getElementById("wifi-loader");

  const showLoader = (loader) => {
    loader.style.display = "block";
  };

  const hideLoader = (loader) => {
    loader.style.display = "none";
  };

  const disableButton = (button) => {
    button.disabled = true;
    button.style.opacity = "0.6";
    button.innerText = "Loading...";
  };

  const enableButton = (button, originalText) => {
    button.disabled = false;
    button.style.opacity = "1";
    button.innerText = originalText;
  };

  const showOutput = (section, html, loader) => {
    hideLoader(loader);
    section.innerHTML = html;
  };

  const handleApi = (url, formatter, outputSection, button, loader) => {
    showLoader(loader);
    outputSection.innerHTML = "";
    disableButton(button);

    fetch(url)
      .then((res) => res.json())
      .then((data) => {
        if (!data || data.success === false) {
          throw new Error(data.message || "Unknown error");
        }
        showOutput(outputSection, formatter(data), loader);
      })
      .catch((err) => {
        showOutput(
          outputSection,
          `<p style="color: var(--error-color);">${err.message}</p>`,
          loader
        );
      })
      .finally(() => {
        hideLoader(loader);
        enableButton(button, button.dataset.originalText);
      });
  };

  const formatStatus = (data) => {
    const date = new Date(data.sys_timestamp * 1000);
    const formattedDate = date.toLocaleString();

    return `
      <ul>
        <li><strong>Status:</strong> ${data.status}</li>
        <li><strong>System Time:</strong> ${formattedDate}</li>
        <li><strong>IP Address:</strong> ${data.ip}</li>
        <li><strong>Main DNS :</strong> ${data.main_dns}</li>
        <li><strong>Uptime:</strong> ${data.uptime}</li>
        <li><strong>Free Heap:</strong> ${data.free_heap}</li>
      </ul>
    `;
  };

  const formatWifiScan = (data) => {
    if (!Array.isArray(data.networks) || data.networks.length === 0) {
      return `<p>No networks found.</p>`;
    }

    return `
      <h3>Available Wi-Fi Networks</h3>
      <div class="wifi-list">
        ${data.networks
          .map(
            (ap) => `
          <div class="wifi-card">
            <h4>📶 ${ap.ssid || "(Hidden SSID)"}</h4>
            <ul>
              <li><strong>Signal Strength:</strong> ${ap.rssi} dBm</li>
              <li><strong>Channel:</strong> ${ap.channel}</li>
              <li><strong>Security:</strong> ${ap.authmode}</li>
              <li><strong>BSSID:</strong> ${ap.bssid}</li>
            </ul>
            <button class="connect-btn" data-ssid="${encodeURIComponent(
              ap.ssid
            )}">Connect</button>
          </div>
        `
          )
          .join("")}
      </div>
    `;
  };

  document.addEventListener("click", (e) => {
    if (e.target.classList.contains("connect-btn")) {
      const ssid = decodeURIComponent(e.target.dataset.ssid);
      const password = prompt(`Enter password for "${ssid}"`);
      if (password === null || password.trim() === "") return;

      const connectButton = e.target;
      disableButton(connectButton);

      fetch("/api/v1/wifi/connect", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ ssid, password }),
      })
        .then((res) => res.json())
        .then((data) => {
          alert(
            data.success
              ? `Successfully connected to ${ssid}`
              : `Connection failed: ${data.message || "Unknown error"}`
          );
        })
        .catch((err) => {
          alert("Connection error: " + err.message);
        })
        .finally(() => {
          enableButton(connectButton, "Connect");
        });
    }
  });

  const statusBtn = document.getElementById("api-status-btn");
  statusBtn.dataset.originalText = statusBtn.innerText;
  statusBtn.onclick = () =>
    handleApi(
      "/api/v1/status",
      formatStatus,
      statusOutput,
      statusBtn,
      statusLoader
    );

  const wifiBtn = document.getElementById("wifi-scan-btn");
  wifiBtn.dataset.originalText = wifiBtn.innerText;
  wifiBtn.onclick = () =>
    handleApi(
      "/api/v1/wifi/scan",
      formatWifiScan,
      wifiOutput,
      wifiBtn,
      wifiLoader
    );
});
