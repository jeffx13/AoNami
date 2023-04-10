#ifndef CLIENT_H
#define CLIENT_H

#include "curl/curl.h"
#include "CSoup.h"
#include <nlohmann/json.hpp>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QMap>
#include <iostream>
#include <memory>
#include <QUrlQuery>
#include <QEventLoop>
#include <QNetworkCookie>




class NetworkClient
{
private:
    CURL *curl;
    void initHandle(CURL* curl){
        if (curl) {
            // Set the timeouts
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

            // Set the user agent
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3");

            // Set SSL options
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &WriteCallback);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, defaultUA.c_str());
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        }
    }

public:
    NetworkClient() {
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init ();
        initHandle (curl);
    }
    ~NetworkClient(){
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        curl = nullptr;
    }
public:
    struct Response{
        long code;
        std::string url;
        std::string redirectUrl;
//        std::map<std::string, std::string> headers;
        std::string headers;
        std::string body;
        std::map<std::string, std::string> cookies;
        CSoup document(){
            return CSoup(body);
        }
        nlohmann::json json(){
            return nlohmann::json::parse (body);
        }
        ~Response(){
        }
    };
    Response get(const std::string_view &url, const std::map<std::string, std::string> &headers = {{"user-agent","Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/111.0.0.0 Safari/537.36"}}, const std::map<std::string, std::string> &params = {}){
        Response response;
        std::stringstream ss;
        std::string queryParams;

        // Build the query parameters
        if (!params.empty()) {
            ss << "?";
            for (auto const& [key, value] : params) {
                ss << key << "=" << value << "&";
            }
            queryParams = ss.str();
            // remove the last '&'
            queryParams.pop_back();
        }

        // Set the URL
        std::string fullUrl = url.data ()+ queryParams;
        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());

        // Set the headers
        struct curl_slist* headerLista = NULL;
        for (auto const& [key, value] : headers) {
            std::string header = std::string(key) + ":" + std::string(value);
            headerLista = curl_slist_append(headerLista, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerLista);

        // Set the response callback function
        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);

        // Perform the GET request
        CURLcode res = curl_easy_perform(curl);

        // Get the response code
        long code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        curl_slist_free_all(headerLista);

        // Parse the response
        response.url = fullUrl;
        response.body = readBuffer;
        response.code = code;

        return response;

    }

    Response post(const std::string_view& url, const std::map<std::string, std::string>& headers, const std::map<std::string, std::string>& data) {
        Response response;

        // Set the URL
        curl_easy_setopt(curl, CURLOPT_URL, url.data ());

        // Set the headers
        struct curl_slist* headerLista = NULL;
        for (auto const& x : headers) {
            std::string header = std::string(x.first) + ":" + std::string(x.second);
            headerLista = curl_slist_append(headerLista, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerLista);

        // Set the post data
        std::string postData;
        for (auto const& x : data) {
            postData += std::string(x.first) + "=" + std::string(x.second) + "&";
        }
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());

        // Set the response callback function
        std::string readBuffer;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);

        // Perform the POST request
        CURLcode res = curl_easy_perform(curl);

        // Get the response code
        long code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        curl_slist_free_all(headerLista);

        // Parse the response
        response.url = url;
        response.body = readBuffer;
        response.code = code;

        return response;
    }

private:

    // WriteCallback function to handle the response body
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*) userp)->append((char*) contents, size * nmemb);
        return size * nmemb;
    }

    // HeaderCallback function to handle the response headers
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
        size_t numbytes = size * nitems;
        std::string* header = static_cast<std::string*>(userdata);
        header->append(buffer, numbytes);
        return numbytes;
    }

private:

    std::string defaultUA{"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36"};


};



#endif // CLIENT_H
