#pragma once

#include <string>

namespace settings {
	inline const std::string& read_file_path{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/transposed_multimeasure.musicxml" };
	inline const std::string& write_file_path{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/output.musicxml" };
	inline const int minor_key{ false };
	inline const int max_voices{ 4 };
	//inline const int octave_shift{ 0 };
	inline const int h_shift_increments_per_beat{ 1 };
	inline const int measures_separation_between_output_textures{ 0 };
	inline const int leader_length_over_max_h_shift{ 2 }; // How tight the canon has to be
	//inline const double warning_weight{ 0.5 }; // Not sure when you would need to use this
	inline const std::size_t error_threshold{ 0 };
	inline const std::size_t warning_threshold{ 0 };
	//inline const double combined_threshold{-1};
}