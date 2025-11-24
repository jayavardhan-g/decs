import http from 'k6/http';
import { sleep } from 'k6';

const BASE = __ENV.BASE_URL || 'http://localhost:1234';
const DURATION = __ENV.DURATION || '40s';

// k6 options: VUs and duration are supplied via env var VUS and DURATION at runtime
export let options = {
  vus: __ENV.VUS ? parseInt(__ENV.VUS) : 1,
  duration: DURATION,
};

export default function () {
  const id = Math.floor(Math.random() * (parseInt(__ENV.KEYSPACE || '10000'))) + 1;
  // FIX: Append ID to URL string
  res = http.get(`${BASE}/val?id=${id}`);
}