#ifndef _DICT_H
#define _DICT_H

#include <string>
#include <vector>
#include <curl/curl.h>

enum Result {FAILED, SUCCESS};
typedef void (*setopt_callback)(CURL *curl, const char *s);
enum dict_type {ICIBA, BING};

class Dict
{
    public:
        Dict() = default;
        Dict(const std::string& s): key(s) {}
        void print() const;
        void pron();
        bool has_pron() {
            return !en_prone.second.empty();
        }
        const std::string& get_pron_url() {
            if (!us_prone.second.empty())
                return us_prone.second;
            return en_prone.second;
        }

        const char* word() {return key.c_str();}
        virtual dict_type type() = 0;

        const char* en_pron() {
            return en_prone.first.c_str();
        }

        const char* en_pron_url() {
            return en_prone.second.c_str();
        }

        const char* us_pron() {
            return us_prone.first.c_str();
        }

        const char* us_pron_url() {
            return us_prone.second.c_str();
        }

        std::vector<std::pair<std::string, std::string>>& brief() {
            return pos;
        }

        std::vector<std::pair<std::string, std::string>>& detail()
        {
            return trans;
        }

        operator bool() const
        {
            return !key.empty();
        }
        friend void set_dict(Dict *dict, std::string key, std::string en_prone, std::string en_prone_url, std::string us_prone, std::string us_prone_url);
        friend void set_dict_pos(Dict *dict, std::string origin, std::string trans);
        friend void set_dict_trans(Dict *dict, std::string origin, std::string trans);

    protected:
        std::string key; // 查找的单词
        std::pair<std::string, std::string> en_prone; // 英式发音, 英式发音地址
        std::pair<std::string, std::string> us_prone; // 美式发音, 美式发音地址
        std::vector<std::pair<std::string, std::string>> pos; // 简介
        std::vector<std::pair<std::string, std::string>> trans; // 详细翻译
};

class Iciba: public Dict
{
    public:
        using Dict::Dict;
        Result parse(const std::string& content);
        dict_type type() {return ICIBA;}
};

class Bing: public Dict
{
    public:
        using Dict::Dict;
        Result parse(const std::string& content);
        dict_type type() {return BING;}
};

std::string get_response(Dict &dict);
void save(Dict &dict);
Iciba query(const std::string& key);

#endif
