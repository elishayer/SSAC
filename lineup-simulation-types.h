#ifndef LINEUPSIMULATIONTYPES_H
#define LINEUPSIMULATIONTYPES_H

struct player {
    double outInPlayRate;
    double kRate;
    double bbRate;
    double singleRate;
    double doubleRate;
    double tripleRate;
    double homeRunRate;
};

struct battingOrder {
    player* players;
};

struct Bases{
    bool firstBase;
    bool secondBase;
    bool thirdBase;
};

enum plateAppearanceResult {
    OUT_IN_PLAY,
    STRIKOUT,
    WALK,
    SINGLE,
    DOUBLE,
    TRIPLE,
    HOME_RUN
};

#endif // LINEUPSIMULATIONTYPES_H
