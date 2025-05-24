#include "canon.h"
#include "settings.h"

#include "mx/api/ScoreData.h"

#include <vector>

void print_messages(Canon& canon) {
	if (canon.error_message_box().size() == 0 && canon.warning_message_box().size() == 0) {
		std::cout << "No errors or warnings!\n";
	}
	for (const Message& error : canon.error_message_box()) {
		std::cout << "Error:   Notes at ticks " << (error.sonority_1_index) << " and " << (error.sonority_2_index) << ". " << (error.description) << "!\n";
	}
	for (const Message& warning : canon.warning_message_box()) {
		std::cout << "Warning: Notes at ticks " << (warning.sonority_1_index) << " and " << (warning.sonority_2_index) << ". " << (warning.description) << "!\n";
	}

	// Summary
	std::cout << "\n\n";
	std::cout << "Canon is " << ((canon.error_message_box().size() == 0 && canon.warning_message_box().size() <= settings::warning_threshold) ? "valid\n" : "invalid\n");

	if (canon.error_message_box().size() == 0) {
		std::cout << "Warning count: " << canon.warning_message_box().size() << '\n';
		std::cout << "Voice count: " << canon.get_voice_count() << '\n';
		std::cout << "Max horizontal shift: " << canon.get_max_h_shift_proportion() << '\n';
		std::cout << "Invertibility proportion: " << 1 - canon.get_non_invertible_voice_pairs_proportion() << '\n';
		std::cout << "Valid outer voice pairs proportion: " << 1 - canon.get_invalid_outer_voice_pairs_proportion() << '\n';
		std::cout << "Quality score: " << canon.get_quality_score() << '\n';
		std::cout << '\n';

		std::cout << "Invalid voice pairs:";
		for (const std::pair<int, int>& pair : canon.get_non_invertible_voice_pairs()) {
			std::cout << " (" << std::to_string(pair.first) << ", " << std::to_string(pair.second) << ").";
		}
		std::cout << '\n';

		std::cout << "Invalid bass voices:";
		for (const int voice : canon.get_invalid_bass_voices()) {
			std::cout << ' ' << std::to_string(voice) << '.';
		}
		std::cout << '\n';

		std::cout << "Invalid top voices:";
		for (const int voice : canon.get_invalid_top_voices()) {
			std::cout << ' ' << std::to_string(voice) << '.';
		}
		std::cout << '\n';

		std::cout << "Invalid outer voice pairs:";
		for (const std::pair<int, int>& pair : canon.get_invalid_outer_voice_pairs()) {
			std::cout << " (" << std::to_string(pair.first) << ", " << std::to_string(pair.second) << ").";
		}
		std::cout << '\n';
	}
	std::cout << "\n---------------------------------\n\n";
}