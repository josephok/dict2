#include <unistd.h>
#include <regex>
#include <sys/types.h>
#include <pwd.h>
#include <iostream>
#include <filesystem>
#include "dict.h"
#include "sqlite3.h"

#define PATH ".cache/dict.db"
#define PRINT_SQL_ERROR(zErrMsg) fprintf(stderr, "SQL error: %s at line %d\n", zErrMsg, __LINE__);

static sqlite3 *db;

namespace fs = std::filesystem;
using namespace std;

static void inline handle_sql_error(char *zErrMsg)
{
#ifdef DEBUG
    PRINT_SQL_ERROR(zErrMsg);
#endif
    sqlite3_free(zErrMsg);
}

static string get_cache_path()
{
    struct passwd *pw = getpwuid(getuid());

    const char *homedir = pw->pw_dir;
    fs::path cache_path = fs::path(homedir)  / PATH;

    return cache_path.string();
}

static Result open_db()
{
    string path = get_cache_path();
    if (sqlite3_open(path.c_str(), &db)) {
#ifdef DEBUG
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
#endif
        sqlite3_close(db);
        return FAILED;
    }

    return SUCCESS;
}

static Result create_table()
{
    if (!open_db())
        return FAILED;

    const char *sql = " PRAGMA foreign_keys = ON;"
        "CREATE TABLE IF NOT EXISTS dict ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "key TEXT NOT NULL,"
        "en_prone TEXT,"
        "en_prone_url TEXT,"
        "us_prone TEXT,"
        "us_prone_url TEXT"
    ");"
    "CREATE UNIQUE INDEX IF NOT EXISTS key_index on dict (key);"
    "CREATE TABLE IF NOT EXISTS brief ("
    "    dict_id INTEGER,"
    "    origin TEXT NOT NULL,"
    "    trans TEXT NOT NULL,"
    "    FOREIGN KEY (dict_id) REFERENCES dict(id) ON DELETE CASCADE"
    ");"
    "CREATE TABLE IF NOT EXISTS detail ("
    "    dict_id INTEGER,"
    "    origin TEXT NOT NULL,"
    "    trans TEXT NOT NULL,"
    "    FOREIGN KEY (dict_id) REFERENCES dict(id) ON DELETE CASCADE"
    ");";

    char *zErrMsg = 0;
    if (sqlite3_exec(db, sql, NULL, 0, &zErrMsg) != SQLITE_OK) {
#ifdef DEBUG
        fprintf(stderr, "SQL error create_table: %s\n", zErrMsg);
#endif
        sqlite3_free(zErrMsg);
    }
    sqlite3_close(db);

    return SUCCESS;
}

static int get_id(void *id, int argc, char **argv, char **azColName)
{
    uint64_t *dict_id = (uint64_t *)id;
    *dict_id = strtoull(argv[0], NULL, 10);
    return 0;
}

static inline string escape_quote(const string &s)
{
    return regex_replace(s, regex("'"), "''");
}

void save(Dict &dict)
{
    if (!create_table())
        return;

    const char* sql = "BEGIN; INSERT INTO dict (key, en_prone, en_prone_url, us_prone, us_prone_url)"
        " VALUES ('%s', '%s', '%s', '%s', '%s') RETURNING id;";

    char buf[BUFSIZ];
    snprintf(buf, BUFSIZ, sql, dict.word(), escape_quote(dict.en_pron()).c_str(), dict.en_pron_url(),
            escape_quote(dict.us_pron()).c_str(), dict.us_pron_url());

    if (!open_db())
        return;

    char *zErrMsg = 0;
    uint64_t id;
    if (sqlite3_exec(db, buf, get_id, &id, &zErrMsg) != SQLITE_OK) {
        PRINT_SQL_ERROR(zErrMsg);
        goto CLOSE;
    }

    if (!dict.brief().empty()) {
        string sql = "INSERT INTO brief (dict_id, origin, trans) VALUES ";
        size_t index = 0;
        for (auto b: dict.brief()) {
            snprintf(buf, BUFSIZ, ++index == dict.brief().size() ? "(%lu, '%s', '%s');": "(%lu, '%s', '%s'),", id, b.first.c_str(), b.second.c_str());
            sql += buf;
        }

        if (sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK) {
            handle_sql_error(zErrMsg);
            goto CLOSE;
        }
    }

    if (!dict.detail().empty()) {
        string sql = "INSERT INTO detail (dict_id, origin, trans) VALUES ";
        size_t index = 0;
        for (auto b: dict.detail()) {
            snprintf(buf, BUFSIZ, ++index == dict.detail().size() ? "(%lu, '%s', '%s');": "(%lu, '%s', '%s'),",
                    id, escape_quote(b.first).c_str(), escape_quote(b.second).c_str());
            sql += buf;
        }

        if (sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg) != SQLITE_OK) {
            handle_sql_error(zErrMsg);
            goto CLOSE;
        }
    }

    sqlite3_exec(db, "COMMIT;", NULL, 0, NULL);

CLOSE:
    sqlite3_close(db);

}

void set_dict(Dict *dict, string key, string en_prone, string en_prone_url, string us_prone, string us_prone_url)
{
    dict->key = key;
    dict->en_prone.first = en_prone;
    dict->en_prone.second = en_prone_url;
    dict->us_prone.first = us_prone;
    dict->us_prone.second = us_prone_url;
}

void set_dict_pos(Dict *dict, string origin, string trans)
{
    dict->pos.push_back(make_pair(origin, trans));
}

void set_dict_trans(Dict *dict, string origin, string trans)
{
    dict->trans.push_back(make_pair(origin, trans));
}

static int get_dict(void *dict, int argc, char **argv, char **azColName)
{
    auto d = static_cast<pair<uint64_t, Dict*>*>(dict);
    set_dict(d->second, argv[0], argv[1], argv[2], argv[3], argv[4]);
    d->first = strtoull(argv[5], NULL, 10);
    return 0;
}

static int get_dict_brief(void *dict, int argc, char **argv, char **azColName)
{
    auto d = static_cast<Dict*>(dict);
    set_dict_pos(d, argv[0], argv[1]);
    return 0;
}

static int get_dict_detail(void *dict, int argc, char **argv, char **azColName)
{
    auto d = static_cast<Dict*>(dict);
    set_dict_trans(d, argv[0], argv[1]);
    return 0;
}

Iciba query(const string& key)
{
    Iciba dict;

    const char* sql = "SELECT key, en_prone, en_prone_url, us_prone, us_prone_url, id from dict where key = '%s';";
    char buf[BUFSIZ];
    snprintf(buf, BUFSIZ, sql, key.c_str());

    if (!open_db())
        return dict;

    char *zErrMsg = 0;

    pair<uint64_t, Dict*> p(0, &dict);

    if (sqlite3_exec(db, buf, get_dict, &p, &zErrMsg) != SQLITE_OK) {
        handle_sql_error(zErrMsg);
    }

    sql = "SELECT origin, trans from brief where dict_id = %lu";
    snprintf(buf, BUFSIZ, sql, p.first);
    if (sqlite3_exec(db, buf, get_dict_brief, &dict, &zErrMsg) != SQLITE_OK) {
        handle_sql_error(zErrMsg);
    }

    sql = "SELECT origin, trans from detail where dict_id = %lu";
    snprintf(buf, BUFSIZ, sql, p.first);
    if (sqlite3_exec(db, buf, get_dict_detail, &dict, &zErrMsg) != SQLITE_OK) {
        handle_sql_error(zErrMsg);
    }
    sqlite3_close(db);

    return dict;
}
