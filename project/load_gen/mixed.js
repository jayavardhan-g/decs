import http from 'k6/http';

const BASE = __ENV.BASE_URL || 'http://localhost:1234';
const DURATION = __ENV.DURATION || '40s';

// Ratios: Default to 0.7 GET, 0.2 POST, 0.1 DELETE
const GET_RATIO = __ENV.GET_RATIO ? parseFloat(__ENV.GET_RATIO) : 0.7;
const PUT_RATIO = __ENV.PUT_RATIO ? parseFloat(__ENV.PUT_RATIO) : 0.2;

export let options = {
  vus: __ENV.VUS ? parseInt(__ENV.VUS) : 1,
  duration: DURATION,
};

export default function () {
  const keyspace = parseInt(__ENV.KEYSPACE || '10000');
  const id = Math.floor(Math.random() * keyspace) + 1;
  const r = Math.random();

  if (r < GET_RATIO) {
    // --- GET Request ---
    // Fix: Append 'id' directly to the URL string
    http.get(`${BASE}/val?id=${id}`);

  } else if (r < GET_RATIO + PUT_RATIO) {
    // --- POST Request ---
    // Fix: Your C++ server expects 'id' and 'val' as query parameters.
    // We generate the value and append it to the URL.
    const val = `val-${__VU}-${Math.random().toString(36).slice(2,8)}`;
    http.post(`${BASE}/save?id=${id}&val=${val}`);

  } else {
    // --- DELETE Request ---
    // Fix: Use http.del shortcut and append 'id' to the URL
    http.del(`${BASE}/delete?id=${id}`);
  }
}