#pragma once

#include "mx/api/ScoreData.h"

#include <utility>

using Voice = std::vector<mx::api::NoteData>;

const std::pair<int, int> check_counterpoint(const std::vector<Voice>&);