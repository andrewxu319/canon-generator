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

struct Key {
	const ScaleDegree tonic{};
	const ScaleDegree dominant{};
	const ScaleDegree leading_tone{};
};

void check_counterpoint(Canon& canon, const int ticks_per_measure, const Key& key, const std::vector<int>& rhythmic_hierarchy_array, const int rhythmic_hierarchy_max_depth, const int rhythmic_hierarchy_of_beat, const Settings& settings);
