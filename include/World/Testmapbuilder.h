#pragma once
#include "SectorMap.h"
#include <iostream>

/**
 * @brief Test map builder for sector engine
 * 
 * Creates test maps to demonstrate sector functionality.
 * All coordinates use world units (1 unit = 64 pixels typically).
 * 
 * Doom-style sector engine conventions:
 * - Sectors are convex polygons defined by walls
 * - Walls are line segments with start/end points
 * - Portals connect sectors (walls with neighborSector set)
 * - Heights are in "world units" (0.5 = half a wall height)
 */
class TestMapBuilder {
public:
    // Scale factor - multiply by this for world coordinates
    static constexpr float UNIT = 64.0f;  // 1 map unit = 64 pixels

    /**
     * @brief Build a larger multi-room map (Doom E1M1 inspired)
     * 
     * Layout (each cell = 4x4 units = 256x256 pixels):
     * 
     *    +--------+--------+--------+
     *    |        |        |        |
     *    |   0    |   1    |   2    |   <- Main corridor
     *    | START  | Step   | Step   |
     *    +---++---+---++---+---++---+
     *        ||       ||       ||
     *    +---++---+---++---+---++---+
     *    |        |        |        |
     *    |   3    |   4    |   5    |   <- South rooms
     *    |        | PILLAR |        |
     *    +--------+--------+--------+
     * 
     * Sector heights:
     * - 0: Ground (0.0)
     * - 1: Step up (0.3)
     * - 2: Step up (0.6)
     * - 3: Lowered (-0.3)
     * - 4: Ground (0.0) with pillar in center
     * - 5: Raised (0.5)
     */
    static SectorMap buildSimpleStepMap() {
        SectorMap map;
        std::cout << "[TestMapBuilder] Building large sector map..." << std::endl;

        const float U = UNIT;  // Shorthand

        // ================================================================
        // SECTOR 0 - Starting room (4x4 units, ground level)
        // ================================================================
        Sector sector0(0, 0.0f, 3.0f);
        sector0.setFloorTexture("floor_stone.png");
        sector0.setCeilingTexture("ceiling_metal.png");
        sector0.setLightLevel(220);

        // Walls: North, East (portal), South (portal), West
        sector0.addWall(Wall(sf::Vector2f(0*U, 0*U), sf::Vector2f(4*U, 0*U)));   // North
        sector0.addWall(Wall(sf::Vector2f(4*U, 0*U), sf::Vector2f(4*U, 4*U)));   // East -> portal to 1
        sector0.addWall(Wall(sf::Vector2f(4*U, 4*U), sf::Vector2f(2.5f*U, 4*U)));// South-East
        sector0.addWall(Wall(sf::Vector2f(2.5f*U, 4*U), sf::Vector2f(1.5f*U, 4*U))); // South portal -> 3
        sector0.addWall(Wall(sf::Vector2f(1.5f*U, 4*U), sf::Vector2f(0*U, 4*U)));// South-West  
        sector0.addWall(Wall(sf::Vector2f(0*U, 4*U), sf::Vector2f(0*U, 0*U)));   // West

        map.addSector(sector0);

        // ================================================================
        // SECTOR 1 - Middle corridor (4x4 units, raised floor)
        // ================================================================
        Sector sector1(1, 0.3f, 3.0f);  // Slightly raised
        sector1.setFloorTexture("floor_tile.png");
        sector1.setCeilingTexture("ceiling_metal.png");
        sector1.setLightLevel(200);

        sector1.addWall(Wall(sf::Vector2f(4*U, 0*U), sf::Vector2f(8*U, 0*U)));   // North
        sector1.addWall(Wall(sf::Vector2f(8*U, 0*U), sf::Vector2f(8*U, 4*U)));   // East -> portal to 2
        sector1.addWall(Wall(sf::Vector2f(8*U, 4*U), sf::Vector2f(6.5f*U, 4*U)));// South-East
        sector1.addWall(Wall(sf::Vector2f(6.5f*U, 4*U), sf::Vector2f(5.5f*U, 4*U))); // South portal -> 4
        sector1.addWall(Wall(sf::Vector2f(5.5f*U, 4*U), sf::Vector2f(4*U, 4*U)));// South-West
        sector1.addWall(Wall(sf::Vector2f(4*U, 4*U), sf::Vector2f(4*U, 0*U)));   // West -> portal to 0

        map.addSector(sector1);

        // ================================================================
        // SECTOR 2 - East corridor (4x4 units, more raised)
        // ================================================================
        Sector sector2(2, 0.6f, 3.0f);  // Higher floor
        sector2.setFloorTexture("floor_wood.png");
        sector2.setCeilingTexture("ceiling_wood.png");
        sector2.setLightLevel(180);

        sector2.addWall(Wall(sf::Vector2f(8*U, 0*U), sf::Vector2f(12*U, 0*U)));  // North
        sector2.addWall(Wall(sf::Vector2f(12*U, 0*U), sf::Vector2f(12*U, 4*U))); // East (solid)
        sector2.addWall(Wall(sf::Vector2f(12*U, 4*U), sf::Vector2f(10.5f*U, 4*U)));// South-East
        sector2.addWall(Wall(sf::Vector2f(10.5f*U, 4*U), sf::Vector2f(9.5f*U, 4*U))); // South portal -> 5
        sector2.addWall(Wall(sf::Vector2f(9.5f*U, 4*U), sf::Vector2f(8*U, 4*U)));// South-West
        sector2.addWall(Wall(sf::Vector2f(8*U, 4*U), sf::Vector2f(8*U, 0*U)));   // West -> portal to 1

        map.addSector(sector2);

        // ================================================================
        // SECTOR 3 - South-West room (lowered, darker)
        // ================================================================
        Sector sector3(3, -0.3f, 2.5f);  // Lowered floor, lower ceiling
        sector3.setFloorTexture("floor_dirt.png");
        sector3.setCeilingTexture("ceiling_rock.png");
        sector3.setLightLevel(140);  // Darker

        sector3.addWall(Wall(sf::Vector2f(0*U, 4*U), sf::Vector2f(1.5f*U, 4*U)));// North-West
        sector3.addWall(Wall(sf::Vector2f(1.5f*U, 4*U), sf::Vector2f(2.5f*U, 4*U))); // North portal -> 0
        sector3.addWall(Wall(sf::Vector2f(2.5f*U, 4*U), sf::Vector2f(4*U, 4*U)));// North-East
        sector3.addWall(Wall(sf::Vector2f(4*U, 4*U), sf::Vector2f(4*U, 8*U)));   // East
        sector3.addWall(Wall(sf::Vector2f(4*U, 8*U), sf::Vector2f(0*U, 8*U)));   // South
        sector3.addWall(Wall(sf::Vector2f(0*U, 8*U), sf::Vector2f(0*U, 4*U)));   // West

        map.addSector(sector3);

        // ================================================================
        // SECTOR 4 - Central hall (larger room with pillar area)
        // ================================================================
        Sector sector4(4, 0.0f, 3.5f);  // Taller ceiling
        sector4.setFloorTexture("floor_marble.png");
        sector4.setCeilingTexture("ceiling_ornate.png");
        sector4.setLightLevel(255);  // Bright

        sector4.addWall(Wall(sf::Vector2f(4*U, 4*U), sf::Vector2f(5.5f*U, 4*U)));// North-West
        sector4.addWall(Wall(sf::Vector2f(5.5f*U, 4*U), sf::Vector2f(6.5f*U, 4*U))); // North portal -> 1
        sector4.addWall(Wall(sf::Vector2f(6.5f*U, 4*U), sf::Vector2f(8*U, 4*U)));// North-East
        sector4.addWall(Wall(sf::Vector2f(8*U, 4*U), sf::Vector2f(8*U, 8*U)));   // East
        sector4.addWall(Wall(sf::Vector2f(8*U, 8*U), sf::Vector2f(4*U, 8*U)));   // South
        sector4.addWall(Wall(sf::Vector2f(4*U, 8*U), sf::Vector2f(4*U, 4*U)));   // West

        map.addSector(sector4);

        // ================================================================
        // SECTOR 5 - South-East room (raised platform)
        // ================================================================
        Sector sector5(5, 0.5f, 3.0f);  // Raised floor
        sector5.setFloorTexture("floor_metal.png");
        sector5.setCeilingTexture("ceiling_tech.png");
        sector5.setLightLevel(190);

        sector5.addWall(Wall(sf::Vector2f(8*U, 4*U), sf::Vector2f(9.5f*U, 4*U)));// North-West
        sector5.addWall(Wall(sf::Vector2f(9.5f*U, 4*U), sf::Vector2f(10.5f*U, 4*U))); // North portal -> 2
        sector5.addWall(Wall(sf::Vector2f(10.5f*U, 4*U), sf::Vector2f(12*U, 4*U)));// North-East
        sector5.addWall(Wall(sf::Vector2f(12*U, 4*U), sf::Vector2f(12*U, 8*U))); // East
        sector5.addWall(Wall(sf::Vector2f(12*U, 8*U), sf::Vector2f(8*U, 8*U)));  // South
        sector5.addWall(Wall(sf::Vector2f(8*U, 8*U), sf::Vector2f(8*U, 4*U)));   // West

        map.addSector(sector5);

        // ================================================================
        // CONNECT PORTALS
        // ================================================================
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
    /**
     * @brief Connect portal walls between sectors
     */
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

        // Sector 0 <-> Sector 1 (East wall of 0, West wall of 1)
        s0->getWall(1).setNeighborSector(s1);  // 0's East wall -> 1
        s1->getWall(5).setNeighborSector(s0);  // 1's West wall -> 0

        // Sector 1 <-> Sector 2 (East wall of 1, West wall of 2)
        s1->getWall(1).setNeighborSector(s2);  // 1's East wall -> 2
        s2->getWall(5).setNeighborSector(s1);  // 2's West wall -> 1

        // Sector 0 <-> Sector 3 (South portal of 0, North portal of 3)
        s0->getWall(3).setNeighborSector(s3);  // 0's South portal -> 3
        s3->getWall(1).setNeighborSector(s0);  // 3's North portal -> 0

        // Sector 1 <-> Sector 4 (South portal of 1, North portal of 4)
        s1->getWall(3).setNeighborSector(s4);  // 1's South portal -> 4
        s4->getWall(1).setNeighborSector(s1);  // 4's North portal -> 1

        // Sector 2 <-> Sector 5 (South portal of 2, North portal of 5)
        s2->getWall(3).setNeighborSector(s5);  // 2's South portal -> 5
        s5->getWall(1).setNeighborSector(s2);  // 5's North portal -> 2

        // Set textures for step transitions
        s0->getWall(1).setLowerTexture("wall_step.png");  // Step up to sector 1
        s1->getWall(1).setLowerTexture("wall_step.png");  // Step up to sector 2
        s0->getWall(3).setUpperTexture("wall_brick.png"); // Step down to sector 3
        s3->getWall(1).setLowerTexture("wall_stone.png"); // Step up from sector 3

        std::cout << "[TestMapBuilder] All portals connected" << std::endl;
    }

public:

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