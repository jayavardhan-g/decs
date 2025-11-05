// g++ 2s.cpp -D_WIN32_WINNT=0x0A00 -lws2_32 -lwsock32 -o s
#include "httplib.h"
#include <bits/stdc++.h>

using namespace std;
using namespace httplib;

int id(string &s) {

  int id;
  try {
    // 1. Attempt the conversion
    id = stoi(s);

  } catch (const invalid_argument &e) {
    return -1;

  } catch (const out_of_range &e) {
    return -1;
  }
  return id;
}

int main() {
  const int maxx = 700;

  string kv[maxx];

  Server srv;
  srv.Get("/", [](const Request &req, Response &res) {
    // cout << req.remote_addr << endl;
    // cout << req.remote_port << endl;
    string addr = req.remote_addr + " " + to_string(req.remote_port);
    // cout << "Response sent" << endl;
    string s = "Hello world!" + addr + "\n";
    res.set_content(s, "text/plain");
  });

  srv.Get("/val", [&](const auto &req, auto &res) {
    if (!req.has_param("id")) {
      res.status = 400; // Bad Request
      res.set_content("Error: 'id' parameter is missing.", "text/plain");
      return; // Stop processing
    }

    string id_str = req.get_param_value("id");
    int id_int = id(id_str);

    if (id_int == -1) {
      res.status = 400;
      res.set_content("Not a valid id", "text/plain");
      return;
    }

    res.set_content(kv[id_int % maxx], "text/plain");
  });

  srv.Post("/save", [&](const auto &req, auto &res) {
    if (!req.has_param("id")) {
      res.status = 400; // Bad Request
      res.set_content("Error: 'id' parameter is missing.", "text/plain");
      return; // Stop processing
    }
    if (!req.has_param("val")) {
      res.status = 400; // Bad Request
      res.set_content("Error: 'val' parameter is missing.", "text/plain");
      return; // Stop processing
    }

    string id_str = req.get_param_value("id");
    int id_int = id(id_str);

    if (id_int == -1) {
      res.status = 400;
      res.set_content("Not a valid id", "text/plain");
      return;
    }

    string val = req.get_param_value("val");
    kv[id_int % maxx] = val;

    res.set_content("Key value pair created \nID = " + id_str +
                        " \nValue = " + kv[id_int % maxx],
                    "text/plain");
  });

  srv.Put("/put", [&](const auto &req, auto &res) {
    if (!req.has_param("id")) {
      res.status = 400; // Bad Request
      res.set_content("Error: 'id' parameter is missing.", "text/plain");
      return; // Stop processing
    }
    if (!req.has_param("val")) {
      res.status = 400; // Bad Request
      res.set_content("Error: 'val' parameter is missing.", "text/plain");
      return; // Stop processing
    }

    string id_str = req.get_param_value("id");
    int id_int = id(id_str);

    if (id_int == -1) {
      res.status = 400;
      res.set_content("Not a valid id", "text/plain");
      return;
    }

    if (kv[id_int % maxx].empty()) {
      res.status = 400;
      res.set_content("There is no key value pair for this key:" + id_str,
                      "text/plain");
      return;
    }

    string val = req.get_param_value("val");
    kv[id_int % maxx] = val;

    res.set_content("Key value pair created \nID = " + id_str +
                        " \nValue = " + kv[id_int % maxx],
                    "text/plain");
  });

  srv.Delete("/delete", [&](const auto &req, auto &res) {
    if (!req.has_param("id")) {
      res.status = 400; // Bad Request
      res.set_content("Error: 'id' parameter is missing.", "text/plain");
      return; // Stop processing
    }

    string id_str = req.get_param_value("id");
    int id_int = id(id_str);

    if (id_int == -1) {
      res.status = 400;
      res.set_content("Not a valid id", "text/plain");
      return;
    }

    if (kv[id_int % maxx].empty()) {
      res.status = 400;
      res.set_content("There is no key value pair for this key:" + id_str,
                      "text/plain");
      return;
    }

    kv[id_int % maxx] = "";

    res.set_content("Key value pair deleted \nID = " + id_str, "text/plain");
  });

  srv.listen("localhost", 1234);
}