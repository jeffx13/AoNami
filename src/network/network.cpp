#include "network.h"
#include "myexception.h"
#include <utils/errorhandler.h>



void Client::setDefaultOpts(CURL *curl) {
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &WriteCallback);
        // curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
    }
}


bool Client::isOk(const QString &url, long timeout) {
    auto m_curl = curl_easy_init();
    if (!m_curl) {
        throw MyException("Failed to initialize CURL.");
    }
    setDefaultOpts(m_curl);
    auto urlString = url.toStdString();
    curl_easy_setopt(m_curl, CURLOPT_URL, urlString.c_str());
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);
    CURLcode res = curl_easy_perform(m_curl);
    if (res != CURLE_OK){
        return false;
    }
    long code;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(m_curl);
    return code == 200;
}

Client::Response Client::request(int type, const std::string &url, const QMap<QString, QString> &headersMap, const std::string &postData){
    auto m_curl = curl_easy_init();
    if (!m_curl) {
        throw MyException("Failed to get curl");
    }
    setDefaultOpts(m_curl);


    if (m_isCancelled) {
        if (*m_isCancelled)
            throw MyException("Request canceled!");
        curl_easy_setopt(m_curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(m_curl, CURLOPT_XFERINFODATA, this);
        curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
    }


    Response response;

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());

    struct curl_slist* curlHeaders = NULL;
    if (!headersMap.isEmpty()) {
        for(auto it = headersMap.begin(); it != headersMap.end(); ++it) {
            std::string header = it.key().toStdString() + ": " + it.value().toStdString();
            curlHeaders = curl_slist_append(curlHeaders, header.c_str());
        }
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, curlHeaders);
    }


    switch (type) {
    case POST:
        // std::string dataString = postData.toStdString();
        curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, postData.c_str());
        break;
    }

    // Set the response callback function
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &response.headers);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response.body);

    qDebug() << (type == GET ? "[GET]: ":"[POST]") << url;

    //qDebug() << m_curl;
    // Perform the request
    CURLcode res = curl_easy_perform(m_curl);

    // Check for errors
    if (res != CURLE_OK){
        throw MyException(QString("curl_easy_perform() failed: ") + curl_easy_strerror(res));
    }

    // Get the response code
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response.code);

    // Clean up
    if (curlHeaders)
        curl_slist_free_all(curlHeaders);

    curl_easy_cleanup(m_curl);

    return response;
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




