#include "mx/api/ScoreData.h"

#include <utility>

using Voice = std::vector<mx::api::NoteData>;

const std::pair<int, int> check_counterpoint(const std::vector<Voice>&) {
	// Simple array, only note changes

	// Generate interval array
	
	// Generate step/leap array
	return std::pair<int, int> { 0, 0 };
}