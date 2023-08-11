#include <iostream>
#include <cstring>
#include <cstdlib>
#include <curl/curl.h>
#include "dict.h"

using namespace std;

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s word\n", argv[0]);
        return EXIT_FAILURE;
    }

    string word;
    for (auto i=1; i<argc; i++) {
        word.append(argv[i]);
        if (i != argc-1)
            word.append(" ");
    }

    Iciba dict = query(word);
    if (!dict) {
        dict = Iciba(word);
        string content = get_response(dict);
        if (content.empty())
            return EXIT_FAILURE;
#ifdef DEBUG
        cout << content << endl;
#endif
        if (dict.parse(content)) {
            dict.print();
            save(dict);
        }
    } else {
        dict.print();
    }

#ifdef PRON
    dict.pron();
#endif
}
