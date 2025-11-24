import http from 'k6/http';

const BASE = __ENV.BASE_URL || 'http://localhost:1234';
const DURATION = __ENV.DURATION || '40s';
// Check if DEBUG env var is set to 'true'
const DEBUG = __ENV.DEBUG === 'true';

export let options = {
  vus: __ENV.VUS ? parseInt(__ENV.VUS) : 1,
  duration: DURATION,
};

const SEQ_MAX = 500;

export default function () {
  // __ITER is 0-based, so we mod 500 and add 1 to get 1..500
  const key = (__ITER % SEQ_MAX) + 1;
  const url = `${BASE}/val?id=${key}`;

  const res = http.get(url);

  // Only log if DEBUG is enabled
  if (DEBUG) {
    const logEntry = {
      url: url,
      status: res.status,
      body: res.body
    };
    console.log(JSON.stringify(logEntry));
  }
}