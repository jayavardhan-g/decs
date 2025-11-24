import http from 'k6/http';
import { sleep } from 'k6';

const BASE = __ENV.BASE_URL || 'http://localhost:1234';
const DURATION = __ENV.DURATION || '40s';
// Check if DEBUG env var is set to 'true'
const DEBUG = __ENV.DEBUG === 'true';

export let options = {
  vus: __ENV.VUS ? parseInt(__ENV.VUS) : 1,
  duration: DURATION,
};

export default function () {
  // Generate random ID from full keyspace
  const id = Math.floor(Math.random() * (parseInt(__ENV.KEYSPACE || '10000'))) + 1;
  const url = `${BASE}/val?id=${id}`;

  // Send the request
  const res = http.get(url);

  // --- LOGGING LOGIC ---
  // Only log if DEBUG is enabled
  if (DEBUG) {
    const logEntry = {
      url: url,
      status: res.status,
      body: res.body
    };
    // Print JSON to stdout
    console.log(JSON.stringify(logEntry));
  }
}