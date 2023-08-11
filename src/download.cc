#include <stdio.h>
#include <string.h>
#include <iostream>
#include <curl/curl.h>
#include "dict.h"

#define ICIBA_BASE_URL "http://dict-co.iciba.com/api/dictionary.php?key=D191EBD014295E913574E1EAF8E06666"
#define BING_BASE_URL "https://cn.bing.com/dict/search?q="

using namespace std;

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    auto buf = static_cast<string *>(userdata);
    auto realsize = size * nmemb;
#ifdef DEBUG
    printf("Read %lu bytes from server\n", realsize);
#endif

    buf->append(ptr, realsize);
    return realsize;
}

string get_response(Dict &dict)
{

    CURL *curl;
    CURLcode res;
    string content;

    /* In windows, this will init the winsock stuff */
    // curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if(curl) {
        std::string query = "w=";
        string url = BING_BASE_URL;
        char *output = NULL;
        switch (dict.type()) {
            case ICIBA:
                query.append(dict.word());
                curl_easy_setopt(curl, CURLOPT_URL, ICIBA_BASE_URL);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.c_str());
                break;
            case BING:
                output = curl_easy_escape(curl, dict.word(), 0);
                if (!output)
                    return "";
#ifdef DEBUG
                printf("Encoded: %s\n", output);
#endif
                url.append(output);
                curl_free(output);
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                break;
            default:
                return "";
        }

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
        /* Perform the request, res will get the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    // curl_global_cleanup();

    return content;
}

