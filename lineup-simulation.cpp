#include <iostream>
#include <iomanip>
#include "lineup-simulation-types.h"
#include "random.h"
#include "console.h"
using namespace std;

// Constants
static const int kNumGames = 1000000;
static const int kNumBases = 3;
static const int kNumInningsPerGame = 9;
static const int kNumPlayers = 9;
static const int kNumOutsPerInning = 3;

// Prototypes
static void initializeBattingOrder(battingOrder &lineup);
static int simulateGame(const battingOrder &lineup);
static void simulateInning(const battingOrder &lineup, int &runs, int &battingPosition);
static plateAppearanceResult determinePlateAppearanceResult(const player &p);
static void updateStatus(const plateAppearanceResult &result, Bases &bases, int &runs, int &outs);
static void implementWalk(Bases &bases, int &runs);
static int getNumBases(const plateAppearanceResult &result);
static void implementHit(int numBases, Bases &bases, int &runs);
static void moveBaserunner(Bases &bases, int numBases, int &runs, int initialBase);
static void clearBases(Bases &bases);
static void outputResults(int* runs);

/**
 * @brief main simulates kNumGames 9-inning games with a particular batting order and outputs
 * the results to a file.
 * @return error code
 */

int main() {
    int *runs = new int[kNumGames];
    battingOrder lineup;
    initializeBattingOrder(lineup);
    for (int game = 0; game < kNumGames; game++) runs[game] = simulateGame(lineup);
    outputResults(runs);
    delete lineup.players;
    delete runs;
    return 0;
}

/**
 * @brief initializeBattingOrder initializes the batting order
 * TODO: initialize based on average production for lineup position, probably from file
 * @param lineup the data representing the batting order
 */

static void initializeBattingOrder(battingOrder &lineup) {
    lineup.players = new player[kNumPlayers];
    for (int i = 0; i < kNumPlayers; i++) {
        player nextPlayer;
        // Arbitrarily chose to take average of STL Cardinals from 1984-2014.
        // TODO: get actual league average numbers for each lineup position
        nextPlayer.outInPlayRate = 0.506757;
        nextPlayer.kRate = 0.169351;
        nextPlayer.bbRate = 0.087831;
        nextPlayer.singleRate = 0.158561;
        nextPlayer.doubleRate = 0.046271;
        nextPlayer.tripleRate = 0.004316;
        nextPlayer.homeRunRate = 0.026912;
        lineup.players[i] = nextPlayer;
    }
}

/**
 * @brief simulateGame simulates a single game, returning the number of runs scored
 * @param lineup the lineup for the game
 * @return the number of runs scored
 */

static int simulateGame(const battingOrder &lineup){
    int runs = 0;
    int battingPostition = 0;
    for (int i = 0; i < kNumInningsPerGame; i++) {
        simulateInning(lineup, runs, battingPostition);
    }
    return runs;
}

/**
 * @brief simulateInning simulates a single inning
 * @param lineup the batting order
 * @param runs the number of runs scored in the game as a whole
 * @param battingPosition the current position in the lineup that is at bat
 */

static void simulateInning(const battingOrder &lineup, int &runs, int &battingPosition) {
    Bases bases;
    clearBases(bases);
    int outs = 0;
    while (outs < kNumOutsPerInning) {
        plateAppearanceResult result = determinePlateAppearanceResult(lineup.players[battingPosition]);
        updateStatus(result, bases, runs, outs);
        battingPosition = (battingPosition + 1) % kNumPlayers;
        if (outs == 3) { clearBases(bases); return; }
    }
}

/**
 * @brief determinePlateAppearanceResult randomly selects the plate appearance result
 * based on the player's result distribution
 * @param p the player who is at the plate
 * @return the result of the plate appearance
 */

static plateAppearanceResult determinePlateAppearanceResult(const player &p) {
    double rand = randomReal(0, 1);
    if (rand < p.outInPlayRate) return OUT_IN_PLAY; else rand -= p.outInPlayRate;
    if (rand < p.kRate)         return STRIKOUT;    else rand -= p.kRate;
    if (rand < p.bbRate)        return WALK;        else rand -= p.bbRate;
    if (rand < p.singleRate)    return SINGLE;      else rand -= p.singleRate;
    if (rand < p.doubleRate)    return DOUBLE;      else rand -= p.doubleRate;
    if (rand < p.tripleRate)    return TRIPLE;      else rand -= p.tripleRate;
    if (rand < p.homeRunRate)   return HOME_RUN;    else rand -= p.homeRunRate;
    return OUT_IN_PLAY; // in case there is a problem with probabilities summing to less than 1
}

/**
 * @brief updateStatus updates the status of the game, in terms of the base-out state and the runs scored
 * @param result the result of the plate appearance
 * @param bases the current state of the bases
 * @param runs the runs scored
 * @param outs the number of outs
 */

static void updateStatus(const plateAppearanceResult &result, Bases &bases, int &runs, int &outs) {
    if (result == OUT_IN_PLAY || result == STRIKOUT) outs++;
    else if (result == WALK) implementWalk(bases, runs);
    else implementHit(getNumBases(result), bases, runs);
}

/**
 * @brief implementWalk implements a walk, adding a base runner to the first unoccupied base,
 * or adding a run if all bases are occupied
 * @param bases the current state of the bases
 * @param runs the runs scored in the game
 */

static void implementWalk(Bases &bases, int &runs) {
    if (!bases.firstBase) bases.firstBase = true;
    else if (!bases.secondBase) bases.secondBase = true;
    else if (!bases.thirdBase) bases.thirdBase = true;
    else runs++;
}

static int getNumBases(const plateAppearanceResult &result) {
    if (result == SINGLE) return 1;
    if (result == DOUBLE) return 2;
    if (result == TRIPLE) return 3;
    if (result == HOME_RUN) return 4;
    return 0;
}

/**
 * @brief implementHit implements a hit, advancing each runner including the batter by numBases,
 * and adding runs as needed. Ordered from third to batter so that the final base of each runner
 * will not be temporarily occupied.
 * @param numBases the number of bases each runner will advance
 * @param bases the current state of the bases
 * @param runs the runs scored in the game
 */

static void implementHit(int numBases, Bases &bases, int &runs) {
    if (bases.thirdBase)  { moveBaserunner(bases, numBases, runs, 3); bases.thirdBase  = false; }
    if (bases.secondBase) { moveBaserunner(bases, numBases, runs, 2); bases.secondBase = false; }
    if (bases.firstBase)  { moveBaserunner(bases, numBases, runs, 1); bases.firstBase  = false; }
    moveBaserunner(bases, numBases, runs, 0);
}

/**
 * @brief moveBaserunner moves a baserunner from initialBase to a base numBases later. Updates
 * either bases or runs as appropriate.
 * @param bases the current state of the bases
 * @param numBases the number of bases the runner is advancing
 * @param runs the number of runs scored in the game
 * @param initialBase the base that the runner is beginning from
 */

static void moveBaserunner(Bases &bases, int numBases, int &runs, int initialBase) {
    int finalBase = initialBase + numBases;
    if (finalBase >= 4) runs++;
    else if (finalBase == 3) bases.thirdBase = true;
    else if (finalBase == 2) bases.secondBase = true;
    else bases.firstBase = true;
}

/**
 * @brief clearBases returns the base state to all empty
 * @param bases the current state of the bases
 */

static void clearBases(Bases &bases) {
    bases.firstBase = bases.secondBase = bases.thirdBase = false;
}

/**
 * @brief outputResult prints results, currently to the console
 * TODO: print results to a file
 * @param runs the number of runs scored over all games
 */

static void outputResults(int *runs) {
    int totalRuns = 0;
    for (int i = 0; i < kNumGames; i++) {
        totalRuns += runs[i];
    }
    cout << totalRuns << " runs scored in " << kNumGames << " games." << endl;
    cout << "(Average of " << setprecision(5) << totalRuns / double (kNumGames) << " runs)" << endl;
}
