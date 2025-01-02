#include <iostream>
#include <curl/curl.h>
#include <string>
#include <regex>
#include <vector>

// Callback to write response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Helper function to extract form fields from HTML
std::string extractField(const std::string& html, const std::string& fieldName) {
    std::regex fieldRegex("name=\"" + fieldName + "\".*?value=\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(html, match, fieldRegex)) {
        return match[1];
    }
    return "";
}

// Helper function to extract subject list
std::vector<std::string> extractSubjects(const std::string& html) {
    std::regex optionRegex("<option value=\"(.*?)\">");
    std::smatch match;
    std::vector<std::string> subjects;

    auto it = html.begin();
    auto end = html.end();
    while (std::regex_search(it, end, match, optionRegex)) {
        subjects.push_back(match[1]);
        it = match[0].second; // Move the iterator forward
    }

    return subjects;
}

int main() {
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();

    if (!curl) {
        std::cerr << "Failed to initialize CURL." << std::endl;
        return 1;
    }

    // Session cookies
    std::string cookies = "cookies.txt";

    // Step 1: Login to the main portal
    std::string loginPageHtml;
    curl_easy_setopt(curl, CURLOPT_URL, "https://portals.au.edu.pk/students/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &loginPageHtml);
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookies.c_str());
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookies.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "Error fetching login page: " << curl_easy_strerror(res) << std::endl;
        return 1;
    }

    // Extract hidden fields from login page
    std::string viewState = extractField(loginPageHtml, "__VIEWSTATE");
    std::string viewStateGenerator = extractField(loginPageHtml, "__VIEWSTATEGENERATOR");
    std::string eventValidation = extractField(loginPageHtml, "__EVENTVALIDATION");

    if (viewState.empty() || viewStateGenerator.empty() || eventValidation.empty()) {
        std::cerr << "Failed to extract hidden fields from login page." << std::endl;
        return 1;
    }

    // Login credentials for the main page

    // Display the title
    std::cout << "\033[1;36m\033[4mQEC STUDENT EVALUATION FORM AUTO SUBMISSION\033[0m\n" << std::endl;

    // Get user input for credentials
    std::string user, pass;
    std::cout << "\033[1;33mEnter your username: \033[0m";
    std::cin >> user;

    std::cout << "\033[1;33mEnter your password: \033[0m";
    std::cin >> pass;
    std::string username = user;  // Replace with your username
    std::string password = pass;  // Replace with your password

    std::string postData =
        "TextBox1=" + username +
        "&TextBox2=" + password +
        "&__VIEWSTATE=" + curl_easy_escape(curl, viewState.c_str(), viewState.size()) +
        "&__VIEWSTATEGENERATOR=" + curl_easy_escape(curl, viewStateGenerator.c_str(), viewStateGenerator.size()) +
        "&__EVENTVALIDATION=" + curl_easy_escape(curl, eventValidation.c_str(), eventValidation.size()) +
        "&button1=Login";

    std::string loginResponse;
    curl_easy_setopt(curl, CURLOPT_URL, "https://portals.au.edu.pk/students/");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &loginResponse);
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "Error during main portal login: " << curl_easy_strerror(res) << std::endl;
        return 1;
    }

    if (loginResponse.find("Main.aspx") != std::string::npos) {
        std::cout << "Main portal login successful!" << std::endl;

        // Step 2: Navigate to the QEC Feedback form
        std::string qecLoginHtml;
        curl_easy_setopt(curl, CURLOPT_URL, "https://portals.au.edu.pk/qec/login.aspx");
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &qecLoginHtml);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "Error accessing QEC feedback login page: " << curl_easy_strerror(res) << std::endl;
            return 1;
        }

        // Extract hidden fields for QEC login
        std::string qecViewState = extractField(qecLoginHtml, "__VIEWSTATE");
        std::string qecViewStateGenerator = extractField(qecLoginHtml, "__VIEWSTATEGENERATOR");
        std::string qecEventValidation = extractField(qecLoginHtml, "__EVENTVALIDATION");

        std::string qecPostData =
            "ctl00$ContentPlaceHolder2$ddlcampus=Islamabad"
            "&ctl00$ContentPlaceHolder2$ddlUserType=Student/Alumni"
            "&ctl00$ContentPlaceHolder2$txt_regid=" + username +
            "&ctl00$ContentPlaceHolder2$txt_password=" + password +
            "&__VIEWSTATE=" + curl_easy_escape(curl, qecViewState.c_str(), qecViewState.size()) +
            "&__VIEWSTATEGENERATOR=" + curl_easy_escape(curl, qecViewStateGenerator.c_str(), qecViewStateGenerator.size()) +
            "&__EVENTVALIDATION=" + curl_easy_escape(curl, qecEventValidation.c_str(), qecEventValidation.size()) +
            "&ctl00$ContentPlaceHolder2$btnAccountlogin=Login";

        std::string qecLoginResponse;
        curl_easy_setopt(curl, CURLOPT_URL, "https://portals.au.edu.pk/qec/login.aspx");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, qecPostData.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &qecLoginResponse);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            std::cerr << "Error during QEC login: " << curl_easy_strerror(res) << std::endl;
            return 1;
        }

        if (qecLoginResponse.find("student-perfomas.aspx") != std::string::npos) {
            std::cout << "Successfully logged into QEC Feedback form!" << std::endl;

            // Step 3: Access Student Course Evaluation Questionnaire
            std::string questionnaireHtml;
            curl_easy_setopt(curl, CURLOPT_URL, "https://portals.au.edu.pk/qec/p1.aspx");
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &questionnaireHtml);
            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::cerr << "Error accessing Questionnaire page: " << curl_easy_strerror(res) << std::endl;
                return 1;
            }

            std::cout << "Questionnaire page accessed successfully!" << std::endl;

            // Extract subjects from the dropdown menu
            std::vector<std::string> subjects = extractSubjects(questionnaireHtml);

            if (subjects.empty()) {
                std::cerr << "No subjects found in the dropdown menu!" << std::endl;
                return 1;
            }

            for (const auto& subject : subjects) {
                std::cout << "Submitting questionnaire for subject: " << subject << std::endl;

                // Step 4: Submit form for each subject
                std::string questionnaireViewState = extractField(questionnaireHtml, "__VIEWSTATE");
                std::string questionnaireViewStateGenerator = extractField(questionnaireHtml, "__VIEWSTATEGENERATOR");
                std::string questionnaireEventValidation = extractField(questionnaireHtml, "__EVENTVALIDATION");

                std::string questionnairePostData =
                    "ctl00$ContentPlaceHolder2$cmb_courses=" + subject +
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
                    "&ctl00$ContentPlaceHolder2$q12=D"
                    "&ctl00$ContentPlaceHolder2$btnSave=Submit+Proforma"
                    "&__VIEWSTATE=" + curl_easy_escape(curl, questionnaireViewState.c_str(), questionnaireViewState.size()) +
                    "&__VIEWSTATEGENERATOR=" + curl_easy_escape(curl, questionnaireViewStateGenerator.c_str(), questionnaireViewStateGenerator.size()) +
                    "&__EVENTVALIDATION=" + curl_easy_escape(curl, questionnaireEventValidation.c_str(), questionnaireEventValidation.size());

                std::string submissionResponse;
                curl_easy_setopt(curl, CURLOPT_URL, "https://portals.au.edu.pk/qec/p1.aspx");
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, questionnairePostData.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &submissionResponse);
                res = curl_easy_perform(curl);

                if (res != CURLE_OK) {
                    std::cerr << "Error submitting questionnaire for subject: " << subject
                        << ". Error: " << curl_easy_strerror(res) << std::endl;
                    continue;
                }

                std::cout << "Questionnaire for subject " << subject << " submitted successfully!" << std::endl;
            }
        }
        else {
            std::cerr << "QEC login failed. Check credentials or form structure." << std::endl;
        }
    }
    else {
        std::cerr << "Main portal login failed. Check credentials or form structure." << std::endl;
    }

    curl_easy_cleanup(curl);

    return 0;
}