#include "includes.hpp"

int main() {
    //load config
    ConfigLoader* cl = new ConfigLoader();
    int prevRadarSize;
    bool newRadarSize;
    int prevRadarPos;
    int secCounter;

    //basic checks
    if (getuid()) { std::cout << "[ ERROR ] Run program using as root (sudo)\n" << std::flush; return -1; }
    if (mem::GetPID() == 0) { std::cout << "[ ERROR ] Game not found (using r5apex.exe). Open the game first!\n" << std::flush; return -1; }

    //create basic objects
    XDisplay* display = new XDisplay();
    Level* level = new Level();
    LocalPlayer* localPlayer = new LocalPlayer();
    std::vector<Player*>* humanPlayers = new std::vector<Player*>;
    std::vector<Player*>* dummyPlayers = new std::vector<Player*>;
    std::vector<Player*>* players = new std::vector<Player*>;

    //fill in slots for players, dummies and items
    for (int i = 0; i < 70; i++) humanPlayers->push_back(new Player(i, localPlayer));
    for (int i = 0; i < 15000; i++) dummyPlayers->push_back(new Player(i, localPlayer));

    //create features     
    NoRecoil* noRecoil = new NoRecoil(cl, display, level, localPlayer);
    //Aim* Aimbot = new Aim(cl, display, localPlayer, players);
    AimBot* Aimbot = new AimBot(cl, display, level, localPlayer, players);
    TriggerBot* triggerBot = new TriggerBot(cl, display, level, localPlayer, players);
    Sense* sense = new Sense(cl, display, level, localPlayer, players);
    Radar* radar = new Radar(cl, display, level, localPlayer, players);
    Random* randyRandom = new Random(cl, display, level, localPlayer, players);

    //begin main loop
    int counter = 0;

    while(1) {
        try {
            //record time so we know how long a single loop iteration takes
            long long startTime = util::currentEpochMillis();

            // will attempt to reload config if there have been any updates to it
            if (counter % 20 == 0) {
                prevRadarSize = cl->RADAR_SIZE;
                prevRadarPos = cl->RADAR_POSITION;
                cl->reloadFile();
                if (prevRadarSize != cl->RADAR_SIZE || prevRadarPos != cl->RADAR_POSITION) {
                    //printf("Resizing and Moving Window to %d...\n", cl->RADAR_POSITION);
                    radar->resizeWindow();
                    radar->moveWindow();
                } 
            }

            //read level and make sure it is playable
            level->readFromMemory();
            if (!level->playable) {
                //printf("[ INFO  ] Not in game! Sleeping 10 seconds...\n");
                secCounter = secCounter + 10;
                std::cout << "\r" 
                    <<  "[ INFO  ] Player in Lobby - Waiting for the game to start...                                "
                    << std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(10000));
                continue;
            }

            //read localPlayer and make sure he is valid
            localPlayer->readFromMemory();
            if (!localPlayer->isValid()) throw std::invalid_argument("Selecting Legend");

            //read players
            players->clear();
            if (level->trainingArea)
                for (int i = 0; i < dummyPlayers->size(); i++) {
                    Player* p = dummyPlayers->at(i);
                    p->readFromMemory(cl);
                    if (p->isValid()) players->push_back(p);
                }
            else
                for (int i = 0; i < humanPlayers->size(); i++) {
                    Player* p = humanPlayers->at(i);
                    p->readFromMemory(cl);
                    if (p->isValid()) players->push_back(p);
                    p->MapRadar(cl, display);
                }

            //run features
            noRecoil->controlWeapon(counter);
            triggerBot->shootAtEnemy(counter);
            Aimbot->aimAssist(counter);
            sense->update(counter);
            radar->processEvents(counter);
            radar->repaint();
            //Random
            randyRandom->printLevels();
            randyRandom->quickTurn();
            randyRandom->superGlide();
            randyRandom->spectatorView();
            randyRandom->SkinChange();

            //check how fast we completed all the processing and if we still have time left to sleep
            int processingTime = static_cast<int>(util::currentEpochMillis() - startTime);
            int goalSleepTime = 6; // 16.67ms=60HZ | 6.97ms=144HZ
            int timeLeftToSleep = std::max(0, goalSleepTime - processingTime);
            std::this_thread::sleep_for(std::chrono::milliseconds(timeLeftToSleep));

            //print loop info every now and then
            if (counter % 500 == 0) {
                if (processingTime >= 20) {
                    std::cout << "\r" 
                        <<  "[ INFO  ] [" 
                        << std::setw(4) << counter << "] \033[32mGAME STARTED\033[0m - Processing Time: \033[31m" 
                        << std::setw(2) << processingTime << "ms\033[0m                                               "
                        << std::flush;
                } else {
                        std::cout << "\r" 
                        <<  "[ INFO  ] [" 
                        << std::setw(4) << counter << "] \033[32mGAME STARTED\033[0m - Processing Time: \033[32m" 
                        << std::setw(2) << processingTime << "ms\033[0m                                               "
                        << std::flush;
                }
            }
            //update counter
            counter = (counter < 1000) ? ++counter : counter = 0;
        }
        catch (std::invalid_argument& e) {
                std::cout << "\r" 
                    << "[ INFO  ] " << e.what() << " - Waiting for the game to start...                                         "
                    << std::flush;
                std::this_thread::sleep_for(std::chrono::seconds(5));

        }
        catch (...) {
            //printf("\n[ ERROR ] Unknown Error - Sleeping... \n");
            std::cout << "\r" 
                    << "[ ERROR ] Unknown Error - Sleeping...                                                "
                    << std::flush;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

}

