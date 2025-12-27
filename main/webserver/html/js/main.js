document.addEventListener("DOMContentLoaded", () => {
  document.getElementById("current-year").textContent =
    new Date().getFullYear();

  const statusOutput = document.getElementById("status-output");
  const wifiOutput = document.getElementById("wifi-output");
  const statusLoader = document.getElementById("status-loader");
  const wifiLoader = document.getElementById("wifi-loader");
  const ntpInput = document.getElementById("ntp-input");
  const ntpOutput = document.getElementById("ntp-output");
  const ntpLoader = document.getElementById("ntp-loader");
  const ntpSetBtn = document.getElementById("ntp-set-btn");
  const nrf24DurationInput = document.getElementById("nrf24-duration-input");
  const nrf24Output = document.getElementById("nrf24-output");
  const nrf24Loader = document.getElementById("nrf24-loader");
  const nrf24ScanBtn = document.getElementById("nrf24-scan-btn");

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

  const handleNtpUpdate = () => {
    const ntpDomain = ntpInput.value.trim();
    if (!ntpDomain) {
      ntpOutput.innerHTML = `<p style="color: var(--error-color);">Please enter an NTP server domain.</p>`;
      return;
    }

    disableButton(ntpSetBtn);
    showLoader(ntpLoader);
    ntpOutput.innerHTML = "";

    fetch("/api/v1/ntp/set", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ ntp_domain: ntpDomain }),
    })
      .then((res) => res.json())
      .then((data) => {
        if (!data || data.success === false) {
          throw new Error(data.message || "Unknown error");
        }
        ntpOutput.innerHTML = `<p style="color: #4CAF50;">NTP server updated successfully to <strong>${ntpDomain}</strong></p>`;
      })
      .catch((err) => {
        ntpOutput.innerHTML = `<p style="color: var(--error-color);">${err.message}</p>`;
      })
      .finally(() => {
        hideLoader(ntpLoader);
        enableButton(ntpSetBtn, "Set NTP Server");
      });
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

  ntpSetBtn.dataset.originalText = ntpSetBtn.innerText;
  ntpSetBtn.onclick = handleNtpUpdate;

  const formatNrf24Scan = (data) => {
    if (!data.success || !data.xiaomi_remote_id) {
      return `<p style="color: var(--error-color);">Remote ID not detected. Try pressing buttons on your remote during the scan.</p>`;
    }
    return `
      <div class="nrf24-result" style="padding: 20px; background: var(--container-bg); border: 2px solid var(--border-color); border-radius: 10px; margin-top: 10px; box-shadow: 0 4px 8px rgba(136, 54, 150, 0.2);">
        <h3 style="color: var(--accent-color); margin-top: 0;">Remote ID Detected</h3>
        <div style="font-size: 28px; font-weight: bold; color: var(--text-color); font-family: monospace; margin: 20px 0; padding: 15px; background: rgba(165, 90, 195, 0.15); border-radius: 8px; border-left: 4px solid var(--accent-color);">
          ${data.xiaomi_remote_id}
        </div>
        <p style="font-size: 12px; color: rgba(255, 255, 255, 0.7); margin-top: 15px;">
          The ID was saved to the device configuration.
        </p>
      </div>
    `;
  };

  const handleNrf24Scan = () => {
    const duration = nrf24DurationInput.value || 10;
    if (duration < 1 || duration > 60) {
      nrf24Output.innerHTML = `<p style="color: var(--error-color);">Duration must be between 1 and 60 seconds.</p>`;
      return;
    }

    disableButton(nrf24ScanBtn);
    showLoader(nrf24Loader);
    nrf24Output.innerHTML =
      "<p>Scanning... Please press buttons on your remote.</p>";

    fetch(`/api/v1/nrf24/scan?duration=${duration}`)
      .then((res) => res.json())
      .then((data) => {
        if (!data) {
          throw new Error("Empty response");
        }
        showOutput(nrf24Output, formatNrf24Scan(data), nrf24Loader);
      })
      .catch((err) => {
        showOutput(
          nrf24Output,
          `<p style="color: var(--error-color);">Error: ${err.message}</p>`,
          nrf24Loader
        );
      })
      .finally(() => {
        hideLoader(nrf24Loader);
        enableButton(nrf24ScanBtn, "Start NRF24 Scan");
      });
  };

  nrf24ScanBtn.dataset.originalText = nrf24ScanBtn.innerText;
  nrf24ScanBtn.onclick = handleNrf24Scan;
});
