#pragma once
enum GraphFlags { ManualMode = 1, ShowWeights = 2, ShowBandwidth = 4, ShowFlow = 8 };

enum EdgeType { SingleDirection = 0, BiDirectionalSame = 1, BiDirectionalDiff = 2, Loop = 3, Error = -1 };
