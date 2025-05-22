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

	std::vector<Message>& quality_message_box() {
		return m_quality_message_box;
	}

	const int get_error_count() const {
		return m_error_message_box.size();
	}

	const int get_warning_count() const {
		return m_warning_message_box.size();
	}

	const std::vector<int>& get_invalid_bass_voices() const {
		return m_invalid_bass_voices;
	}

	void add_invalid_bass_voice(const int value) {
		if (std::find(m_invalid_bass_voices.begin(), m_invalid_bass_voices.end(), value) == m_invalid_bass_voices.end()) {
			// Check for duplicates
			m_invalid_bass_voices.push_back(value);
		}
	}

	const std::vector<std::pair<int, int>>& get_non_invertible_voice_pairs() const {
		return m_non_invertible_voice_pairs;
	}

	void add_non_invertible_voice_pair(const std::pair<int, int>& pair) {
		// Lower, upper
		m_non_invertible_voice_pairs.emplace_back(pair);
	}

private:
	std::vector<Voice> m_texture{};
	int m_max_h_shift{}; // Also tightness
	std::vector<Message> m_error_message_box{};
	std::vector<Message> m_warning_message_box{};
	std::vector<Message> m_quality_message_box{};
	//int m_voice_combinations{};
	std::vector<int> m_invalid_bass_voices{};
	std::vector<std::pair<int, int>> m_non_invertible_voice_pairs{}; // Number of voice pairs that are not invertible (weighted equally regardless of how many there are)
	int m_outer_voice_errors{}; // Number of voice pairs that have outer voice-specific errors 
	int m_canon_quality{}; // Combined weighted score of warning_count, max_h_shift, invertibility, outer_voice_errors, and valid_bass_voices
};

void print_messages(Canon& canon);