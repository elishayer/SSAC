#include <iostream>
#include <iomanip>
#include "lineup-simulation-types.h"
#include "random.h"
#include "console.h"
#include "simpio.h"
using namespace std;

// Constants
static const int kNumGames = 1000000;
static const int kNumBases = 3;
static const int kNumInningsPerGame = 9;
static const int kNumPlayers = 9;
static const int kNumOutsPerInning = 3;
static const int kOutSentinal = -1;

// Prototypes
static void initializeBattingOrder(battingOrder &lineup);
static int simulateGame(const battingOrder &lineup);
static void simulateInning(const battingOrder &lineup, int &runs, int &battingPosition);
static plateAppearanceResult determinePlateAppearanceResult(player &p);
static void updateStatus(const plateAppearanceResult &result, Bases &bases, int &runs, int &outs, player &hitter);
static void implementWalk(Bases &bases, int &runs, player &hitter);
static void implementHit(const plateAppearanceResult &result, Bases &bases, int &runs, int &outs, player &hitter);
static int countBaserunners(Bases &bases);
static void implementDouble(Bases &bases, int &runs, int &outs, player &hitter);
static void implementSingle(Bases &bases, int &runs, int &outs, player &hitter);
static void simulateBaserunning(Bases &bases, int initBase, int &runs, int &outs, double rate1, int base1, double rate2, int base2);
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
        // Arbitrarily chose to take average of STL Cardinals from 1984-2014 to get some working data.
        // TODO: get actual league average numbers for each lineup position
        nextPlayer.kRate = 0.169351;
        nextPlayer.bbRate = 0.087831;
        nextPlayer.singleRate = 0.158561;
        nextPlayer.doubleRate = 0.046271;
        nextPlayer.tripleRate = 0.004316;
        nextPlayer.homeRunRate = 0.026912;
        // Base running stats taken from NL league average from 2014.
        // url: http://www.vivaelbirdos.com/st-louis-cardinals-sabermetrics-analysis/2015/2/9/8001723/st-louis-cardinals-base-running-taking-extra-base-on-hits-2014
        // admittedly a questionable source given that he doesn't cite any website that the numbers come from. Fine for a temporary solution!
        nextPlayer.br1s2Rate = 0.7060;
        nextPlayer.br1s3Rate = 0.2801;
        nextPlayer.br1d3Rate = 0.5549;
        nextPlayer.br1dHRate = 0.4145;
        nextPlayer.br2s3Rate = 0.3664;
        nextPlayer.br2sHRate = 0.5922;
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
        updateStatus(result, bases, runs, outs, lineup.players[battingPosition]);
/*
        // Use for debugging on plate appearance basis
        cout << "outs: " << outs << " runs: " << runs << " first: ";
        if (bases.firstBase) cout << "taken";
        cout << " second: ";
        if (bases.secondBase) cout << "taken";
        cout << " third: ";
        if (bases.thirdBase) cout << "taken";
        cout << " result: " << result << endl; // numbers correspond to lineup-simulation-types.h plateAppearanceResult enumeration
        getLine("Press [ENTER]");
*/
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

static plateAppearanceResult determinePlateAppearanceResult(player &p) {
    double rand = randomReal(0, 1);
    if (rand < p.kRate)       return STRIKOUT; else rand -= p.kRate;
    if (rand < p.bbRate)      return WALK;     else rand -= p.bbRate;
    if (rand < p.singleRate)  return SINGLE;   else rand -= p.singleRate;
    if (rand < p.doubleRate)  return DOUBLE;   else rand -= p.doubleRate;
    if (rand < p.tripleRate)  return TRIPLE;   else rand -= p.tripleRate;
    if (rand < p.homeRunRate) return HOME_RUN; else rand -= p.homeRunRate;
    return OUT_IN_PLAY;
}

/**
 * @brief updateStatus updates the status of the game, in terms of the base-out state and the runs scored
 * @param result the result of the plate appearance
 * @param bases the current state of the bases
 * @param runs the runs scored
 * @param outs the number of outs
 * @param hitter the player who was just at the plate
 */

static void updateStatus(const plateAppearanceResult &result, Bases &bases, int &runs, int &outs, player &hitter) {
    if (result == OUT_IN_PLAY || result == STRIKOUT) outs++;
    else if (result == WALK) implementWalk(bases, runs, hitter);
    else implementHit(result, bases, runs, outs, hitter);
}

/**
 * @brief implementWalk implements a walk, moving baserunners forward by one position
 * and adding the hitter to
 * @param bases the current state of the bases
 * @param runs the runs scored in the game
 * @param hitter the plater who was just at the plate
 */

static void implementWalk(Bases &bases, int &runs, player &hitter) {
    if (bases.firstBase) {
        if (bases.secondBase) {
            if (bases.thirdBase) {
                runs++;
            }
            bases.thirdBase = bases.secondBase;
        }
        bases.secondBase = bases.firstBase;
    }
    bases.firstBase = &hitter;
}

/**
 * @brief implementHit implements a hit, advancing each runner according to their baserunning skill
 * @param result the type of hit
 * @param bases the current state of the bases
 * @param runs the runs scored in the game
 * @param outs the number of outs in the inning
 * @param hitter the player who was just at the plate
 */

static void implementHit(const plateAppearanceResult &result, Bases &bases, int &runs, int &outs, player &hitter) {
    if (result == HOME_RUN) { runs += countBaserunners(bases) + 1; clearBases(bases); }
    if (result == TRIPLE) { runs += countBaserunners(bases); clearBases(bases); bases.thirdBase = &hitter; }
    if (result == DOUBLE) implementDouble(bases, runs, outs, hitter);
    if (result == SINGLE) implementSingle(bases, runs, outs, hitter);
}

/**
 * @brief countBaserunners counts the number of baserunners currently on the basepaths
 * @param bases the current state of the bases
 * @return the number of baserunners
 */

static int countBaserunners(Bases &bases) {
    int numBaserunners = 0;
    if (bases.firstBase) numBaserunners++;
    if (bases.secondBase) numBaserunners++;
    if (bases.thirdBase) numBaserunners++;
    return numBaserunners;
}

/**
 * @brief implementDouble implements a double, scoring all runners on 2nd or 3rd,
 * and moving the runner on 1st according to his baserunning skill
 * @param bases the current state of the bases
 * @param runs the runs scored in the game
 * @param outs the number of outs in the inning
 * @param hitter the player who was just at the plate
 */

static void implementDouble(Bases &bases, int &runs, int &outs, player &hitter) {
    if (bases.thirdBase) { runs++; bases.thirdBase = NULL; }
    if (bases.secondBase) { runs++; bases.secondBase = NULL; }
    if (bases.firstBase) simulateBaserunning(bases, 1, runs, outs, bases.firstBase->br1d3Rate, 3, bases.firstBase->br1dHRate, 4);
    bases.secondBase = &hitter;
}

/**
 * @brief implementSingle implements a single, scoring all runners on 3rd, and moving
 * runners from 2nd and 1st according to their baserunning skill
 * @param bases the current state of the bases
 * @param runs the runs scored in the game
 * @param outs the number of outs in the inning
 * @param hitter the player who was just at the plate
 */

static void implementSingle(Bases &bases, int &runs, int &outs, player &hitter) {
    if (bases.thirdBase) { runs++; bases.thirdBase = NULL; }
    if (bases.secondBase) simulateBaserunning(bases, 2, runs, outs, bases.secondBase->br2s3Rate, 3, bases.secondBase->br2sHRate, 4);
    if (bases.firstBase) simulateBaserunning(bases, 1, runs, outs, bases.firstBase->br1s2Rate, 2, bases.firstBase->br1s3Rate, 3);
    bases.firstBase = &hitter;
}

/**
 * @brief simulateBaserunning simulates baserunning based on initial base and baserunning skill
 * @param bases the current state of the bases
 * @param initBase the initial base
 * @param runs the number of runs scored in the game
 * @param outs the number of outs in the inning
 * @param rate1 the probability of outcome one
 * @param base1 the base result of outcome one
 * @param rate2 the probability of outcome two
 * @param base2 the base result of outcome two
 */

static void simulateBaserunning(Bases &bases, int initBase, int &runs, int &outs, double rate1, int base1, double rate2, int base2) {
    // determine destination
    double rand = randomReal(0, 1);
    int finalBase;
    if (rand < rate1)           // chose first outcome
        finalBase = base1;
    else {
        rand -= rate1;
        if (rand < rate2)       // chose second outcome
            finalBase = base2;
        else {                  // final outcome: an out on the basepaths
            outs++;
            return;
        }
    }

    // going home
    if (finalBase == 4) {
        runs++;
        if (initBase == 3) bases.thirdBase = NULL;
        if (initBase == 2) bases.secondBase = NULL;
        if (initBase == 1) bases.firstBase = NULL;
        return;
    }

    // either going to second or blocked from going to third resulting in second, always originating from first
    if (finalBase == 2 || (finalBase == 3 && bases.thirdBase)) { bases.secondBase = bases.firstBase; bases.firstBase = NULL; return; }

    // final base is an unblocked third, originating from either first or second
    if (initBase == 1) { bases.thirdBase = bases.firstBase; bases.firstBase = NULL; return; }
    else { bases.thirdBase = bases.secondBase; bases.secondBase = NULL; return; }
}

/**
 * @brief clearBases returns the base state to all empty
 * @param bases the current state of the bases
 */

static void clearBases(Bases &bases) {
    bases.firstBase = bases.secondBase = bases.thirdBase = NULL;
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
