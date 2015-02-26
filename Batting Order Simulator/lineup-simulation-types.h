#ifndef LINEUPSIMULATIONTYPES_H
#define LINEUPSIMULATIONTYPES_H

// Any remaining probability in plate appearance results is for out in play
// Any remaining probability in baserunning results is for out on bases
struct player {
    // plate appearance results
    double kRate;
    double bbRate;
    double singleRate;
    double doubleRate;
    double tripleRate;
    double homeRunRate;
    // base running
    double br1s2Rate; // runner on first rate going to second on a single
    double br1s3Rate; // runner on first rate going to third on a single
    double br1d3Rate; // runner on first rate going to third on a double
    double br1dHRate; // runner on first rate scoring on a double
    double br2s3Rate; // runner on second rate going to third on a single
    double br2sHRate; // runner on second rate scoring on a single
};

struct battingOrder {
    player* players;
};

struct Bases{
    player* firstBase;
    player* secondBase;
    player* thirdBase;
};

enum plateAppearanceResult {
    OUT_IN_PLAY,// 0
    STRIKOUT,   // 1
    WALK,       // 2
    SINGLE,     // 3
    DOUBLE,     // 4
    TRIPLE,     // 5
    HOME_RUN    // 6
};

#endif // LINEUPSIMULATIONTYPES_H
