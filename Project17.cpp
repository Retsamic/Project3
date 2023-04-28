#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <chrono>
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

struct TreeNode {
    string key;
    int value;
    TreeNode* left;
    TreeNode* right;

    TreeNode(string key, int value) : key(key), value(value), left(nullptr), right(nullptr) {}
    size_t size() const {
        size_t leftSize = left ? left->size() : 0;
        size_t rightSize = right ? right->size() : 0;
        return 1 + leftSize + rightSize;
    }
};

TreeNode* insertNode(TreeNode* root, const string& key, int value) {
    if (root == nullptr) {
        return new TreeNode(key, value);
    }

    if (key < root->key) {
        root->left = insertNode(root->left, key, value);
    }
    else if (key > root->key) {
        root->right = insertNode(root->right, key, value);
    }
    else {
        root->value += value;
    }

    return root;
}

TreeNode* searchNode(TreeNode* root, const string& key) {
    if (root == nullptr || root->key == key) {
        return root;
    }

    if (key < root->key) {
        return searchNode(root->left, key);
    }

    return searchNode(root->right, key);
}

void updateTagViewsAndInteractionsBST(const Video& video, TreeNode*& countryTagViewsRoot, TreeNode*& countryTagInteractionsRoot, TreeNode*& globalTagViewsRoot, TreeNode*& globalTagInteractionRoot) {
    double engagement = engagementRate(video);
    const auto& tags = splitTags(video.tags);
    for (const string& tag : tags) {
        if (isAscii(tag)) {
            int tagViews = video.views;
            double weightedEngagement = engagement * tagViews;

            TreeNode* countryNode = searchNode(countryTagViewsRoot, tag);
            if (countryNode) {
                countryNode->value += tagViews;
            }
            else {
                countryTagViewsRoot = insertNode(countryTagViewsRoot, tag, tagViews);
            }

            TreeNode* countryInteractionNode = searchNode(countryTagInteractionsRoot, tag);
            if (countryInteractionNode) {
                countryInteractionNode->value += weightedEngagement;
            }
            else {
                countryTagInteractionsRoot = insertNode(countryTagInteractionsRoot, tag, weightedEngagement);
            }

            TreeNode* globalNode = searchNode(globalTagViewsRoot, tag);
            if (globalNode) {
                globalNode->value += tagViews;
            }
            else {
                globalTagViewsRoot = insertNode(globalTagViewsRoot, tag, tagViews);
            }

            TreeNode* globalInteractionNode = searchNode(globalTagInteractionRoot, tag);
            if (globalInteractionNode) {
                globalInteractionNode->value += weightedEngagement;
            }
            else {
                globalTagInteractionRoot = insertNode(globalTagInteractionRoot, tag, weightedEngagement);
            }
        }
    }
}

void inOrderTraversal(TreeNode* root, vector<pair<string, int>>& result) {
    if (root == nullptr) {
        return;
    }

    inOrderTraversal(root->left, result);
    result.push_back(make_pair(root->key, root->value));
    inOrderTraversal(root->right, result);
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

vector<pair<string, int>> topNElementsFromBST(TreeNode* root, size_t n) {
    vector<pair<string, int>> elements;
    inOrderTraversal(root, elements);

    sort(elements.begin(), elements.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
        });

    if (elements.size() > n) {
        elements.resize(n);
    }

    return elements;
}

vector<string> split(const string& str, char delimiter) {
    vector<string> result;
    stringstream ss(str);
    string item;

    while (getline(ss, item, delimiter)) {
        result.push_back(item);
    }

    return result;
}

void trim(string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == string::npos) {
        str = "";
        return;
    }
    size_t last = str.find_last_not_of(' ');
    str = str.substr(first, last - first + 1);
}

void printElements(const vector<pair<string, int>>& elements) {
    for (const auto& element : elements) {
        cout << element.first << ": " << element.second << "\n";
    }
}




int main() {
    string foldername = "archive"; // Replace with the name of the folder containing the dataset files
    vector<Video> videos;

    cout << "Choose a data structure for parsing (map or bst): ";
    string dataStructure;
    getline(cin, dataStructure);
    transform(dataStructure.begin(), dataStructure.end(), dataStructure.begin(), ::tolower);

    while (dataStructure != "map" && dataStructure != "bst") {
        cout << "Please choose a valid data structure (map or bst): ";
        getline(cin, dataStructure);
        transform(dataStructure.begin(), dataStructure.end(), dataStructure.begin(), ::tolower);
    }

    auto start = chrono::high_resolution_clock::now();

    TreeNode* countryTagViewsRoot = nullptr;
    TreeNode* countryTagInteractionsRoot = nullptr;
    TreeNode* globalTagViewsRoot = nullptr;
    TreeNode* globalTagInteractionRoot = nullptr;

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

                        if (dataStructure == "map") {
                            updateTagViewsAndInteractions(video);
                        }
                        else {
                            updateTagViewsAndInteractionsBST(video, countryTagViewsRoot, countryTagInteractionsRoot, globalTagViewsRoot, globalTagInteractionRoot);
                        }
                    }
                }
                catch (const std::exception& e) {
                    cerr << "Error parsing a line in file " << entry.path() << ": " << e.what() << "\n";
                    continue;
                }

            }
            catch (const std::exception& e) {
                cerr << "Error parsing file " << entry.path() << ": " << e.what() << "\n";
                continue;
            }
        }
    }


    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Time taken to parse data using " << dataStructure << ": " << duration << " milliseconds" << "\n";

    set<string> countries;
    for (const Video& video : videos) {
        countries.insert(video.country);
    }

    cout << "Available countries: ";
    for (const string& country : countries) {
        cout << country << " ";
    }
    cout << "\n";

    bool validInput = false;
    vector<string> selectedCountries;

    while (!validInput) {
        cout << "Select countries (use comma-separated values): ";
        string input;
        getline(cin, input);

        selectedCountries = split(input, ',');
        for (string& country : selectedCountries) {
            trim(country);
            if (countries.find(country) != countries.end()) {
                validInput = true;
            }
            else {
                cout << "Invalid country code: " << country << "\n";
                validInput = false;
                break;
            }
        }
    }



    // Convert the vector to a set
    set<string> selectedCountriesSet(selectedCountries.begin(), selectedCountries.end());

    for (const string& country : selectedCountries) {

        cout << "                                                       " << "\n";
        cout << "                                                       " << "\n";
        cout << "                                                       " << "\n";
        cout << "\nCountry: " << country << "\n";

        // Top 25 keywords/tags for views
        if (dataStructure == "map") {
            auto topViews = topNElements(countryTagViews[country], 25);
            cout << "Top 25 keywords/tags for views:" << "\n";
            printElements(topViews);
        }
        else {
            auto topViews = topNElementsFromBST(countryTagViewsRoot, 25);
            cout << "Top 25 keywords/tags for views:" << "\n";
            printElements(topViews);
        }

        // Top 25 keywords/tags to avoid for views
        if (dataStructure == "map") {
            auto bottomViews = topNElements(countryTagViews[country], countryTagViews[country].size());
            reverse(bottomViews.begin(), bottomViews.end());
            bottomViews.resize(25);
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "Top 25 keywords/tags to avoid for views:" << "\n";
            printElements(bottomViews);
        }
        else {
            auto bottomViews = topNElementsFromBST(countryTagViewsRoot, countryTagViewsRoot->size());
            reverse(bottomViews.begin(), bottomViews.end());
            bottomViews.resize(25);
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "Top 25 keywords/tags to avoid for views:" << "\n";
            printElements(bottomViews);
        }

        // Top 25 keywords/tags for positive interaction
        if (dataStructure == "map") {
            auto topInteraction = topNElements(countryTagInteractions[country], 25);
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "Top 25 keywords/tags for positive interaction:" << "\n";
            printElements(topInteraction);
        }
        else {
            auto topInteraction = topNElementsFromBST(countryTagInteractionsRoot, 25);
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "Top 25 keywords/tags for positive interaction:" << "\n";
            printElements(topInteraction);
        }

        // Top 25 keywords/tags to avoid for positive interaction
        if (dataStructure == "map") {
            auto bottomInteraction = topNElements(countryTagInteractions[country], countryTagInteractions[country].size());
            reverse(bottomInteraction.begin(), bottomInteraction.end());
            bottomInteraction.resize(25);
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "Top 25 keywords/tags to avoid for positive interaction:" << "\n";
            printElements(bottomInteraction);
        }
        else {
            auto bottomInteraction = topNElementsFromBST(countryTagInteractionsRoot, countryTagInteractionsRoot->size());
            reverse(bottomInteraction.begin(), bottomInteraction.end());
            bottomInteraction.resize(25);
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "                                                       " << "\n";
            cout << "Top 25 keywords/tags to avoid for positive interaction:" << "\n";
            printElements(bottomInteraction);
        }
    }

    return 0;
}
