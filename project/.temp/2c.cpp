// g++ 2c.cpp -D_WIN32_WINNT=0x0A00 -lssl -lcrypto -lcrypt32 -lws2_32 -lwsock32
// -o c
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <bits/stdc++.h>
using namespace std;
using namespace httplib;

void cl(int tno) {
//   cout << "Enter the host name: ";
  //   string s;
  //   cin >> s;

//   int thread = *((int *)arg);

  string s = "localhost";
  Client cli(s, 1234);

   for(int i=0;i<5;i++){
    // cout << "Enter the path: ";
    string path="/";
    // cin >> path;
    cli.set_keep_alive(true);
    auto res = cli.Get(path);
    cout <<tno<< res->body << endl;
  }
}

int main() {
//   cout << "Enter the host name: ";
//   string s;
//   cin >> s;
//   Client cli(s, 1234);

//   while (1) {
//     cout << "Enter the path: ";
//     string path;
//     cin >> path;
//     cli.set_keep_alive(true);
//     auto res = cli.Get(path);
//     cout << res->body << endl;
//   }

  vector<thread> threads;
  for(int i=0;i<15;i++){
    threads.push_back(thread(cl,i));
  }

  for(int i=0;i<threads.size();i++){
    threads[i].join();
  }
  //   if (auto res = cli.Get("/hi")) {
  //     if (res->status == 200) {
  //       std::cout << res->body << std::endl;
  //     }
  //   } else {
  //     auto err = res.error();
  //     std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
  //   }
}