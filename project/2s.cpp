// g++ 2s.cpp -D_WIN32_WINNT=0x0A00 -lws2_32 -lwsock32 -o s
#include "httplib.h"
#include <bits/stdc++.h>

using namespace std;
using namespace httplib;

int main() {
  Server srv;
  srv.Get("/", [](const Request &req, Response &res) {
    cout<< req.remote_addr <<endl;
    cout<< req.remote_port<<endl;
    string addr = req.remote_addr;
    cout<<"Response sent"<<endl;
    string s = "Hello world!" + addr + "\n";
    res.set_content(s, "text/plain");
  });
  srv.listen("localhost", 1234);
}