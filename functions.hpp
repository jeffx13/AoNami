#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <algorithm>
#include <array>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>

#include <QString>
#include <QDebug>

#include <cryptopp/base64.h>
#include <cryptopp/des.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/hex.h>


#define QS(str) QString::fromStdString(str)

// Convert std::string to QString
inline QString toQString(const std::string& str)
{
    return QString::fromStdString(str);
}

// Convert QString to QString (identity function)
inline QString toQString(const QString& str)
{
    return str;
}
inline QString toQString(const char* str)
{
    return str;
}
// Variadic LOG function that accepts both std::string and QString arguments
template <typename... Args>
inline void LOG(Args... args)
{
    std::ostringstream stream;
    (stream << ... << toQString(args).toStdString());
    qDebug() << QString::fromStdString(stream.str());
}


class Functions{
public:
    static void httpsIfy(std::string& text) {
        if (text.substr(0, 2) == "//") {
            text = "https:" + text;

        }
    }
    static std::string urlDecode(const std::string &str) {
        std::string ret;
        char ch;
        int i, ii;
        for (i=0; i<str.length(); i++) {
            if (str[i]=='%') {
                sscanf(str.substr(i+1,2).c_str(), "%x", &ii);
                ch=static_cast<char>(ii);
                ret+=ch;
                i=i+2;
            } else {
                ret+=str[i];
            }
        }
        return (ret);
    }

    static std::string urlEncode(const std::string &str) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (std::string::const_iterator i = str.begin(), n = str.end(); i != n; ++i) {
            std::string::value_type c = (*i);

            // Keep alphanumeric and other accepted characters intact
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
                continue;
            }

            // Any other characters are percent-encoded
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char) c);
            escaped << std::nouppercase;
        }

        return escaped.str();
    }

    static std::string findBetween(const std::string& str, const std::string& a, const std::string& b) {
        auto start = str.find(a);
        if (start != std::string::npos) {
            auto end = str.find(b, start + a.length());
            if (end != std::string::npos) {
                return str.substr(start + a.length(), end - start - a.length());
            }
        }
        return "";
    }

    static std::string substringAfter(const std::string& str, const std::string& delimiter) {
        auto pos = str.find(delimiter);
        if (pos != std::string::npos) {
            return str.substr(pos + delimiter.length());
        }
        return "";
    }

    static std::string substringBefore(const std::string& str, const std::string& delimiter) {
        auto pos = str.find(delimiter);
        if (pos != std::string::npos) {
            return str.substr(0, pos);
        }
        return str;
    }

    static void replaceAll(std::string& s, const std::string& search, const std::string& replace ){

        for( size_t pos = 0; ; pos += replace.length() ) {
            // Locate the substring to replace
            pos = s.find( search, pos );
            if( pos == std::string::npos ) break;
            // Replace by erasing and inserting
            s.erase( pos, search.length() );
            s.insert( pos, replace );
        }
    }

    static std::string reverseString(const std::string& str) {
        std::string result;
        result.reserve(str.length()); // Reserve memory to avoid reallocation

        for (auto it = str.rbegin(); it != str.rend(); ++it) {
            result.push_back(*it);
        }

        return result;
    }

    static std::string rc4(const std::string& key, const std::string& input) {
        std::vector<int> s(256);
        std::string output = "";
        int j = 0;
        for (int i = 0; i < 256; i++) {
            s[i] = i;
        }
        for (int i = 0; i < 256; i++) {
            j = (j + s[i] + key[i % key.size()]) % 256;
            std::swap(s[i], s[j]);
        }
        int i = 0;
        j = 0;
        for (int k = 0; k < input.size(); k++) {
            i = (i + 1) % 256;
            j = (j + s[i]) % 256;
            std::swap(s[i], s[j]);
            int idx = (s[i] + s[j]) % 256;
            output += (char)(input[k] ^ s[idx]);
        }
        return output;
    }

    static std::string rot13(std::string n) {
        for (char& c : n) {
            if (std::isalpha(c)) {
                if (std::isupper(c)) {
                    c = (c - 'A' + 13) % 26 + 'A';
                } else {
                    c = (c - 'a' + 13) % 26 + 'a';
                }
            }
        }
        return n;
    }

    static std::string base64Encode(const std::string& t,const std::string& baseTable="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")  {
        auto a = [&](int t) -> char {
            if (t >= 0 && t < 64) {
                return baseTable[t];
            }
            return '\0';
        };

        auto n = t.length();
        std::string u;
        for (size_t r = 0; r < n; r++) {
            if (static_cast<unsigned char>(t[r]) > 255) {
                return "";
            }
        }
        for (size_t r = 0; r < n; r += 3) {
            int e[] = { 0, 0, 0, 0 };
            e[0] = static_cast<unsigned char>(t[r]) >> 2;
            e[1] = ((static_cast<unsigned char>(t[r]) & 3) << 4);
            if (n > r + 1) {
                e[1] |= (static_cast<unsigned char>(t[r + 1]) >> 4);
                e[2] = ((static_cast<unsigned char>(t[r + 1]) & 15) << 2);
            }
            if (n > r + 2) {
                e[2] |= (static_cast<unsigned char>(t[r + 2]) >> 6);
                e[3] = (static_cast<unsigned char>(t[r + 2]) & 63);
            }
            for (size_t o = 0; o < sizeof(e) / sizeof(e[0]); o++) {
                u += (e[o] != 0) ? a(e[o]) : '=';
            }
        }
        return u;
    }

    static std::string base64Decode(const std::string& input,const std::string& baseTable =  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/") {
        std::string output;
        std::vector<unsigned char> temp;
        temp.reserve(input.size() / 4 * 3);

        int bits = 0, acc = 0;
        for (char c : input) {
            int index = baseTable.find (c);
            if (index == std::string::npos) {
                index = 0 ;
            }

            acc = (acc << 6) | index;
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                temp.push_back(static_cast<unsigned char>((acc >> bits) & 0xFF));
            }
        }
        output.resize(temp.size());
        std::copy(temp.begin(), temp.end(), output.begin());
        return output;
    }

    static bool containsSubstring(const std::string& str, const std::string& substr) {
        return str.find(substr) != std::string::npos;
    }
    static std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> result;
        std::istringstream iss(str);
        std::string token;
        while (std::getline(iss, token, delimiter)) {
            result.push_back(std::move(token));
        }
        return result;
    }

    static std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
        std::vector<std::string> result;
        std::string::size_type start = 0;
        std::string::size_type end = str.find(delimiter);
        while (end != std::string::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + delimiter.length();
            end = str.find(delimiter, start);
        }
        result.push_back(str.substr(start));
        return result;
    }

#include <locale>
    static std::string MD5(const std::string& str){
        CryptoPP::Weak1::MD5 hash;
        unsigned char digest[CryptoPP::Weak1::MD5::DIGESTSIZE];
        hash.CalculateDigest(digest, reinterpret_cast<const unsigned char*>(str.data()), str.length());
        std::string output;
        CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(output), false);
        encoder.Put(digest, sizeof(digest));
        encoder.MessageEnd();
        return output;
    }
    Functions() = delete;
};


#endif // FUNCTIONS_H
