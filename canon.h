#pragma once

#include "mx/api/ScoreData.h"

#include <vector>

using Voice = std::vector<mx::api::NoteData>;

struct Message {
	std::string_view description{};
	int sonority_1_index{};
	int sonority_2_index{};
};

class Canon {
public:
	Canon(std::vector<Voice> texture, int max_h_shift)
		: m_texture{ texture }, m_max_h_shift{ max_h_shift } {
	}

	std::vector<Voice>& texture() {
		return m_texture;
	}

	const int get_max_h_shift() const {
		return m_max_h_shift;
	}

	std::vector<Message>& error_message_box() {
		return m_error_message_box;
	}

	std::vector<Message>& warning_message_box() {
		return m_warning_message_box;
	}

	const int get_error_count() const {
		return m_error_message_box.size();
	}

	const int get_warning_count() const {
		return m_warning_message_box.size();
	}

private:
	std::vector<Voice> m_texture{};
	int m_max_h_shift{}; // Also tightness
	std::vector<Message> m_error_message_box{};
	std::vector<Message> m_warning_message_box{};
	int m_invertibility{}; // Number of voice pairs that are not invertible (weighted equally regardless of how many there are)
	int m_outer_voice_errors{}; // Number of voice pairs that have outer voice-specific errors 
	int m_bass_voice_errors{}; // Number of voice pairs that have bass voice-specific errors
	int m_canon_quality{}; // Combined weighted score of warning_count, max_h_shift, invertibility, outer_voice_errors, and bass_voice_errors
};

void print_messages(Canon& canon);