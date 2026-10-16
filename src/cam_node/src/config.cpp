#include "config.hpp"
#include "stream.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>


namespace {

int positive_int(const nlohmann::json& object, const char* key) {
    const auto& value = object.at(key);
    if (!value.is_number_integer() || value < 1 ||
        value > std::numeric_limits<int>::max()) {
        throw std::runtime_error(std::string(key) + " must be a positive integer");
    }
    return value.get<int>();
}

}

CameraConfig load_camera_config(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file) {throw std::runtime_error("Cannot open file");}
        const auto json = nlohmann::json::parse(file);
        CameraConfig config{
            json.at("serial").get<std::string>(),
            json.at("enabled").get<bool>(),
            {}
        };
        if (config.serial.empty()) {throw std::runtime_error("serial must not be empty");}
        const auto& streams = json.at("streams");
        if (!streams.is_object()) {
            throw std::runtime_error("streams must be an object");
        }
        for (const auto& entry : streams.items()) {
            try {
                const auto& value = entry.value();
                if (!value.at("enabled").get<bool>()) {continue;}
                std::string name;
                auto lowercase = [](std::string text) {
                    std::transform(text.begin(), text.end(), text.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    return text;
                };
                for (int type = 1; type < RS2_STREAM_COUNT; ++type) {
                    const std::string sdk_name = rs2_stream_to_string(static_cast<rs2_stream>(type));
                    if (lowercase(entry.key()) == lowercase(sdk_name)) {
                        name = sdk_name;
                        break;
                    }
                }
                if (name.empty()) {throw std::runtime_error("Unknown stream name");}
                StreamConfig stream{
                    name,
                    value.contains("width") ? positive_int(value, "width") : 0,
                    value.contains("height") ? positive_int(value, "height") : 0,
                    positive_int(value, "fps"),
                    value.at("format").get<std::string>()
                };
                if (stream.format.empty()) {
                    throw std::runtime_error("format must not be empty");
                }
                config.streams.push_back(std::move(stream));
            } catch (const std::exception& error) {
                throw std::runtime_error("stream '" + entry.key() + "': " + error.what());
            }
        }
        return config;
    } catch (const std::exception& error) {
        throw std::runtime_error("Invalid camera config '" + path + "': " + error.what());
    }
}
