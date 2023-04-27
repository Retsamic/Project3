#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <filesystem>
#include "csv.h"


using namespace std;
namespace fs = std::filesystem;

struct Video {
    string video_id;
    string country;
    string trending_date;
    string title;
    string channel_title;
    int category_id;
    string publish_time;
    string tags;
    string temp_comments_disabled;
    string temp_ratings_disabled;
    string temp_video_error_or_removed;
    int views;
    int likes;
    int dislikes;
    int comment_count;
    string thumbnail_link;
    int comments_disabled;
    int ratings_disabled;
    int video_error_or_removed;
    string description;
};

//calculates the engagement rate of a video
double engagementRate(const Video& video) {
    double likeWeight = 1.0;
    double dislikeWeight = 0.5;
    double commentWeight = 1.5;

    double totalLikes = likeWeight * video.likes;
    double totalDislikes = dislikeWeight * video.dislikes;
    double totalComments = commentWeight * video.comment_count;

    return (totalLikes + totalDislikes + totalComments) / video.views;
}
//validates and converts the user input for countries
vector<string> validateAndConvertCountryInput(const string& input, const set<string>& valid_countries) {
    string uppercaseInput = input;
    transform(input.begin(), input.end(), uppercaseInput.begin(), ::toupper);
    stringstream ss(uppercaseInput);
    string token;
    vector<string> result;

    while (getline(ss, token, ',')) {
        token.erase(remove(token.begin(), token.end(), ' '), token.end());
        if (valid_countries.count(token) > 0 || token == "ALL") {
            result.push_back(token);
        }
        else {
            return {}; // Invalid input, return an empty vector
        }
    }

    return result;
}
//splits the video tags into a vector of tags
vector<string> splitTags(const string& tags) {
    stringstream ss(tags);
    string tag;
    vector<string> tagList;

    while (getline(ss, tag, '|')) {
        tagList.push_back(tag);
    }

    return tagList;
}
//checks if a string is ASCII
bool isAscii(const std::string& s) {
    return std::all_of(s.begin(), s.end(), [](char c) { return static_cast<unsigned char>(c) < 128; });
}

map<string, map<string, int>> countryTagViews;
map<string, map<string, int>> countryTagInteractions;
map<string, int> globalTagViews;
map<string, int> globalTagInteraction;

void updateTagViewsAndInteractions(const Video& video) {
    double engagement = engagementRate(video);
    const auto& tags = splitTags(video.tags);
    for (const string& tag : tags) {
        if (isAscii(tag)) {
            int tagViews = video.views;
            double weightedEngagement = engagement * tagViews;

            countryTagViews[video.country][tag] += tagViews;
            countryTagInteractions[video.country][tag] += weightedEngagement;
            globalTagViews[tag] += tagViews;
            globalTagInteraction[tag] += weightedEngagement;
        }
    }
}


vector<pair<string, int>> topNElements(const map<string, int>& m, size_t n) {
    vector<pair<string, int>> topElements;
    for (const auto& entry : m) {
        topElements.push_back(entry);
    }

    sort(topElements.begin(), topElements.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
        });

    if (topElements.size() > n) {
        topElements.resize(n);
    }

    return topElements;
}

int main() {
    string foldername = "archive"; // Replace with the name of the folder containing the dataset files
    vector<Video> videos;

    for (const auto& entry : fs::directory_iterator(foldername)) {
        if (entry.path().extension() == ".csv") {
            string filename = entry.path().filename().string();
            string default_country = filename.substr(0, 2);
            try {
                io::CSVReader<16, io::trim_chars<' ', '\t'>, io::double_quote_escape<',', '\"'>> in(entry.path().string());
                in.read_header(io::ignore_extra_column, "video_id", "trending_date", "title", "channel_title",
                    "category_id", "publish_time", "tags", "views", "likes", "dislikes", "comment_count",
                    "thumbnail_link", "comments_disabled", "ratings_disabled", "video_error_or_removed", "description");

                Video video;
                try {
                    while (in.read_row(video.video_id, video.trending_date, video.title, video.channel_title,
                        video.category_id, video.publish_time, video.tags, video.views, video.likes, video.dislikes,
                        video.comment_count, video.thumbnail_link, video.temp_comments_disabled, video.temp_ratings_disabled,
                        video.temp_video_error_or_removed, video.description)) {

                        video.comments_disabled = (video.temp_comments_disabled == "True");
                        video.ratings_disabled = (video.temp_ratings_disabled == "True");
                        video.video_error_or_removed = (video.temp_video_error_or_removed == "True");
                        video.country = default_country;
                        videos.push_back(video);
                        updateTagViewsAndInteractions(video);
                    }
                }
                catch (const std::exception& e) {
                    cerr << "Error parsing a line in file " << entry.path() << ": " << e.what() << endl;
                    continue;
                }

            }
            catch (const std::exception& e) {
                cerr << "Error parsing file " << entry.path() << ": " << e.what() << endl;
                continue;
            }
        }
    }


    set<string> countries;
    for (const Video& video : videos) {
        countries.insert(video.country);
    }

    string allCountries = "ALL";
    cout << "Available countries: ";
    for (const string& country : countries) {
        cout << country << ", ";
    }
    cout << allCountries << endl;

    string selectedCountriesStr;
    bool validInput = false;
    cout.flush();
    vector<string> selectedCountriesVec;

    while (!validInput) {
        cout << "Enter the countries you want to analyze (comma-separated), or type 'All': ";
        getline(cin, selectedCountriesStr);

        selectedCountriesVec = validateAndConvertCountryInput(selectedCountriesStr, countries);

        if (selectedCountriesVec.empty()) {
            cout << "Please type one of the options" << endl;
        }
        else {
            validInput = true;
        }
    }

    // Convert the vector to a set
    set<string> selectedCountries;
    if (selectedCountriesVec.size() == 1 && selectedCountriesVec[0] == "ALL") {
        selectedCountries = countries;
    }
    else {
        for (const string& country : selectedCountriesVec) {
            selectedCountries.insert(country);
        }
    }

    for (const string& country : selectedCountries) {
        if (countries.count(country) == 0) {
            cerr << "Invalid country: " << country << endl;
            continue;
        }

        cout << "\nCountry: " << country << endl;

        // Top 25 keywords/tags for views
        auto topViews = topNElements(countryTagViews[country], 25);
        cout << "Top 25 keywords/tags for views:" << endl;
        for (const auto& [tag, views] : topViews) {
            cout << tag << ": " << views << endl;
        }

        // Top 25 keywords/tags to avoid for views
        auto bottomViews = topNElements(countryTagViews[country], countryTagViews[country].size());
        cout << "\nTop 25 keywords/tags to avoid for views:" << endl;
        reverse(bottomViews.begin(), bottomViews.end());
        bottomViews.resize(25);
        for (const auto& [tag, views] : bottomViews) {
            cout << tag << ": " << views << endl;
        }

        // Top 25 keywords/tags for positive interaction
        auto topInteraction = topNElements(countryTagInteractions[country], 25);
        cout << "\nTop 25 keywords/tags for positive interaction:" << endl;
        for (const auto& [tag, interaction] : topInteraction) {
            cout << tag << ": " << interaction << endl;
        }

        // Top 25 keywords/tags to avoid for positive interaction
        auto bottomInteraction = topNElements(countryTagInteractions[country], countryTagInteractions[country].size());
        cout << "\nTop 25 keywords/tags to avoid for positive interaction:" << endl;
        reverse(bottomInteraction.begin(), bottomInteraction.end());
        bottomInteraction.resize(25);
        for (const auto& [tag, interaction] : bottomInteraction) {
            cout << tag << ": " << interaction << endl;
        }
    }

    auto topGlobalViews = topNElements(globalTagViews, 25);
    cout << "\nTop 25 keywords/tags for a global audience:" << endl;
    for (const auto& [tag, views] : topGlobalViews) {
        cout << tag << ": " << views << endl;
    }

    return 0;
}

