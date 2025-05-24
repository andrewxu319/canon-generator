#pragma once

#include <string>

namespace settings {
	inline const std::string& read_file_path{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/transposed_multimeasure.musicxml" };
	inline const std::string& write_file_path{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/output.musicxml" };
	inline const int minor_key{ false };
	inline const int max_voices{ 5 };
	//inline const int octave_shift{ 0 };
	inline const int h_shift_increments_per_beat{ 1 };
	inline const int measures_separation_between_output_canons{ 0 };
	inline const double h_shift_limit{ 0.7 }; // Maximum h_shift as a proportion of leader length. (0, 1]. LOWER = TIGHTER
	inline const std::size_t warning_threshold{ 3 }; // Reject if MORE WARNINGS THAN this value
	//inline const double combined_threshold{-1};

	inline const double warnings_count_weight{ 0.05 }; // Deduction per warning
	inline const double voice_count_weight{ 0.2 }; // Additional points per extra voice over 2
	inline const double tightness_weight{ 0.4 }; // Maximum deduction out of 1, if tightness is at min_tightness
	inline const double invertibility_weight{ 0.15 }; // Maximum deduction for inveribility
	inline const double outer_voice_weight{ 0.15 }; // Maximum deduction for outer voice validity
}