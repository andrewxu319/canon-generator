#pragma once

#include "settings.h"

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
	Canon(const std::vector<Voice> texture, const double max_h_shift_proportion)
		: m_texture{ texture }, m_max_h_shift_proportion{ max_h_shift_proportion }, m_voice_count{ static_cast<int>(texture.size()) } {
	}

	std::vector<Voice>& texture() {
		return m_texture; // Can't change texture size
	}

	void add_voice(const Voice& voice) {
		m_texture.emplace_back(voice);
		++m_voice_count;
		m_total_voice_pairs = m_voice_count * (m_voice_count - 1) / 2;
	}

	const int get_voice_count() const {
		return m_voice_count;
	}

	const int get_max_h_shift_proportion() const {
		return m_max_h_shift_proportion;
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

		for (int top_voice{ 0 }; top_voice < m_voice_count; ++top_voice) {
			if (top_voice == value) {
				continue;
			}
			const std::pair<int, int> pair{ value, top_voice };
			if (std::find(m_invalid_outer_voice_pairs.begin(), m_invalid_outer_voice_pairs.end(), pair) == m_invalid_outer_voice_pairs.end()) {
				// Check for duplicates
				add_invalid_outer_voice_pair(pair);
			}
		}
	}

	const std::vector<std::pair<int, int>>& get_non_invertible_voice_pairs() const {
		return m_non_invertible_voice_pairs;
	}

	void add_non_invertible_voice_pair(const std::pair<int, int>& pair) {
		// Lower, upper
		m_non_invertible_voice_pairs.emplace_back(pair);
	}

	const std::vector<int>& get_invalid_top_voices() const {
		return m_invalid_top_voices;
	}

	void add_invalid_top_voice(const int value) {
		if (std::find(m_invalid_top_voices.begin(), m_invalid_top_voices.end(), value) == m_invalid_top_voices.end()) {
			// Check for duplicates
			m_invalid_top_voices.push_back(value);
		}

		for (int bottom_voice{ 0 }; bottom_voice < m_voice_count; ++bottom_voice) {
			if (bottom_voice == value) {
				continue;
			}
			const std::pair<int, int> pair{ bottom_voice, value };
			if (std::find(m_invalid_outer_voice_pairs.begin(), m_invalid_outer_voice_pairs.end(), pair) == m_invalid_outer_voice_pairs.end()) {
				// Check for duplicates
				add_invalid_outer_voice_pair(pair);
			}
		}
	}

	const std::vector<std::pair<int, int>>& get_invalid_outer_voice_pairs() const {
		return m_invalid_outer_voice_pairs;
	}

	void add_invalid_outer_voice_pair(const std::pair<int, int>& pair) {
		// Lower, upper

		if (std::find(m_invalid_outer_voice_pairs.begin(), m_invalid_outer_voice_pairs.end(), pair) == m_invalid_outer_voice_pairs.end()) {
			// Check for duplicates
			m_invalid_outer_voice_pairs.emplace_back(pair);
		}
	}

	const double get_non_invertible_voice_pairs_proportion() {
		m_non_invertible_voice_pairs_proportion = static_cast<double>(m_non_invertible_voice_pairs.size()) / m_total_voice_pairs;
		return m_non_invertible_voice_pairs_proportion;
	}

	const double get_invalid_outer_voice_pairs_proportion() {
		m_invalid_outer_voice_pairs_proportion = static_cast<double>(m_invalid_outer_voice_pairs.size()) / (m_total_voice_pairs * 2);
		return m_invalid_outer_voice_pairs_proportion;
	}

	const double get_quality_score() {
		m_quality_score = 
			- m_warning_message_box.size() * settings::warnings_count_weight
			+ (m_voice_count - 2) * settings::voice_count_weight
			- (settings::tightness_weight / settings::h_shift_limit) * m_max_h_shift_proportion + 0.5 * settings::tightness_weight // Add half of tightness_weight to center it on 0
			- m_non_invertible_voice_pairs_proportion * settings::invertibility_weight
			- m_invalid_outer_voice_pairs_proportion * settings::outer_voice_weight;
		return m_quality_score;
	}

private:
	std::vector<Voice> m_texture{};
	int m_voice_count{};
	double m_max_h_shift_proportion{}; // Also tightness
	std::vector<Message> m_error_message_box{};
	std::vector<Message> m_warning_message_box{};
	std::vector<Message> m_quality_message_box{};

	int m_total_voice_pairs{};
	std::vector<std::pair<int, int>> m_non_invertible_voice_pairs{}; // Number of voice pairs that are not invertible (weighted equally regardless of how many there are)
	std::vector<int> m_invalid_bass_voices{};
	std::vector<int> m_invalid_top_voices{};
	std::vector<std::pair<int, int>> m_invalid_outer_voice_pairs{}; // Ordered

	double m_non_invertible_voice_pairs_proportion{};
	double m_invalid_outer_voice_pairs_proportion{}; // Multiply by 2 to make it ordered

	double m_quality_score{}; // Combined weighted score of warning_count, max_h_shift_proportion, invertibility, outer_voice_errors, and valid_bass_voices
};

void print_messages(Canon& canon);