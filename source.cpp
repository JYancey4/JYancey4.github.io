/*
 * Political Party Affiliation Predictor
 * Author: Joshua Yancey
 * Contact: joshua.yancey@snhu.edu
 * Date: 02/05/2024
 * Version: 1.1
 *
 * Program Description:
 * This program collects user responses to a series of political questions and predicts
 * their political party affiliation based on their answers. It utilizes a simple scoring
 * system to tally the user's preferences and associates them with a political party. The
 * responses are stored in an SQLite database for persistent storage and future analysis.
 *
 * Key Features:
 * - Interactive question-answer CLI interface
 * - Mapping of answers to political party preferences
 * - Storage of responses in an SQLite database
 * - Prediction of political party affiliation based on responses
 *
 * Revision History:
 * - Version 1.0 (10/28/2023): Initial creation
 *  - Created 20 questions and answers
 *  - Developed the algorithm for predicting party affiliation.
 *
 * 
 * -Version 1.1 (02/05/2024): Database Change
 *  - Implemented basic CLI for user interaction.
 *  - Created SQLite database integration for response storage.
 *  - Added comments for clarity and maintainability of code.
 *  - Added two additional political parties, Independent and Libertarian
 */

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sqlite3.h> // Optimization: Enables integration with SQLite database for data persistence.

 // Structure to hold user responses
struct UserResponse {
    std::string question; // The political question
    std::string answer;   // User's answer to the question
};

// Global variables for SQLite database
sqlite3* db;        //Optimization: Pointer to SQLite database
char* errMsg = nullptr; //Optimization: Pointer to error message
int rc;             //Optimization: Return code for SQLite operations

// Optimization: Provides a reusable function for executing SQL commands, improving code modularity and error handling.
void executeSQL(const char* sql) {
    // Execute SQL command and store the return code
    rc = sqlite3_exec(db, sql, 0, 0, &errMsg);

    // Check if execution was successful
    if (rc != SQLITE_OK) {
        std::cerr << "SQL Error: " << errMsg << std::endl; // Print error message
        sqlite3_free(errMsg); // Free the memory allocated for the error message
    }
}

// Optimization: Makes sure the database structure is in place for data storage, enhancing data management.
void createTable() {
    // SQL statement to create a table if it does not exist
    const char* sql = "CREATE TABLE IF NOT EXISTS PartyCounts ("
        "Party TEXT PRIMARY KEY, " // 'Party' column for storing political party names (Primary Key)
        "Count INT NOT NULL);";    // 'Count' column for storing the count of user responses

    // Execute the SQL statement to create the table
    executeSQL(sql);
}

// Function to store data into the SQLite database
// Optimization: Function for the tally of political affiliations to the database which enables data analysis.
void storeDataToSQLite(const std::map<std::string, int>& partyCounts) {
    // Iterate over each entry in the partyCounts map
    for (const auto& entry : partyCounts) {
        // Construct an SQL INSERT command to add or update data
        std::string sql = "INSERT INTO PartyCounts (Party, Count) VALUES ('"
            + entry.first + "', " + std::to_string(entry.second) +
            ") ON CONFLICT(Party) DO UPDATE SET Count = Count + " + std::to_string(entry.second) + ";";

        // Execute the constructed SQL command
        executeSQL(sql.c_str());
    }
}

// Optimization: Initializes the database connection, enabling the use of database features throughout the application.
int main() {
    // Open the SQLite database
    rc = sqlite3_open("party_counts.db", &db);
    // Check if the database opens successfully
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 0;
    }

    // Create a table in the database for storing data
    createTable(); // Optimization: Ensures the data structure is ready for data insertion, facilitating data organization and retrieval.


    // List of political questions for the user
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

    // Map for correlating answers with political parties
    std::map<std::string, std::string> answers = {
        {"A", "Democrat"},
        {"B", "Republican"},
        // Optimization: Expands the political spectrum covered by the survey, allowing for a more nuanced analysis of political views.
        {"C", "Independent"},
        {"D", "Libertarian"},
        // Add more answers here.
    };

    std::vector<UserResponse> userResponses; // Optimization: Collects user responses for analysis and database storage.

    // Question loop and response collection mechanism unchanged but essential for the survey's operation.
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
        // Optimization: Utilizes a map for dynamic tallying of responses across an expanded range of political affiliations.
        {"Independent", 0},
        {"Libertarian", 0}
        // Add more parties
    };

    // Response analysis loop, determining the count of each political preference.
    for (const UserResponse& response : userResponses) {
        std::string userAnswer = response.answer;
        if (answers.find(userAnswer) != answers.end()) {
            partyCounts[answers[userAnswer]]++;
        }
    }

    storeDataToSQLite(partyCounts); // Optimization: Persists the collected data for future analysis and insights.

    // Determine the predicted political party.
    std::string predictedParty = std::max_element(partyCounts.begin(), partyCounts.end(),
        [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
            return a.second < b.second;
        })->first; // Optimization: Enhances the determination of political affiliation by considering the broadened political spectrum.

    // Display the predicted political party.
    std::cout << "Predicted Political Party: " << predictedParty << std::endl;

    // Close database
    sqlite3_close(db); // Optimization: Properly closes the database connection, ensuring data integrity and releasing resources.

    return 0;
}
