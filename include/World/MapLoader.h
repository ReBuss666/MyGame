#pragma once
#include "SectorMap.h"
#include "Sector.h"
#include "Wall.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_map>

class MapLoader {
public:
    // Scale factor: map editor units to game units
    // Map editor uses pixels, game uses UNIT (64.0f)
    static constexpr float DEFAULT_SCALE = 1.0f / 64.0f;
    
    struct LoadResult {
        bool success;
        std::string error;
        std::vector<std::string> textureList;
    };

    static LoadResult loadFromFile(const std::string& filepath, SectorMap& outMap, float scale = DEFAULT_SCALE) {
        LoadResult result{false, "", {}};
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            result.error = "Failed to open file: " + filepath;
            std::cerr << "[MapLoader] " << result.error << std::endl;
            return result;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonContent = buffer.str();
        file.close();

        return loadFromString(jsonContent, outMap, scale);
    }

    static LoadResult loadFromString(const std::string& jsonContent, SectorMap& outMap, float scale = DEFAULT_SCALE) {
        LoadResult result{false, "", {}};
        
        outMap.clear();
        
        std::cout << "[MapLoader] Parsing JSON map..." << std::endl;

        try {
            // Simple JSON parser (no external dependencies)
            std::vector<SectorParseData> parsedSectors;
            
            if (!parseJSON(jsonContent, parsedSectors, result.textureList, result.error)) {
                return result;
            }

            // Create sectors
            std::unordered_map<int, std::vector<int>> neighborIds; // sector id -> wall neighbor ids
            
            for (const auto& parsed : parsedSectors) {
                Sector sector(parsed.id, parsed.floorHeight, parsed.ceilingHeight);
                sector.setFloorTexture(parsed.floorTexture);
                sector.setCeilingTexture(parsed.ceilingTexture);
                sector.setLightLevel(static_cast<uint8_t>(parsed.lightLevel));
                sector.setFlags(parsed.flags);

                std::vector<int> wallNeighbors;
                
                for (const auto& wallData : parsed.walls) {
                    sf::Vector2f start(wallData.startX * scale, wallData.startY * scale);
                    sf::Vector2f end(wallData.endX * scale, wallData.endY * scale);
                    
                    Wall wall(start, end);
                    wall.setUpperTexture(wallData.upperTexture);
                    wall.setMiddleTexture(wallData.middleTexture);
                    wall.setLowerTexture(wallData.lowerTexture);
                    wall.setFlags(wallData.flags);
                    
                    sector.addWall(wall);
                    wallNeighbors.push_back(wallData.neighborId);
                }
                
                neighborIds[parsed.id] = wallNeighbors;
                outMap.addSector(sector);
            }

            // Link portals (neighbor sectors)
            linkPortals(outMap, neighborIds);

            result.success = true;
            std::cout << "[MapLoader] Loaded " << outMap.getSectorCount() << " sectors" << std::endl;
            
        } catch (const std::exception& e) {
            result.error = std::string("Parse error: ") + e.what();
            std::cerr << "[MapLoader] " << result.error << std::endl;
        }

        return result;
    }

private:
    struct WallParseData {
        float startX, startY;
        float endX, endY;
        int neighborId; // -1 means null/no neighbor
        std::string upperTexture;
        std::string middleTexture;
        std::string lowerTexture;
        uint32_t flags;
    };

    struct SectorParseData {
        int id;
        float floorHeight;
        float ceilingHeight;
        int lightLevel;
        std::string floorTexture;
        std::string ceilingTexture;
        uint32_t flags;
        std::vector<WallParseData> walls;
    };

    /**
     * @brief Link wall portals to neighbor sectors
     */
    static void linkPortals(SectorMap& map, const std::unordered_map<int, std::vector<int>>& neighborIds) {
        std::cout << "[MapLoader] Linking portals..." << std::endl;
        int portalCount = 0;

        for (auto& [sectorId, sector] : map.getSectors()) {
            auto it = neighborIds.find(sectorId);
            if (it == neighborIds.end()) continue;

            const auto& neighbors = it->second;
            auto& walls = sector.getWalls();
            
            for (size_t i = 0; i < walls.size() && i < neighbors.size(); ++i) {
                int neighborId = neighbors[i];
                if (neighborId >= 0) {
                    Sector* neighborSector = map.getSector(neighborId);
                    if (neighborSector) {
                        walls[i].setNeighborSector(neighborSector);
                        portalCount++;
                    } else {
                        std::cerr << "[MapLoader] Warning: Sector " << sectorId 
                                  << " wall " << i << " references non-existent sector " 
                                  << neighborId << std::endl;
                    }
                }
            }
        }
        
        std::cout << "[MapLoader] Linked " << portalCount << " portals" << std::endl;
    }

    static bool parseJSON(const std::string& json, 
                         std::vector<SectorParseData>& sectors,
                         std::vector<std::string>& textures,
                         std::string& error) {
        size_t pos = 0;
        
        // Find "sectors" array
        size_t sectorsPos = json.find("\"sectors\"");
        if (sectorsPos == std::string::npos) {
            error = "No 'sectors' array found";
            return false;
        }

        // Find the opening bracket of sectors array
        size_t arrayStart = json.find('[', sectorsPos);
        if (arrayStart == std::string::npos) {
            error = "Invalid sectors array";
            return false;
        }

        // Parse each sector
        pos = arrayStart + 1;
        while (pos < json.length()) {
            skipWhitespace(json, pos);
            
            if (json[pos] == ']') break;
            if (json[pos] == ',') { pos++; continue; }
            
            if (json[pos] == '{') {
                SectorParseData sector;
                if (!parseSector(json, pos, sector, error)) {
                    return false;
                }
                sectors.push_back(sector);
            } else {
                pos++;
            }
        }

        // Parse textures array (optional)
        size_t texturesPos = json.find("\"textures\"");
        if (texturesPos != std::string::npos) {
            size_t texArrayStart = json.find('[', texturesPos);
            if (texArrayStart != std::string::npos) {
                parseStringArray(json, texArrayStart, textures);
            }
        }

        return true;
    }

    static void skipWhitespace(const std::string& json, size_t& pos) {
        while (pos < json.length() && std::isspace(json[pos])) pos++;
    }

    static std::string parseString(const std::string& json, size_t& pos) {
        if (json[pos] != '"') return "";
        pos++; // skip opening quote
        
        std::string result;
        while (pos < json.length() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.length()) {
                pos++;
                switch (json[pos]) {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case '\\': result += '\\'; break;
                    case '"': result += '"'; break;
                    default: result += json[pos]; break;
                }
            } else {
                result += json[pos];
            }
            pos++;
        }
        if (pos < json.length()) pos++; // skip closing quote
        return result;
    }

    static double parseNumber(const std::string& json, size_t& pos) {
        size_t start = pos;
        if (json[pos] == '-') pos++;
        while (pos < json.length() && (std::isdigit(json[pos]) || json[pos] == '.' || json[pos] == 'e' || json[pos] == 'E' || json[pos] == '+' || json[pos] == '-')) {
            if ((json[pos] == 'e' || json[pos] == 'E' || json[pos] == '+' || json[pos] == '-') && pos == start) break;
            pos++;
        }
        return std::stod(json.substr(start, pos - start));
    }

    static bool parseSector(const std::string& json, size_t& pos, SectorParseData& sector, std::string& error) {
        // Default values
        sector.id = -1;
        sector.floorHeight = 0.0f;
        sector.ceilingHeight = 3.0f;
        sector.lightLevel = 255;
        sector.flags = 0;

        if (json[pos] != '{') return false;
        pos++;

        while (pos < json.length() && json[pos] != '}') {
            skipWhitespace(json, pos);
            if (json[pos] == '}') break;
            if (json[pos] == ',') { pos++; continue; }

            std::string key = parseString(json, pos);
            skipWhitespace(json, pos);
            
            if (json[pos] != ':') { pos++; continue; }
            pos++; // skip colon
            skipWhitespace(json, pos);

            if (key == "id") {
                sector.id = static_cast<int>(parseNumber(json, pos));
            } else if (key == "floorHeight") {
                sector.floorHeight = static_cast<float>(parseNumber(json, pos));
            } else if (key == "ceilingHeight") {
                sector.ceilingHeight = static_cast<float>(parseNumber(json, pos));
            } else if (key == "lightLevel") {
                sector.lightLevel = static_cast<int>(parseNumber(json, pos));
            } else if (key == "floorTexture") {
                sector.floorTexture = parseString(json, pos);
            } else if (key == "ceilingTexture") {
                sector.ceilingTexture = parseString(json, pos);
            } else if (key == "flags") {
                sector.flags = static_cast<uint32_t>(parseNumber(json, pos));
            } else if (key == "walls") {
                if (!parseWallsArray(json, pos, sector.walls, error)) {
                    return false;
                }
            } else {
                // Skip unknown value
                skipValue(json, pos);
            }
        }

        if (pos < json.length() && json[pos] == '}') pos++;
        return true;
    }

    static bool parseWallsArray(const std::string& json, size_t& pos, std::vector<WallParseData>& walls, std::string& error) {
        skipWhitespace(json, pos);
        if (json[pos] != '[') return false;
        pos++;

        while (pos < json.length() && json[pos] != ']') {
            skipWhitespace(json, pos);
            if (json[pos] == ']') break;
            if (json[pos] == ',') { pos++; continue; }

            if (json[pos] == '{') {
                WallParseData wall;
                if (!parseWall(json, pos, wall, error)) {
                    return false;
                }
                walls.push_back(wall);
            } else {
                pos++;
            }
        }

        if (pos < json.length() && json[pos] == ']') pos++;
        return true;
    }

    static bool parseWall(const std::string& json, size_t& pos, WallParseData& wall, std::string& error) {
        wall.startX = wall.startY = 0;
        wall.endX = wall.endY = 0;
        wall.neighborId = -1;
        wall.flags = 0;

        if (json[pos] != '{') return false;
        pos++;

        while (pos < json.length() && json[pos] != '}') {
            skipWhitespace(json, pos);
            if (json[pos] == '}') break;
            if (json[pos] == ',') { pos++; continue; }

            std::string key = parseString(json, pos);
            skipWhitespace(json, pos);
            
            if (json[pos] != ':') { pos++; continue; }
            pos++;
            skipWhitespace(json, pos);

            if (key == "start") {
                std::vector<double> coords;
                parseNumberArray(json, pos, coords);
                if (coords.size() >= 2) {
                    wall.startX = static_cast<float>(coords[0]);
                    wall.startY = static_cast<float>(coords[1]);
                }
            } else if (key == "end") {
                std::vector<double> coords;
                parseNumberArray(json, pos, coords);
                if (coords.size() >= 2) {
                    wall.endX = static_cast<float>(coords[0]);
                    wall.endY = static_cast<float>(coords[1]);
                }
            } else if (key == "neighborId") {
                if (json[pos] == 'n') { // null
                    wall.neighborId = -1;
                    while (pos < json.length() && std::isalpha(json[pos])) pos++;
                } else {
                    wall.neighborId = static_cast<int>(parseNumber(json, pos));
                }
            } else if (key == "upperTexture") {
                wall.upperTexture = parseString(json, pos);
            } else if (key == "middleTexture") {
                wall.middleTexture = parseString(json, pos);
            } else if (key == "lowerTexture") {
                wall.lowerTexture = parseString(json, pos);
            } else if (key == "flags") {
                wall.flags = static_cast<uint32_t>(parseNumber(json, pos));
            } else {
                skipValue(json, pos);
            }
        }

        if (pos < json.length() && json[pos] == '}') pos++;
        return true;
    }

    static void parseNumberArray(const std::string& json, size_t& pos, std::vector<double>& result) {
        skipWhitespace(json, pos);
        if (json[pos] != '[') return;
        pos++;

        while (pos < json.length() && json[pos] != ']') {
            skipWhitespace(json, pos);
            if (json[pos] == ']') break;
            if (json[pos] == ',') { pos++; continue; }
            
            if (std::isdigit(json[pos]) || json[pos] == '-' || json[pos] == '.') {
                result.push_back(parseNumber(json, pos));
            } else {
                pos++;
            }
        }

        if (pos < json.length() && json[pos] == ']') pos++;
    }

    static void parseStringArray(const std::string& json, size_t& pos, std::vector<std::string>& result) {
        skipWhitespace(json, pos);
        if (json[pos] != '[') return;
        pos++;

        while (pos < json.length() && json[pos] != ']') {
            skipWhitespace(json, pos);
            if (json[pos] == ']') break;
            if (json[pos] == ',') { pos++; continue; }
            
            if (json[pos] == '"') {
                result.push_back(parseString(json, pos));
            } else {
                pos++;
            }
        }

        if (pos < json.length() && json[pos] == ']') pos++;
    }

    static void skipValue(const std::string& json, size_t& pos) {
        skipWhitespace(json, pos);
        
        if (json[pos] == '"') {
            parseString(json, pos);
        } else if (json[pos] == '[') {
            int depth = 1;
            pos++;
            while (pos < json.length() && depth > 0) {
                if (json[pos] == '[') depth++;
                else if (json[pos] == ']') depth--;
                pos++;
            }
        } else if (json[pos] == '{') {
            int depth = 1;
            pos++;
            while (pos < json.length() && depth > 0) {
                if (json[pos] == '{') depth++;
                else if (json[pos] == '}') depth--;
                pos++;
            }
        } else if (json[pos] == 'n' || json[pos] == 't' || json[pos] == 'f') {
            while (pos < json.length() && std::isalpha(json[pos])) pos++;
        } else if (std::isdigit(json[pos]) || json[pos] == '-') {
            parseNumber(json, pos);
        }
    }
};
