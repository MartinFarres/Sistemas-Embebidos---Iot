const API_BASE = window.location.origin;

const { createApp, ref, computed, onMounted, onUnmounted } = Vue;

createApp({
  setup() {
    // ── Theme ──
    const darkMode = ref(true);

    function toggleTheme() {
      darkMode.value = !darkMode.value;
      document.documentElement.setAttribute(
        "data-theme",
        darkMode.value ? "dark" : "light"
      );
      localStorage.setItem("theme", darkMode.value ? "dark" : "light");
    }

    // Restore saved theme
    const saved = localStorage.getItem("theme");
    if (saved === "light") {
      darkMode.value = false;
      document.documentElement.setAttribute("data-theme", "light");
    }

    // ── State ──
    const pwmLeds = ref([
      { key: "led9",  label: "LED 9 (PWM)",  value: 0, color: "#f43f5e" },
      { key: "led10", label: "LED 10 (PWM)", value: 0, color: "#3b82f6" },
      { key: "led11", label: "LED 11 (PWM)", value: 0, color: "#10b981" },
    ]);

    const led13 = ref(false);
    const ldrValue = ref(0);
    const statusMessage = ref("Connecting...");
    const statusClass = ref("info");

    let ldrInterval = null;
    let sendTimeout = null;

    // ── Computed ──
    const sensorPercent = computed(() =>
      Math.round((ldrValue.value / 1023) * 100)
    );

    const sensorRingStyle = computed(() => {
      const pct = sensorPercent.value;
      const trackBg = darkMode.value ? "#1e1b3a" : "#ddd6fe";
      return {
        background: `conic-gradient(var(--accent) ${pct * 3.6}deg, ${trackBg} ${pct * 3.6}deg)`,
      };
    });

    // ── Methods ──
    function sliderTrackStyle(led) {
      const pct = (led.value / 255) * 100;
      return {
        background: `linear-gradient(to right, ${led.color} ${pct}%, var(--border) ${pct}%)`,
      };
    }

    function buildPayload() {
      const payload = { led13: led13.value ? 1 : 0 };
      pwmLeds.value.forEach((l) => (payload[l.key] = l.value));
      return payload;
    }

    // Debounced send
    function sendLedValues() {
      clearTimeout(sendTimeout);
      sendTimeout = setTimeout(() => doSend(), 80);
    }

    async function doSend() {
      try {
        const res = await fetch(`${API_BASE}/ledValues`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify(buildPayload()),
        });
        const data = await res.json();
        if (data.status === "ok") {
          statusMessage.value = "Connected — values sent";
          statusClass.value = "ok";
        } else {
          const msg = typeof data.error_msg === "string" ? data.error_msg : JSON.stringify(data.error_msg);
          statusMessage.value = "Error: " + (msg || "unknown");
          statusClass.value = "error";
        }
      } catch (e) {
        statusMessage.value = "Connection lost";
        statusClass.value = "error";
      }
    }

    function toggleLed13() {
      led13.value = !led13.value;
      sendLedValues();
    }

    async function fetchLdr() {
      try {
        const res = await fetch(`${API_BASE}/ldrSensor`);
        const data = await res.json();
        ldrValue.value = data.ldrSensor;
        if (statusClass.value !== "ok") {
          statusMessage.value = "Connected";
          statusClass.value = "ok";
        }
      } catch {
        statusMessage.value = "Cannot reach server";
        statusClass.value = "error";
      }
    }

    // ── Lifecycle ──
    onMounted(() => {
      fetchLdr();
      ldrInterval = setInterval(fetchLdr, 5000);
    });

    onUnmounted(() => clearInterval(ldrInterval));

    return {
      darkMode,
      toggleTheme,
      pwmLeds,
      led13,
      ldrValue,
      statusMessage,
      statusClass,
      sensorPercent,
      sensorRingStyle,
      sliderTrackStyle,
      sendLedValues,
      toggleLed13,
    };
  },
}).mount("#app");
