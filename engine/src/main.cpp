#include <iostream>

#include "david/EngineMaster.h"

int main (int argc, char * argv[])
{
  david::EngineMaster em("float_ANNFile_6_83_1_1497360313.net");

  int mainEngineID      = em.spawnEngine();
  int opponentEngineID  = em.spawnEngine();
  int battleID          = em.battle(mainEngineID, opponentEngineID);

  if (em.battleWinner(battleID, mainEngineID)) {
    std::cout << mainEngineID << std::endl;
    //em.trainUntilWinner(mainEngineID, opponentEngineID);
  }
  std::cout << "lost" << std::endl;

  // Close program with exit code 0 after all threads have joined.
  return 0;
}

