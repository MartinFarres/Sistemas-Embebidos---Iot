const { createApp, ref, onMounted, onUnmounted } = Vue;

createApp({
  setup() {
    /* ---- state ---- */
    const events        = ref([]);
    const loading       = ref(false);
    const statusMessage = ref('Connecting...');
    const statusClass   = ref('info');
    const darkMode      = ref(true);
    let pollTimer       = null;

    /* ---- helpers ---- */
    function showStatus(msg, cls) {
      statusMessage.value = msg;
      statusClass.value   = cls;
    }

    function isOk(status) {
      return status && !status.startsWith('error');
    }

    /* ---- API: fetch events (polling) ---- */
    async function fetchEvents() {
      try {
        const res  = await fetch('/events');
        const data = await res.json();
        events.value = Array.isArray(data) ? data : [];
        if (statusClass.value === 'info') {
          showStatus('Connected', 'ok');
        }
      } catch (e) {
        showStatus('Connection error: ' + String(e), 'error');
      }
    }

    /* ---- API: update time from NTP ---- */
    async function updateTime() {
      loading.value = true;
      try {
        const res  = await fetch('/updateTime');
        const data = await res.json();
        if (isOk(data.status)) {
          showStatus('Hora NTP enviada al Arduino', 'ok');
        } else {
          showStatus('Error: ' + String(data.error_msg || data.status), 'error');
        }
      } catch (e) {
        showStatus('Error NTP: ' + String(e), 'error');
      } finally {
        loading.value = false;
      }
    }

    /* ---- API: read EEPROM ---- */
    async function readEEPROM() {
      loading.value = true;
      try {
        const res  = await fetch('/readEEPROM', { method: 'POST' });
        const data = await res.json();
        if (isOk(data.status)) {
          showStatus('Lectura de EEPROM solicitada...', 'info');
        } else {
          showStatus('Error: ' + String(data.error_msg || data.status), 'error');
        }
      } catch (e) {
        showStatus('Error lectura: ' + String(e), 'error');
      } finally {
        loading.value = false;
      }
    }

    /* ---- API: delete EEPROM ---- */
    async function deleteEEPROM() {
      loading.value = true;
      try {
        const res  = await fetch('/deleteEEPROM', { method: 'POST' });
        const data = await res.json();
        if (isOk(data.status)) {
          events.value = [];
          showStatus('Memoria borrada', 'ok');
        } else {
          showStatus('Error: ' + String(data.error_msg || data.status), 'error');
        }
      } catch (e) {
        showStatus('Error borrado: ' + String(e), 'error');
      } finally {
        loading.value = false;
      }
    }

    /* ---- theme ---- */
    function toggleTheme() {
      darkMode.value = !darkMode.value;
      document.documentElement.setAttribute('data-theme', darkMode.value ? 'dark' : 'light');
      localStorage.setItem('theme', darkMode.value ? 'dark' : 'light');
    }

    function loadTheme() {
      const saved = localStorage.getItem('theme');
      if (saved) {
        darkMode.value = saved === 'dark';
        document.documentElement.setAttribute('data-theme', saved);
      }
    }

    /* ---- lifecycle ---- */
    onMounted(() => {
      loadTheme();
      fetchEvents();
      pollTimer = setInterval(fetchEvents, 2000);
    });

    onUnmounted(() => {
      if (pollTimer) clearInterval(pollTimer);
    });

    return {
      events, loading,
      statusMessage, statusClass,
      darkMode, toggleTheme,
      updateTime, readEEPROM, deleteEEPROM
    };
  }
}).mount('#app');
