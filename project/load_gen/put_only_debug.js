import http from 'k6/http';

const BASE = __ENV.BASE_URL || 'http://localhost:1234';
const DURATION = __ENV.DURATION || '40s';
const DEBUG = __ENV.DEBUG === 'true';

export let options = {
  vus: __ENV.VUS ? parseInt(__ENV.VUS) : 1,
  duration: DURATION,
};

export default function () {
  const id = Math.floor(Math.random() * (parseInt(__ENV.KEYSPACE || '10000'))) + 1;
  
  // 1. Your specific POST style (Form Data payload)
  const payload = { 
    id: String(id), 
    val: `val-${__VU}-${Math.random().toString(36).slice(2,8)}` 
  };

  // 2. Define the URL explicitly so we can log it later
  const url = `${BASE}/save`;

  // 3. Send the request
  const res = http.post(url, payload);
   
  if (DEBUG) {
    const logEntry = {
      url: url,          // Now this variable exists
      method: "POST",
      status: res.status,
      request_body: payload, // Log the payload so you see the data sent
      response_body: res.body
    };
    console.log(JSON.stringify(logEntry));
  }
}