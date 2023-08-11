#include <iostream>
#include <stdlib.h>
#include <regex>
#include "pugixml.hpp"
#include "dict.h"


using namespace std;


Result Iciba::parse(const string& content)
{
    pugi::xml_document doc;
    auto result = doc.load_string(content.c_str(), pugi::parse_trim_pcdata);

    if (result) {
        // success
        auto dict = doc.child("dict");
        auto en_prone = dict.child("ps");
        if (en_prone) {
            this->en_prone.first = en_prone.child_value();
            auto pron = en_prone.next_sibling("pron");
            if (pron)
                this->en_prone.second = pron.child_value();

            auto us_prone = en_prone.next_sibling("ps");
            if (us_prone) {
                this->us_prone.first = us_prone.child_value();
                pron = us_prone.next_sibling("pron");
                if (pron)
                    this->us_prone.second = pron.child_value();
            }
        }

        auto pos = dict.children("pos");
        if (!pos.empty()) {
            for (auto &p : pos) {
                string acceptation;
                if (p.next_sibling("acceptation"))
                    acceptation = p.next_sibling("acceptation").child_value();
                this->pos.push_back(make_pair(p.child_value(), acceptation));
            }
        }

        auto sent = dict.children("sent");
        if (!sent.empty()) {
            for (auto &s : sent) {
                pair<string, string> trans(s.child("orig").child_value(), s.child("trans").child_value());
                this->trans.push_back(trans);
            }
        }

        return SUCCESS;
    }
    else
        return FAILED;

}

static inline string purple(const string& s)
{
    return "\033[0;35m" + s + "\033[0m";
}

static inline string green(const string& s)
{
    return "\033[0;32m" + s + "\033[0m";
}

static inline string cyan(const string& s)
{
    return "\033[0;36m" + s + "\033[0m";
}

static inline string decode_html_entity(const string& s)
{
    return regex_replace(s, regex("&amp;"), "&");
}

void Dict::print() const
{
    cout << "\n";
    auto en = purple("[ " + en_prone.first + " ]");
    auto us = purple("[ " + us_prone.first + " ]");
    cout << " " << key << " 发音: 英" << en << " 美" << us << "\n\n";

    for (auto &p: pos) {
        cout << green(decode_html_entity("- " + p.first + " " + p.second)) << endl;
    }
    cout << endl;

    int i = 0;
    for (auto &t : trans) {
        cout << ++i << ".  " << t.first << endl;
        cout << "    " << cyan(t.second) << endl;
    }
}

void Dict::pron()
{
    auto ret = system("which mpg123 > /dev/null 2>&1");
    if (ret == 0) {
        if (has_pron()) {
            string command = "mpg123 ";
            command += get_pron_url() + " > /dev/null 2>&1";
            system(command.c_str());
        }
    }
}
