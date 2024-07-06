#include "network.h"
#include "myexception.h"
#include <utils/errorhandler.h>



void Client::init(int maxCurls) {
    if (initialised) return;

    curl_global_init(CURL_GLOBAL_ALL);
    for (int i = 0; i < maxCurls; i++) {
        CURL *curl = curl_easy_init();
        if (curl)
            curls.push_back (curl);
    }
    initialised = true;
}

void Client::cleanUp() {
    if (!initialised) return;
    for (auto& curl: curls){
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    curls.clear();
    initialised = false;
}

bool Client::isUrlValid(const QString &url) {
    CURL* curl = getCurl();
    if (!curl) {
        throw MyException("Failed to initialize CURL.");
    }
    auto urlString = url.toStdString();
    curl_easy_setopt(curl, CURLOPT_URL, urlString.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);

    CURLcode res = curl_easy_perform(curl);

    returnCurl(curl);
    return res == CURLE_OK;
}

Client::Response Client::get(const QString &url, const QMap<QString, QString> &headers, const QMap<QString, QString> &params) {
    auto fullUrl = url;
    if (!params.isEmpty()) {
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            fullUrl += "&" + it.key() + "=" + it.value();
        }
    }
    auto urlString = url.toStdString();
    return request(GET, fullUrl.toStdString(), headers);
}

Client::Response Client::post(const QString &url, const QMap<QString, QString> &headers, const QMap<QString, QString> &data){
    QString postData;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        postData += it.key() + "=" + it.value() + "&";
    }
    return request(POST, url.toStdString(), headers, postData.toStdString());
}

    Client::Response Client::request(int type, const std::string &url, const QMap<QString, QString> &headersMap, const std::string &postData){
    CURL *curl = getCurl();
    if (!curl) {
        throw MyException("Failed to get curl");
    }

    Response response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    struct curl_slist* curlHeaders = NULL;
    if (!headersMap.isEmpty()) {
        for(auto it = headersMap.begin(); it != headersMap.end(); ++it) {
            std::string header = it.key().toStdString() + ": " + it.value().toStdString();
            curlHeaders = curl_slist_append(curlHeaders, header.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curlHeaders);
    }


    switch (type) {
    case POST:
        // std::string dataString = postData.toStdString();
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
        break;
    }

    // Set the response callback function
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);

    qDebug() << (type == GET ? "[GET]: ":"[POST]") << url;

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK){
        throw MyException(QString("curl_easy_perform() failed: ") + curl_easy_strerror(res));
    }

    // Get the response code
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.code);

    // Clean up
    if (curlHeaders)
        curl_slist_free_all(curlHeaders);

    returnCurl(curl);
    return response;
}

size_t Client::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalBytes(size * nmemb);

    // Cast userp to QString pointer
    QString* str = static_cast<QString*>(userp);

    // Convert the incoming data to QString assuming it's UTF-8 encoded
    // If your data is in another encoding, adjust this part accordingly
    QString newData = QString::fromUtf8(static_cast<char*>(contents), static_cast<int>(totalBytes));

    // Append the new data to the provided QString
    str->append(newData);

    // Return the number of bytes taken care of
    return totalBytes;
}

size_t Client::HeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata) {
    // Calculate the total size of the incoming header data
    size_t numbytes = size * nitems;
    // Cast userdata to QString pointer
    QString* header = static_cast<QString*>(userdata);
    // Convert the incoming header data to QString assuming it's UTF-8 encoded
    QString newData = QString::fromUtf8(buffer, static_cast<int>(numbytes));
    // Append the new header data to the provided QString
    header->append(newData);
    // Return the number of bytes processed
    return numbytes;
}
