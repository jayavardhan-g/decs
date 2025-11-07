// g++ 5s.cpp -lpqxx -lpq -lpthread -o server
#include "httplib.h"
#include <condition_variable>
#include <list>
#include <mutex>
#include <pqxx/pqxx>
#include <queue>
#include <unordered_map>

using namespace std;
using namespace httplib;
namespace db = pqxx;

class KVCache {
  size_t capacity;
  list<pair<int, string>> items;
  unordered_map<int, list<pair<int, string>>::iterator> index;
  mutex mtx;

public:
  KVCache(size_t cap) : capacity(cap) {}

  bool get(int key, string &value) {
    lock_guard<mutex> lock(mtx);
    auto it = index.find(key);
    if (it == index.end())
      return false;

    // Move to front (LRU)
    items.splice(items.begin(), items, it->second);
    value = it->second->second;
    // cout << "Cache hit: " << key << " -> " << value << endl;
    return true;
  }

  void put(int key, const string &value) {
    lock_guard<mutex> lock(mtx);
    if (index.count(key)) {
      items.erase(index[key]);
    } else if (items.size() >= capacity) {
      auto last = items.back();
      index.erase(last.first);
      items.pop_back();
    }
    items.emplace_front(key, value);
    index[key] = items.begin();
    // cout << "Cache put: " << key << " -> " << value << endl;
  }

  void erase(int key) {
    lock_guard<mutex> lock(mtx);
    if (index.count(key)) {
      items.erase(index[key]);
      index.erase(key);
    }
    // cout << "Cache delete: " << key << endl;
  }
};

class LibpqxxPool {
private:
  queue<db::connection *> pool;
  mutex mtx;
  condition_variable cv;
  string conn_string;

public:
  LibpqxxPool(int size, const string &connStr) : conn_string(connStr) {
    for (int i = 0; i < size; i++) {
      try {
        pool.push(new pqxx::connection(conn_string));
      } catch (const std::exception &e) {
        cerr << "DB connection failed in pool: " << e.what() << endl;
        exit(1);
      }
    }
  }

  db::connection *acquire() {
    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [&] { return !pool.empty(); });
    db::connection *conn = pool.front();
    pool.pop();
    return conn;
  }

  void release(db::connection *conn) {
    unique_lock<mutex> lock(mtx);
    pool.push(conn);
    lock.unlock();
    cv.notify_one();
  }

  ~LibpqxxPool() {
    while (!pool.empty()) {
      delete pool.front();
      pool.pop();
    }
  }
};

int parse_id(const string &s) {
  try {
    return stoi(s);
  } catch (...) {
    return -1;
  }
}

int main() {
  const string conn_str =
      "dbname=decs user=postgres password=kali host=192.168.137.1";

  cout << "Enter pool size: " << endl;
  ;
  int pool_size;
  cin >> pool_size;
  cout << "Enter cache size: " << endl;
  ;
  int cache_size;
  cin >> cache_size;
  LibpqxxPool pool(pool_size, conn_str);
  try {
    db::connection *init_conn = pool.acquire();
    {
      db::work setup_txn{*init_conn};
      setup_txn.exec("CREATE TABLE IF NOT EXISTS kv_store ("
                     "  id INT PRIMARY KEY,"
                     "  value TEXT NOT NULL"
                     ");");
      setup_txn.commit();
    }
    cout << "Database table 'kv_store' is ready." << endl;
    pool.release(init_conn);
  } catch (const std::exception &e) {
    cerr << "Fatal error: Could not initialize database. " << e.what() << endl;
    return 1;
  }

  KVCache cache(cache_size);
  Server srv;
  cout << "Enter http_threads: ";
  int http_thread;
  cin >> http_thread;
  srv.new_task_queue = [http_thread] {
    return new httplib::ThreadPool(http_thread);
  };

  srv.Get("/", [](const Request &req, Response &res) {
    string s = "Hello world! Your IP: " + req.remote_addr + "\n";
    res.set_content(s, "text/plain");
  });

  // GET
  srv.Get("/val", [&](const Request &req, Response &res) {
    if (!req.has_param("id")) {
      res.status = 400;
      res.set_content("Error: 'id' parameter is missing.", "text/plain");
      return;
    }

    int id_int = parse_id(req.get_param_value("id"));
    if (id_int == -1) {
      res.status = 400;
      res.set_content("Invalid ID", "text/plain");
      return;
    }

    string cached_value;

    if (cache.get(id_int, cached_value)) {
      res.set_content(cached_value, "text/plain");
      return;
    }

    db::connection *conn = nullptr;
    try {
      conn = pool.acquire();
      db::work txn{*conn};
      db::result r =
          txn.exec_params("SELECT value FROM kv_store WHERE id = $1", id_int);
      txn.commit();
      pool.release(conn);

      if (r.empty()) {
        res.status = 404;
        res.set_content("No value found for id: " + to_string(id_int),
                        "text/plain");
      } else {
        string db_val = r[0][0].as<string>();
        cache.put(id_int, db_val);
        res.set_content(db_val, "text/plain");
      }
    } catch (const std::exception &e) {
      if (conn)
        pool.release(conn);
      res.status = 500;
      res.set_content(string("Database error: ") + e.what(), "text/plain");
    }
  });

  // POST
  srv.Post("/save", [&](const Request &req, Response &res) {
    if (!req.has_param("id") || !req.has_param("val")) {
      res.status = 400;
      res.set_content("Error: Missing 'id' or 'val'.", "text/plain");
      return;
    }

    int id_int = parse_id(req.get_param_value("id"));
    string val = req.get_param_value("val");

    if (id_int == -1) {
      res.status = 400;
      res.set_content("Invalid ID", "text/plain");
      return;
    }

    db::connection *conn = nullptr;

    try {
      conn = pool.acquire();
      db::work txn{*conn};
      txn.exec_params("INSERT INTO kv_store (id, value) VALUES ($1, $2)",
                      id_int, val);
      txn.commit();
      pool.release(conn);

      cache.put(id_int, val);
      res.set_content("Key saved: " + to_string(id_int), "text/plain");
    } catch (const db::unique_violation &e) {
      if (conn)
        pool.release(conn);
      res.status = 409;
      res.set_content("Duplicate key: " + to_string(id_int), "text/plain");
    } catch (const std::exception &e) {
      if (conn)
        pool.release(conn);
      res.status = 500;
      res.set_content(string("Database error: ") + e.what(), "text/plain");
    }
  });

  // PUT
  srv.Put("/put", [&](const Request &req, Response &res) {
    if (!req.has_param("id") || !req.has_param("val")) {
      res.status = 400;
      res.set_content("Error: Missing 'id' or 'val'.", "text/plain");
      return;
    }

    int id_int = parse_id(req.get_param_value("id"));
    string val = req.get_param_value("val");

    if (id_int == -1) {
      res.status = 400;
      res.set_content("Invalid ID", "text/plain");
      return;
    }

    db::connection *conn = nullptr;
    try {
      db::connection *conn = pool.acquire();
      db::work txn{*conn};
      db::result r = txn.exec_params(
          "UPDATE kv_store SET value = $1 WHERE id = $2", val, id_int);
      txn.commit();
      pool.release(conn);

      if (r.affected_rows() == 0) {
        res.status = 404;
        res.set_content("No entry for ID: " + to_string(id_int), "text/plain");
      } else {
        cache.put(id_int, val);
        res.set_content("Updated ID: " + to_string(id_int), "text/plain");
      }
    } catch (const std::exception &e) {
      if (conn)
        pool.release(conn);
      res.status = 500;
      res.set_content(string("Database error: ") + e.what(), "text/plain");
    }
  });

  // DELETE
  srv.Delete("/delete", [&](const Request &req, Response &res) {
    if (!req.has_param("id")) {
      res.status = 400;
      res.set_content("Error: Missing 'id'.", "text/plain");
      return;
    }

    int id_int = parse_id(req.get_param_value("id"));
    if (id_int == -1) {
      res.status = 400;
      res.set_content("Invalid ID", "text/plain");
      return;
    }

    db::connection *conn = nullptr;
    try {
      db::connection *conn = pool.acquire();
      db::work txn{*conn};
      db::result r =
          txn.exec_params("DELETE FROM kv_store WHERE id = $1", id_int);
      txn.commit();
      pool.release(conn);

      if (r.affected_rows() == 0) {
        res.status = 404;
        res.set_content("No entry for ID: " + to_string(id_int), "text/plain");
      } else {
        cache.erase(id_int);
        res.set_content("Deleted ID: " + to_string(id_int), "text/plain");
      }
    } catch (const std::exception &e) {
      if (conn)
        pool.release(conn);
      res.status = 500;
      res.set_content(string("Database error: ") + e.what(), "text/plain");
    }
  });

  // GET No Cache
  srv.Get("/nocache/val", [&](const Request &req, Response &res) {
    if (!req.has_param("id")) {
      res.status = 400;
      res.set_content("Error: 'id' parameter is missing.", "text/plain");
      return;
    }

    int id_int = parse_id(req.get_param_value("id"));
    if (id_int == -1) {
      res.status = 400;
      res.set_content("Invalid ID", "text/plain");
      return;
    }

    string cached_value;

    db::connection *conn = nullptr;
    try {
      db::connection *conn = pool.acquire();
      db::work txn{*conn};
      db::result r =
          txn.exec_params("SELECT value FROM kv_store WHERE id = $1", id_int);
      txn.commit();
      pool.release(conn);

      if (r.empty()) {
        res.status = 404;
        res.set_content("No value found for id: " + to_string(id_int),
                        "text/plain");
      } else {
        string db_val = r[0][0].as<string>();
        res.set_content(db_val, "text/plain");
      }
    } catch (const std::exception &e) {
      if (conn)
        pool.release(conn);
      res.status = 500;
      res.set_content(string("Database error: ") + e.what(), "text/plain");
    }
  });

  cout << "🚀 Server running on http://localhost:1234 ..." << endl;
  srv.listen("0.0.0.0", 1234);
  return 0;
}
