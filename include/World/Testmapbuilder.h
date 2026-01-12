#pragma once
#include "SectorMap.h"
#include <iostream>

class TestMapBuilder {
public:
    static constexpr float UNIT = 64.0f;

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