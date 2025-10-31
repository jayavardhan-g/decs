// g++ 1c.cpp -D_WIN32_WINNT=0x0A00 -lssl -lcrypto -lcrypt32 -lws2_32 -lwsock32
// -o c
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <bits/stdc++.h>
using namespace std;
using namespace httplib;

int main(void) {
  cout << "Enter the host name: ";
  string s;
  cin >> s;
  cout << "Enter the path: ";
  string path;
  cin >> path;
  Client cli(s);

  auto res = cli.Get(path);
  cout << res->body << endl;

  //   if (auto res = cli.Get("/hi")) {
  //     if (res->status == 200) {
  //       std::cout << res->body << std::endl;
  //     }
  //   } else {
  //     auto err = res.error();
  //     std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
  //   }
}