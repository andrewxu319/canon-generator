#include "canon.h"

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
}