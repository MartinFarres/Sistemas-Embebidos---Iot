const { createApp, ref, computed, onMounted, onUnmounted } = Vue;

createApp({
  setup() {
    /* ---- state ---- */
    const ldrValue      = ref(0);
    const alarma        = ref(false);
    const lecturaActiva = ref(false);
    const statusMessage = ref('Connecting...');
    const statusClass   = ref('info');
    const darkMode      = ref(true);
    let pollTimer       = null;

    /* ---- computed ---- */
    const sensorPercent = computed(() => Math.min(100, Math.round((ldrValue.value / 1023) * 100)));

    const sensorRingStyle = computed(() => {
      const pct = sensorPercent.value;
      const deg = Math.round((pct / 100) * 360);
      return { background: `conic-gradient(var(--accent) ${deg}deg, var(--border) ${deg}deg)` };
    });

    /* ---- API calls ---- */
    async function fetchStates() {
      try {
        const res  = await fetch('/sistemStates');
        const data = await res.json();
        ldrValue.value      = data.ldrSensor  ?? 0;
        alarma.value        = !!data.alarma;
        lecturaActiva.value = !!data.lecturaActiva;
        if (data.serialConnected === false) {
          const detail = data.lastSerialError ? `: ${String(data.lastSerialError)}` : '';
          statusMessage.value = 'Serial offline (start Wokwi)' + detail;
          statusClass.value   = 'error';
        } else {
          statusMessage.value = 'Connected';
          statusClass.value   = 'ok';
        }
      } catch (e) {
        statusMessage.value = 'Connection error: ' + String(e);
        statusClass.value   = 'error';
      }
    }

    async function toggleLectura() {
      try {
        const res  = await fetch('/toggleLectura', { method: 'POST' });
        const data = await res.json();
        if (data.status === 'ok' || data.status === 'comando_enviado') {
          statusMessage.value = 'Toggle sent';
          statusClass.value   = 'ok';
        } else if (data.status === 'error_puerto_cerrado') {
          statusMessage.value = 'Serial port closed (is Wokwi running?)';
          statusClass.value   = 'error';
        } else {
          statusMessage.value = 'Error: ' + String(data.error_msg || data.status || 'unknown');
          statusClass.value   = 'error';
        }
        /* immediate refresh */
        await fetchStates();
      } catch (e) {
        statusMessage.value = 'Toggle error: ' + String(e);
        statusClass.value   = 'error';
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
      fetchStates();
      pollTimer = setInterval(fetchStates, 2000);
    });

    onUnmounted(() => {
      if (pollTimer) clearInterval(pollTimer);
    });

    return {
      ldrValue, alarma, lecturaActiva,
      sensorPercent, sensorRingStyle,
      statusMessage, statusClass,
      darkMode, toggleTheme, toggleLectura
    };
  }
}).mount('#app');
