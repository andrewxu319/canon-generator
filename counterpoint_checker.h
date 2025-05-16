#pragma once

#include "sonority.h"

#include "mx/api/ScoreData.h"

#include <utility>
#include <tuple>

using Message = std::tuple<const std::string, const int, const int>; // Sonority 1 id, sonority 2 id

const std::pair<std::vector<Message>, std::vector<Message>>
	check_counterpoint(const std::vector<SonorityArray>& sonority_arrays, const int ticks_per_measure, const std::vector<int>& rhythmic_hierarchy_array, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat);