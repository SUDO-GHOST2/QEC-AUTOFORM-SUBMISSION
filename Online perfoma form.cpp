
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

// Submit feedback for a subject
void submitFeedback(CURL* curl, const std::string& subjectValue, const std::string& viewState, const std::string& viewStateGenerator, const std::string& eventValidation) {
    std::string postData =
        "ctl00$ContentPlaceHolder1$cmb_courses=" + subjectValue +
        "&ctl00$ContentPlaceHolder1$q1=A" // Example responses
        "&ctl00$ContentPlaceHolder1$q2=A"
        "&ctl00$ContentPlaceHolder1$q3=A"
        "&ctl00$ContentPlaceHolder1$q4=A"
        "&ctl00$ContentPlaceHolder1$q5=A"
        "&ctl00$ContentPlaceHolder1$q6=A"
        "&ctl00$ContentPlaceHolder1$q7=A"
        "&ctl00$ContentPlaceHolder1$txt_comments=Great+learning+experience"
        "&ctl00$ContentPlaceHolder1$btnSave=Submit"
        "&__VIEWSTATE=" + curl_easy_escape(curl, viewState.c_str(), viewState.size()) +
        "&__VIEWSTATEGENERATOR=" + curl_easy_escape(curl, viewStateGenerator.c_str(), viewStateGenerator.size()) +
        "&__EVENTVALIDATION=" + curl_easy_escape(curl, eventValidation.c_str(), eventValidation.size());

    curl_easy_setopt(curl, CURLOPT_URL, "https://portals.au.edu.pk/qec/p10a_learning_online_form.aspx");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_REFERER, "https://portals.au.edu.pk/qec/p10a_learning_online_form.aspx");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Error submitting feedback for subject: " << subjectValue
            << ". Error: " << curl_easy_strerror(res) << std::endl;
    }
    else {
        std::ofstream responseFile("response_" + subjectValue + ".html");
        responseFile << response;
        responseFile.close();
        std::cout << "Feedback submitted successfully for subject: " << subjectValue << ". Response saved." << std::endl;
    }
}

int main() {

    // Display the title
    std::cout << "\033[1;36m\033[4mQEC ONLINE EVALUATION FORM AUTO SUBMISSION\033[0m\n" << std::endl;
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();

    if (!curl) {
        std::cerr << "Failed to initialize CURL." << std::endl;
        return 1;
    }

    // Session cookies
    std::string cookies = "cookies.txt";

    // Step 1: Access Online Learning Feedback Form
    std::string feedbackFormHtml;
    curl_easy_setopt(curl, CURLOPT_URL, "https://portals.au.edu.pk/qec/p10a_learning_online_form.aspx");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &feedbackFormHtml);
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookies.c_str());
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookies.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Error accessing Online Learning Feedback Form page: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return 1;
    }

    // Save the feedback form HTML for debugging
    std::ofstream feedbackFormFile("online_feedback_form.html");
    feedbackFormFile << feedbackFormHtml;
    feedbackFormFile.close();

    // Extract hidden fields
    std::string viewState = extractField(feedbackFormHtml, "__VIEWSTATE");
    std::string viewStateGenerator = extractField(feedbackFormHtml, "__VIEWSTATEGENERATOR");
    std::string eventValidation = extractField(feedbackFormHtml, "__EVENTVALIDATION");

    if (viewState.empty() || viewStateGenerator.empty() || eventValidation.empty()) {
        std::cerr << "Failed to extract hidden fields!" << std::endl;
        return 1;
    }

    // Extract subject options
    std::vector<std::string> subjects = extractDropdownOptions(feedbackFormHtml, "ctl00_ContentPlaceHolder1_cmb_courses");

    if (subjects.empty()) {
        std::cerr << "No subjects found in the dropdown menu!" << std::endl;
        curl_easy_cleanup(curl);
        return 1;
    }

    // Submit feedback for each subject
    for (const auto& subject : subjects) {
        submitFeedback(curl, subject, viewState, viewStateGenerator, eventValidation);
    }

    curl_easy_cleanup(curl);
    return 0;
}
