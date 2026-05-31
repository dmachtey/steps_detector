#include "Classifier.h"

/**
* Predict class for features vector
*/
int predict(float *x) {
    uint8_t votes[2] = { 0 };
    // tree #1
    if (x[400] <= 21.0) {
        votes[1] += 1;
    }

    else {
        if (x[377] <= -4038.0) {
            votes[1] += 1;
        }

        else {
            if (x[107] <= -4041.0) {
                votes[1] += 1;
            }

            else {
                votes[0] += 1;
            }
        }
    }

    // tree #2
    if (x[166] <= 30.5) {
        votes[1] += 1;
    }

    else {
        if (x[46] <= 18.5) {
            votes[1] += 1;
        }

        else {
            if (x[346] <= 6.0) {
                votes[1] += 1;
            }

            else {
                votes[0] += 1;
            }
        }
    }

    // tree #3
    if (x[319] <= 19.0) {
        votes[1] += 1;
    }

    else {
        if (x[187] <= 24.5) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #4
    if (x[352] <= 23.5) {
        votes[1] += 1;
    }

    else {
        if (x[385] <= 25.5) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #5
    if (x[394] <= 23.0) {
        votes[1] += 1;
    }

    else {
        if (x[247] <= 19.0) {
            votes[1] += 1;
        }

        else {
            if (x[270] <= 12.5) {
                votes[1] += 1;
            }

            else {
                votes[0] += 1;
            }
        }
    }

    // tree #6
    if (x[376] <= 23.5) {
        votes[1] += 1;
    }

    else {
        if (x[23] <= -4039.0) {
            votes[1] += 1;
        }

        else {
            if (x[228] <= -1.0) {
                votes[1] += 1;
            }

            else {
                votes[0] += 1;
            }
        }
    }

    // tree #7
    if (x[382] <= 19.5) {
        votes[1] += 1;
    }

    else {
        if (x[49] <= 14.0) {
            votes[1] += 1;
        }

        else {
            if (x[447] <= 9.0) {
                votes[1] += 1;
            }

            else {
                votes[0] += 1;
            }
        }
    }

    // tree #8
    if (x[76] <= 26.5) {
        votes[1] += 1;
    }

    else {
        if (x[269] <= -4043.0) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #9
    if (x[244] <= 24.5) {
        votes[1] += 1;
    }

    else {
        if (x[236] <= -4041.0) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #10
    if (x[292] <= 31.0) {
        if (x[450] <= 83.32999038696289) {
            votes[0] += 1;
        }

        else {
            votes[1] += 1;
        }
    }

    else {
        if (x[275] <= -4043.0) {
            votes[1] += 1;
        }

        else {
            if (x[48] <= 154.5) {
                votes[0] += 1;
            }

            else {
                votes[1] += 1;
            }
        }
    }

    // tree #11
    if (x[235] <= 27.5) {
        votes[1] += 1;
    }

    else {
        if (x[134] <= -4039.0) {
            votes[1] += 1;
        }

        else {
            votes[0] += 1;
        }
    }

    // tree #12
    if (x[313] <= 14.5) {
        votes[1] += 1;
    }

    else {
        if (x[365] <= -4034.5) {
            votes[1] += 1;
        }

        else {
            if (x[166] <= -20.0) {
                votes[1] += 1;
            }

            else {
                votes[0] += 1;
            }
        }
    }

    // tree #13
    if (x[379] <= 23.0) {
        votes[1] += 1;
    }

    else {
        if (x[176] <= -4049.0) {
            votes[1] += 1;
        }

        else {
            if (x[51] <= 139.5) {
                votes[0] += 1;
            }

            else {
                votes[1] += 1;
            }
        }
    }

    // tree #14
    if (x[253] <= 28.0) {
        votes[1] += 1;
    }

    else {
        if (x[303] <= 11.0) {
            votes[1] += 1;
        }

        else {
            if (x[375] <= 149.5) {
                votes[0] += 1;
            }

            else {
                votes[1] += 1;
            }
        }
    }

    // tree #15
    if (x[280] <= 10.0) {
        votes[1] += 1;
    }

    else {
        if (x[328] <= 20.0) {
            votes[1] += 1;
        }

        else {
            if (x[296] <= -4057.5) {
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