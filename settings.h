#pragma once

#include <string>

//namespace settings {
//	inline const std::string& read_file_path{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/input.musicxml" };
//	inline const std::string& write_file_path{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/output.musicxml" };
//	inline const bool minor_key{ true };
//	inline const int max_voices{ 2 };
//	//inline const int octave_shift{ 0 };
//	//inline const int leader_length_ticks{ 33 }; // TEMP
//	inline const int h_shift_increments_per_beat{ 1 }; // TODO: 6/8 TIME IS 3 TICKS PER BEAT
//	inline const int measures_separation_between_output_canons{ 0 };
//	inline const double h_shift_limit{ 0.65 }; // Maximum h_shift as a proportion of leader length. (0, 1]. LOWER = TIGHTER
//	inline const std::size_t warning_threshold{ 3 }; // Reject if MORE WARNINGS THAN this value
//	//inline const double combined_threshold{-1};
//
//	//inline const double warnings_count_weight{ 0.2 }; // Deduction per warning
//	//inline const double voice_count_weight{ 0.0 }; // Additional points per extra voice over 2
//	//inline const double tightness_weight{ 0.8 }; // Maximum deduction out of 1, if tightness is at min_tightness
//	//inline const double invertibility_weight{ 0.15 }; // Maximum deduction for inveribility
//	//inline const double outer_voice_weight{ 0.15 }; // Maximum deduction for outer voice validity
//}

struct Settings {
	std::string input_file{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/input.musicxml" };
	std::string output_file{ "C:/Users/spagh/Desktop/brain rot/C++/canon_generator/output.musicxml" };
	bool minor_key{ false };
	int max_voices{ 5 };
	int h_shift_increments_per_beat{ 1 };
	double h_shift_limit{ 0.8 };
	int measures_separation_between_output_canons{ 0 };
	std::size_t warning_threshold{ 3 };
};

const std::string help_message{
	"-i / --input: string, input file path\n"
	"-o / --output: string, output file path\n"
	"-m / --minor: true/false, whether key is minor\n"
	"-v / --voices: integer, max number of voices & number of staves\n"
	"-n / --shift-increment: integer, reciprocal of minimum increment of shift in beats\n"
	"-l / --shift-limit: decimal, maximum shift as a proportion of total leader length\n"
	"-s / --separation: integer, measures of separation between canons\n"
	"-w / --warning: integer, maximum number of warnings allowed\n"
	"-h / --help: help\n"
};

//const std::string help_message{ "hi" };