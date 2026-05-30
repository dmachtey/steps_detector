#include "Classifier.h"

/**
* Predict class for features vector
*/
int predict(float *x) {
    uint8_t votes[2] = { 0 };
    // tree #1
    if (x[4] <= 17.5) {
        votes[1] += 1;
    }

    else {
        if (x[427] <= 14.5) {
            votes[1] += 1;
        }

        else {
            if (x[221] <= -4065.5) {
                votes[1] += 1;
            }

            else {
                votes[0] += 1;
            }
        }
    }

    // tree #2
    if (x[34] <= 24.0) {
        votes[1] += 1;
    }

    else {
        if (x[330] <= -3.0) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #3
    if (x[319] <= 24.5) {
        votes[1] += 1;
    }

    else {
        if (x[403] <= 23.5) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #4
    if (x[373] <= 29.0) {
        votes[1] += 1;
    }

    else {
        if (x[190] <= 12.5) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #5
    if (x[316] <= 26.5) {
        votes[1] += 1;
    }

    else {
        if (x[15] <= 22.5) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #6
    if (x[67] <= 29.5) {
        if (x[82] <= 37.5) {
            votes[1] += 1;
        }

        else {
            if (x[359] <= -3969.0) {
                votes[1] += 1;
            }

            else {
                votes[0] += 1;
            }
        }
    }

    else {
        if (x[335] <= -4042.5) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #7
    if (x[406] <= 27.0) {
        votes[1] += 1;
    }

    else {
        if (x[417] <= 135.0) {
            if (x[281] <= -4189.0) {
                votes[1] += 1;
            }

            else {
                votes[0] += 1;
            }
        }

        else {
            votes[1] += 1;
        }
    }

    // tree #8
    if (x[49] <= 14.5) {
        votes[1] += 1;
    }

    else {
        if (x[269] <= -4050.0) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #9
    if (x[82] <= 28.5) {
        votes[1] += 1;
    }

    else {
        if (x[328] <= -30.0) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #10
    if (x[169] <= 28.5) {
        votes[1] += 1;
    }

    else {
        if (x[215] <= -4039.0) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #11
    if (x[16] <= 28.5) {
        votes[1] += 1;
    }

    else {
        if (x[200] <= -4036.0) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #12
    if (x[97] <= 24.5) {
        votes[1] += 1;
    }

    else {
        if (x[61] <= 6.0) {
            votes[1] += 1;
        }

        else {
            if (x[57] <= 10.5) {
                if (x[265] <= 34.5) {
                    votes[1] += 1;
                }

                else {
                    votes[0] += 1;
                }
            }

            else {
                votes[0] += 1;
            }
        }
    }

    // tree #13
    if (x[415] <= 33.5) {
        votes[1] += 1;
    }

    else {
        if (x[107] <= -4043.5) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #14
    if (x[187] <= 24.5) {
        votes[1] += 1;
    }

    else {
        if (x[142] <= 28.0) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #15
    if (x[109] <= 11.0) {
        votes[1] += 1;
    }

    else {
        if (x[224] <= -3961.0) {
            votes[0] += 1;
        }

        else {
            if (x[41] <= -3972.0) {
                votes[1] += 1;
            }

            else {
                votes[0] += 1;
            }
        }
    }

    // return argmax of votes
    uint8_t classIdx = 0;
    float maxVotes = votes[0];

    for (uint8_t i = 1; i < 2; i++) {
        if (votes[i] > maxVotes) {
            classIdx = i;
            maxVotes = votes[i];
        }
    }

    return classIdx;
};
/**
* Predict readable class name
*/
const char* predictLabel(float *x) {
    return idxToLabel(predict(x));
};
/**
* Convert class idx to readable name
*/
const char* idxToLabel(uint8_t classIdx) {
    switch (classIdx) {
        case 0:
        return "none";
        case 1:
        return "steps";
        default:
        return "Houston we have a problem";
    }
};