'use strict';

const byId = (id) => document.getElementById(id);
const page = document.body.dataset.page || 'home';

const messageFeed = byId('message-feed');
const messageSearch = byId('message-search');
const currentIp = byId('current-ip');
const currentMac = byId('current-mac');
const ipInput = byId('ip');
const macInput = byId('mac');
const messageCount = byId('message-count');
const deviceUptime = byId('device-uptime');
const lastUpdated = byId('last-updated');
const refreshButton = byId('refresh-button');
const connectionPill = byId('connection-pill');
const connectionLabel = byId('connection-label');
const settingsForm = byId('settings-form');
const settingsStatus = byId('settings-status');
const toast = byId('toast');

let messages = [];
let toastTimer = 0;
let refreshInProgress = false;

function formatUptime(totalSeconds) {
  const seconds = Number(totalSeconds) || 0;
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);

  if (days > 0) return `${days}d ${hours}h`;
  if (hours > 0) return `${hours}h ${minutes}m`;
  if (minutes > 0) return `${minutes}m`;
  return `${seconds}s`;
}

function messageType(message) {
  const text = message.toLowerCase();
  if (text.includes('error') || text.includes('mismatch') || text.includes('cancelled')) return 'error';
  if (text.includes('open') || text.includes('start')) return 'warning';
  if (text.includes('ok') || text.includes('finished') || text.includes('closed') || text.includes('started')) return 'success';
  return 'info';
}

function messageLabel(type) {
  if (type === 'error') return 'Attention required';
  if (type === 'warning') return 'Device event';
  if (type === 'success') return 'Normal operation';
  return 'System message';
}

function setConnection(state, label) {
  if (!connectionPill || !connectionLabel) return;
  connectionPill.className = `connection-pill ${state}`;
  connectionLabel.textContent = label;
}

function showStatus(message, type = '') {
  if (!settingsStatus) return;
  settingsStatus.textContent = message;
  settingsStatus.className = `form-status ${type}`.trim();
}

function showToast(message, type = '') {
  if (!toast) return;
  window.clearTimeout(toastTimer);
  toast.textContent = message;
  toast.className = `toast visible ${type}`.trim();
  toastTimer = window.setTimeout(() => {
    toast.className = 'toast';
  }, 3600);
}

function renderMessages() {
  if (!messageFeed) return;

  const query = messageSearch ? messageSearch.value.trim().toLowerCase() : '';
  const visible = query
    ? messages.filter((message) => message.toLowerCase().includes(query))
    : messages;

  messageFeed.replaceChildren();

  if (visible.length === 0) {
    const empty = document.createElement('div');
    empty.className = 'empty-state';
    empty.innerHTML = query
      ? '<strong>No matching messages</strong><p>Try another filter.</p>'
      : '<strong>No messages yet</strong><p>New device activity will appear here automatically.</p>';
    messageFeed.appendChild(empty);
    return;
  }

  visible.forEach((message, index) => {
    const type = messageType(message);
    const row = document.createElement('div');
    row.className = `message-row ${type}`;

    const indicator = document.createElement('span');
    indicator.className = 'message-indicator';
    indicator.setAttribute('aria-hidden', 'true');

    const copy = document.createElement('div');
    copy.className = 'message-copy';

    const text = document.createElement('div');
    text.className = 'message-text';
    text.textContent = message;

    const tag = document.createElement('small');
    tag.className = 'message-tag';
    tag.textContent = messageLabel(type);

    const number = document.createElement('span');
    number.className = 'message-number';
    number.textContent = String(messages.length - index).padStart(2, '0');

    copy.append(text, tag);
    row.append(indicator, copy, number);
    messageFeed.appendChild(row);
  });

  if (!query) messageFeed.scrollTop = messageFeed.scrollHeight;
}

function updateDeviceData(data) {
  if (currentIp) currentIp.textContent = data.ip || 'Unknown';
  if (currentMac) currentMac.textContent = data.mac || 'Unknown';
  if (messageCount) messageCount.textContent = String(data.count ?? messages.length);
  if (deviceUptime) deviceUptime.textContent = formatUptime(data.uptimeSeconds);
  if (lastUpdated) {
    lastUpdated.textContent = new Date().toLocaleTimeString([], {
      hour: '2-digit', minute: '2-digit', second: '2-digit'
    });
  }

  if (ipInput && ipInput.dataset.initialized !== 'true') {
    ipInput.value = data.ip || '';
    ipInput.dataset.initialized = 'true';
  }
  if (macInput && macInput.dataset.initialized !== 'true') {
    macInput.value = data.mac || '';
    macInput.dataset.initialized = 'true';
  }
}

async function loadDeviceData(manual = false) {
  if (refreshInProgress) return;
  refreshInProgress = true;

  if (refreshButton) {
    refreshButton.disabled = true;
    refreshButton.classList.add('loading');
  }

  try {
    const response = await fetch('/api/messages', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Device returned HTTP ${response.status}`);

    const data = await response.json();
    messages = Array.isArray(data.messages) ? data.messages : [];
    updateDeviceData(data);
    setConnection('online', 'Device online');
    renderMessages();

    if (manual) showToast('Device data refreshed.', 'success');
  } catch (error) {
    if (currentIp) currentIp.textContent = 'Unavailable';
    if (currentMac) currentMac.textContent = 'Unavailable';
    if (lastUpdated) lastUpdated.textContent = 'Connection lost';
    setConnection('offline', 'Device offline');

    if (messageFeed && messages.length === 0) {
      messageFeed.innerHTML = '<div class="empty-state"><strong>Unable to reach device</strong><p>Check the Ethernet connection and device IP.</p></div>';
    }
    if (manual) showToast(error.message, 'error');
  } finally {
    refreshInProgress = false;
    if (refreshButton) {
      refreshButton.disabled = false;
      refreshButton.classList.remove('loading');
    }
  }
}

if (refreshButton) {
  refreshButton.addEventListener('click', () => loadDeviceData(true));
}

if (messageSearch) {
  messageSearch.addEventListener('input', renderMessages);
}

document.querySelectorAll('.password-toggle').forEach((button) => {
  button.addEventListener('click', () => {
    const input = byId(button.dataset.target);
    if (!input) return;
    const show = input.type === 'password';
    input.type = show ? 'text' : 'password';
    button.textContent = show ? 'Hide' : 'Show';
    button.setAttribute('aria-label', show ? 'Hide password' : 'Show password');
  });
});

if (settingsForm) {
  settingsForm.addEventListener('submit', async (event) => {
    event.preventDefault();
    showStatus('Saving configuration...');

    const submitButton = settingsForm.querySelector('button[type="submit"]');
    if (submitButton) submitButton.disabled = true;

    try {
      const body = new URLSearchParams(new FormData(settingsForm));
      const response = await fetch('/api/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: body.toString()
      });

      const data = await response.json();
      if (!response.ok || !data.ok) {
        throw new Error(data.message || `Device returned HTTP ${response.status}`);
      }

      const passwordInput = byId('password');
      const newUsernameInput = byId('new-username');
      const newPasswordInput = byId('new-password');
      const usernameInput = byId('username');

      if (newUsernameInput && newUsernameInput.value && usernameInput) {
        usernameInput.value = newUsernameInput.value;
      }
      if (passwordInput) passwordInput.value = '';
      if (newUsernameInput) newUsernameInput.value = '';
      if (newPasswordInput) newPasswordInput.value = '';

      if (data.ipChanged) {
        showStatus(`Saved. The settings page is moving to ${data.ip}...`, 'success');
        showToast('Network identity updated. Reconnecting...', 'success');
        window.setTimeout(() => {
          window.location.href = `http://${data.ip}/settings`;
        }, 2800);
      } else if (data.macChanged) {
        showStatus('Saved. Ethernet is restarting with the new MAC address...', 'success');
        showToast('MAC address updated. Reconnecting...', 'success');
        window.setTimeout(() => {
          window.location.href = '/settings';
        }, 2800);
      } else {
        showStatus('Configuration saved successfully.', 'success');
        showToast('Configuration saved.', 'success');
        await loadDeviceData();
      }
    } catch (error) {
      showStatus(error.message, 'error');
      showToast(error.message, 'error');
    } finally {
      if (submitButton) submitButton.disabled = false;
    }
  });
}

loadDeviceData();
window.setInterval(loadDeviceData, page === 'home' ? 3000 : 10000);
