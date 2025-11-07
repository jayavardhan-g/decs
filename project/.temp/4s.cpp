#include "httplib.h"
#include <bits/stdc++.h>
#include <pqxx/pqxx> 

using namespace std;
using namespace httplib;

int id(string &s) {
  int id;
  try {
    id = stoi(s);
  } catch (const invalid_argument &e) {
    return -1;
  } catch (const out_of_range &e) {
    return -1;
  }
  return id;
}

namespace db= pqxx;

int main() {

  try {
    db::connection conn("dbname=decs user=postgres password=kali host= 192.168.137.1");
    cout << "Successfully connected to " << conn.dbname() << endl;

    { 
      db::work setup_txn{conn};
      setup_txn.exec(
          "CREATE TABLE IF NOT EXISTS kv_store ("
          "  id INT PRIMARY KEY,"
          "  value TEXT NOT NULL"
          ");");
      setup_txn.commit();
      cout << "Database table 'kv_store' is ready." << endl;
    }

    Server srv;

    srv.Get("/", [](const Request &req, Response &res) {
      string addr = req.remote_addr + " " + to_string(req.remote_port);
      string s = "Hello world! " + addr + "\n";
      res.set_content(s, "text/plain");
    });

//GET
    srv.Get("/val", [&](const Request &req, Response & res) {
      if (!req.has_param("id")) {
        res.status = 400; 
        res.set_content("Error: 'id' parameter is missing.", "text/plain");
        return;
      }

      string id_str = req.get_param_value("id");
      int id_int = id(id_str);

      if (id_int == -1) {
        res.status = 400;
        res.set_content("Not a valid id", "text/plain");
        return;
      }

      try {

        db::work txn{conn};

        db::result r = txn.exec_params(
            "SELECT value FROM kv_store WHERE id = $1",
            id_int);

        if (r.empty()) {
          res.status = 404; 
          res.set_content("No value found for id: " + id_str, "text/plain");
        } else {
          res.set_content(r[0][0].as<std::string>(), "text/plain");
        }
      } catch (const std::exception &e) {
        res.status = 500; 
        res.set_content(string("Database error: ") + e.what(), "text/plain");
      }
    });
//POST
    srv.Post("/save", [&](const Request &req, Response & res) {
      if (!req.has_param("id") || !req.has_param("val")) {
        res.status = 400;
        res.set_content("Error: 'id' or 'val' parameter missing.", "text/plain");
        return;
      }

      string id_str = req.get_param_value("id");
      int id_int = id(id_str);

      if (id_int == -1) {
        res.status = 400;
        res.set_content("Not a valid id", "text/plain");
        return;
      }

      string val = req.get_param_value("val");

      try {
        db::work txn{conn};
        
        // INSERT the new key-value pair
        txn.exec_params(
            "INSERT INTO kv_store (id, value) VALUES ($1, $2)",
            id_int,
            val);
        txn.commit(); 

        res.set_content("Key value pair created \nID = " + id_str +
                            " \nValue = " + val,
                        "text/plain");
      } catch (const db::unique_violation &e) {
        // This fires if the ID (Primary Key) already exists
        res.status = 409; // 409 Conflict
        res.set_content("Error: An entry with id " + id_str + " already exists.", "text/plain");
      } catch (const std::exception &e) {
        res.status = 500;
        res.set_content(string("Database error: ") + e.what(), "text/plain");
      }
    });
//PUT
    srv.Put("/put", [&](const Request &req, Response & res) {
      if (!req.has_param("id") || !req.has_param("val")) {
        res.status = 400;
        res.set_content("Error: 'id' or 'val' parameter missing.", "text/plain");
        return;
      }

      string id_str = req.get_param_value("id");
      int id_int = id(id_str);

      if (id_int == -1) {
        res.status = 400;
        res.set_content("Not a valid id", "text/plain");
        return;
      }
      
      string val = req.get_param_value("val");

      try {
        db::work txn{conn};
        
        db::result r = txn.exec_params(
            "UPDATE kv_store SET value = $1 WHERE id = $2",
            val,
            id_int);
        txn.commit();
        
        if (r.affected_rows() == 0) {
          res.status = 404;
          res.set_content("There is no key value pair for this key: " + id_str, "text/plain");
        } else {
          res.set_content("Key value pair updated \nID = " + id_str +
                              " \nValue = " + val,
                          "text/plain");
        }
        
      } catch (const std::exception &e) {
        res.status = 500;
        res.set_content(string("Database error: ") + e.what(), "text/plain");
      }
    });

// DELETE 
    srv.Delete("/delete", [&](const Request &req, Response & res) {
      if (!req.has_param("id")) {
        res.status = 400;
        res.set_content("Error: 'id' parameter is missing.", "text/plain");
        return;
      }

      string id_str = req.get_param_value("id");
      int id_int = id(id_str);

      if (id_int == -1) {
        res.status = 400;
        res.set_content("Not a valid id", "text/plain");
        return;
      }

      try {
        db::work txn{conn};
        
        db::result r = txn.exec_params(
            "DELETE FROM kv_store WHERE id = $1",
            id_int);
        txn.commit();

        if (r.affected_rows() == 0) {
          res.status = 404; 
          res.set_content("There is no key value pair for this key: " + id_str, "text/plain");
        } else {
          res.set_content("Key value pair deleted \nID = " + id_str, "text/plain");
        }

      } catch (const std::exception &e) {
        res.status = 500;
        res.set_content(string("Database error: ") + e.what(), "text/plain");
      }
    });

    // --- START SERVER ---
    cout << "Server listening on http://localhost:1234 ..." << endl;
    srv.listen("localhost", 1234);

  } catch (const std::exception &e) {
    cerr << "Fatal error: Could not connect to database or setup table." << endl;
    cerr << e.what() << endl;
    return 1; 
  }

  return 0;
}