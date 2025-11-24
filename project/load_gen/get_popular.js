import http from 'k6/http';

const BASE = __ENV.BASE_URL || 'http://localhost:1234';
const DURATION = __ENV.DURATION || '40s';

// k6 options: VUs and duration are supplied via env vars
export let options = {
  vus: __ENV.VUS ? parseInt(__ENV.VUS) : 1,
  duration: DURATION,
};

// The sequence limit
const SEQ_MAX = 500;

export default function () {
  // __ITER starts at 0 for each VU.
  // The modulo operator (%) ensures it wraps around after 500.
  // (+ 1) shifts the range from 0-499 to 1-500.
  const key = (__ITER % SEQ_MAX) + 1;

  http.get(`${BASE}/val?id=${key}`);
}