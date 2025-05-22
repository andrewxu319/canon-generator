#pragma once

#include <string>

namespace settings {
	inline const std::string& read_file_path{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/transposed_multimeasure.musicxml" };
	inline const std::string& write_file_path{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/output.musicxml" };
	inline const int minor_key{ false };
	inline const int max_voices{ 3 };
	//inline const int octave_shift{ 0 };
	inline const int h_shift_increments_per_beat{ 1 };
	inline const int measures_separation_between_output_canons{ 0 };
	inline const double min_tightness{ 1.0 }; // Maximum h_shift as a proportion of leader length. (0, 1]
	//inline const double warning_weight{ 0.5 }; // Not sure when you would need to use this
	inline const std::size_t warning_threshold{ 0 };
	//inline const double combined_threshold{-1};
}