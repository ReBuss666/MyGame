#pragma once
#include "SectorMap.h"
#include <iostream>

/**
 * @brief Test map builder for sector engine
 * 
 * Creates simple test maps to demonstrate sector functionality.
 */
class TestMapBuilder {
public:
    /**
     * @brief Build a simple 2-sector map with a step
     * 
     * Layout:
     * +-------+-------+
     * |       |       |
     * |   0   |   1   |  Sector 1 has higher floor
     * |       |       |
     * +-------+-------+
     */
    static SectorMap buildSimpleStepMap() {
        SectorMap map;
        std::cout << "[TestMapBuilder] Building simple step map..." << std::endl;

        // Sector 0 (ground level)
        Sector sector0(0, 0.0f, 2.5f);
        sector0.setFloorTexture("floor_stone.png");
        sector0.setCeilingTexture("ceiling_wood.png");
        sector0.setLightLevel(200);

        // Sector 0 walls
        Wall wall0_0(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(5.0f, 0.0f));
        wall0_0.setMiddleTexture("wall_brick.png");
        
        Wall wall0_1(sf::Vector2f(5.0f, 0.0f), sf::Vector2f(5.0f, 5.0f));
        // This is a portal to sector 1
        wall0_1.setUpperTexture("wall_brick.png");
        wall0_1.setLowerTexture("wall_stone.png"); // Step up
        
        Wall wall0_2(sf::Vector2f(5.0f, 5.0f), sf::Vector2f(0.0f, 5.0f));
        wall0_2.setMiddleTexture("wall_brick.png");
        
        Wall wall0_3(sf::Vector2f(0.0f, 5.0f), sf::Vector2f(0.0f, 0.0f));
        wall0_3.setMiddleTexture("wall_brick.png");

        sector0.addWall(wall0_0);
        sector0.addWall(wall0_1);
        sector0.addWall(wall0_2);
        sector0.addWall(wall0_3);

        map.addSector(sector0);

        // Sector 1 (raised platform)
        Sector sector1(1, 0.5f, 2.5f); // Floor 0.5m higher
        sector1.setFloorTexture("floor_wood.png");
        sector1.setCeilingTexture("ceiling_wood.png");
        sector1.setLightLevel(220);

        // Sector 1 walls
        Wall wall1_0(sf::Vector2f(5.0f, 0.0f), sf::Vector2f(10.0f, 0.0f));
        wall1_0.setMiddleTexture("wall_brick.png");
        
        Wall wall1_1(sf::Vector2f(10.0f, 0.0f), sf::Vector2f(10.0f, 5.0f));
        wall1_1.setMiddleTexture("wall_brick.png");
        
        Wall wall1_2(sf::Vector2f(10.0f, 5.0f), sf::Vector2f(5.0f, 5.0f));
        wall1_2.setMiddleTexture("wall_brick.png");
        
        Wall wall1_3(sf::Vector2f(5.0f, 5.0f), sf::Vector2f(5.0f, 0.0f));
        // This is a portal back to sector 0
        wall1_3.setUpperTexture("wall_brick.png");
        wall1_3.setLowerTexture("wall_stone.png"); // Step down

        sector1.addWall(wall1_0);
        sector1.addWall(wall1_1);
        sector1.addWall(wall1_2);
        sector1.addWall(wall1_3);

        map.addSector(sector1);

        // Connect portals
        Sector* s0 = map.getSector(0);
        Sector* s1 = map.getSector(1);
        
        if (s0 && s1) {
            s0->getWall(1).setNeighborSector(s1); // Sector 0, wall 1 → Sector 1
            s1->getWall(3).setNeighborSector(s0); // Sector 1, wall 3 → Sector 0
            std::cout << "[TestMapBuilder] Portals connected" << std::endl;
        }

        // Validate
        if (map.validate()) {
            std::cout << "[TestMapBuilder] Map is valid!" << std::endl;
        } else {
            std::cerr << "[TestMapBuilder] Map validation failed!" << std::endl;
        }

        return map;
    }

    /**
     * @brief Build a 3-sector map with window
     * 
     * Layout:
     * +-------+-------+-------+
     * |       |       |       |
     * |   0   |   1   |   2   |  Sector 1 has lower ceiling (window)
     * |       |       |       |
     * +-------+-------+-------+
     */
    static SectorMap buildWindowMap() {
        SectorMap map;
        std::cout << "[TestMapBuilder] Building window map..." << std::endl;

        // Sector 0
        Sector sector0(0, 0.0f, 2.5f);
        sector0.setLightLevel(200);
        
        sector0.addWall(Wall(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(5.0f, 0.0f)));
        sector0.addWall(Wall(sf::Vector2f(5.0f, 0.0f), sf::Vector2f(5.0f, 5.0f)));
        sector0.addWall(Wall(sf::Vector2f(5.0f, 5.0f), sf::Vector2f(0.0f, 5.0f)));
        sector0.addWall(Wall(sf::Vector2f(0.0f, 5.0f), sf::Vector2f(0.0f, 0.0f)));
        
        map.addSector(sector0);

        // Sector 1 (low ceiling = window)
        Sector sector1(1, 0.0f, 1.8f); // Lower ceiling
        sector1.setLightLevel(255); // Bright (window)
        
        sector1.addWall(Wall(sf::Vector2f(5.0f, 0.0f), sf::Vector2f(10.0f, 0.0f)));
        sector1.addWall(Wall(sf::Vector2f(10.0f, 0.0f), sf::Vector2f(10.0f, 5.0f)));
        sector1.addWall(Wall(sf::Vector2f(10.0f, 5.0f), sf::Vector2f(5.0f, 5.0f)));
        sector1.addWall(Wall(sf::Vector2f(5.0f, 5.0f), sf::Vector2f(5.0f, 0.0f)));
        
        map.addSector(sector1);

        // Sector 2
        Sector sector2(2, 0.0f, 2.5f);
        sector2.setLightLevel(200);
        
        sector2.addWall(Wall(sf::Vector2f(10.0f, 0.0f), sf::Vector2f(15.0f, 0.0f)));
        sector2.addWall(Wall(sf::Vector2f(15.0f, 0.0f), sf::Vector2f(15.0f, 5.0f)));
        sector2.addWall(Wall(sf::Vector2f(15.0f, 5.0f), sf::Vector2f(10.0f, 5.0f)));
        sector2.addWall(Wall(sf::Vector2f(10.0f, 5.0f), sf::Vector2f(10.0f, 0.0f)));
        
        map.addSector(sector2);

        // Connect portals
        Sector* s0 = map.getSector(0);
        Sector* s1 = map.getSector(1);
        Sector* s2 = map.getSector(2);
        
        if (s0 && s1 && s2) {
            s0->getWall(1).setNeighborSector(s1);
            s1->getWall(3).setNeighborSector(s0);
            s1->getWall(1).setNeighborSector(s2);
            s2->getWall(3).setNeighborSector(s1);
            
            // Set textures for window effect
            s0->getWall(1).setUpperTexture("wall_brick.png"); // Above window
            s1->getWall(3).setUpperTexture("wall_brick.png");
            s1->getWall(1).setUpperTexture("wall_brick.png");
            s2->getWall(3).setUpperTexture("wall_brick.png");
        }

        map.validate();
        return map;
    }

    /**
     * @brief Build a complex test map with multiple features
     * 
     * Features:
     * - Multiple sectors
     * - Steps up and down
     * - Window
     * - Different lighting
     */
    static SectorMap buildComplexMap() {
        SectorMap map;
        std::cout << "[TestMapBuilder] Building complex map..." << std::endl;

        // TODO: Implement more complex map
        // This would include:
        // - Multiple height levels
        // - Stairs (multiple small steps)
        // - Outdoor areas (sky ceiling)
        // - Secret areas

        return map;
    }

    /**
     * @brief Build an empty map for custom editing
     */
    static SectorMap buildEmptyMap() {
        SectorMap map;
        std::cout << "[TestMapBuilder] Created empty map" << std::endl;
        return map;
    }
};