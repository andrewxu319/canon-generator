#pragma once

#include <string>

namespace settings {
	inline const std::string& read_file_path{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/transposed_multimeasure.musicxml" };
	inline const std::string& write_file_path{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/output.musicxml" };
	inline const int minor_key{ true };
	inline const int measures_separation_between_output_textures{ 2 };
	inline const int leader_length_over_max_h_shift{ 3 };
}