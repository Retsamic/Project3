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


double engagement_rate(const Video& video) {
    double like_weight = 1.0;
    double dislike_weight = 0.5;
    double comment_weight = 1.5;

    double total_likes = like_weight * video.likes;
    double total_dislikes = dislike_weight * video.dislikes;
    double total_comments = comment_weight * video.comment_count;

    return (total_likes + total_dislikes + total_comments) / video.views;
}

vector<string> validate_and_convert_country_input(const string& input, const set<string>& valid_countries) {
    string uppercase_input = input;
    transform(input.begin(), input.end(), uppercase_input.begin(), ::toupper);
    stringstream ss(uppercase_input);
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





vector<string> split_tags(const string& tags) {
    stringstream ss(tags);
    string tag;
    vector<string> tag_list;

    while (getline(ss, tag, '|')) {
        tag_list.push_back(tag);
    }

    return tag_list;
}

bool is_ascii(const std::string& s) {
    return std::all_of(s.begin(), s.end(), [](char c) { return static_cast<unsigned char>(c) < 128; });
}

map<string, map<string, int>> country_tag_views;
map<string, map<string, int>> country_tag_interaction;
map<string, int> global_tag_views;
map<string, int> global_tag_interaction;

void update_tag_views_and_interaction(const Video& video) {
    double engagement = engagement_rate(video);
    const auto& tags = split_tags(video.tags);
    for (const string& tag : tags) {
        if (is_ascii(tag)) {
            int tag_views = video.views;
            double weighted_engagement = engagement * tag_views;

            country_tag_views[video.country][tag] += tag_views;
            country_tag_interaction[video.country][tag] += weighted_engagement;
            global_tag_views[tag] += tag_views;
            global_tag_interaction[tag] += weighted_engagement;
        }
    }
}


vector<pair<string, int>> top_n_elements(const map<string, int>& m, size_t n) {
    vector<pair<string, int>> top_elements;
    for (const auto& entry : m) {
        top_elements.push_back(entry);
    }

    sort(top_elements.begin(), top_elements.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
        });

    if (top_elements.size() > n) {
        top_elements.resize(n);
    }

    return top_elements;
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
                        update_tag_views_and_interaction(video);
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

    string all_countries = "ALL";
    cout << "Available countries: ";
    for (const string& country : countries) {
        cout << country << ", ";
    }
    cout << all_countries << endl;

    string selected_countries_str;
    bool valid_input = false;
    cout.flush();
    vector<string> selected_countries_vec;

    while (!valid_input) {
        cout << "Enter the countries you want to analyze (comma-separated), or type 'All': ";
        getline(cin, selected_countries_str);

        selected_countries_vec = validate_and_convert_country_input(selected_countries_str, countries);

        if (selected_countries_vec.empty()) {
            cout << "Please type one of the options" << endl;
        }
        else {
            valid_input = true;
        }
    }

    // Convert the vector to a set
    set<string> selected_countries;
    if (selected_countries_vec.size() == 1 && selected_countries_vec[0] == "ALL") {
        selected_countries = countries;
    }
    else {
        for (const string& country : selected_countries_vec) {
            selected_countries.insert(country);
        }
    }

    for (const string& country : selected_countries) {
        if (countries.count(country) == 0) {
            cerr << "Invalid country: " << country << endl;
            continue;
        }

        cout << "\nCountry: " << country << endl;

        // Top 25 keywords/tags for views
        auto top_views = top_n_elements(country_tag_views[country], 25);
        cout << "Top 25 keywords/tags for views:" << endl;
        for (const auto& [tag, views] : top_views) {
            cout << tag << ": " << views << endl;
        }

        // Top 25 keywords/tags to avoid for views
        auto bottom_views = top_n_elements(country_tag_views[country], country_tag_views[country].size());
        cout << "\nTop 25 keywords/tags to avoid for views:" << endl;
        reverse(bottom_views.begin(), bottom_views.end());
        bottom_views.resize(25);
        for (const auto& [tag, views] : bottom_views) {
            cout << tag << ": " << views << endl;
        }

        // Top 25 keywords/tags for positive interaction
        auto top_interaction = top_n_elements(country_tag_interaction[country], 25);
        cout << "\nTop 25 keywords/tags for positive interaction:" << endl;
        for (const auto& [tag, interaction] : top_interaction) {
            cout << tag << ": " << interaction << endl;
        }

        // Top 25 keywords/tags to avoid for positive interaction
        auto bottom_interaction = top_n_elements(country_tag_interaction[country], country_tag_interaction[country].size());
        cout << "\nTop 25 keywords/tags to avoid for positive interaction:" << endl;
        reverse(bottom_interaction.begin(), bottom_interaction.end());
        bottom_interaction.resize(25);
        for (const auto& [tag, interaction] : bottom_interaction) {
            cout << tag << ": " << interaction << endl;
        }
    }

    auto top_global_views = top_n_elements(global_tag_views, 25);
    cout << "\nTop 25 keywords/tags for a global audience:" << endl;
    for (const auto& [tag, views] : top_global_views) {
        cout << tag << ": " << views << endl;
    }

    return 0;
}

