/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MapLoader.h"

#include "GameMap.h"
#include "ODApplication.h"
#include "Goal.h"
#include "Creature.h"
#include "CreatureDefinition.h"
#include "Trap.h"
#include "Seat.h"
#include "MapLight.h"
#include "LogManager.h"

#include <iostream>
#include <sstream>

namespace MapLoader {

bool readGameMapFromFile(const std::string& fileName, GameMap& gameMap)
{
    // Try to open the input file for reading and throw an error if we can't.
    std::ifstream baseLevelFile(fileName.c_str(), std::ifstream::in);
    if (!baseLevelFile.good())
    {
        std::cerr << "ERROR: File not found:  " << fileName << "\n\n\n";
        return false;
    }

    // Read in the whole baseLevelFile, strip it of comments and feed it into
    // the stringstream levelFile, to be read by the rest of the function.
    std::stringstream levelFile;
    std::string nextParam;
    while (baseLevelFile.good())
    {
        std::getline(baseLevelFile, nextParam);
        /* Find the first occurrence of the comment symbol on the
         * line and return everything before that character.
         */
        levelFile << nextParam.substr(0, nextParam.find('#')) << "\n";
    }

    baseLevelFile.close();

    // Read in the version number from the level file
    levelFile >> nextParam;
    if (nextParam.compare(ODApplication::VERSIONSTRING) != 0)
    {
        std::cerr
                << "\n\n\nERROR:  Attempting to load a file produced by a different version of OpenDungeons.\n"
                << "ERROR:  Filename:  " << fileName
                << "\nERROR:  The file is for OpenDungeons:  " << nextParam
                << "\nERROR:  This version of OpenDungeons:  " << ODApplication::VERSION
                << "\n\n\n";
        return false;
    }

    // Read in the name of the next level to load after this one is complete.
    levelFile >> nextParam;
    if (nextParam == "[Next_Level]")
    {
        levelFile >> nextParam;
        gameMap.nextLevel = nextParam;
    }

    int objectsToLoad = 0;

    // TODO: Use an sub-function when encountering [Seats]
    levelFile >> nextParam;
    if (nextParam != "[Seats]")
    {
        std::cout << "Invalid seats start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    // Read in the seats from the level file
    //levelFile >> objectsToLoad;
    while (true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Seats]")
            break;

        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;
        //std::cout << entire_line << std::endl;

        Seat* tempSeat = new Seat;
        Seat::loadFromLine(entire_line, tempSeat);

        gameMap.addEmptySeat(tempSeat);
    }

    // Read in the goals that are shared by all players, the first player to complete all these goals is the winner.
    //levelFile >> objectsToLoad;
    levelFile >> nextParam;

    // TODO: Use an sub-function when encountering [Goals]
    if (nextParam != "[Goals]")
    {
        std::cout << "Invalid Goals start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    while(true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Goals]")
            break;

        Goal* tempGoal = Goal::instantiateFromStream(nextParam, levelFile);

        if (tempGoal != NULL)
            gameMap.addGoalForAllSeats(tempGoal);
    }

    //levelFile >> objectsToLoad;
    levelFile >> nextParam;

    // TODO: Use an sub-function when encountering [Tiles]
    if (nextParam != "[Tiles]")
    {
        std::cout << "Invalid tile start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    // Load the map size on next two lines
    int mapSizeX;
    int mapSizeY;
    levelFile >> mapSizeX;
    levelFile >> mapSizeY;

    if (!gameMap.createNewMap(mapSizeX, mapSizeY))
        return false;

    // Read in the map tiles from disk
    Tile tempTile;
    tempTile.setGameMap(&gameMap);

    gameMap.disableFloodFill();

    while (true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Tiles]")
            break;

        // TEMP: Get all the params together in order to prepare for the new parsing function
        // TODO: Later, we parse directly lines by lines.
        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;
        //std::cout << "Entire line: " << entire_line << std::endl;

        Tile::loadFromLine(entire_line, &tempTile);

        gameMap.addTile(tempTile);
    }

    gameMap.setAllFullnessAndNeighbors();
    gameMap.enableFloodFill();

    // Read in the rooms
    Room* tempRoom;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempRoom = Room::createRoomFromStream(levelFile, &gameMap);

        gameMap.addRoom(tempRoom);
    }

    // Read in the traps
    Trap* tempTrap;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempTrap = Trap::createTrapFromStream(levelFile, &gameMap);
        tempTrap->createMesh();

        gameMap.addTrap(tempTrap);
    }

    // Read in the lights
    MapLight* tempLight;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempLight = new MapLight;
	    tempLight->setGameMap(&gameMap);
        levelFile >> tempLight;

        gameMap.addMapLight(tempLight);
    }

    // Load the creatures defintions filename and file.
    levelFile >> nextParam;
    if (nextParam != "[Creatures_Definition]")
    {
        std::cout << "Invalid Creatures definition start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }
    levelFile >> nextParam;
    MapLoader::loadCreatureDefinition(nextParam, gameMap);
    /*
    // Read in the creature class descriptions
    CreatureDefinition* tempClass;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempClass = new CreatureDefinition;
        levelFile >> tempClass;

        gameMap.addClassDescription(tempClass);
    }
    */

    // Read in the actual creatures themselves
    Creature* tempCreature;
    Ogre::Vector3 tempVector;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {

        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and writeGameMapToFile.
        //Changes to this code should be reflected in that code as well
        tempCreature = new Creature(&gameMap);
        levelFile >> tempCreature;

        gameMap.addCreature(tempCreature);
    }

    return true;
}

void writeGameMapToFile(const std::string& fileName, GameMap& gameMap)
{
    std::ofstream levelFile(fileName.c_str(), std::ifstream::out);
    Tile *tempTile;

    // Write the identifier string and the version number
    levelFile << ODApplication::VERSIONSTRING
            << "  # The version of OpenDungeons which created this file (for compatibility reasons).\n";

    // write out the name of the next level to load after this one is complete.
    levelFile << "[Next_Level]" << std::endl;
    levelFile << gameMap.nextLevel
            << " # The level to load after this level is complete.\n";

    // Write out the seats to the file
    levelFile << "\n[Seats]\n\n";
    levelFile << "# " << Seat::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap.numEmptySeats(); ++i)
    {
        levelFile << gameMap.getEmptySeat(i);
    }

    for (unsigned int i = 0; i < gameMap.numFilledSeats(); ++i)
    {
        levelFile << gameMap.getFilledSeat(i);
    }
    levelFile << "[/Seats]" << std::endl;

    // Write out the goals shared by all players to the file.
    levelFile << "\n[Goals]\n\n";
    levelFile << "# " << Goal::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numGoalsForAllSeats(); i < num; ++i)
    {
        levelFile << gameMap.getGoalForAllSeats(i);
    }
    levelFile << "[/Goals]" << std::endl;

    levelFile << "\n[Tiles]\n";
    int mapSizeX = gameMap.getMapSizeX();
    int mapSizeY = gameMap.getMapSizeY();
    levelFile << "# Map Size" << std::endl;
    levelFile << mapSizeX << " # MapSizeX" << std::endl;
    levelFile << mapSizeY << " # MapSizeY" << std::endl;

    // Write out the tiles to the file
    levelFile << "# " << Tile::getFormat() << "\n";

    for(int ii = 0; ii < gameMap.getMapSizeX(); ++ii)
    {
        for(int jj = 0; jj < gameMap.getMapSizeY(); ++jj)
        {
            tempTile = gameMap.getTile(ii, jj);
            // Don't save standard tiles as they're auto filled in at load time.
            if (tempTile->getType() == Tile::dirt && tempTile->getFullness() >= 100.0)
                continue;

            levelFile << tempTile->x << "\t" << tempTile->y << "\t";
            levelFile << tempTile->getType() << "\t" << tempTile->getFullness();

            levelFile << std::endl;
        }
    }
    levelFile << "[/Tiles]" << std::endl;

    // Write out the rooms to the file
    levelFile << "\n# Rooms\n" << gameMap.numRooms()
            << "  # The number of rooms to load.\n";
    levelFile << "# " << Room::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numRooms(); i < num; ++i)
    {
        levelFile << gameMap.getRoom(i) << std::endl;
    }

    // Write out the traps to the file
    levelFile << "\n# Traps\n" << gameMap.numTraps()
            << "  # The number of traps to load.\n";
    levelFile << "# " << Trap::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap.numTraps(); ++i)
    {
        levelFile << gameMap.getTrap(i) << std::endl;
    }

    // Write out the lights to the file.
    levelFile << "\n# Lights\n" << gameMap.numMapLights()
            << "  # The number of lights to load.\n";
    levelFile << "# " << MapLight::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numMapLights(); i < num; ++i)
    {
        levelFile << gameMap.getMapLight(i) << std::endl;
    }

    // Write out where to find the creatures definition file.
    levelFile << "\n# The file containing the creatures definition." << std::endl
    << "[Creatures_Definition]" << std::endl
    << gameMap.getCreatureDefinitionFileName() << std::endl;

    // Write out the individual creatures to the file
    levelFile << "\n# Creatures\n" << gameMap.numCreatures()
            << "  # The number of creatures to load.\n";
    levelFile << "# " << Creature::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numCreatures(); i < num; ++i)
    {
        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and readGameMapFromFile.
        //Changes to this code should be reflected in that code as well
        levelFile << gameMap.getCreature(i) << std::endl;
    }

    levelFile << std::endl;

    levelFile.close();
}

bool loadCreatureDefinition(const std::string& fileName, GameMap& gameMap)
{
    // First, clear the previous creature definitions
    gameMap.clearClasses();
    gameMap.setCreatureDefinitionFileName(fileName);

    std::cout << "Load creature definition file: " << fileName << std::endl;

    // Try to open the input file for reading and throw an error if we can't.
    std::ifstream creatureDefFile(fileName.c_str(), std::ifstream::in);
    if (!creatureDefFile.good())
    {
        std::cout << "ERROR: Creature definition file not found:  " << fileName << "\n\n\n";
        return false;
    }

    // Read in the whole creatureDefFile, strip it of comments and feed it into
    // the stringstream defFile, to be read by the rest of the function.
    std::stringstream defFile;
    std::string nextParam;
    while (creatureDefFile.good())
    {
        std::getline(creatureDefFile, nextParam);
        /* Find the first occurrence of the comment symbol on the
         * line and return everything before that character.
         */
        defFile << nextParam.substr(0, nextParam.find('#')) << "\n";
    }

    creatureDefFile.close();

    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Creatures_definition]")
    {
        std::cout << "Invalid Creature classes start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    while(true)
    {
        defFile >> nextParam;
        if (nextParam == "[/Creatures_definition]")
            break;

        std::string entire_line = nextParam;
        std::getline(defFile, nextParam);
        entire_line += nextParam;
        //std::cout << entire_line << std::endl;

        CreatureDefinition* tempClass = new CreatureDefinition;
        CreatureDefinition::loadFromLine(entire_line, tempClass);

        gameMap.addClassDescription(tempClass);
    }
}

} // Namespace MapLoader
