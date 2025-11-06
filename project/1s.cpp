// g++ 1s.cpp -D_WIN32_WINNT=0x0A00 -lws2_32 -lwsock32 -o s
#include "httplib.h"
#include <bits/stdc++.h>

using namespace std;
using namespace httplib;

int main() {
  Server srv;
  srv.Get("/", [](const Request &req, Response &res) {
    res.set_content("Hello World!", "text/plain");
  });
  srv.listen("localhost", 1234);
}