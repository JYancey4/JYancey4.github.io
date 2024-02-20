#include <iostream>
#include <vector>
#include <map>
#include <fstream>

struct UserResponse {
    std::string question;
    std::string answer;
};

void storeDataToFile(const std::map<std::string, int>& partyCounts) {
    for (const auto& entry : partyCounts) {
        std::ofstream outFile(entry.first + ".txt", std::ios::app);
        outFile << entry.second << std::endl;
        outFile.close();
    }
}

int main() {
    std::vector<std::string> questions = {
        "What should the government do to help the poor?\nA. Make it easier to apply for assistance\nB. Allow parents to use education funds for charter schools\nC. Create welfare to work programs\nD. Nothing",
        "What is your stance on healthcare?\nA. Universal healthcare for all\nB. Market-driven healthcare system\nC. A mix of public and private healthcare\nD. No government involvement in healthcare",
        "What is your view on taxation?\nA. Progressive taxation\nB. Flat tax rate for everyone\nC. Tax cuts for businesses\nD. No income tax",
        "How should the government approach environmental regulations?\nA. Strict environmental regulations\nB. Minimal government intervention\nC. Support for renewable energy\nD. No government involvement in environmental matters",
        "What is your view on gun control?\nA. Strict gun control laws\nB. No restrictions on gun ownership\nC. Background checks and certain restrictions\nD. Only restrictions for mentally unstable individuals",
        "What is your stance on immigration?\nA. Open borders and amnesty for undocumented immigrants\nB. Strict immigration policies and border security\nC. A path to citizenship for those already here\nD. Deport all undocumented immigrants",
        "How should the government handle education?\nA. Fully funded public education\nB. Privatize education and support charter schools\nC. Increase funding for schools in impoverished areas\nD. No government involvement in education",
        "What's your opinion on military spending?\nA. Reduce the military budget\nB. Increase the military budget\nC. Maintain the current budget\nD. Prioritize veterans' benefits over new spending",
        "What's your stance on women's reproductive rights?\nA. Support abortion rights without restrictions\nB. Oppose all forms of abortion\nC. Allow abortion in certain circumstances\nD. Leave the decision to states",
        "How should the government handle the economy?\nA. Increase regulation and oversight\nB. Reduce government intervention and regulations\nC. Implement policies favoring the middle class\nD. Promote a free-market system"
    };

    std::map<std::string, std::string> answers = {
        {"A", "Democrat"},
        {"B", "Republican"},
        {"C", "Independent"},
        {"D", "Libertarian"},
        // Add more answers here.
    };

    std::vector<UserResponse> userResponses;

    for (const std::string& question : questions) {
        std::cout << question << std::endl;
        std::string userAnswer;
        std::cin >> userAnswer;
        userResponses.push_back({ question, userAnswer });
    }

    // Count the user's answers for each party.
    std::map<std::string, int> partyCounts = {
        {"Democrat", 0},
        {"Republican", 0},
        {"Independent", 0},
        {"Libertarian", 0}
        // Add more parties here if needed.
    };

    for (const UserResponse& response : userResponses) {
        std::string userAnswer = response.answer;
        if (answers.find(userAnswer) != answers.end()) {
            partyCounts[answers[userAnswer]]++;
        }
    }

    storeDataToFile(partyCounts);

    // Determine the predicted political party.
    std::string predictedParty = std::max_element(partyCounts.begin(), partyCounts.end(),
        [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
            return a.second < b.second;
        })->first;

    // Display the predicted political party.
    std::cout << "Predicted Political Party: " << predictedParty << std::endl;

    return 0;
}





