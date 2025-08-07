#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/algorithm/string/trim.hpp>

#define MAJ_VERSION "3"
#define MIN_VERSION "1"
#define VERSION "v" MAJ_VERSION "." MIN_VERSION

constexpr const char* TARGET_URL = "https://www.dnsleaktest.com/";
constexpr const char* LOCAL_URL = "https://example.com";

size_t writefunc(void *ptr, size_t size, size_t nmemb, std::string *buf) {
    size_t new_len = size * nmemb, tmp = 0;

    while (tmp < new_len)
        (*buf) += ((char*)ptr)[tmp++];

    return new_len;
}

CURL *init_curl(std::string* buf) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL *curl;
 
    if(!(curl = curl_easy_init())) {
        fputs("myip: curl_easy_init(): failed", stderr);
        return NULL;
    }

#ifdef SKIP_PEER_VERIFICATION
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif
#ifdef SKIP_HOSTNAME_VERIFICATION
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);

    return curl;
}

void close_curl(CURL *curl) {
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

int print_global_ip(CURL *curl, std::string *buf, bool single = false) {
    curl_easy_setopt(curl, CURLOPT_URL, TARGET_URL);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        fprintf(stderr, "myip: curl_easy_perform(): failed: %s\n"
        "[-] Maybe you have not the internet connetion\n", curl_easy_strerror(res));
        return 1;
    }

    size_t pos_tag_hello_start = buf->find("<p class=\"hello");
    pos_tag_hello_start = buf->find(">", pos_tag_hello_start) + 7;
    size_t pos_tag_hello_end = buf->find("</p>", pos_tag_hello_start);

    size_t pos_tag_from_start = buf->find(">from ") + 6;
    size_t pos_tag_from_end = buf->find("<", pos_tag_from_start);

    std::cout << "IP" << ((single) ? ":\t\t" : " (global):\t" )
            << buf->substr(pos_tag_hello_start, pos_tag_hello_end - pos_tag_hello_start) 
            << std::endl;

    std::string location = buf->substr(pos_tag_from_start, pos_tag_from_end - pos_tag_from_start);
    boost::algorithm::trim(location);
    std::cout << "Location:\t" 
            << location
            << std::endl;

    return 0;
}

int print_local_ip(CURL *curl, bool single = false) {
    char *ip;
    
    curl_easy_setopt(curl, CURLOPT_URL, LOCAL_URL);

    CURLcode res = curl_easy_perform(curl);
    if((res == CURLE_OK) && !curl_easy_getinfo(curl, CURLINFO_LOCAL_IP, &ip) && ip) {
        std::cout << "IP" 
            << ((single) ? ":\t\t" : " (local):\t" ) 
            << ip << std::endl;
    }
    else {
        fprintf(stderr, "myip: curl_easy_perform(): failed: %s\n", curl_easy_strerror(res));
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    std::string buf;
    CURL *curl = init_curl(&buf);

    if (curl == NULL) {
        return -1;
    }

    if (argc == 1) {
        if (print_local_ip(curl) || print_global_ip(curl, &buf)) {
            return -1;
        }
    }
    else if (argc == 2) {
        std::string arg1 = argv[1];

        if (arg1 == "-l" || arg1 == "--local") {
            if (print_local_ip(curl, true)) {
                return -1;
            }
        }
        else if (arg1 == "-g" || arg1 == "--global") {
            if (print_global_ip(curl, &buf, true)) {
                return -1;
            }
        }
        else if (arg1 == "-a" || arg1 == "--all") {
            if (print_local_ip(curl) || print_global_ip(curl, &buf)) {
                return -1;
            }
        }
        else if (arg1 == "-v" || arg1 == "--version") {
            std::cout << "myip " VERSION "\n";
        }
        else if (arg1 == "-h" || arg1 == "--help") {
            std::cout << "myip [OPNIONS]\n"
                "myip (" << VERSION << ") - A mini utility for getting your ip address\n\n"
                "Options:\n"
                "\t-l/--local\t\t- print local IP address\n"
                "\t-g/--global\t\t- print global IP address and location\n"
                "\t-a/--all\t\t- print local and global IP addresses and location (default)\n"
                "\t-v/--version\t\t- print program version\n"
                "\t-h/--help\t\t- print this message\n\n";
        }
        else {
            std::cout << "myip: invalid argument, try 'myip --help'\n";
        }
    }

    close_curl(curl);

    return 0;
}