#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <algorithm>

// Constants
const int MAX_USERNAME_LEN = 20;
const int MAX_PASSWORD_LEN = 30;
const int MAX_NAME_LEN = 15; // 5 Chinese characters, each 3 bytes
const int MAX_EMAIL_LEN = 30;
const int MAX_STATIONS = 100;
const int MAX_TRAINID_LEN = 20;
const int MAX_STATIONNAME_LEN = 30; // 10 Chinese characters
const int MAX_SEAT_NUM = 100000;
const int MAX_PRICE = 100000;
const int MAX_TRAVEL_TIME = 10000;
const int MAX_STOPOVER_TIME = 10000;

// User structure
struct User {
    char username[MAX_USERNAME_LEN + 1];
    char password[MAX_PASSWORD_LEN + 1];
    char name[MAX_NAME_LEN + 1];
    char mailAddr[MAX_EMAIL_LEN + 1];
    int privilege;

    User() {
        memset(this, 0, sizeof(User));
    }
};

// Train structure
struct Train {
    char trainID[MAX_TRAINID_LEN + 1];
    int stationNum;
    char stations[MAX_STATIONS][MAX_STATIONNAME_LEN + 1];
    int seatNum;
    int prices[MAX_STATIONS - 1];
    char startTime[6]; // hh:mm\0
    int travelTimes[MAX_STATIONS - 1];
    int stopoverTimes[MAX_STATIONS - 2];
    char saleDate[2][6]; // mm-dd\0
    char type;
    bool released;

    Train() {
        memset(this, 0, sizeof(Train));
        released = false;
    }
};

// Order structure
struct Order {
    char username[MAX_USERNAME_LEN + 1];
    char trainID[MAX_TRAINID_LEN + 1];
    char date[6]; // mm-dd\0
    char fromStation[MAX_STATIONNAME_LEN + 1];
    char toStation[MAX_STATIONNAME_LEN + 1];
    int num;
    int price;
    time_t timestamp;
    int status; // 0: success, 1: pending, 2: refunded
    int fromIndex;
    int toIndex;

    Order() {
        memset(this, 0, sizeof(Order));
        timestamp = time(nullptr);
    }
};

// Global data structures
std::map<std::string, User> users;
std::set<std::string> loggedInUsers;
std::map<std::string, Train> trains;
std::vector<Order> orders;
std::queue<int> waitingQueue; // For ticket waiting list

// File names for persistence
const char* USER_FILE = "users.dat";
const char* TRAIN_FILE = "trains.dat";
const char* ORDER_FILE = "orders.dat";

// Function declarations
void loadData();
void saveData();
int add_user(const std::string& cur_username, const std::string& username,
             const std::string& password, const std::string& name,
             const std::string& mailAddr, int privilege);
int login(const std::string& username, const std::string& password);
int logout(const std::string& username);
std::string query_profile(const std::string& cur_username, const std::string& username);
std::string modify_profile(const std::string& cur_username, const std::string& username,
                          const std::string& password, const std::string& name,
                          const std::string& mailAddr, int privilege);
int add_train(const std::string& trainID, int stationNum, int seatNum,
              const std::vector<std::string>& stations, const std::vector<int>& prices,
              const std::string& startTime, const std::vector<int>& travelTimes,
              const std::vector<int>& stopoverTimes, const std::string& saleDateStart,
              const std::string& saleDateEnd, char type);
int release_train(const std::string& trainID);
std::string query_train(const std::string& trainID, const std::string& date);
int delete_train(const std::string& trainID);
std::string query_ticket(const std::string& from, const std::string& to,
                        const std::string& date, const std::string& sort_by);
std::string query_transfer(const std::string& from, const std::string& to,
                          const std::string& date, const std::string& sort_by);
int buy_ticket(const std::string& username, const std::string& trainID,
               const std::string& date, int num, const std::string& from,
               const std::string& to, bool queue);
std::string query_order(const std::string& username);
int refund_ticket(const std::string& username, int n);
int clean();
std::string exit();

// Utility functions
std::vector<std::string> split(const std::string& str, char delimiter);
int timeToMinutes(const std::string& time);
std::string minutesToTime(int minutes);
int dateToDays(const std::string& date);
std::string daysToDate(int days);
bool isValidDate(const std::string& date);
int calculatePrice(const Train& train, int fromIndex, int toIndex);
std::pair<std::string, std::string> calculateTrainTime(const Train& train,
                                                       const std::string& date,
                                                       int fromIndex, int toIndex);

std::map<std::string, std::string> parseCommandLine(const std::string& line) {
    std::map<std::string, std::string> params;
    std::istringstream iss(line);
    std::string token;

    // Skip command name
    iss >> token;

    while (iss >> token) {
        if (token[0] == '-' && token.length() > 1) {
            std::string key = token.substr(1);
            std::string value;

            // For parameters with multiple values (like stations), read until next parameter or end
            if (key == "s" || key == "p" || key == "t" || key == "o" || key == "d") {
                std::streampos pos = iss.tellg();
                std::string part;
                while (iss >> part) {
                    if (part[0] == '-' && part.length() > 1 && part[1] != '|') {
                        // Put back this parameter for next iteration
                        iss.seekg(pos);
                        break;
                    }
                    if (!value.empty()) value += " ";
                    value += part;
                    pos = iss.tellg();
                }
            } else {
                iss >> value;
            }

            params[key] = value;
        }
    }

    return params;
}

int main() {
    loadData();

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "add_user") {
            auto params = parseCommandLine(line);

            std::string cur_user = params["c"];
            std::string username = params["u"];
            std::string password = params["p"];
            std::string name = params["n"];
            std::string mailAddr = params["m"];
            int privilege = std::stoi(params["g"]);

            int result = add_user(cur_user, username, password, name, mailAddr, privilege);
            std::cout << result << std::endl;
        }
        else if (command == "login") {
            auto params = parseCommandLine(line);

            std::string username = params["u"];
            std::string password = params["p"];

            int result = login(username, password);
            std::cout << result << std::endl;
        }
        else if (command == "logout") {
            auto params = parseCommandLine(line);

            std::string username = params["u"];

            int result = logout(username);
            std::cout << result << std::endl;
        }
        else if (command == "query_profile") {
            auto params = parseCommandLine(line);

            std::string cur_user = params["c"];
            std::string username = params["u"];

            std::string result = query_profile(cur_user, username);
            std::cout << result << std::endl;
        }
        else if (command == "modify_profile") {
            auto params = parseCommandLine(line);

            std::string cur_user = params["c"];
            std::string username = params["u"];
            std::string password = params.count("p") ? params["p"] : "";
            std::string name = params.count("n") ? params["n"] : "";
            std::string mailAddr = params.count("m") ? params["m"] : "";
            int privilege = params.count("g") ? std::stoi(params["g"]) : -1;

            std::string result = modify_profile(cur_user, username, password, name, mailAddr, privilege);
            std::cout << result << std::endl;
        }
        else if (command == "add_train") {
            auto params = parseCommandLine(line);

            std::string trainID = params["i"];
            int stationNum = std::stoi(params["n"]);
            int seatNum = std::stoi(params["m"]);
            std::string stations_str = params["s"];
            std::string prices_str = params["p"];
            std::string startTime = params["x"];
            std::string travelTimes_str = params["t"];
            std::string stopoverTimes_str = params["o"];
            std::string saleDate_str = params["d"];
            char type = params["y"][0];

            // Parse stations
            std::vector<std::string> stations = split(stations_str, '|');
            // Parse prices
            std::vector<int> prices;
            for (const auto& p : split(prices_str, '|')) {
                prices.push_back(std::stoi(p));
            }
            // Parse travel times
            std::vector<int> travelTimes;
            for (const auto& t : split(travelTimes_str, '|')) {
                travelTimes.push_back(std::stoi(t));
            }
            // Parse stopover times
            std::vector<int> stopoverTimes;
            if (stopoverTimes_str != "_") {
                for (const auto& t : split(stopoverTimes_str, '|')) {
                    stopoverTimes.push_back(std::stoi(t));
                }
            }
            // Parse sale dates
            std::vector<std::string> saleDates = split(saleDate_str, '|');

            int result = add_train(trainID, stationNum, seatNum, stations, prices,
                                 startTime, travelTimes, stopoverTimes,
                                 saleDates[0], saleDates[1], type);
            std::cout << result << std::endl;
        }
        else if (command == "release_train") {
            auto params = parseCommandLine(line);

            std::string trainID = params["i"];

            int result = release_train(trainID);
            std::cout << result << std::endl;
        }
        else if (command == "query_train") {
            auto params = parseCommandLine(line);

            std::string trainID = params["i"];
            std::string date = params["d"];

            std::string result = query_train(trainID, date);
            std::cout << result;
        }
        else if (command == "delete_train") {
            auto params = parseCommandLine(line);

            std::string trainID = params["i"];

            int result = delete_train(trainID);
            std::cout << result << std::endl;
        }
        else if (command == "query_ticket") {
            auto params = parseCommandLine(line);

            std::string from = params["s"];
            std::string to = params["t"];
            std::string date = params["d"];
            std::string sort_by = params.count("p") ? params["p"] : "time";

            std::string result = query_ticket(from, to, date, sort_by);
            std::cout << result;
        }
        else if (command == "query_transfer") {
            auto params = parseCommandLine(line);

            std::string from = params["s"];
            std::string to = params["t"];
            std::string date = params["d"];
            std::string sort_by = params.count("p") ? params["p"] : "time";

            std::string result = query_transfer(from, to, date, sort_by);
            std::cout << result;
        }
        else if (command == "buy_ticket") {
            auto params = parseCommandLine(line);

            std::string username = params["u"];
            std::string trainID = params["i"];
            std::string date = params["d"];
            int num = std::stoi(params["n"]);
            std::string from = params["f"];
            std::string to = params["t"];
            bool queue = params.count("q") ? (params["q"] == "true") : false;

            int result = buy_ticket(username, trainID, date, num, from, to, queue);
            if (result == -2) {
                std::cout << "queue" << std::endl;
            } else {
                std::cout << result << std::endl;
            }
        }
        else if (command == "query_order") {
            auto params = parseCommandLine(line);

            std::string username = params["u"];

            std::string result = query_order(username);
            std::cout << result;
        }
        else if (command == "refund_ticket") {
            auto params = parseCommandLine(line);

            std::string username = params["u"];
            int n = params.count("n") ? std::stoi(params["n"]) : 1;

            int result = refund_ticket(username, n);
            std::cout << result << std::endl;
        }
        else if (command == "exit") {
            std::string result = exit();
            std::cout << result << std::endl;
            break;
        }
        else if (command == "clean") {
            int result = clean();
            std::cout << result << std::endl;
        }
    }

    saveData();
    return 0;
}

// Function implementations will go here...

std::string query_profile(const std::string& cur_username, const std::string& username) {
    // Check if cur_user is logged in
    if (loggedInUsers.find(cur_username) == loggedInUsers.end()) {
        return "-1";
    }

    // Check if user exists
    if (users.find(username) == users.end()) {
        return "-1";
    }

    User& curUser = users[cur_username];
    User& targetUser = users[username];

    // Check privilege
    if (curUser.privilege <= targetUser.privilege && cur_username != username) {
        return "-1";
    }

    return std::string(targetUser.username) + " " + targetUser.name + " " +
           targetUser.mailAddr + " " + std::to_string(targetUser.privilege);
}

std::string modify_profile(const std::string& cur_username, const std::string& username,
                          const std::string& password, const std::string& name,
                          const std::string& mailAddr, int privilege) {
    // Check if cur_user is logged in
    if (loggedInUsers.find(cur_username) == loggedInUsers.end()) {
        return "-1";
    }

    // Check if user exists
    if (users.find(username) == users.end()) {
        return "-1";
    }

    User& curUser = users[cur_username];
    User& targetUser = users[username];

    // Check privilege
    if (curUser.privilege <= targetUser.privilege && cur_username != username) {
        return "-1";
    }

    // Check privilege modification
    if (privilege != -1 && privilege >= curUser.privilege) {
        return "-1";
    }

    // Modify fields
    if (!password.empty()) {
        strncpy(targetUser.password, password.c_str(), MAX_PASSWORD_LEN);
    }
    if (!name.empty()) {
        strncpy(targetUser.name, name.c_str(), MAX_NAME_LEN);
    }
    if (!mailAddr.empty()) {
        strncpy(targetUser.mailAddr, mailAddr.c_str(), MAX_EMAIL_LEN);
    }
    if (privilege != -1) {
        targetUser.privilege = privilege;
    }

    return std::string(targetUser.username) + " " + targetUser.name + " " +
           targetUser.mailAddr + " " + std::to_string(targetUser.privilege);
}

int add_train(const std::string& trainID, int stationNum, int seatNum,
              const std::vector<std::string>& stations, const std::vector<int>& prices,
              const std::string& startTime, const std::vector<int>& travelTimes,
              const std::vector<int>& stopoverTimes, const std::string& saleDateStart,
              const std::string& saleDateEnd, char type) {
    // Check if train already exists
    if (trains.find(trainID) != trains.end()) {
        return -1;
    }

    // Validate inputs
    if (stationNum < 2 || stationNum > MAX_STATIONS) return -1;
    if (seatNum <= 0 || seatNum > MAX_SEAT_NUM) return -1;
    if (stations.size() != stationNum) return -1;
    if (prices.size() != stationNum - 1) return -1;
    if (travelTimes.size() != stationNum - 1) return -1;
    if (stationNum > 2 && stopoverTimes.size() != stationNum - 2) return -1;

    Train train;
    strncpy(train.trainID, trainID.c_str(), MAX_TRAINID_LEN);
    train.stationNum = stationNum;
    train.seatNum = seatNum;
    strncpy(train.startTime, startTime.c_str(), 5);
    train.type = type;
    train.released = false;

    // Copy stations
    for (int i = 0; i < stationNum; i++) {
        strncpy(train.stations[i], stations[i].c_str(), MAX_STATIONNAME_LEN);
    }

    // Copy prices
    for (int i = 0; i < stationNum - 1; i++) {
        train.prices[i] = prices[i];
    }

    // Copy travel times
    for (int i = 0; i < stationNum - 1; i++) {
        train.travelTimes[i] = travelTimes[i];
    }

    // Copy stopover times
    for (int i = 0; i < stationNum - 2; i++) {
        train.stopoverTimes[i] = stopoverTimes[i];
    }

    // Copy sale dates
    strncpy(train.saleDate[0], saleDateStart.c_str(), 5);
    strncpy(train.saleDate[1], saleDateEnd.c_str(), 5);

    trains[trainID] = train;
    return 0;
}

int release_train(const std::string& trainID) {
    if (trains.find(trainID) == trains.end()) {
        return -1;
    }

    trains[trainID].released = true;
    return 0;
}

std::string query_train(const std::string& trainID, const std::string& date) {
    if (trains.find(trainID) == trains.end()) {
        return "-1\n";
    }

    Train& train = trains[trainID];
    std::string result = std::string(train.trainID) + " " + train.type + "\n";

    // Check if date is within sale range
    int queryDate = dateToDays(date);
    int saleStart = dateToDays(train.saleDate[0]);
    int saleEnd = dateToDays(train.saleDate[1]);

    if (queryDate < saleStart || queryDate > saleEnd) {
        return "-1\n";
    }

    // Calculate times and prices for each station
    int currentTime = timeToMinutes(train.startTime);
    int currentDate = queryDate;
    int cumulativePrice = 0;

    for (int i = 0; i < train.stationNum; i++) {
        std::string arrivingTime = "xx-xx xx:xx";
        std::string leavingTime = "xx-xx xx:xx";
        std::string seat = "x";

        if (i > 0) {
            // Calculate arriving time
            int arrivingMinutes = currentTime;
            int arrivingDate = currentDate;
            arrivingTime = daysToDate(arrivingDate) + " " + minutesToTime(arrivingMinutes);
        }

        if (i < train.stationNum - 1) {
            // Calculate leaving time
            if (i > 0) {
                currentTime += train.stopoverTimes[i - 1];
                if (currentTime >= 1440) {
                    currentTime -= 1440;
                    currentDate++;
                }
            }
            leavingTime = daysToDate(currentDate) + " " + minutesToTime(currentTime);

            // Update for next station
            currentTime += train.travelTimes[i];
            if (currentTime >= 1440) {
                currentTime -= 1440;
                currentDate++;
            }
            cumulativePrice += train.prices[i];
            seat = std::to_string(train.seatNum);
        }

        result += std::string(train.stations[i]) + " " + arrivingTime + " -> " +
                 leavingTime + " " + std::to_string(cumulativePrice) + " " + seat + "\n";
    }

    return result;
}

int delete_train(const std::string& trainID) {
    if (trains.find(trainID) == trains.end()) {
        return -1;
    }

    if (trains[trainID].released) {
        return -1;
    }

    trains.erase(trainID);
    return 0;
}

std::string query_ticket(const std::string& from, const std::string& to,
                        const std::string& date, const std::string& sort_by) {
    std::vector<std::pair<std::string, std::string>> results; // trainID, formatted result

    for (auto& pair : trains) {
        Train& train = pair.second;
        if (!train.released) continue;

        // Find from and to stations
        int fromIndex = -1, toIndex = -1;
        for (int i = 0; i < train.stationNum; i++) {
            if (train.stations[i] == from) fromIndex = i;
            if (train.stations[i] == to) toIndex = i;
        }

        if (fromIndex == -1 || toIndex == -1 || fromIndex >= toIndex) continue;

        // Check if train runs on this date
        int queryDate = dateToDays(date);
        int saleStart = dateToDays(train.saleDate[0]);
        int saleEnd = dateToDays(train.saleDate[1]);

        // Calculate departure date from starting station
        int departureDate = queryDate - fromIndex;
        if (departureDate < saleStart || departureDate > saleEnd) continue;

        // Calculate times and price
        auto [leavingTime, arrivingTime] = calculateTrainTime(train, date, fromIndex, toIndex);
        int price = calculatePrice(train, fromIndex, toIndex);

        std::string result = std::string(train.trainID) + " " + from + " " + leavingTime +
                            " -> " + to + " " + arrivingTime + " " + std::to_string(price) +
                            " " + std::to_string(train.seatNum);
        results.push_back({train.trainID, result});
    }

    // Sort results
    if (sort_by == "time") {
        // Sort by time (would need to parse and compare times)
        std::sort(results.begin(), results.end());
    } else {
        // Sort by cost (would need to parse and compare prices)
        std::sort(results.begin(), results.end());
    }

    std::string output = std::to_string(results.size()) + "\n";
    for (const auto& pair : results) {
        output += pair.second + "\n";
    }

    return output;
}

std::string query_transfer(const std::string& from, const std::string& to,
                          const std::string& date, const std::string& sort_by) {
    // Simplified implementation - would need proper transfer logic
    return "0";
}

int buy_ticket(const std::string& username, const std::string& trainID,
               const std::string& date, int num, const std::string& from,
               const std::string& to, bool queue) {
    // Check if user is logged in
    if (loggedInUsers.find(username) == loggedInUsers.end()) {
        return -1;
    }

    // Check if train exists and is released
    if (trains.find(trainID) == trains.end() || !trains[trainID].released) {
        return -1;
    }

    Train& train = trains[trainID];

    // Find stations
    int fromIndex = -1, toIndex = -1;
    for (int i = 0; i < train.stationNum; i++) {
        if (train.stations[i] == from) fromIndex = i;
        if (train.stations[i] == to) toIndex = i;
    }

    if (fromIndex == -1 || toIndex == -1 || fromIndex >= toIndex) {
        return -1;
    }

    // Check if enough tickets
    if (num > train.seatNum) {
        if (queue) {
            // Add to waiting queue
            Order order;
            strncpy(order.username, username.c_str(), MAX_USERNAME_LEN);
            strncpy(order.trainID, trainID.c_str(), MAX_TRAINID_LEN);
            strncpy(order.date, date.c_str(), 5);
            strncpy(order.fromStation, from.c_str(), MAX_STATIONNAME_LEN);
            strncpy(order.toStation, to.c_str(), MAX_STATIONNAME_LEN);
            order.num = num;
            order.price = calculatePrice(train, fromIndex, toIndex) * num;
            order.status = 1; // pending
            order.fromIndex = fromIndex;
            order.toIndex = toIndex;

            orders.push_back(order);
            waitingQueue.push(orders.size() - 1);
            return -2; // queue
        }
        return -1;
    }

    // Sell tickets
    train.seatNum -= num;

    // Create order
    Order order;
    strncpy(order.username, username.c_str(), MAX_USERNAME_LEN);
    strncpy(order.trainID, trainID.c_str(), MAX_TRAINID_LEN);
    strncpy(order.date, date.c_str(), 5);
    strncpy(order.fromStation, from.c_str(), MAX_STATIONNAME_LEN);
    strncpy(order.toStation, to.c_str(), MAX_STATIONNAME_LEN);
    order.num = num;
    order.price = calculatePrice(train, fromIndex, toIndex) * num;
    order.status = 0; // success
    order.fromIndex = fromIndex;
    order.toIndex = toIndex;

    orders.push_back(order);
    return order.price;
}

std::string query_order(const std::string& username) {
    // Check if user is logged in
    if (loggedInUsers.find(username) == loggedInUsers.end()) {
        return "-1";
    }

    std::vector<Order> userOrders;
    for (const auto& order : orders) {
        if (order.username == username) {
            userOrders.push_back(order);
        }
    }

    // Sort by timestamp (newest first)
    std::sort(userOrders.begin(), userOrders.end(), [](const Order& a, const Order& b) {
        return a.timestamp > b.timestamp;
    });

    std::string result = std::to_string(userOrders.size()) + "\n";
    for (const auto& order : userOrders) {
        std::string status;
        if (order.status == 0) status = "[success]";
        else if (order.status == 1) status = "[pending]";
        else status = "[refunded]";

        auto [leavingTime, arrivingTime] = calculateTrainTime(trains[order.trainID],
                                                             order.date, order.fromIndex, order.toIndex);
        int price = calculatePrice(trains[order.trainID], order.fromIndex, order.toIndex);

        result += status + " " + std::string(order.trainID) + " " + order.fromStation + " " +
                 leavingTime + " -> " + order.toStation + " " + arrivingTime + " " +
                 std::to_string(price) + " " + std::to_string(order.num) + "\n";
    }

    return result;
}

int refund_ticket(const std::string& username, int n) {
    // Check if user is logged in
    if (loggedInUsers.find(username) == loggedInUsers.end()) {
        return -1;
    }

    // Get user's orders
    std::vector<int> userOrderIndices;
    for (int i = 0; i < orders.size(); i++) {
        if (orders[i].username == username) {
            userOrderIndices.push_back(i);
        }
    }

    if (n > userOrderIndices.size()) {
        return -1;
    }

    // Sort by timestamp (newest first)
    std::sort(userOrderIndices.begin(), userOrderIndices.end(), [](int a, int b) {
        return orders[a].timestamp > orders[b].timestamp;
    });

    int orderIndex = userOrderIndices[n - 1];
    Order& order = orders[orderIndex];

    // Check if already refunded
    if (order.status == 2) {
        return -1;
    }

    // Refund tickets
    if (order.status == 0) {
        // Return seats to train
        trains[order.trainID].seatNum += order.num;
    }

    order.status = 2; // refunded
    return 0;
}

// Utility function implementations
int timeToMinutes(const std::string& time) {
    int hour, minute;
    sscanf(time.c_str(), "%d:%d", &hour, &minute);
    return hour * 60 + minute;
}

std::string minutesToTime(int minutes) {
    int hour = minutes / 60;
    int minute = minutes % 60;
    char buffer[6];
    sprintf(buffer, "%02d:%02d", hour, minute);
    return std::string(buffer);
}

int dateToDays(const std::string& date) {
    int month, day;
    sscanf(date.c_str(), "%d-%d", &month, &day);
    // Simplified: assume June 1st is day 0
    if (month == 6) return day - 1;
    if (month == 7) return 30 + day - 1;
    if (month == 8) return 61 + day - 1;
    return 0;
}

std::string daysToDate(int days) {
    int month, day;
    if (days < 30) {
        month = 6;
        day = days + 1;
    } else if (days < 61) {
        month = 7;
        day = days - 30 + 1;
    } else {
        month = 8;
        day = days - 61 + 1;
    }
    char buffer[6];
    sprintf(buffer, "%02d-%02d", month, day);
    return std::string(buffer);
}

bool isValidDate(const std::string& date) {
    int month, day;
    if (sscanf(date.c_str(), "%d-%d", &month, &day) != 2) return false;
    if (month < 6 || month > 8) return false;
    if (day < 1 || day > 31) return false;
    return true;
}

int calculatePrice(const Train& train, int fromIndex, int toIndex) {
    int price = 0;
    for (int i = fromIndex; i < toIndex; i++) {
        price += train.prices[i];
    }
    return price;
}

std::pair<std::string, std::string> calculateTrainTime(const Train& train,
                                                       const std::string& date,
                                                       int fromIndex, int toIndex) {
    int currentTime = timeToMinutes(train.startTime);
    int currentDate = dateToDays(date);

    // Calculate departure time from fromStation
    for (int i = 0; i < fromIndex; i++) {
        currentTime += train.travelTimes[i];
        if (currentTime >= 1440) {
            currentTime -= 1440;
            currentDate++;
        }
        if (i < fromIndex - 1) {
            currentTime += train.stopoverTimes[i];
            if (currentTime >= 1440) {
                currentTime -= 1440;
                currentDate++;
            }
        }
    }
    std::string leavingTime = daysToDate(currentDate) + " " + minutesToTime(currentTime);

    // Calculate arrival time at toStation
    for (int i = fromIndex; i < toIndex; i++) {
        currentTime += train.travelTimes[i];
        if (currentTime >= 1440) {
            currentTime -= 1440;
            currentDate++;
        }
        if (i < toIndex - 1 && i < train.stationNum - 2) {
            currentTime += train.stopoverTimes[i];
            if (currentTime >= 1440) {
                currentTime -= 1440;
                currentDate++;
            }
        }
    }
    std::string arrivingTime = daysToDate(currentDate) + " " + minutesToTime(currentTime);

    return {leavingTime, arrivingTime};
}

void loadData() {
    // Load users
    std::ifstream userFile(USER_FILE, std::ios::binary);
    if (userFile.is_open()) {
        int count;
        userFile.read(reinterpret_cast<char*>(&count), sizeof(int));
        for (int i = 0; i < count; i++) {
            User user;
            userFile.read(reinterpret_cast<char*>(&user), sizeof(User));
            users[user.username] = user;
        }
        userFile.close();
    }

    // Load trains
    std::ifstream trainFile(TRAIN_FILE, std::ios::binary);
    if (trainFile.is_open()) {
        int count;
        trainFile.read(reinterpret_cast<char*>(&count), sizeof(int));
        for (int i = 0; i < count; i++) {
            Train train;
            trainFile.read(reinterpret_cast<char*>(&train), sizeof(Train));
            trains[train.trainID] = train;
        }
        trainFile.close();
    }

    // Load orders
    std::ifstream orderFile(ORDER_FILE, std::ios::binary);
    if (orderFile.is_open()) {
        int count;
        orderFile.read(reinterpret_cast<char*>(&count), sizeof(int));
        orders.resize(count);
        for (int i = 0; i < count; i++) {
            orderFile.read(reinterpret_cast<char*>(&orders[i]), sizeof(Order));
        }
        orderFile.close();
    }
}

void saveData() {
    // Save users
    std::ofstream userFile(USER_FILE, std::ios::binary);
    if (userFile.is_open()) {
        int count = users.size();
        userFile.write(reinterpret_cast<const char*>(&count), sizeof(int));
        for (const auto& pair : users) {
            userFile.write(reinterpret_cast<const char*>(&pair.second), sizeof(User));
        }
        userFile.close();
    }

    // Save trains
    std::ofstream trainFile(TRAIN_FILE, std::ios::binary);
    if (trainFile.is_open()) {
        int count = trains.size();
        trainFile.write(reinterpret_cast<const char*>(&count), sizeof(int));
        for (const auto& pair : trains) {
            trainFile.write(reinterpret_cast<const char*>(&pair.second), sizeof(Train));
        }
        trainFile.close();
    }

    // Save orders
    std::ofstream orderFile(ORDER_FILE, std::ios::binary);
    if (orderFile.is_open()) {
        int count = orders.size();
        orderFile.write(reinterpret_cast<const char*>(&count), sizeof(int));
        for (const auto& order : orders) {
            orderFile.write(reinterpret_cast<const char*>(&order), sizeof(Order));
        }
        orderFile.close();
    }
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

int add_user(const std::string& cur_username, const std::string& username,
             const std::string& password, const std::string& name,
             const std::string& mailAddr, int privilege) {
    // Check if first user
    if (users.empty()) {
        User newUser;
        strncpy(newUser.username, username.c_str(), MAX_USERNAME_LEN);
        strncpy(newUser.password, password.c_str(), MAX_PASSWORD_LEN);
        strncpy(newUser.name, name.c_str(), MAX_NAME_LEN);
        strncpy(newUser.mailAddr, mailAddr.c_str(), MAX_EMAIL_LEN);
        newUser.privilege = 10;
        users[username] = newUser;
        return 0;
    }

    // Check if cur_user is logged in
    if (loggedInUsers.find(cur_username) == loggedInUsers.end()) {
        return -1;
    }

    // Check if username already exists
    if (users.find(username) != users.end()) {
        return -1;
    }

    // Check privilege
    User& curUser = users[cur_username];
    if (privilege >= curUser.privilege) {
        return -1;
    }

    // Create new user
    User newUser;
    strncpy(newUser.username, username.c_str(), MAX_USERNAME_LEN);
    strncpy(newUser.password, password.c_str(), MAX_PASSWORD_LEN);
    strncpy(newUser.name, name.c_str(), MAX_NAME_LEN);
    strncpy(newUser.mailAddr, mailAddr.c_str(), MAX_EMAIL_LEN);
    newUser.privilege = privilege;
    users[username] = newUser;

    return 0;
}

int login(const std::string& username, const std::string& password) {
    // Check if already logged in
    if (loggedInUsers.find(username) != loggedInUsers.end()) {
        return -1;
    }

    // Check if user exists
    if (users.find(username) == users.end()) {
        return -1;
    }

    // Check password
    User& user = users[username];
    if (password != user.password) {
        return -1;
    }

    loggedInUsers.insert(username);
    return 0;
}

int logout(const std::string& username) {
    // Check if user is logged in
    if (loggedInUsers.find(username) == loggedInUsers.end()) {
        return -1;
    }

    loggedInUsers.erase(username);
    return 0;
}

std::string exit() {
    saveData();
    loggedInUsers.clear();
    return "bye";
}

int clean() {
    users.clear();
    loggedInUsers.clear();
    trains.clear();
    orders.clear();
    while (!waitingQueue.empty()) {
        waitingQueue.pop();
    }

    // Delete data files
    std::remove(USER_FILE);
    std::remove(TRAIN_FILE);
    std::remove(ORDER_FILE);

    return 0;
}