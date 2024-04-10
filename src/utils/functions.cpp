#include "functions.h"


QString Functions::findBetween(const QString &str, const QString &a, const QString &b) {
    int start = str.indexOf(a);
    if (start != -1) {
        start += a.length(); // Move start to the end of the first string
        int end = str.indexOf(b, start);
        if (end != -1) {
            return str.mid(start, end - start);
        }
    }
    return "";
}

QString Functions::substringAfter(const QString &str, const QString &delimiter) {
    int pos = str.indexOf(delimiter);
    if (pos != -1) {
        return str.mid(pos + delimiter.length());
    }
    return "";
}
QString Functions::substringBefore(const QString &str, const QString &delimiter) {
    int pos = str.indexOf(delimiter);
    if (pos != -1) {
        return str.left(pos);
    }
    return str;
}



QString Functions::getHostFromUrl(const QString &url) {
    static QRegularExpression regex(R"((?:https?://)?(?:www\.)?([^:/\s]+))");
    QRegularExpressionMatch match = regex.match(url);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return "";
}

// std::string Functions::reverseString(const std::string &str){
//     std::string result;
//     result.reserve(str.length()); // Reserve memory to avoid reallocation

//     for (auto it = str.rbegin(); it != str.rend(); ++it){
//         result.push_back(*it);
//     }

//     return result;
// }

// std::string Functions::rc4(const std::string &key, const std::string &input){
//     std::vector<int> s(256);
//     std::string output = "";
//     int j = 0;
//     for (int i = 0; i < 256; i++){
//         s[i] = i;
//     }
//     for (int i = 0; i < 256; i++){
//         j = (j + s[i] + key[i % key.size()]) % 256;
//         std::swap(s[i], s[j]);
//     }
//     int i = 0;
//     j = 0;
//     for (int k = 0; k < input.size(); k++){
//         i = (i + 1) % 256;
//         j = (j + s[i]) % 256;
//         std::swap(s[i], s[j]);
//         int idx = (s[i] + s[j]) % 256;
//         output += (char)(input[k] ^ s[idx]);
//     }
//     return output;
// }

// std::string Functions::rot13(std::string n){
//     for (char& c : n){
//         if (std::isalpha(c)){
//             if (std::isupper(c)){
//                 c = (c - 'A' + 13) % 26 + 'A';
//             } else {
//                 c = (c - 'a' + 13) % 26 + 'a';
//             }
//         }
//     }
//     return n;
// }

// std::string Functions::base64Encode(const std::string &t, const std::string &baseTable)  {
//     auto a = [&](int t) -> char {
//         if (t >= 0 && t < 64){
//             return baseTable[t];
//         }
//         return '\0';
//     };

//     auto n = t.length();
//     std::string u;
//     for (size_t r = 0; r < n; r++){
//         if (static_cast<unsigned char>(t[r]) > 255){
//             return "";
//         }
//     }
//     for (size_t r = 0; r < n; r += 3){
//         int e[] = { 0, 0, 0, 0 };
//         e[0] = static_cast<unsigned char>(t[r]) >> 2;
//         e[1] = ((static_cast<unsigned char>(t[r]) & 3) << 4);
//         if (n > r + 1){
//             e[1] |= (static_cast<unsigned char>(t[r + 1]) >> 4);
//             e[2] = ((static_cast<unsigned char>(t[r + 1]) & 15) << 2);
//         }
//         if (n > r + 2){
//             e[2] |= (static_cast<unsigned char>(t[r + 2]) >> 6);
//             e[3] = (static_cast<unsigned char>(t[r + 2]) & 63);
//         }
//         for (size_t o = 0; o < sizeof(e) / sizeof(e[0]); o++){
//             u += (e[o] != 0) ? a(e[o]) : '=';
//         }
//     }
//     return u;
// }

// std::string Functions::base64Decode(const std::string &input, const std::string &baseTable){
//     std::string output;
//     std::vector<unsigned char> temp;
//     temp.reserve(input.size() / 4 * 3);

//     int bits = 0, acc = 0;
//     for (char c : input){
//         int index = baseTable.find (c);
//         if (index == std::string::npos){
//             index = 0 ;
//         }

//         acc = (acc << 6) | index;
//         bits += 6;
//         if (bits >= 8){
//             bits -= 8;
//             temp.push_back(static_cast<unsigned char>((acc >> bits) & 0xFF));
//         }
//     }
//     output.resize(temp.size());
//     std::copy(temp.begin(), temp.end(), output.begin());
//     return output;
// }



// std::vector<std::string> Functions::split(const std::string &str, char delimiter){
//     std::vector<std::string> result;
//     std::istringstream iss(str);
//     std::string token;
//     while (std::getline(iss, token, delimiter)){
//         result.push_back(std::move(token));
//     }
//     return result;
// }

// std::vector<std::string> Functions::split(const std::string &str, const std::string &delimiter){
//     std::vector<std::string> result;
//     std::string::size_type start = 0;
//     std::string::size_type end = str.find(delimiter);
//     while (end != std::string::npos){
//         result.push_back(str.substr(start, end - start));
//         start = end + delimiter.length();
//         end = str.find(delimiter, start);
//     }
//     result.push_back(str.substr(start));
//     return result;
// }


