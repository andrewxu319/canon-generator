#pragma once

#include "sonority.h"
#include "canon.h"

#include "mx/api/ScoreData.h"

#include <utility>
#include <tuple>

struct ScaleDegree {
	const mx::api::Step step{};
	const int alter{};
};

const std::pair<std::vector<Message>, std::vector<Message>> check_counterpoint(const SonorityArray& sonority_array, const std::vector<std::vector<int>> index_arrays_for_sonority_arrays, const int ticks_per_measure, const ScaleDegree& tonic, const ScaleDegree& dominant, const ScaleDegree& leading_tone, const std::vector<int>& rhythmic_hierarchy_array, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat);
