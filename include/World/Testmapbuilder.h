#pragma once
#include "SectorMap.h"
#include <iostream>
#include <vector>
#include <stack>
#include <random>
#include <ctime>

class TestMapBuilder {
public:
    static constexpr float UNIT = 64.0f;

    struct MazeCell {
        bool visited = false;
        bool walls[4] = {true, true, true, true}; // Top, Right, Bottom, Left
        int sectorId = -1;
    };

    static SectorMap generateRandomMap(int width = 10, int height = 10) {
        SectorMap map;
        const float CELL_SIZE = 4.0f * UNIT; // Bigger rooms
        
        std::cout << "[TestMapBuilder] Generating random map " << width << "x" << height << "..." << std::endl;
        
        // 1. Initialize Grid
        std::vector<MazeCell> grid(width * height);
        std::srand(static_cast<unsigned int>(std::time(nullptr)));

        // 2. Maze Generation (Recursive Backtracker)
        std::stack<int> stack;
        int current = 0;
        grid[current].visited = true;
        stack.push(current);

        while (!stack.empty()) {
            current = stack.top();
            
            // Find unvisited neighbors
            std::vector<int> neighbors;
            int cx = current % width;
            int cy = current / width;
            
            // Top
            if (cy > 0 && !grid[current - width].visited) neighbors.push_back(0);
            // Right
            if (cx < width - 1 && !grid[current + 1].visited) neighbors.push_back(1);
            // Bottom
            if (cy < height - 1 && !grid[current + width].visited) neighbors.push_back(2);
            // Left
            if (cx > 0 && !grid[current - 1].visited) neighbors.push_back(3);

            if (!neighbors.empty()) {
                // Choose random neighbor
                int dir = neighbors[std::rand() % neighbors.size()];
                int next = -1;

                if (dir == 0) { // Top
                    next = current - width;
                    grid[current].walls[0] = false;
                    grid[next].walls[2] = false;
                } else if (dir == 1) { // Right
                    next = current + 1;
                    grid[current].walls[1] = false;
                    grid[next].walls[3] = false;
                } else if (dir == 2) { // Bottom
                    next = current + width;
                    grid[current].walls[2] = false;
                    grid[next].walls[0] = false;
                } else if (dir == 3) { // Left
                    next = current - 1;
                    grid[current].walls[3] = false;
                    grid[next].walls[1] = false;
                }

                grid[next].visited = true;
                stack.push(next);
            } else {
                stack.pop();
            }
        }

        // 3. Convert to Sectors
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int index = y * width + x;
                // Random height variations
                float floorH = (std::rand() % 5) * 0.5f; // 0.0 to 2.0
                float ceilH = 4.0f + (std::rand() % 4) * 0.5f; // 4.0 to 5.5
                
                // Random sector color tint
                sf::Color floorColor(
                    150 + std::rand() % 100,
                    150 + std::rand() % 100,
                    150 + std::rand() % 100
                );
                
                Sector sector(index + 1, floorH, ceilH); // IDs start at 1
                sector.setFloorColor(floorColor);
                sector.setCeilingColor(sf::Color(50, 50, 60)); // Dark ceiling
                
                // Coordinates
                float px = x * CELL_SIZE;
                float py = y * CELL_SIZE;
                
                // Walls: Top, Right, Bottom, Left
                // Top Wall (y)
                if (grid[index].walls[0]) {
                    Wall w({px + CELL_SIZE, py}, {px, py});
                    w.setColor(sf::Color::White);
                    sector.addWall(w);
                } else {
                    int neighborId = (index - width) + 1;
                    Wall w({px + CELL_SIZE, py}, {px, py});
                    w.setColor(sf::Color::Red);
                    w.setNextSectorId(neighborId);
                    sector.addWall(w);
                }
                
                // Left Wall (x)
                if (grid[index].walls[3]) {
                    Wall w({px, py}, {px, py + CELL_SIZE});
                    w.setColor(sf::Color::White);
                    sector.addWall(w);
                } else {
                     int neighborId = (index - 1) + 1;
                    Wall w({px, py}, {px, py + CELL_SIZE});
                    w.setColor(sf::Color::Green);
                    w.setNextSectorId(neighborId);
                    sector.addWall(w);
                }
                
                // Bottom Wall (y + size) - Note: standard winding order
                if (grid[index].walls[2]) {
                    Wall w({px, py + CELL_SIZE}, {px + CELL_SIZE, py + CELL_SIZE});
                    w.setColor(sf::Color::White);
                    sector.addWall(w);
                } else {
                    int neighborId = (index + width) + 1;
                    Wall w({px, py + CELL_SIZE}, {px + CELL_SIZE, py + CELL_SIZE});
                    w.setColor(sf::Color::Blue);
                    w.setNextSectorId(neighborId);
                    sector.addWall(w);
                }

                // Right Wall (x + size)
                if (grid[index].walls[1]) {
                    Wall w({px + CELL_SIZE, py + CELL_SIZE}, {px + CELL_SIZE, py});
                    w.setColor(sf::Color::White);
                    sector.addWall(w);
                } else {
                    int neighborId = (index + 1) + 1;
                    Wall w({px + CELL_SIZE, py + CELL_SIZE}, {px + CELL_SIZE, py});
                    w.setColor(sf::Color::Yellow);
                    w.setNextSectorId(neighborId);
                    sector.addWall(w);
                }

                // Randomly mark roughly 1 in 50 sectors as an Exit (but ensure high index to be far)
                // Actually, just picking the very last valid sector is more reliable for "far" in this generation scheme
                if (index == width * height - 1) {
                    sector.setFloorColor(sf::Color::Green);
                    sector.setFlag(Sector::FLAG_EXIT);
                }
                
                map.addSector(std::move(sector));
                grid[index].sectorId = index + 1;
            }
        }
        
        // Resolve pointers
        for (auto& [id, sector] : map.getSectors()) {
            for (auto& wall : sector.getWalls()) {
               if (wall.getNextSectorId() != -1) {
                   wall.setNeighborSector(map.getSector(wall.getNextSectorId()));
               }
            }
        }
        
        std::cout << "[TestMapBuilder] Generated " << map.getSectorCount() << " sectors." << std::endl;
        return map;
    }

    static SectorMap buildSimpleStepMap() {
        SectorMap map;
        std::cout << "[TestMapBuilder] Building large sector map..." << std::endl;

        const float U = UNIT;

        // ================================================================
        // SECTOR 0 - Starting room (4x4 units, ground level)
        // ================================================================
        Sector sector0(0, 0.0f, 3.0f);
        sector0.setFloorTexture("floor_stone.png");
        sector0.setCeilingTexture("ceiling_metal.png");
        sector0.setLightLevel(255);  // Bright start area

        // Walls: North, East (portal), South (portal), West
        sector0.addWall(Wall(sf::Vector2f(0*U, 0*U), sf::Vector2f(4*U, 0*U)));   // North
        sector0.addWall(Wall(sf::Vector2f(4*U, 0*U), sf::Vector2f(4*U, 4*U)));   // East -> portal to 1
        sector0.addWall(Wall(sf::Vector2f(4*U, 4*U), sf::Vector2f(2.5f*U, 4*U)));// South-East
        sector0.addWall(Wall(sf::Vector2f(2.5f*U, 4*U), sf::Vector2f(1.5f*U, 4*U))); // South portal -> 3
        sector0.addWall(Wall(sf::Vector2f(1.5f*U, 4*U), sf::Vector2f(0*U, 4*U)));// South-West  
        sector0.addWall(Wall(sf::Vector2f(0*U, 4*U), sf::Vector2f(0*U, 0*U)));   // West

        map.addSector(sector0);

        Sector sector1(1, 0.4f, 3.0f);
        sector1.setFloorTexture("floor_tile.png");
        sector1.setCeilingTexture("ceiling_metal.png");
        sector1.setLightLevel(220);

        sector1.addWall(Wall(sf::Vector2f(4*U, 0*U), sf::Vector2f(8*U, 0*U)));
        sector1.addWall(Wall(sf::Vector2f(8*U, 0*U), sf::Vector2f(8*U, 4*U)));
        sector1.addWall(Wall(sf::Vector2f(8*U, 4*U), sf::Vector2f(6.5f*U, 4*U)));
        sector1.addWall(Wall(sf::Vector2f(6.5f*U, 4*U), sf::Vector2f(5.5f*U, 4*U)));
        sector1.addWall(Wall(sf::Vector2f(5.5f*U, 4*U), sf::Vector2f(4*U, 4*U)));
        sector1.addWall(Wall(sf::Vector2f(4*U, 4*U), sf::Vector2f(4*U, 0*U)));

        map.addSector(sector1);

        Sector sector2(2, 0.8f, 3.0f);
        sector2.setFloorTexture("floor_wood.png");
        sector2.setCeilingTexture("ceiling_wood.png");
        sector2.setLightLevel(200);

        sector2.addWall(Wall(sf::Vector2f(8*U, 0*U), sf::Vector2f(12*U, 0*U)));
        sector2.addWall(Wall(sf::Vector2f(12*U, 0*U), sf::Vector2f(12*U, 4*U)));
        sector2.addWall(Wall(sf::Vector2f(12*U, 4*U), sf::Vector2f(10.5f*U, 4*U)));
        sector2.addWall(Wall(sf::Vector2f(10.5f*U, 4*U), sf::Vector2f(9.5f*U, 4*U)));
        sector2.addWall(Wall(sf::Vector2f(9.5f*U, 4*U), sf::Vector2f(8*U, 4*U)));
        sector2.addWall(Wall(sf::Vector2f(8*U, 4*U), sf::Vector2f(8*U, 0*U)));

        map.addSector(sector2);

        Sector sector3(3, -0.5f, 2.5f);
        sector3.setFloorTexture("floor_dirt.png");
        sector3.setCeilingTexture("ceiling_rock.png");
        sector3.setLightLevel(120);

        sector3.addWall(Wall(sf::Vector2f(0*U, 4*U), sf::Vector2f(1.5f*U, 4*U)));
        sector3.addWall(Wall(sf::Vector2f(1.5f*U, 4*U), sf::Vector2f(2.5f*U, 4*U)));
        sector3.addWall(Wall(sf::Vector2f(2.5f*U, 4*U), sf::Vector2f(4*U, 4*U)));
        sector3.addWall(Wall(sf::Vector2f(4*U, 4*U), sf::Vector2f(4*U, 8*U)));
        sector3.addWall(Wall(sf::Vector2f(4*U, 8*U), sf::Vector2f(0*U, 8*U)));
        sector3.addWall(Wall(sf::Vector2f(0*U, 8*U), sf::Vector2f(0*U, 4*U)));

        map.addSector(sector3);

        Sector sector4(4, 0.0f, 4.0f);
        sector4.setFloorTexture("floor_marble.png");
        sector4.setCeilingTexture("ceiling_ornate.png");
        sector4.setLightLevel(255);

        sector4.addWall(Wall(sf::Vector2f(4*U, 4*U), sf::Vector2f(5.5f*U, 4*U)));
        sector4.addWall(Wall(sf::Vector2f(5.5f*U, 4*U), sf::Vector2f(6.5f*U, 4*U)));
        sector4.addWall(Wall(sf::Vector2f(6.5f*U, 4*U), sf::Vector2f(8*U, 4*U)));
        sector4.addWall(Wall(sf::Vector2f(8*U, 4*U), sf::Vector2f(8*U, 8*U)));
        sector4.addWall(Wall(sf::Vector2f(8*U, 8*U), sf::Vector2f(4*U, 8*U)));
        sector4.addWall(Wall(sf::Vector2f(4*U, 8*U), sf::Vector2f(4*U, 4*U)));

        map.addSector(sector4);

        Sector sector5(5, 0.6f, 3.0f);
        sector5.setFloorTexture("floor_metal.png");
        sector5.setCeilingTexture("ceiling_tech.png");
        sector5.setLightLevel(180);

        sector5.addWall(Wall(sf::Vector2f(8*U, 4*U), sf::Vector2f(9.5f*U, 4*U)));
        sector5.addWall(Wall(sf::Vector2f(9.5f*U, 4*U), sf::Vector2f(10.5f*U, 4*U)));
        sector5.addWall(Wall(sf::Vector2f(10.5f*U, 4*U), sf::Vector2f(12*U, 4*U)));
        sector5.addWall(Wall(sf::Vector2f(12*U, 4*U), sf::Vector2f(12*U, 8*U)));
        sector5.addWall(Wall(sf::Vector2f(12*U, 8*U), sf::Vector2f(8*U, 8*U)));
        sector5.addWall(Wall(sf::Vector2f(8*U, 8*U), sf::Vector2f(8*U, 4*U)));

        map.addSector(sector5);

        connectPortals(map);

        // Validate
        if (map.validate()) {
            std::cout << "[TestMapBuilder] Map is valid! (" << map.getSectorCount() << " sectors)" << std::endl;
        } else {
            std::cerr << "[TestMapBuilder] Map validation failed!" << std::endl;
        }

        return map;
    }

private:
    static void connectPortals(SectorMap& map) {
        Sector* s0 = map.getSector(0);
        Sector* s1 = map.getSector(1);
        Sector* s2 = map.getSector(2);
        Sector* s3 = map.getSector(3);
        Sector* s4 = map.getSector(4);
        Sector* s5 = map.getSector(5);

        if (!s0 || !s1 || !s2 || !s3 || !s4 || !s5) {
            std::cerr << "[TestMapBuilder] ERROR: Missing sectors for portal connection!" << std::endl;
            return;
        }

        s0->getWall(1).setNeighborSector(s1);
        s1->getWall(5).setNeighborSector(s0);

        s1->getWall(1).setNeighborSector(s2);
        s2->getWall(5).setNeighborSector(s1);

        s0->getWall(3).setNeighborSector(s3);
        s3->getWall(1).setNeighborSector(s0);

        s1->getWall(3).setNeighborSector(s4);
        s4->getWall(1).setNeighborSector(s1);

        s2->getWall(3).setNeighborSector(s5);
        s5->getWall(1).setNeighborSector(s2);

        s0->getWall(1).setLowerTexture("wall_step.png");
        s1->getWall(1).setLowerTexture("wall_step.png");
        s0->getWall(3).setUpperTexture("wall_brick.png");
        s3->getWall(1).setLowerTexture("wall_stone.png");

        std::cout << "[TestMapBuilder] All portals connected" << std::endl;
    }

public:

    static SectorMap buildWindowMap() {
        SectorMap map;
        std::cout << "[TestMapBuilder] Building window map..." << std::endl;

        Sector sector0(0, 0.0f, 2.5f);
        sector0.setLightLevel(200);
        
        sector0.addWall(Wall(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(5.0f, 0.0f)));
        sector0.addWall(Wall(sf::Vector2f(5.0f, 0.0f), sf::Vector2f(5.0f, 5.0f)));
        sector0.addWall(Wall(sf::Vector2f(5.0f, 5.0f), sf::Vector2f(0.0f, 5.0f)));
        sector0.addWall(Wall(sf::Vector2f(0.0f, 5.0f), sf::Vector2f(0.0f, 0.0f)));
        
        map.addSector(sector0);

        Sector sector1(1, 0.0f, 1.8f);
        sector1.setLightLevel(255);
        
        sector1.addWall(Wall(sf::Vector2f(5.0f, 0.0f), sf::Vector2f(10.0f, 0.0f)));
        sector1.addWall(Wall(sf::Vector2f(10.0f, 0.0f), sf::Vector2f(10.0f, 5.0f)));
        sector1.addWall(Wall(sf::Vector2f(10.0f, 5.0f), sf::Vector2f(5.0f, 5.0f)));
        sector1.addWall(Wall(sf::Vector2f(5.0f, 5.0f), sf::Vector2f(5.0f, 0.0f)));
        
        map.addSector(sector1);

        Sector sector2(2, 0.0f, 2.5f);
        sector2.setLightLevel(200);
        
        sector2.addWall(Wall(sf::Vector2f(10.0f, 0.0f), sf::Vector2f(15.0f, 0.0f)));
        sector2.addWall(Wall(sf::Vector2f(15.0f, 0.0f), sf::Vector2f(15.0f, 5.0f)));
        sector2.addWall(Wall(sf::Vector2f(15.0f, 5.0f), sf::Vector2f(10.0f, 5.0f)));
        sector2.addWall(Wall(sf::Vector2f(10.0f, 5.0f), sf::Vector2f(10.0f, 0.0f)));
        
        map.addSector(sector2);

        Sector* s0 = map.getSector(0);
        Sector* s1 = map.getSector(1);
        Sector* s2 = map.getSector(2);
        
        if (s0 && s1 && s2) {
            s0->getWall(1).setNeighborSector(s1);
            s1->getWall(3).setNeighborSector(s0);
            s1->getWall(1).setNeighborSector(s2);
            s2->getWall(3).setNeighborSector(s1);
            
            s0->getWall(1).setUpperTexture("wall_brick.png");
            s1->getWall(3).setUpperTexture("wall_brick.png");
            s1->getWall(1).setUpperTexture("wall_brick.png");
            s2->getWall(3).setUpperTexture("wall_brick.png");
        }

        map.validate();
        return map;
    }

    static SectorMap buildComplexMap() {
        SectorMap map;
        std::cout << "[TestMapBuilder] Building complex map..." << std::endl;
        return map;
    }

    static SectorMap buildEmptyMap() {
        SectorMap map;
        std::cout << "[TestMapBuilder] Created empty map" << std::endl;
        return map;
    }
};