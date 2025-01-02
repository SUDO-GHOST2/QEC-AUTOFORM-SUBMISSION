#include <iostream>
#include <curl/curl.h>
#include <string>
#include <regex>
#include <vector>
#include <fstream>

// Callback to write response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Extract hidden field value
std::string extractField(const std::string& html, const std::string& fieldName) {
    std::regex fieldRegex("name=\"" + fieldName + "\".*?value=\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(html, match, fieldRegex)) {
        return match[1];
    }
    return "";
}

// Extract dropdown options from HTML
std::vector<std::string> extractDropdownOptions(const std::string& html, const std::string& dropdownId) {
    std::vector<std::string> options;
    std::string dropdownPattern = "<select[^>]*id=\\\"" + dropdownId + "\\\"[^>]*>([\\s\\S]*?)</select>";
    std::regex dropdownRegex(dropdownPattern);
    std::smatch dropdownMatch;

    if (std::regex_search(html, dropdownMatch, dropdownRegex)) {
        std::string dropdownHtml = dropdownMatch[1].str();
        std::regex optionRegex("<option value=\\\"(.*?)\\\">(.*?)</option>");
        auto optionsBegin = std::sregex_iterator(dropdownHtml.begin(), dropdownHtml.end(), optionRegex);
        auto optionsEnd = std::sregex_iterator();

        for (auto it = optionsBegin; it != optionsEnd; ++it) {
            std::cout << "Found option: value=" << (*it)[1].str() << " text=" << (*it)[2].str() << std::endl;
            options.push_back((*it)[1].str());
        }
    }
    else {
        std::cerr << "Dropdown with ID '" << dropdownId << "' not found in HTML." << std::endl;
    }

    return options;
}

// Submit evaluation for a teacher
void submitEvaluation(CURL* curl, const std::string& teacherValue, const std::string& viewState, const std::string& viewStateGenerator, const std::string& eventValidation) {
    std::string postData =
        "ctl00$ContentPlaceHolder2$ddlTeacher=" + teacherValue +
        "&ctl00$ContentPlaceHolder2$q1=A"
        "&ctl00$ContentPlaceHolder2$q2=A"
        "&ctl00$ContentPlaceHolder2$q3=A"
        "&ctl00$ContentPlaceHolder2$q4=A"
        "&ctl00$ContentPlaceHolder2$q5=A"
        "&ctl00$ContentPlaceHolder2$q6=A"
        "&ctl00$ContentPlaceHolder2$q7=A"
        "&ctl00$ContentPlaceHolder2$q8=A"
        "&ctl00$ContentPlaceHolder2$q9=A"
        "&ctl00$ContentPlaceHolder2$q10=A"
        "&ctl00$ContentPlaceHolder2$q11=A"
        "&ctl00$ContentPlaceHolder2$q12=A"
        "&ctl00$ContentPlaceHolder2$q13=A"
        "&ctl00$ContentPlaceHolder2$q14=A"
        "&ctl00$ContentPlaceHolder2$q15=A"
        "&ctl00$ContentPlaceHolder2$q16=A"
        "&ctl00$ContentPlaceHolder2$txt_comments_instructor=Good+teaching"
        "&ctl00$ContentPlaceHolder2$txt_comments_course=Informative+course"
        "&ctl00$ContentPlaceHolder2$btnSave=Submit"
        "&__VIEWSTATE=" + curl_easy_escape(curl, viewState.c_str(), viewState.size()) +
        "&__VIEWSTATEGENERATOR=" + curl_easy_escape(curl, viewStateGenerator.c_str(), viewStateGenerator.size()) +
        "&__EVENTVALIDATION=" + curl_easy_escape(curl, eventValidation.c_str(), eventValidation.size());

    curl_easy_setopt(curl, CURLOPT_URL, "https://portals.au.edu.pk/qec/p10.aspx");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_REFERER, "https://portals.au.edu.pk/qec/p10.aspx");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Error submitting evaluation for teacher: " << teacherValue
            << ". Error: " << curl_easy_strerror(res) << std::endl;
    }
    else {
        std::ofstream responseFile("response_" + teacherValue + ".html");
        responseFile << response;
        responseFile.close();
        std::cout << "Evaluation submitted successfully for teacher: " << teacherValue << ". Response saved." << std::endl;
    }
}

int main() {


    // Display the title
    std::cout << "\033[1;36m\033[4mQEC TEACHER EVALUATION FORM AUTO SUBMISSION\033[0m\n" << std::endl;
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();

    if (!curl) {
        std::cerr << "Failed to initialize CURL." << std::endl;
        return 1;
    }

    // Session cookies
    std::string cookies = "cookies.txt";

    // Step 1: Access Teacher Evaluation Form
    std::string teacherFormHtml;
    curl_easy_setopt(curl, CURLOPT_URL, "https://portals.au.edu.pk/qec/p10.aspx");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &teacherFormHtml);
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookies.c_str());
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookies.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Error accessing Teacher Evaluation Form page: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return 1;
    }

    // Extract hidden fields
    std::string viewState = extractField(teacherFormHtml, "__VIEWSTATE");
    std::string viewStateGenerator = extractField(teacherFormHtml, "__VIEWSTATEGENERATOR");
    std::string eventValidation = extractField(teacherFormHtml, "__EVENTVALIDATION");

    if (viewState.empty() || viewStateGenerator.empty() || eventValidation.empty()) {
        std::cerr << "Failed to extract hidden fields!" << std::endl;
        return 1;
    }

    // Extract teacher options
    std::vector<std::string> teachers = extractDropdownOptions(teacherFormHtml, "ctl00_ContentPlaceHolder2_ddlTeacher");

    if (teachers.empty()) {
        std::cerr << "No teachers found in the dropdown menu!" << std::endl;
        curl_easy_cleanup(curl);
        return 1;
    }

    // Submit evaluation for each teacher
    for (const auto& teacher : teachers) {
        submitEvaluation(curl, teacher, viewState, viewStateGenerator, eventValidation);
    }

    curl_easy_cleanup(curl);
    return 0;
}
