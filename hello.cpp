#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <random>
#include <iomanip>
using namespace std;

const string STATE_FILE = "mental_health_state.txt";
const string CONFIG_FILE = "mental_health_config.txt";

struct MentalHealthState {
    vector<string> moodHistory;
    vector<pair<string, string>> thoughtJournal; // thought + timestamp
    deque<string> copingStrategies;
    string currentMood;
    string lastStrategyUsed;
    time_t lastStrategyTime;
    map<string, int> moodStatistics;
};

struct AppConfig {
    vector<string> availableMoods;
    map<string, string> moodEmojis;
    vector<string> defaultStrategies;
    int maxHistoryItems;
    bool enableTimestamps;
};

MentalHealthState loadState() {
    MentalHealthState state;
    ifstream file(STATE_FILE);
    string line;
    
    if (file.is_open()) {
        // Read current mood
        getline(file, line);
        if (line.find("CURRENT_MOOD:") != string::npos) {
            state.currentMood = line.substr(line.find(":") + 2);
        }
        
        // Read last strategy used
        getline(file, line);
        if (line.find("LAST_STRATEGY:") != string::npos) {
            state.lastStrategyUsed = line.substr(line.find(":") + 2);
        }
        
        // Read last strategy time
        getline(file, line);
        if (line.find("LAST_STRATEGY_TIME:") != string::npos) {
            state.lastStrategyTime = stol(line.substr(line.find(":") + 2));
        }
        
        // Read mood statistics
        while (getline(file, line) && line != "MOOD_HISTORY:") {
            if (line.find("MOOD_STAT:") != string::npos) {
                size_t colon = line.find(":");
                size_t dash = line.find("-");
                if (dash != string::npos) {
                    string mood = line.substr(colon + 2, dash - colon - 2);
                    int count = stoi(line.substr(dash + 1));
                    state.moodStatistics[mood] = count;
                }
            }
        }
        
        // Read mood history
        while (getline(file, line) && line != "THOUGHT_JOURNAL:") {
            if (!line.empty()) {
                state.moodHistory.push_back(line);
            }
        }
        
        // Read thought journal
        while (getline(file, line) && line != "COPING_STRATEGIES:") {
            if (!line.empty()) {
                // Check if line contains timestamp
                size_t sep = line.find("|");
                if (sep != string::npos) {
                    string thought = line.substr(0, sep);
                    string timestamp = line.substr(sep + 1);
                    state.thoughtJournal.push_back({thought, timestamp});
                } else {
                    state.thoughtJournal.push_back({line, ""});
                }
            }
        }
        
        // Read coping strategies
        while (getline(file, line)) {
            if (!line.empty()) {
                state.copingStrategies.push_back(line);
            }
        }
        
        file.close();
    } else {
        // Initialize with default data if no state file exists
        state.currentMood = "Neutral";
        state.lastStrategyUsed = "None yet";
        state.lastStrategyTime = time(0);
        
        // Initialize mood statistics
        state.moodStatistics["Neutral"] = 1;
    }
    
    return state;
}

AppConfig loadConfig() {
    AppConfig config;
    
    // Default configuration
    config.availableMoods = {"Happy", "Sad", "Anxious", "Angry", "Tired", "Stressed", "Neutral", "Excited", "Calm"};
    config.moodEmojis = {
        {"Happy", "😊"},
        {"Sad", "😢"},
        {"Anxious", "😰"},
        {"Angry", "😠"},
        {"Tired", "😴"},
        {"Stressed", "😫"},
        {"Neutral", "😐"},
        {"Excited", "😃"},
        {"Calm", "😌"}
    };
    config.defaultStrategies = {
        "Deep breathing for 5 minutes",
        "Take a short walk outside",
        "Write down three things you're grateful for",
        "Listen to calming music",
        "Practice mindfulness meditation",
        "Drink a glass of water",
        "Stretch your body",
        "Read a book for 15 minutes",
        "Call a friend or family member"
    };
    config.maxHistoryItems = 50;
    config.enableTimestamps = true;
    
    // Try to load configuration from file
    ifstream file(CONFIG_FILE);
    string line;
    
    if (file.is_open()) {
        while (getline(file, line)) {
            if (line.find("MAX_HISTORY:") != string::npos) {
                config.maxHistoryItems = stoi(line.substr(line.find(":") + 1));
            } else if (line.find("TIMESTAMPS:") != string::npos) {
                config.enableTimestamps = (line.substr(line.find(":") + 1) == "1");
            }
        }
        file.close();
    }
    
    return config;
}

void saveState(const MentalHealthState& state, const AppConfig& config) {
    ofstream file(STATE_FILE);
    if (file.is_open()) {
        file << "CURRENT_MOOD: " << state.currentMood << "\n";
        file << "LAST_STRATEGY: " << state.lastStrategyUsed << "\n";
        file << "LAST_STRATEGY_TIME: " << state.lastStrategyTime << "\n";
        
        // Save mood statistics
        for (const auto& stat : state.moodStatistics) {
            file << "MOOD_STAT: " << stat.first << "-" << stat.second << "\n";
        }
        
        file << "MOOD_HISTORY:\n";
        int startIdx = max(0, static_cast<int>(state.moodHistory.size()) - config.maxHistoryItems);
        for (size_t i = startIdx; i < state.moodHistory.size(); i++) {
            file << state.moodHistory[i] << "\n";
        }
        
        file << "THOUGHT_JOURNAL:\n";
        startIdx = max(0, static_cast<int>(state.thoughtJournal.size()) - config.maxHistoryItems);
        for (size_t i = startIdx; i < state.thoughtJournal.size(); i++) {
            file << state.thoughtJournal[i].first << "|" << state.thoughtJournal[i].second << "\n";
        }
        
        file << "COPING_STRATEGIES:\n";
        for (const auto& strategy : state.copingStrategies) {
            file << strategy << "\n";
        }
        
        file.close();
    }
}

string getTimestamp() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", ltm);
    return string(buffer);
}

string formatTimeAgo(time_t pastTime) {
    time_t now = time(0);
    double seconds = difftime(now, pastTime);
    
    if (seconds < 60) return "just now";
    if (seconds < 3600) return to_string(static_cast<int>(seconds/60)) + " minutes ago";
    if (seconds < 86400) return to_string(static_cast<int>(seconds/3600)) + " hours ago";
    return to_string(static_cast<int>(seconds/86400)) + " days ago";
}

void printPage(const MentalHealthState& state, const AppConfig& config) {
    cout << "Content-type: text/html\r\n\r\n";
    cout << "<!DOCTYPE html>\n";
    cout << "<html lang='en'>\n";
    cout << "<head>\n";
    cout << "<meta charset='UTF-8'>\n";
    cout << "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n";
    cout << "<title>Enhanced Mental Health Tracker</title>\n";
    
    // Chat widget (internet चाहिए)
    cout << "<script src='https://cdn.jotfor.ms/agent/embedjs/0198cbf6508f730a8d1f0df6d65ae0f3d772/embed.js'></script>\n";
    
    cout << "<style>\n";
    cout << "        * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; }\n";
    cout << "        body { background: linear-gradient(135deg, #1a2a6c, #b21f1f, #fdbb2d); padding: 20px; color: #333; min-height: 100vh; }\n";
    cout << "        .header { text-align: center; margin-bottom: 30px; color: white; text-shadow: 0 2px 4px rgba(0,0,0,0.3); }\n";
    cout << "        .header h1 { margin-bottom: 10px; font-size: 2.5em; }\n";
    cout << "        .header p { font-size: 1.2em; opacity: 0.9; }\n";
    cout << "        .container { display: grid; grid-template-columns: repeat(auto-fit, minmax(350px, 1fr)); gap: 25px; max-width: 1400px; margin: 0 auto; }\n";
    cout << "        .panel { background: rgba(255, 255, 255, 0.95); border-radius: 15px; box-shadow: 0 8px 30px rgba(0,0,0,0.15); overflow: hidden; transition: transform 0.3s ease, box-shadow 0.3s ease; }\n";
    cout << "        .panel:hover { transform: translateY(-5px); box-shadow: 0 12px 40px rgba(0,0,0,0.2); }\n";
    cout << "        .panel-header { padding: 20px; color: white; font-weight: bold; font-size: 1.3em; display: flex; align-items: center; gap: 12px; }\n";
    cout << "        .panel-header.mood { background: linear-gradient(to right, #ff7e5f, #feb47b); }\n";
    cout << "        .panel-header.thoughts { background: linear-gradient(to right, #00cdac, #02aab0); }\n";
    cout << "        .panel-header.strategies { background: linear-gradient(to right, #7474bf, #348ac7); }\n";
    cout << "        .panel-header.stats { background: linear-gradient(to right, #8e2de2, #4a00e0); }\n";
    cout << "        .panel-content { padding: 25px; }\n";
    cout << "        .current-value { margin: 15px 0; padding: 18px; background: #f8f9fa; border-radius: 10px; border-left: 5px solid #3498db; font-size: 1.1em; }\n";
    cout << "        .visualization { border: 1px solid #e0e0e0; border-radius: 10px; padding: 15px; margin: 15px 0; min-height: 200px; max-height: 300px; overflow-y: auto; background: #fafafa; }\n";
    cout << "        .mood-item, .thought-item, .strategy-item { padding: 12px; border-radius: 8px; margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center; }\n";
    cout << "        .mood-item { background: linear-gradient(to right, #fff5f5, #ffecec); border-left: 4px solid #ff7e5f; }\n";
    cout << "        .thought-item { background: linear-gradient(to right, #f0fdfa, #e6fcf7); border-left: 4px solid #00cdac; }\n";
    cout << "        .strategy-item { background: linear-gradient(to right, #f0f4ff, #e6edff); border-left: 4px solid #7474bf; }\n";
    cout << "        .empty-message { color: #7f8c8d; text-align: center; padding: 30px; font-style: italic; }\n";
    cout << "        form { margin-top: 20px; }\n";
    cout << "        select, input[type='text'], textarea { width: 100%; padding: 14px; margin-bottom: 15px; border: 1px solid #ddd; border-radius: 8px; font-size: 1em; }\n";
    cout << "        textarea { min-height: 100px; resize: vertical; }\n";
    cout << "        button { padding: 14px 20px; border: none; border-radius: 8px; color: white; cursor: pointer; font-weight: bold; margin-right: 10px; margin-bottom: 10px; transition: all 0.2s ease; }\n";
    cout << "        .btn-primary { background: #3498db; }\n";
    cout << "        .btn-primary:hover { background: #2980b9; transform: scale(1.05); }\n";
    cout << "        .btn-warning { background: #e67e22; }\n";
    cout << "        .btn-warning:hover { background: #d35400; transform: scale(1.05); }\n";
    cout << "        .btn-success { background: #2ecc71; }\n";
    cout << "        .btn-success:hover { background: #27ae60; transform: scale(1.05); }\n";
    cout << "        .btn-info { background: #9b59b6; }\n";
    cout << "        .btn-info:hover { background: #8e44ad; transform: scale(1.05); }\n";
    cout << "        .mood-emoji { font-size: 1.8em; }\n";
    cout << "        .timestamp { font-size: 0.85em; color: #7f8c8d; }\n";
    cout << "        .stats-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 15px; margin: 15px 0; }\n";
    cout << "        .stat-item { background: #f8f9fa; padding: 15px; border-radius: 8px; text-align: center; border: 1px solid #e0e0e0; }\n";
    cout << "        .stat-value { font-size: 1.8em; font-weight: bold; color: #2c3e50; }\n";
    cout << "        .stat-label { font-size: 0.9em; color: #7f8c8d; }\n";
    cout << "        .progress-bar { height: 10px; background: #ecf0f1; border-radius: 5px; overflow: hidden; margin-top: 5px; }\n";
    cout << "        .progress-fill { height: 100%; background: linear-gradient(to right, #3498db, #2ecc71); }\n";
    cout << "        .chat-widget-container { position: fixed; top: 20px; right: 20px; z-index: 1000; }\n";
    cout << "        .chat-toggle-btn { padding: 12px 20px; background: linear-gradient(to right, #3498db, #2ecc71); color: white; border: none; border-radius: 8px; font-weight: bold; cursor: pointer; box-shadow: 0 4px 8px rgba(0,0,0,0.2); transition: all 0.3s ease; }\n";
    cout << "        .chat-toggle-btn:hover { transform: scale(1.05); box-shadow: 0 6px 12px rgba(0,0,0,0.3); }\n";
    cout << "        @media (max-width: 768px) { .container { grid-template-columns: 1fr; } .chat-widget-container { top: 10px; right: 10px; } .chat-toggle-btn { padding: 10px 15px; font-size: 0.9em; } }\n";
    cout << "</style>\n";
    cout << "</head>\n";
    cout << "<body>\n";
    
    // Chat widget button
    cout << "<div class='chat-widget-container'>\n";
    cout << "<button class='chat-toggle-btn' onclick=\"window.JFAgent && window.JFAgent.open(); return false;\">💬 Chat with AI Counselor</button>\n";
    cout << "</div>\n";
    
    cout << "<div class='header'>\n";
    cout << "<h1>🌱 Enhanced Mental Health Tracker</h1>\n";
    cout << "<p>Comprehensive mood tracking, thought journaling, and coping strategies</p>\n";
    cout << "</div>\n";
    cout << "<div class='container'>\n";
    
    // Mood Tracker Panel
    cout << "<div class='panel'>\n";
    cout << "<div class='panel-header mood'><span>📊</span> Mood Tracker</div>\n";
    cout << "<div class='panel-content'>\n";
    cout << "<div class='current-value'>\n";
    cout << "<strong>Current mood:</strong> " << state.currentMood << " " << config.moodEmojis.at(state.currentMood) << "\n";
    cout << "</div>\n";
    
    // Mood History Visualization
    cout << "<div class='visualization'>\n";
    if (state.moodHistory.empty()) {
        cout << "<div class='empty-message'>No mood history yet. Start tracking your moods!</div>\n";
    } else {
        int startIdx = max(0, static_cast<int>(state.moodHistory.size()) - 10);
        for (int i = (int)state.moodHistory.size() - 1; i >= startIdx; i--) {
            string mood = state.moodHistory[i];
            cout << "<div class='mood-item'>" 
                 << "<span>" << mood << " " << config.moodEmojis.at(mood) << "</span>"
                 << "<span class='timestamp'>Recorded</span>"
                 << "</div>\n";
        }
    }
    cout << "</div>\n";
    
    // Mood Form
    cout << "<form method='GET'>\n";
    cout << "<select name='moodInput'>\n";
    cout << "<option value=''>Select a mood</option>\n";
    for (const auto& mood : config.availableMoods) {
        cout << "<option value='" << mood << "'>" << mood << " " << config.moodEmojis.at(mood) << "</option>\n";
    }
    cout << "</select>\n";
    cout << "<button type='submit' name='action' value='logMood' class='btn-primary'>Log Mood</button>\n";
    cout << "</form>\n";
    cout << "</div>\n";
    cout << "</div>\n";
    
    // Thought Journal Panel
    cout << "<div class='panel'>\n";
    cout << "<div class='panel-header thoughts'><span>📝</span> Thought Journal</div>\n";
    cout << "<div class='panel-content'>\n";
    cout << "<div class='current-value'>\n";
    cout << "<strong>Recent thoughts:</strong> " << (state.thoughtJournal.empty() ? "No thoughts recorded yet" : "") << "\n";
    cout << "</div>\n";
    
    // Thought Journal Visualization
    cout << "<div class='visualization'>\n";
    if (state.thoughtJournal.empty()) {
        cout << "<div class='empty-message'>Your thoughts will appear here. Journaling helps process emotions.</div>\n";
    } else {
        int startIdx = max(0, static_cast<int>(state.thoughtJournal.size()) - 5);
        for (int i = (int)state.thoughtJournal.size() - 1; i >= startIdx; i--) {
            cout << "<div class='thought-item'>" 
                 << "<div>" << state.thoughtJournal[i].first << "</div>"
                 << "<div class='timestamp'>" << state.thoughtJournal[i].second << "</div>"
                 << "</div>\n";
        }
    }
    cout << "</div>\n";
    
    // Thought Journal Form
    cout << "<form method='GET'>\n";
    cout << "<textarea name='thoughtInput' placeholder='What&apos;s on your mind? Writing can help process emotions...'></textarea>\n";
    cout << "<button type='submit' name='action' value='addThought' class='btn-success'>Journal Thought</button>\n";
    cout << "</form>\n";
    cout << "</div>\n";
    cout << "</div>\n";
    
    // Coping Strategies Panel
    cout << "<div class='panel'>\n";
    cout << "<div class='panel-header strategies'><span>🛠️</span> Coping Strategies</div>\n";
    cout << "<div class='panel-content'>\n";
    cout << "<div class='current-value'>\n";
    cout << "<strong>Last strategy used:</strong> " << state.lastStrategyUsed << "\n";
    if (state.lastStrategyTime > 0) {
        cout << "<div class='timestamp'>" << formatTimeAgo(state.lastStrategyTime) << "</div>\n";
    }
    cout << "</div>\n";
    
    // Strategies Visualization
    cout << "<div class='visualization'>\n";
    if (state.copingStrategies.empty()) {
        cout << "<div class='empty-message'>No strategies available. Add some below!</div>\n";
    } else {
        // Show up to 5 strategies
        int count = 0;
        for (const auto& strategy : state.copingStrategies) {
            if (count++ >= 5) break;
            cout << "<div class='strategy-item'>" << strategy << "</div>\n";
        }
    }
    cout << "</div>\n";
    
    // Strategies Form
    cout << "<form method='GET'>\n";
    cout << "<button type='submit' name='action' value='suggestStrategy' class='btn-warning'>Suggest a Strategy</button>\n";
    cout << "<button type='submit' name='action' value='useStrategy' class='btn-success'>Use This Strategy</button>\n";
    cout << "<button type='submit' name='action' value='addStrategy' class='btn-info'>Add New Strategy</button>\n";
    cout << "</form>\n";
    cout << "<form method='GET' style='margin-top: 10px;'>\n";
    cout << "<input type='text' name='newStrategy' placeholder='Enter a new coping strategy'>\n";
    cout << "<button type='submit' name='action' value='addCustomStrategy' class='btn-primary'>Add Custom</button>\n";
    cout << "</form>\n";
    cout << "</div>\n";
    cout << "</div>\n";
    
    // Statistics Panel
    cout << "<div class='panel'>\n";
    cout << "<div class='panel-header stats'><span>📈</span> Mood Statistics</div>\n";
    cout << "<div class='panel-content'>\n";
    
    if (state.moodStatistics.empty()) {
        cout << "<div class='empty-message'>No statistics yet. Start tracking your mood!</div>\n";
    } else {
        // Calculate total mood entries
        int total = 0;
        for (const auto& stat : state.moodStatistics) {
            total += stat.second;
        }
        
        cout << "<div class='stats-grid'>\n";
        for (const auto& stat : state.moodStatistics) {
            int percentage = total > 0 ? (stat.second * 100) / total : 0;
            cout << "<div class='stat-item'>\n";
            cout << "<div class='stat-value'>" << stat.second << "</div>\n";
            cout << "<div class='stat-label'>" << stat.first << " " << config.moodEmojis.at(stat.first) << "</div>\n";
            cout << "<div class='progress-bar'><div class='progress-fill' style='width: " << percentage << "%'></div></div>\n";
            cout << "</div>\n";
        }
        cout << "</div>\n";
        
        // Find most common mood
        string mostCommonMood;
        int maxCount = 0;
        for (const auto& stat : state.moodStatistics) {
            if (stat.second > maxCount) {
                maxCount = stat.second;
                mostCommonMood = stat.first;
            }
        }
        
        cout << "<div class='current-value'>\n";
        cout << "<strong>Most common mood:</strong> " << mostCommonMood << " " << config.moodEmojis.at(mostCommonMood) << "\n";
        cout << "<div class='timestamp'>" << maxCount << " recorded instances</div>\n";
        cout << "</div>\n";
    }
    cout << "</div>\n";
    cout << "</div>\n";
    
    cout << "</div>\n";
    cout << "</body>\n";
    cout << "</html>\n";
}

string urlDecode(const string& encoded) {
    string decoded = encoded;
    size_t pos = 0;
    while ((pos = decoded.find('+', pos)) != string::npos) {
        decoded.replace(pos, 1, " ");
        pos++;
    }
    pos = 0;
    while ((pos = decoded.find('%', pos)) != string::npos) {
        if (pos + 2 < decoded.length()) {
            string hex = decoded.substr(pos + 1, 2);
            char chr = static_cast<char>(stoi(hex, nullptr, 16));
            decoded.replace(pos, 3, string(1, chr));
        }
        pos++;
    }
    return decoded;
}

int main() {
    AppConfig config = loadConfig();
    MentalHealthState state = loadState();
    
    if (state.copingStrategies.empty()) {
        for (const auto& strategy : config.defaultStrategies) {
            state.copingStrategies.push_back(strategy);
        }
    }
    
    char* query = getenv("QUERY_STRING");
    string action, moodInput, thoughtInput, newStrategy;
    
    if (query != NULL) {
        string qs(query);
        
        size_t aPos = qs.find("action=");
        if (aPos != string::npos) {
            size_t endPos = qs.find("&", aPos);
            if (endPos == string::npos) endPos = qs.length();
            action = qs.substr(aPos + 7, endPos - (aPos + 7));
        }
        
        size_t mPos = qs.find("moodInput=");
        if (mPos != string::npos) {
            size_t endPos = qs.find("&", mPos);
            if (endPos == string::npos) endPos = qs.length();
            moodInput = urlDecode(qs.substr(mPos + 10, endPos - (mPos + 10)));
        }
        
        size_t tPos = qs.find("thoughtInput=");
        if (tPos != string::npos) {
            size_t endPos = qs.find("&", tPos);
            if (endPos == string::npos) endPos = qs.length();
            thoughtInput = urlDecode(qs.substr(tPos + 13, endPos - (tPos + 13)));
        }
        
        size_t sPos = qs.find("newStrategy=");
        if (sPos != string::npos) {
            size_t endPos = qs.find("&", sPos);
            if (endPos == string::npos) endPos = qs.length();
            newStrategy = urlDecode(qs.substr(sPos + 12, endPos - (sPos + 12)));
        }
    }
    
    if (action == "logMood" && !moodInput.empty()) {
        state.moodStatistics[moodInput]++;
        if (state.currentMood != moodInput) {
            state.moodHistory.push_back(state.currentMood);
        }
        state.currentMood = moodInput;
    }
    else if (action == "addThought" && !thoughtInput.empty()) {
        string timestamp = config.enableTimestamps ? getTimestamp() : "";
        state.thoughtJournal.push_back({thoughtInput, timestamp});
    }
    else if (action == "suggestStrategy" && !state.copingStrategies.empty()) {
        string strategy = state.copingStrategies.front();
        state.copingStrategies.pop_front();
        state.copingStrategies.push_back(strategy);
    }
    else if (action == "useStrategy" && !state.copingStrategies.empty()) {
        state.lastStrategyUsed = state.copingStrategies.front();
        state.lastStrategyTime = time(0);
    }
    else if (action == "addStrategy") {
        if (!config.defaultStrategies.empty()) {
            static random_device rd;
            static mt19937 gen(rd());
            uniform_int_distribution<> dis(0, (int)config.defaultStrategies.size() - 1);
            string strategy = config.defaultStrategies[dis(gen)];
            state.copingStrategies.push_back(strategy);
        }
    }
    else if (action == "addCustomStrategy" && !newStrategy.empty()) {
        state.copingStrategies.push_back(newStrategy);
    }
    
    if (state.moodHistory.size() > (size_t)config.maxHistoryItems) {
        state.moodHistory.erase(state.moodHistory.begin(), 
                               state.moodHistory.end() - config.maxHistoryItems);
    }
    if (state.thoughtJournal.size() > (size_t)config.maxHistoryItems) {
        state.thoughtJournal.erase(state.thoughtJournal.begin(), 
                                  state.thoughtJournal.end() - config.maxHistoryItems);
    }
    
    saveState(state, config);
    printPage(state, config);
    return 0;
}
