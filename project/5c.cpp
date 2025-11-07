//g++ 5c.cpp -o s -lpqxx -lpq -lssl -lcrypto
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
//#include <iostream>
#include <string>
#include <vector>
#include <thread>
using namespace std;
using namespace httplib;

void cl(int tno) {
  string s = "localhost";
  Client cli(s, 1234);

   for(int i=0;i<5;i++){
    // cout << "Enter the path: ";
    string path="/val?id="+to_string(i);
    // cin >> path;
    cli.set_keep_alive(true);
    auto res = cli.Get(path);
    // cout <<tno<< res->body << endl;
  }
}

int main() {
  vector<thread> threads;
  for(int i=0;i<15;i++){
    threads.push_back(thread(cl,i));
  }

  for(int i=0;i<threads.size();i++){
    threads[i].join();
  }
}