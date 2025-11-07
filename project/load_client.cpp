// Compile command (Linux/WSL):
// g++ client.cpp -std=c++17 -lpthread -o client
//
// (If your server is HTTPS, add -lssl -lcrypto and #define CPPHTTPLIB_OPENSSL_SUPPORT)

#include "httplib.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <random> // For the "Mix" mode
#include <chrono> // For seeding the random generator

using namespace std;
using namespace httplib;

// Each thread will run this function
void client_worker(int tno, int mode) {
    // Each thread gets its own random number generator
    // We seed it with the time and the thread number to ensure it's unique
    auto seed = chrono::high_resolution_clock::now().time_since_epoch().count() + tno;
    mt19937 gen(seed);
    uniform_int_distribution<> distrib(0, 1); // For 50/50 GET/PUT split

    // Create one client per thread. This reuses the connection.
    Client cli("localhost", 1234);
    cli.set_keep_alive(true);

    // Each thread will run 100 requests
    for (int i = 0; i < 100; i++) {
        // Generate a unique ID for this request
        // (tno * 100 + i) ensures no two threads ever hit the same ID
        string id_str = to_string((tno * 100) + i);
        string val_str = "value_" + id_str;
        string path;

        httplib::Result res;

        try {
            switch (mode) {
                case 1: // GET-only
                    path = "/val?id=" + id_str;
                    res = cli.Get(path);
                    cout << "."; // Print a dot for success
                    break;

                case 2: // POST-only
                    path = "/save?id=" + id_str + "&val=" + val_str;
                    res = cli.Post(path);
                    cout << "+"; // Print a plus for success
                    break;

                case 3: // 50/50 MIX of GET and PUT
                    if (distrib(gen) == 0) {
                        // Do a GET
                        path = "/val?id=" + id_str;
                        res = cli.Get(path);
                        cout << ".";
                    } else {
                        // Do a PUT
                        path = "/put?id=" + id_str + "&val=" + val_str + "_updated";
                        res = cli.Put(path);
                        cout << "U"; // Print a 'U' for success
                    }
                    break;
            }

            // Check for connection or response errors
            if (!res) {
                cout << "E"; // 'E' for error
            } else if (res->status >= 400) {
                cout << "F"; // 'F' for HTTP failure (like 404, 500)
            }

        } catch (const std::exception& e) {
            cout << "X"; // 'X' for exception
        }
        
        // Flush cout to see real-time progress
        cout.flush();
    }
}

int main() {
    int num_threads = 0;
    int mode = 0;

    cout << "--- C++ Server Load Tester ---" << endl;
    cout << "Enter number of concurrent clients (threads): ";
    cin >> num_threads;

    cout << "Enter request mode:" << endl;
    cout << "  1: GET-only" << endl;
    cout << "  2: POST-only (Create)" << endl;
    cout << "  3: MIX (50/50 GET and PUT)" << endl;
    cout << "Choice: ";
    cin >> mode;

    if (mode < 1 || mode > 3) {
        cerr << "Invalid mode. Exiting." << endl;
        return 1;
    }
    if (num_threads <= 0) {
        cerr << "Invalid thread count. Exiting." << endl;
        return 1;
    }

    cout << "\nStarting " << num_threads << " clients in mode " << mode << "..." << endl;
    cout << "(. = GET, + = POST, U = PUT, F = HTTP Fail, E/X = Error)" << endl;

    vector<thread> threads;
    auto start_time = chrono::high_resolution_clock::now();

    // Launch all threads
    for (int i = 0; i < num_threads; i++) {
        threads.push_back(thread(client_worker, i, mode));
    }

    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);

    cout << "\n\nTest complete." << endl;
    int total_requests = num_threads * 100;
    cout << "Total requests: " << total_requests << endl;
    cout << "Total time: " << duration.count() << " ms" << endl;
    cout << "Requests per second: " << (total_requests * 1000.0 / duration.count()) << endl;

    return 0;
}