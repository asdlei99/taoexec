#include "model.h"

namespace nbsg {
    namespace model {

        int db_t::open(const std::string& file) {
            int rv = ::sqlite3_open(file.c_str(), &_db);
            if(rv != SQLITE_OK) {
                ::sqlite3_close(_db);
                _db = nullptr;
                return -1;
            }

            _create_tables();

            return 0;
        }

        int db_t::close() {
            if(::sqlite3_close(_db) == SQLITE_OK) {
                _db = nullptr;
                return 0;
            }

            return -1;
        }

        int db_t::_create_tables() {
            char* err;
            const char* sql = "CREATE TABLE IF NOT EXISTS items ("
                "id INTEGER PRIMARY KEY,"
                "index_ TEXT,"
                "group_ TEXT,"
                "comment TEXT,"
                "path TEXT,"
                "params TEXT,"
                "work_dir TEXT,"
                "env TEXT,"
                "visibility INTEGER DEFAULT 1"
                ")";

            if(::sqlite3_exec(_db, sql, nullptr, nullptr, &err) == SQLITE_OK) {
                return 0;
            }

            std::string strerr(err);
            ::sqlite3_free(err);

            return -1;
        }

        // fails on return value <= 0
        int db_t::insert(const item_t* item) {
            if(!item || item->id != -1) {
                return -1;
            }

            const char* sql = "INSERT INTO items (index_,group_,comment,path,params,work_dir,env,visibility)"
                " VALUES (?,?,?,?,?,?,?,?);";

            sqlite3_stmt* stmt;
            if(::sqlite3_prepare(_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
                return -1;
            }

            bool next = true;
            next = next && ::sqlite3_bind_text(stmt, 1, item->index.c_str(), item->index.size(), nullptr) == SQLITE_OK;
            next = next && ::sqlite3_bind_text(stmt, 2, item->group.c_str(), item->group.size(), nullptr) == SQLITE_OK;
            next = next && ::sqlite3_bind_text(stmt, 3, item->comment.c_str(), item->comment.size(), nullptr) == SQLITE_OK;
            next = next && ::sqlite3_bind_text(stmt, 4, item->path.c_str(), item->path.size(), nullptr) == SQLITE_OK;
            next = next && ::sqlite3_bind_text(stmt, 5, item->params.c_str(), item->params.size(), nullptr) == SQLITE_OK;
            next = next && ::sqlite3_bind_text(stmt, 6, item->work_dir.c_str(), item->work_dir.size(), nullptr) == SQLITE_OK;
            next = next && ::sqlite3_bind_text(stmt, 7, item->env.c_str(), item->env.size(), nullptr) == SQLITE_OK;
            next = next && ::sqlite3_bind_int (stmt, 8, item->visibility ? 1 : 0) == SQLITE_OK;

            if(next == false) {
                ::sqlite3_finalize(stmt);
                return -1;
            }

            if(::sqlite3_step(stmt) != SQLITE_DONE) {
                ::sqlite3_finalize(stmt);
                return -1;
            }

            ::sqlite3_finalize(stmt);

            return (int)::sqlite3_last_insert_rowid(_db);
        }

        int db_t::remove(const std::string& where) {
            return -1;
        }

        bool db_t::has(int i) {
            return false;
        }

        int db_t::query(const std::string& pattern, std::vector<item_t*>* items) {
            return -1;
        }

        int db_t::modify(const item_t* item) {
            return -1;
        }

    }
}
